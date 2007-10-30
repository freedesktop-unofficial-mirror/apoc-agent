/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 * 
 * Copyright 2007 Sun Microsystems, Inc. All rights reserved.
 * 
 * The contents of this file are subject to the terms of either
 * the GNU General Public License Version 2 only ("GPL") or
 * the Common Development and Distribution License("CDDL")
 * (collectively, the "License"). You may not use this file
 * except in compliance with the License. You can obtain a copy
 * of the License at www.sun.com/CDDL or at COPYRIGHT. See the
 * License for the specific language governing permissions and
 * limitations under the License. When distributing the software,
 * include this License Header Notice in each file and include
 * the License file at /legal/license.txt. If applicable, add the
 * following below the License Header, with the fields enclosed
 * by brackets [] replaced by your own identifying information:
 * "Portions Copyrighted [year] [name of copyright owner]"
 * 
 * Contributor(s):
 * 
 * If you wish your version of this file to be governed by
 * only the CDDL or only the GPL Version 2, indicate your
 * decision by adding "[Contributor] elects to include this
 * software in this distribution under the [CDDL or GPL
 * Version 2] license." If you don't indicate a single choice
 * of license, a recipient has the option to distribute your
 * version of this file under either the CDDL, the GPL Version
 * 2 or to extend the choice of license to its licensees as
 * provided above. However, if you add GPL Version 2 code and
 * therefore, elected the GPL Version 2 license, then the
 * option applies only if the new code is made subject to such
 * option by the copyright holder.
 */
package com.sun.apoc.daemon.apocd;

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.localdatabase.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transaction.*;
import com.sun.apoc.daemon.transport.*;

import java.util.*;

