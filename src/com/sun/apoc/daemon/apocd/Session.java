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

import com.sun.apoc.daemon.config.DaemonConfig;
import com.sun.apoc.daemon.localdatabase.*;
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transport.*;

import com.sun.apoc.spi.entities.*;
import com.sun.apoc.spi.environment.*;

import java.io.*;
import java.nio.*;
import java.net.*;
import java.util.*;
import java.util.logging.*;


public class Session
{
	private Client	mClient;
	private String	mSessionId;
	private HashMap	mListeners	= new HashMap();
	private Cache[]	mCaches;
	private Object	mLock		= new Object();
    private Hashtable mEntities = null ;

	private static final String	sSep			= ".";
	private static final Random	sRandom			= new Random();
	private static int			sAuthBufferSize	= 20;
	private static int			sCantLock		= 0;
	private static int			sUnlocked		= 1;
	private static int			sLocked			= 2;
	private int					mState			= sUnlocked;

	Session() {}

	public Session( final Client inClient,
					final String inUserId,
					String inEntityName )
		throws APOCException
	{
        Hashtable theEntities = new Hashtable();

        if (inEntityName == null) { inEntityName = inUserId ; }
        theEntities.put( EnvironmentConstants.USER_SOURCE, inEntityName );
        initialise( inClient, inUserId, theEntities );
    }
    public Session( final Client inClient,
                    final String inUserId,
                    final Hashtable inEntities)
        throws APOCException
    {
        initialise( inClient, inUserId, inEntities );
    }
    private void initialise( final Client inClient, 
                             final String inUserId,
                             Hashtable inoutEntities) throws APOCException {
		APOCLogger.finer( "Sess001" );
		try
		{
			authenticate( inClient, inUserId );
			mClient		= inClient;
			mSessionId	= String.valueOf ( sRandom.nextLong() );
            if (!inoutEntities.containsKey(EnvironmentConstants.HOST_SOURCE)) {
                String hostName = DaemonConfig.getHostIdentifier() ;
               
                if (hostName != null) {
                    inoutEntities.put(EnvironmentConstants.HOST_SOURCE,
                                      hostName) ;
                }
            }
            mEntities = inoutEntities ;
			mCaches		= CacheFactory
							.getInstance()
								.openCaches(
									inUserId,
									mEntities,
                                    this,
									new SaslAuthCallbackHandler( mClient ) );
		}
		catch( APOCException theException )
		{
			close();
			throw theException;
		}
		APOCLogger.finer( "Sess002", toString( inUserId ) );
	}

	public Cache[]	getCaches()		{ return mCaches;		 }
	public Client	getClient()		{ return mClient;		 }
	public String	getSessionId()	{ return mSessionId;	 }

	public void addListeners( final Vector	inComponentNames,
							  final String	inClientData )
	{
		synchronized( mListeners )
		{
			for ( int theIndex = 0;
				  theIndex < inComponentNames.size();
				  ++theIndex )
			{
				mListeners.put( inComponentNames.elementAt( theIndex ),
								inClientData );
			}
		}
	}

	public void removeListeners( final Vector inComponentNames )
	{
		synchronized( mListeners )
		{
			for ( int theIndex = 0;
				  theIndex < inComponentNames.size(); 
				  ++ theIndex )
			{
				mListeners.remove( inComponentNames.elementAt( theIndex ) );
			}
		}
	}

	public void close()
	{
		synchronized( mLock )
		{
			if ( mState == sLocked )
			{
				try
				{
					mLock.wait();
				}
				catch( Exception e )
				{}
			}
			mState = sCantLock;
			mLock.notify();
		}
		if ( mCaches != null )
		{
			CacheFactory.getInstance().closeCaches( mCaches, this );
			mCaches = null;
		}
		if ( mSessionId != null )
		{
			APOCLogger.finer( "Sess003", mSessionId );
		}
	}

	public String getListenerClientData( final String inComponentName )
	{
		return ( String )mListeners.get( inComponentName );
	}

