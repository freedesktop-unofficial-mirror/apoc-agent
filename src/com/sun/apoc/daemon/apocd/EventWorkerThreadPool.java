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
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transaction.*;

import java.io.*;
import java.util.*;

class EventWorkerThreadPool
{
	private final   EventQueue		mQueue;
	private final   ArrayList		mPool  				= new ArrayList();
	private int						mMaxPoolSize;
	private int						mPoolSize			= 0;
	private final	DaemonTimerTask	mIdleDetector		=
		new DaemonTimerTask( new IdleDetector() );

	private static	long			sIdleDetectorPeriod	=
		( long )DaemonConfig.
			getIntProperty( DaemonConfig.sIdleThreadDetectionInterval ) * 60000;

	public EventWorkerThreadPool( final EventQueue	inQueue,
								  final int			inPoolSize )
		throws APOCException
	{
		mQueue			= inQueue;
		mMaxPoolSize	= inPoolSize;
		mIdleDetector.setPeriod( sIdleDetectorPeriod );
	}

	synchronized public void ensureThread()
		throws APOCException
	{
		if ( mPoolSize < mMaxPoolSize && ! haveIdleThread() )
		{
			final EventWorkerThread theThread = new EventWorkerThread( mQueue );
			mPool.add( theThread );
			mPoolSize ++;
			theThread.start();
		}	
	}

	synchronized public void resizePool( int inPoolSize )
	{
		inPoolSize = inPoolSize > 0 ? inPoolSize : 1;
		if ( inPoolSize != mMaxPoolSize )
		{
			if ( inPoolSize > mMaxPoolSize )
			{
				mMaxPoolSize = inPoolSize;
			}
			else if ( inPoolSize < mMaxPoolSize )
			{
				shrinkPool( inPoolSize );
			}
		}
	}

	public synchronized void terminate()
	{
		terminateThreads();
		waitForLiveThreads();
	}

	private boolean haveIdleThread()
	{
		boolean haveIdleThread = false;
		for ( int theIndex = 0; theIndex < mPoolSize; theIndex ++ )
		{
			if ( ( ( EventWorkerThread )mPool.get( theIndex ) ).isAvailable() )
			{
				haveIdleThread = true;
				break;
			}
		}
		return haveIdleThread;
	}

	private boolean haveLiveThread()
	{
		boolean haveLiveThread = false;
		for ( int theIndex = 0; theIndex < mPoolSize; theIndex ++ )
		{
			if ( ( ( EventWorkerThread )mPool.get( theIndex ) ).isAlive() )
			{
				haveLiveThread = true;
				break;
			}
		}
		return haveLiveThread;
	}

	private void shrinkPool( int inPoolSize )
	{
		for ( int theIndex = mPoolSize; theIndex > inPoolSize; theIndex -- )
		{
			( ( EventWorkerThread )( mPool.remove( 0 ) ) ).terminate( false );
		}
		mPool.trimToSize();
		mMaxPoolSize = inPoolSize;
	}

	private void terminateThreads()
	{
		terminateThreads( false );
	}

	private synchronized void terminateThreads( boolean inIdleOnly )
	{
		int	theTerminatedCount = 0;
		for ( int theIndex = mPoolSize - 1; theIndex >= 0; theIndex -- )
		{
			EventWorkerThread theThread =
				( EventWorkerThread )mPool.get( theIndex );		
			if ( theThread.terminate( inIdleOnly ) )
			{
				mPool.remove( theIndex );
				theTerminatedCount ++;
			}
		}
		if ( theTerminatedCount > 0 )
		{
			mQueue.releaseWaitingThreads();
			mPoolSize -= theTerminatedCount;
		}
	}

	private void waitForLiveThreads()
	{
		for ( int theIndex = 0; theIndex < 5; theIndex ++ )
		{
			if ( haveLiveThread() )
			{
				try
				{
					wait( 1000 );
				}
				catch( Exception theException ){}
			}
			else
			{
				break;
			}
		}
	}

	class IdleDetector implements Runnable
	{
		public void run()
		{
			terminateThreads( true );
		}
	}
}