public class ClientManager implements ClientChannelEventListener,
									  ConfigEventListener,
									  SessionListener
{
	private final HashMap		            mClients			=
		new HashMap( DaemonConfig.getIntProperty(
						DaemonConfig.sMaxClientConnections ) );
	private final EventQueue				mPendingEvents		=
		new EventQueue();
	private final EventWorkerThreadPool		mThreadPool     	=
		new EventWorkerThreadPool(
			mPendingEvents,
			DaemonConfig.getIntProperty( DaemonConfig.sMaxClientThreads ) );
	private final DaemonTimerTask			mChangeDetector		=
		new DaemonTimerTask( new ChangeDetector( this ) );
	private int								mSessionCount		= 0;
	
	public ClientManager( final ChannelManager inChannelManager )
		throws APOCException
	{
		inChannelManager.addClientChannelEventListener( this );
        DaemonConfig.addConfigEventListener( this );
		APOCLogger.fine( "Clmr001" );
	}

	public void changeDetect()
	{
		new ChangeDetector( this ).run();
	}

	public void onPending( final ClientChannel inClientChannel )
	{
		putEventHandler( 
                new ClientEventHandler( getClient( inClientChannel)) );
	}

	public void onClosed( final ClientChannel inClientChannel )
	{
		synchronized( mClients )
		{
			final Client theClient =
				( Client )mClients.remove( inClientChannel );
			if ( theClient != null )
			{
				theClient.close();
				APOCLogger.finer( "Clmr002" );
			}
		}
	}

	public void onConfigEvent()
	{
		startChangeDetectionTimer();

		mThreadPool.resizePool(
			DaemonConfig.getIntProperty(
				DaemonConfig.sMaxClientThreads ) );
	}

	/* Warning: putting this here because we need access to the EventQueue.
	 *          This Queue should be separated from this class in future.
	 */
	public void onGarbageCollect()
	{
		try
		{
			Object[] theDatabases =
				LocalDatabaseFactory.getInstance().getDatabases();
			if ( theDatabases != null )
			{
				for ( int theIndex = 0;
					  theIndex < theDatabases.length;
					  ++ theIndex )
				{
					putEventHandler(
						new GarbageCollectEventHandler(
					    	( Database )theDatabases[ theIndex ] ) );
				}
			}
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "ClientManager",
								 "onGarbageCollect",
								 theException );
		}
	}

	synchronized public void onSessionCreate( final Session inSession )
	{
		mSessionCount ++;
		doInitialChangeDetect( inSession );
	}

	synchronized public void onSessionDestroy( final Session inSession )
	{
		mSessionCount --;
		stopChangeDetectionTimer();
	}

	public void terminate()
	{
		mThreadPool.terminate();
		APOCLogger.fine( "Clmr007" );
	}

	private void doInitialChangeDetect( final Session inSession )
	{
		try
		{
            Cache [] theCaches = inSession.getCaches() ;
            ArrayList theOutOfDateCaches = new ArrayList() ;

            for (int i = 0 ; i < theCaches.length ; ++ i) 
            {
                if (theCaches [i].isOutOfDate())
                {
                    theOutOfDateCaches.add(theCaches [i]) ;
                }
            }
            final DaemonTimerTask theTask = 
                new DaemonTimerTask(
                        new ChangeDetector(
                            this, 
                            (Cache []) theOutOfDateCaches.toArray(
                                                              new Cache [0])), 
                        true) ;

            theTask.setPeriod(
                            DaemonConfig.getIntProperty(
                                DaemonConfig.sInitialChangeDetectionDelay) * 
                            1000) ;
            startChangeDetectionTimer() ;
		}
		catch( Exception theException )
		{}
	}

	private Client getClient( final ClientChannel inClientChannel )
	{
		synchronized( mClients )
		{
			Client theClient = ( Client )mClients.get( inClientChannel );
			if ( theClient == null )
			{
				theClient = new Client( inClientChannel );
				theClient.addSessionListener( this );
				mClients.put( inClientChannel, theClient );
				APOCLogger.finer( "Clmr005" );
			}
			return theClient;
		}
	}

	private boolean putEventHandler( final EventHandler inEventHandler )
	{
		boolean theRC = true;
		mPendingEvents.put( inEventHandler );
		try
		{
			mThreadPool.ensureThread();
		}
		catch( APOCException theException )
		{
			theRC = false;
			APOCLogger.throwing( "ChangeDetector", "run", theException );
		}
		return theRC;
	}

	private void startChangeDetectionTimer()
	{
		if ( mSessionCount > 0 )
		{
			final float theMinuteDelay	=
				DaemonConfig.getFloatProperty(
					DaemonConfig.sChangeDetectionInterval );
			if ( mChangeDetector.setPeriod( ( long )(60000 * theMinuteDelay) ) )
			{
				if ( mChangeDetector.getPeriod() > 0 )
				{
					APOCLogger.fine( "Clmr004",
								 String.valueOf( theMinuteDelay ) );
				}
				else
				{
					APOCLogger.fine( "Clmr006" );
				}
			}
		}
	}

	private void stopChangeDetectionTimer()
	{
		if ( mSessionCount < 1 )
		{
			mChangeDetector.cancel();
		}
	}

	static class ChangeDetector implements Runnable
	{
		private ClientManager	mManager;
		private Cache []		mCaches = null;
        boolean                 mCheckCaches = false ;

		public ChangeDetector( final ClientManager	inManager,
		                       final Cache []       inCaches )
		{
			mManager	 = inManager;
			mCaches  	 = inCaches ;
            mCheckCaches = true ;
		}

		public ChangeDetector( final ClientManager inManager )
		{
			mManager = inManager;
		}

		public void run()
		{
            Cache [] theCaches = mCaches ;

			if ( theCaches == null )
			{
			    synchronized( mManager.mClients )
			    {
				    if ( ! mManager.mClients.isEmpty() )
				    {
					    theCaches = CacheFactory.getInstance().getCaches();
				    }
			    }
			}
			if ( theCaches != null )
			{
				final ArrayList	theHandlers = new ArrayList();
                final UpdateAggregator theAggregator = new UpdateAggregator() ;

				for ( int theIndex = 0;
					  theIndex < theCaches.length;
					  ++ theIndex )
				{
                    if (!mCheckCaches || theCaches [theIndex].isOutOfDate()) {
    					final ChangeDetectEventHandler theHandler =
	    					new ChangeDetectEventHandler(
		    						mManager, theCaches[ theIndex ], 
                                    theAggregator );
				    	if ( mManager.putEventHandler( theHandler ) )
					    {
						    theHandlers.add( theHandler );
    					}
                    }
				}
				int theHandlerCount = theHandlers.size();
				for ( int theIndex = 0;
					  theIndex < theHandlerCount;
					  ++ theIndex )
				{
					( ( ChangeDetectEventHandler )theHandlers
						.get( theIndex ) ).waitForCompletion();
				}
                theAggregator.sendNotifications() ;
				mManager.startChangeDetectionTimer();
			}
		}	
	}
}