	public String getListenerComponentName( final String inComponentName )
	{
		synchronized( mListeners )
		{
			String theComponentName =
				mListeners.containsKey( inComponentName ) ? inComponentName :
															null;
			if ( theComponentName == null )
			{
				final StringTokenizer	theTokenizer	=
					new StringTokenizer( inComponentName, sSep );
				final StringBuffer		theBuffer		= new StringBuffer();
				int	theCount = theTokenizer.countTokens();
				for ( int theIndex = 0; theIndex < theCount - 1 ; theIndex ++ )
				{
					final String theContainerName = 
						theBuffer.append( theTokenizer.nextToken() )
                                     .append( sSep )
                                     .toString();
					if ( mListeners.containsKey( theContainerName ) )
					{
						theComponentName = theContainerName;
						break;
					}
				}
			}
			return theComponentName;
		}
	}

	//
	// lock() & unlock() are provided to support the case where a 
	// change detection transaction and a destroy session transaction are
	// happening at the same time. lock() will allow a change detection to
	// continue using the session until it's complete without fear of the
	// session being closed. Once complete, the change detection transaction
	// will unlock the session, allowing it to be destroyed.
	//
	public boolean lock()
	{
		boolean isLocked = false;
		synchronized( mLock )
		{
			if ( mState == sUnlocked )
			{
				mState = sLocked;
				isLocked = true;
			}
		}
		return isLocked;
	}

	public void unlock()
	{
		synchronized( mLock )
		{
			mState = sUnlocked;
			mLock.notify();		
		}
	}

    public void sendNotification(UpdateItem aUpdate) {
        String component = aUpdate.getComponentName() ;
        String listenerComponent = getListenerComponentName(component) ;

        if (listenerComponent == null) { return ; }
        int notificationType = -1 ;
        StringBuffer logMessage = new StringBuffer(" sessionId      = ")
                                           .append(mSessionId).append("\n")
                                           .append(" component name = ")
                                           .append(component).append("\n")
                                           .append(" type           = ") ;

        switch (aUpdate.getUpdateType()) {
            case UpdateItem.ADD:
                notificationType = APOCSymbols.sSymAddNotification ;
                logMessage.append("Add") ;
                break ;
            case UpdateItem.REMOVE:
                notificationType = APOCSymbols.sSymRemoveNotification ;
                logMessage.append("Remove") ;
                break ;
            case UpdateItem.MODIFY:
                notificationType = APOCSymbols.sSymModifyNotification ;
                logMessage.append("Modify") ;
                break ;
            default:
                notificationType = -1 ;
                logMessage.append("Unknown") ;
                break ;
        }
        APOCLogger.finer("Cdtn003", logMessage.toString()) ;
        final Notification notification = 
            (Notification) MessageFactory.createNotification(mSessionId,
                                                             notificationType) ;

        notification.setComponentName(component) ;
        notification.setClientData(getListenerClientData(listenerComponent)) ;
        mClient.getClientChannel().write(notification.serialise()) ;
    }
    
	private void authenticate( final Client	inClient,
							   final String	inUserId )
		throws APOCException
	{
		APOCAuthenticator theAuthenticator = null;
		try
		{
			theAuthenticator = new APOCAuthenticator( inUserId );
			final Messenger	theMessenger = new Messenger();
			theMessenger.setClientChannel( inClient.getClientChannel() );
			theMessenger.sendResponse(
				MessageFactory.createResponse(
					APOCSymbols.sSymRespSuccessContinueLocalAuth,
					null,
					new String( theAuthenticator.getChallenge() ) ),
				false );
			final CredentialsProviderMessage theMessage =
				( CredentialsProviderMessage )theMessenger.receiveRequest();
			theAuthenticator.processResponse(
				theMessage.getCredentials().getBytes() );
		}
		finally
		{
			if ( theAuthenticator != null )
			{
				theAuthenticator.cleanup();
			}
		}
	}

	private String toString( final String inUserId )
	{
		StringBuffer theMessage = new StringBuffer()
			.append( " userId       = " )
			.append( inUserId )
			.append( "\n" )

			.append( " sessionId    = ")
			.append( mSessionId )
			.append( "\n" );
        for ( int theIndex = 0 ; theIndex < mCaches.length ; ++ theIndex )
        {
			theMessage.append( mCaches[ theIndex ].toString() ).append( "\n ");
        }
		return theMessage.toString();
	}
}
