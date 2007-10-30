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
#include "papiConnection.h"
#include "papiSAXParserContext.h"
#include "papiStringList.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#ifndef WNT
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

typedef struct NotificationThreadArg
{
	PAPIListener	mListener;
	PAPIEvent *		mEvent;
	void *			mUserData;
} NotificationThreadArg;

static PAPIConnection * allocConnection (
							PAPIConnectionListener	inListener,
							void *					inUserDat,
							PAPIListener			inInternalListener,
							void *					inInternalListenerData,
							PAPISocket				inFD,
							PAPIStatus *			outStatus );

static int	daemonConnect(					int				inRetryCount );

static void createNotificationThread(		PAPIListener	inListener,
									  		PAPIEventType	inType,
									  		const char *	inComponentName,
									  		void *			inUserData );

static PAPIListenerThreadStatus getListenerThreadStatus(
											PAPIConnection *inConnection );

static PAPIMessage * getResponse(			PAPIConnection *inConnection,
								  			PAPIStatus *	outStatus );

static PAPIMessage * getResponseMessage(	PAPIConnection *inConnection,
								  		 	PAPIStatus *	outStatus );

static void			 handleSASLAuth(		PAPIConnection *inConnection,
											PAPIMessage *	inChallenge );

static int	  haveMessage(					PAPIConnection *inConnection );

static void * listener(						void *			inConnection );

static void * notifier(						void * inNotificationThreadArg );

static void sendInternalNotification(		PAPIConnection *inConnection,
											PAPIMessage *	inMessage );

static void sendNotification(				PAPIConnection *inConnection,
							  				PAPIMessage *	theMessage );

static void setListenerThreadStatus(		PAPIConnection *inConnection,
									 		PAPIListenerThreadStatus inStatus );

static void startListeningThread(			PAPIConnection *inConnection,
	   							  			PAPIStatus *	outStatus );

static void stopListeningThread(			PAPIConnection *inConnection,
								 			PAPIStatus *	outStatus );

static void takeFive(						void );

static int sDefaultDaemonPort = 38900;

PAPIListenerId connectionAddListener( PAPIConnection *	inConnection,
									  const char *		inComponentName,
									  PAPIListener		inListener,
									  void *			inUserData,
									  PAPIStatus *		outStatus )
{
	PAPIListenerId theListenerId	= 0;
	*outStatus			= PAPISuccess;
	if ( lockMutex( inConnection->mListenersMutex ) == 0 )
	{
		theListenerId = addListenerList( &( inConnection->mListeners ),
						inComponentName,
						inListener,
						inUserData );
		*outStatus = theListenerId == 0 ?
							PAPIOutOfMemoryFailure :
							PAPISuccess;
		unlockMutex( inConnection->mListenersMutex );
	}
	else
	{
		*outStatus = PAPIUnknownFailure;
	}
	return theListenerId;
}

void connectionRemoveListener( PAPIConnection *	inConnection,
							   PAPIListenerId	inListenerId,
							   PAPIStatus *		outStatus )
{
	*outStatus = PAPISuccess;
	if ( lockMutex( inConnection->mListenersMutex ) == 0 )
	{
		removeListenerList( & ( inConnection->mListeners ),
							( PAPIListenerList * )inListenerId );
		unlockMutex( inConnection->mListenersMutex );
	}
	else
	{
		*outStatus = PAPIUnknownFailure;
	}
}

void connectionSetSASLAuthHandler( PAPIConnection *			inConnection,
								   PAPIAuthHandler 			inHandler,
								   void *					inHandlerUserData )
{
	inConnection->mAuthHandler						= inHandler;
	inConnection->mAuthHandlerUserData->mUserData	= inHandlerUserData;
}

void deleteConnection( PAPIConnection * inConnection )
{
	if ( inConnection != 0 )
	{
		PAPIStatus theStatus;
		stopListeningThread( inConnection, &theStatus );
		deleteListenerList( inConnection->mListeners );
		deleteMutex( inConnection->mListenersMutex );
		deleteMutex( inConnection->mSavedMessageMutex );
		deleteMutex( inConnection->mListenerThreadStatusMutex );
		deleteMutex( inConnection->mConnectionStateChangeMutex );
		if ( inConnection->mAuthHandlerUserData != 0 )
		{
			free( ( void * )inConnection->mAuthHandlerUserData );
		}
		free( ( void * )inConnection );
	}
}

PAPIConnection * newConnection( PAPIConnectionListener	inConnectionListener,
								void *					inUserData,
								PAPIListener 			inInternalListener,
								void * 					inInternalListenerData,
								PAPIStatus *			outStatus )
{
	PAPIStatus			theStatus;
	PAPIConnection *	theConnection	= 0;
	int					theSocket		= daemonConnect( 10 );

	if ( theSocket == INVALID_SOCKET )
	{
		*outStatus = PAPIConnectionFailure;
	}
	else
	{
		*outStatus = PAPISuccess;
	}
	if ( *outStatus == PAPISuccess || isDaemonEnabled() == 1 )
	{
		theConnection = allocConnection( inConnectionListener,
									 	 inUserData,
										 inInternalListener,
										 inInternalListenerData,
									 	 theSocket,
									 	 &theStatus );
		if ( theStatus == PAPISuccess )
		{
			startListeningThread( theConnection, &theStatus );
		}
		if ( theStatus != PAPISuccess )
		{
			deleteConnection( theConnection );
			theConnection = 0;
			*outStatus = theStatus;	
		}
	}
	return theConnection;
}

int readBytes( PAPIConnection * inConnection, char * inBuffer, int inLen )
{
	int theTotalByteCount	= 0;
	while ( inLen > 0 )
	{
		int theByteCount = -1;
		if ( haveMessage( inConnection ) != -1 )
		{
			theByteCount =
				recv(inConnection->mFD, inBuffer + theTotalByteCount, inLen, 0);
			if ( theByteCount == -1 && errno == EINTR )
			{
				theByteCount = 0;
			}
		}
		if ( theByteCount == SOCKET_ERROR )
		{
			theTotalByteCount = -1;
			break;
		}
		else
		{
			inLen				-= theByteCount;
			theTotalByteCount	+= theByteCount;
		}
	}
	return theTotalByteCount;
}

int readContentLength( PAPIConnection * inConnection )
{
	char	theBuffer[ 15 ];
	int		theIndex;
	int		theContentLength	= -1;
	if ( readBytes( inConnection, theBuffer, 15 ) != 15 )
	{
		return -1;
	}
	if ( strncmp( theBuffer,
				  CONTENTLENGTH,
				  sizeof( CONTENTLENGTH ) - 1 ) != 0)
	{
		return -1;
	}
	theIndex = 0;
	while ( readBytes( inConnection, theBuffer + theIndex, 1 ) == 1 )
	{
		if ( theBuffer[ theIndex ] == '\n' )
		{
			theBuffer[ theIndex - 1 ] = 0;
			theContentLength = atoi( theBuffer );
			break;
		}
		theIndex ++;
	}
	return theContentLength;
}

PAPIMessage * sendRequest( PAPIConnection *	inConnection,
						   PAPIMessage *	inRequestMessage,
						   PAPIStatus *		outStatus )
{
	int				theContentLength;
	char 			theContentLengthString[ MAX_CONTENT_LENGTH_SIZE ];
	char *			theRequest;
	PAPIMessage *	theResponse = 0;

	*outStatus =	PAPISuccess;

	if ( inConnection == 0 || inConnection->mFD == INVALID_SOCKET )
	{
		*outStatus = PAPIConnectionFailure;
		return 0;
	}
	
	theRequest = messageToString( inRequestMessage );
	if ( theRequest != 0 && ( theContentLength = strlen( theRequest ) ) > 0 )
	{
		char *	theBuf;
		int		theSize;
		snprintf( theContentLengthString,
				  MAX_CONTENT_LENGTH_SIZE,
			      "%s%d\r\n",
			      CONTENTLENGTH, 
			      theContentLength );

		deleteMessage( inConnection->mSavedMessage );
		inConnection->mSavedMessage = 0;
		theSize = strlen( theContentLengthString ) + theContentLength + 1;
		theBuf = ( char * )malloc( theSize );
		if ( theBuf == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			free( ( void * )theRequest );
			return 0;
		}
		snprintf( theBuf, theSize, "%s%s", theContentLengthString, theRequest );
		if ( send( inConnection->mFD,
		  	   	( void * )theBuf,
		  	   	theSize - 1,
		  	   	0 ) == SOCKET_ERROR )
		{
			*outStatus = PAPIConnectionFailure;
			free( ( void * )theBuf );
			free( ( void * )theRequest );
			return 0;
		}
		free( ( void * )theBuf );
		free( ( void * )theRequest );
		theResponse = getResponse( inConnection, outStatus );
	}
	else
	{
		free( ( void * )theRequest );
	}
	return theResponse;
}

void setBlocking( PAPIConnection * inConnection, int inIsBlocking )
{
	inConnection->mFDPollTimeout = inIsBlocking == 1 ?
		-1 : getTransactionTimeout();
}

PAPIConnection* allocConnection( PAPIConnectionListener	inConnectionListener,
								 void *					inUserData,
								 PAPIListener			inInternalListener,
								 void *					inInternalListenerData,
								 PAPISocket				inSocket,
								 PAPIStatus *			outStatus  )
{
	PAPIConnection * theConnection =
		( PAPIConnection * )malloc( sizeof( PAPIConnection ) );
	if ( theConnection != 0 )
	{
		theConnection->mFD						= inSocket;
		theConnection->mFDPollTimeout			= getTransactionTimeout();
		theConnection->mListeners				= 0;
		theConnection->mSavedMessage			= 0;
		theConnection->mListenerThreadStatus	= NOT_RUNNING;
		theConnection->mConnectionListener		= inConnectionListener;
		theConnection->mUserData				= inUserData;
		theConnection->mListenersMutex			= newMutex();
		theConnection->mSavedMessageMutex		= newMutex();
		theConnection->mListenerThreadStatusMutex= newMutex();
		theConnection->mConnectionStateChangeMutex= newMutex();
		theConnection->mInternalListener		= inInternalListener;
		theConnection->mInternalListenerData	= inInternalListenerData;
		theConnection->mAuthHandler				= 0;
		theConnection->mAuthHandlerUserData		=
			( PAPIAuthHandlerData * )malloc( sizeof( PAPIAuthHandlerData ) );
		*outStatus = PAPISuccess;
	}
	else
	{
		*outStatus = PAPIOutOfMemoryFailure;
	}
	return theConnection;
}

PAPISocket daemonConnect( int inRetryCount )
{
	int					thePort	= getDaemonPort();
	PAPISocket			theSocket;
	PAPIConnectResult	theRC	=
		getConnectedSocket( thePort, SOCK_STREAM, 0, &theSocket );

	if ( theRC == PAPIConnectRejected ||
	     ( theRC == PAPIConnectFailed && startDaemon()	== 0 ) )
	{
		theRC =
			getConnectedSocket( thePort, SOCK_STREAM, inRetryCount, &theSocket);
	}
	return theSocket;
}

void createNotificationThread( PAPIListener  inListener,
                               PAPIEventType inType,
                               const char *	 inComponentName,
                               void *        inUserData )
{
	NotificationThreadArg * theArg =
		( NotificationThreadArg * )malloc( sizeof( NotificationThreadArg ) );	
	if ( theArg != 0 )
	{
		theArg->mEvent = ( PAPIEvent * )malloc( sizeof( PAPIEvent ) );
		if ( theArg->mEvent != 0 )
		{			
			theArg->mEvent->eventType		= inType;
        	theArg->mEvent->componentName	= strdup( inComponentName );
			theArg->mListener				= inListener;
			theArg->mUserData				= inUserData;

			createThread( notifier, ( void *)theArg );
		}
	}
}

PAPIListenerThreadStatus getListenerThreadStatus( 
							PAPIConnection * inConnection )
{
	PAPIListenerThreadStatus theListenerThreadStatus = NOT_RUNNING;
	if ( lockMutex( inConnection->mListenerThreadStatusMutex ) == 0 )
	{
		theListenerThreadStatus = inConnection->mListenerThreadStatus;
		unlockMutex( inConnection->mListenerThreadStatusMutex );
	}
	return theListenerThreadStatus;	
}

PAPIMessage * getResponse( PAPIConnection * inConnection,
						   PAPIStatus *		outStatus )
{
	PAPIMessage * theResponse = 0;
	*outStatus = PAPISuccess;
	while ( theResponse == 0 && inConnection->mFD != INVALID_SOCKET )
	{
		if ( lockMutex( inConnection->mSavedMessageMutex ) == 0 )
		{
			theResponse = inConnection->mSavedMessage;
			inConnection->mSavedMessage = 0;
			unlockMutex( inConnection->mSavedMessageMutex );
		}
		takeFive();
	}
	if ( theResponse == 0 || inConnection->mFD == INVALID_SOCKET )
	{
		*outStatus = PAPIConnectionFailure;
	}
	return theResponse;
}

PAPIMessage * getResponseMessage( PAPIConnection *	inConnection,
						   		  PAPIStatus *		outStatus )
{
	PAPISAXUserData			theUserData;
	PAPISAXParserContext	theSAXParserContext;
	PAPIMessage *			theResponse = 0;

	theUserData.mMessage = 0;
	theSAXParserContext	 =
		newSAXParserContext( inConnection, &theUserData );
	if ( parse( theSAXParserContext ) != 0 )
	{
		if ( inConnection->mFD == INVALID_SOCKET )
		{
			*outStatus = PAPIConnectionFailure;
		}
		else
		{
			*outStatus = PAPIUnknownFailure;
		}
	}
	deleteSAXParserContext( theSAXParserContext );
	return theUserData.mMessage;
}

void handleSASLAuth( PAPIConnection * inConnection, PAPIMessage * inChallenge )
{
	PAPIAuthHandler			theHandler	= inConnection->mAuthHandler;
	PAPIAuthHandlerData *	theData		= inConnection->mAuthHandlerUserData;
	theData->mChallenge = inChallenge;	
	/* avoid calling handler repeatedly during sasl auth */
	inConnection->mAuthHandler = 0;	
	createThread( theHandler, theData );
}

int haveMessage( PAPIConnection * inConnection )
{
	int	bHaveMessage = 0;
	int	theRC;

	theRC = socketPoll( inConnection->mFD, inConnection->mFDPollTimeout );
	if ( theRC == 1 )
	{
		char theByte;
		if ( recv( inConnection->mFD, &theByte, 1, MSG_PEEK ) != 1 )
		{
			if ( errno != EINTR )
			{
				bHaveMessage = -1;
			}
		}
		else
		{
			bHaveMessage = 1;
		}
	}
	else
	{
		bHaveMessage = -1;
	}
	return bHaveMessage;
}

void * listener( void * inConnection )
{
#ifndef WNT
	sigset_t			theSigset;
#endif
	PAPIConnection *	theConnection = ( PAPIConnection * )inConnection;

#ifndef WNT
	sigemptyset( &theSigset );
	sigaddset( &theSigset, SIGHUP );
	pthread_sigmask( SIG_BLOCK, &theSigset, 0 );
#endif
	setListenerThreadStatus( inConnection, RUNNING );
	while ( getListenerThreadStatus( inConnection ) == RUNNING )
	{
		PAPIStatus		theStatus;
		PAPIMessage *	theMessage;
		if ( theConnection->mFD != INVALID_SOCKET )
		{
			int	bHaveMessage = haveMessage( theConnection );
			if ( bHaveMessage == 0 )
			{
				continue;
			}
			else if ( bHaveMessage == -1 )
			{
				papiClose( theConnection->mFD );
				theConnection->mFD = INVALID_SOCKET;
				if ( lockMutex( theConnection->mConnectionStateChangeMutex )
				     == 0 )
				{
					theConnection->mConnectionListener( PAPIDisconnected,
													theConnection->mUserData );
					unlockMutex( theConnection->mConnectionStateChangeMutex );
				}
				if ( isDaemonEnabled() != 1 )
				{
					break;
				}
				continue;
			}	
			theMessage = getResponseMessage( theConnection, &theStatus );
			if ( theMessage != 0 )
			{
				if ( theMessage->mName == PAPImodifyNotification	||
				 	 theMessage->mName == PAPIaddNotification		||
				 	 theMessage->mName == PAPIremoveNotification )
				{
					sendInternalNotification( theConnection, theMessage );
					sendNotification( theConnection, theMessage );
					deleteMessage( theMessage );
				}
				else if ( theMessage->mName == PAPIRespSuccessContinueSASLAuth&&
				          theConnection->mAuthHandler != 0 )
				{
					handleSASLAuth( theConnection, theMessage );
				}
				else
				{
					if ( lockMutex( theConnection->mSavedMessageMutex ) == 0 )
					{
						theConnection->mSavedMessage = theMessage;
						unlockMutex( theConnection->mSavedMessageMutex );
					}
				}
			}
		}
		else
		{
			if ( lockMutex( theConnection->mConnectionStateChangeMutex ) == 0 )
			{
				theConnection->mFD = daemonConnect( 1 );
				setBlocking( theConnection, 0 );
				if ( theConnection->mFD != INVALID_SOCKET )
				{
					theConnection->mConnectionListener( PAPIConnected,
													theConnection->mUserData );
				}
				unlockMutex( theConnection->mConnectionStateChangeMutex );
				if ( theConnection->mFD == INVALID_SOCKET )
				{
					papiSleep( 10 );
				}
			}
		}
	}
	setListenerThreadStatus( inConnection, NOT_RUNNING );
	return 0;
}

void * notifier( void * inArg )
{
	NotificationThreadArg * theArg = ( NotificationThreadArg * )inArg;
	theArg->mListener( theArg->mEvent, theArg->mUserData );
	free( ( void *)theArg->mEvent->componentName );
	free( ( void *)theArg->mEvent );
	free( ( void *)theArg );
	return 0;
}

void sendInternalNotification( PAPIConnection * inConnection,
							   PAPIMessage *	inMessage )
{
	PAPIMessageName theName = inMessage->mName;
	if ( theName == PAPIaddNotification || theName == PAPIremoveNotification )
	{
		PAPIStringList * theComponentNames	=
			getParamValue( inMessage, PAPIParamComponentName );
		PAPIStringList * theTmpList			= theComponentNames;
		while ( theTmpList != 0 )
		{
			PAPIEvent theEvent;
			theEvent.eventType =
				theName == PAPIaddNotification ?
					PAPIComponentAdd : PAPIComponentRemove;
			theEvent.componentName = theTmpList->data;
			inConnection->mInternalListener(
							&theEvent,
							inConnection->mInternalListenerData );
			theTmpList = theTmpList->next;
		}
		deleteStringList( theComponentNames );
	}
}

void sendNotification( PAPIConnection * inConnection, PAPIMessage * inMessage )
{
	PAPIStringList *	theComponentNames =
		getParamValue( inMessage, PAPIParamComponentName );
	PAPIStringList * theTmpList = theComponentNames;
	while ( theComponentNames != 0 )
	{
		PAPIListenerList *	theListenerList;

		if ( lockMutex( inConnection->mListenersMutex ) == 0 )
		{
			theListenerList = inConnection->mListeners;
			while ( theListenerList != 0 )
			{
				int theLen = strlen( theListenerList->mComponentName );
				if ( strcmp( theListenerList->mComponentName,
						     theComponentNames->data ) == 0 				||
					 ( theListenerList->mComponentName[ theLen - 1 ] == '.' &&
					   strncmp( theListenerList->mComponentName,
								theComponentNames->data,
								theLen ) == 0 ) )
				{
					PAPIEventType theEventType;
					switch( inMessage->mName )
					{
						case PAPImodifyNotification:
							theEventType = PAPIComponentModify;
							break;

						case PAPIaddNotification:
							theEventType = PAPIComponentAdd;
							break;

						case PAPIremoveNotification:
							theEventType = PAPIComponentRemove;
							break;
					}
					createNotificationThread( theListenerList->mListener,
											  theEventType,
										  	  theComponentNames->data,
										  	  theListenerList->mUserData);
				}
				theListenerList = theListenerList->mNext;
			}
			theComponentNames = theComponentNames->next;		
			unlockMutex( inConnection->mListenersMutex );
		}
	}
	deleteStringList( theTmpList );
}

void setListenerThreadStatus( PAPIConnection *           inConnection,
							  PAPIListenerThreadStatus	inStatus )
{
	if ( lockMutex(inConnection->mListenerThreadStatusMutex ) == 0 )
	{
		inConnection->mListenerThreadStatus = inStatus;
		unlockMutex( inConnection->mListenerThreadStatusMutex );
	}
}

void startListeningThread( PAPIConnection *	inConnection,
						   PAPIStatus *		outStatus )
{
	inConnection->mListenerThread = createThread( listener, inConnection );
	if ( inConnection->mListenerThread != 0 )
	{
		*outStatus = PAPISuccess;
	}
	else
	{
		*outStatus = PAPIUnknownFailure;
	}
}

void stopListeningThread( PAPIConnection * inConnection, PAPIStatus * outStatus)
{
	*outStatus = PAPISuccess;
	if ( getListenerThreadStatus( inConnection ) == RUNNING )
	{
		if ( lockMutex( inConnection->mListenersMutex ) == 0 )
		{
			destroyThread( inConnection->mListenerThread );
			papiClose( inConnection->mFD );
			unlockMutex( inConnection->mListenersMutex );
		}
		else
		{
			*outStatus = PAPIUnknownFailure;
		}
	}
}

void takeFive( void )
{
#ifndef WNT

	struct timespec theTimeout;

	theTimeout.tv_sec	= 0;
	theTimeout.tv_nsec	= 5000000;
	nanosleep( &theTimeout, NULL);

#else

	Sleep( 50 );

#endif
}
