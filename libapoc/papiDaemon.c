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
#include "papiDaemon.h"
#include "papiMessage.h"
#include "papiUtils.h"
#include "papiEntityList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PAPIMessage * createCreateSessionRequest(
									const char *	        inUserId,
									const PAPIEntityList *	inEntityId,
									const char *	        inCredentials );

static void createSession(  		PAPIDaemon *	inDaemon,
									PAPIStatus *	outStatus );

static void daemonReaddListener(	PAPIDaemon *	inDaemon,
									const char *	inComponentName,
									PAPIStatus *	outStatus );

static void destroySession(			PAPIDaemon *	inDaemon );

static const char * getLocalCreds(	const char *	inLocalChallengeFile,
									PAPIStatus *	outStatus );

static void * saslAuthHandler(		void *			inData );

static int sLocalCredentialsBufferSize = 21;

PAPIListenerId daemonAddListener( PAPIDaemon *	inDaemon,
								  const char *	inComponentName,
								  PAPIListener 	inListener,
								  void *		inUserData,
								  PAPIStatus *	outStatus )
{
	PAPIListenerId theListenerId = 0;
	daemonReaddListener( inDaemon, inComponentName, outStatus );
	if ( *outStatus == PAPISuccess )
	{
		theListenerId = connectionAddListener( inDaemon->mConnection,
											   inComponentName,	
											   inListener,	
											   inUserData,
											   outStatus );
	}
	return theListenerId;
}

PAPIStringList * daemonList( PAPIDaemon * inDaemon, PAPIStatus * outStatus )
{
	PAPIStringList * theList = 0;
	if ( inDaemon->mSessionId != 0 )
	{
		PAPIMessage *	theRequest;
		PAPIMessage *	theResponse;

		theRequest = newMessage( PAPIReqList );
		if ( theRequest == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			return 0;
		}
		addParam( theRequest, PAPIParamSessionId, inDaemon->mSessionId );	
		theResponse = sendRequest( inDaemon->mConnection,theRequest,outStatus );	
		if ( theResponse != 0 )
		{
			theList = getParamValue( theResponse, PAPIParamComponentName );
		}

		deleteMessage( theRequest );
		deleteMessage( theResponse );
    }
	return theList;
}

PAPIStringList * daemonRead( PAPIDaemon * inDaemon,
							 const char * inComponentName,
							 PAPIStatus * outStatus )
{
	PAPIStringList * theLayers = 0;
	if ( inDaemon->mSessionId != 0 )
	{
		PAPIMessage *	theRequest;
		PAPIMessage *	theResponse;

		theRequest = newMessage( PAPIReqRead );
		if ( theRequest == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			return 0;
		}
		addParam( theRequest, PAPIParamSessionId, inDaemon->mSessionId );	
		addParam( theRequest, PAPIParamComponentName, inComponentName );
		theResponse = sendRequest( inDaemon->mConnection,theRequest,outStatus );	
		if ( theResponse != 0 )
		{
			theLayers = getParamValue( theResponse, PAPIParamLayer );
		}
	
		deleteMessage( theRequest );
		deleteMessage( theResponse );
    }
	return theLayers;
}

void daemonReconnect( PAPIDaemon *	inDaemon,
					  PAPIStatus *	outStatus )
{
	setBlocking( inDaemon->mConnection, 0 );
	createSession( inDaemon, outStatus );
	if ( *outStatus == PAPISuccess )
	{
		PAPIListenerList * theList = inDaemon->mConnection->mListeners;
		setBlocking( inDaemon->mConnection, 1 );
		while ( theList != 0 )
		{
			daemonReaddListener( inDaemon, theList->mComponentName, outStatus);
			theList = theList->mNext;
		}
	}
}

void daemonRemoveListener( PAPIDaemon *		inDaemon,
						   PAPIListenerId 	inListenerId,
						   PAPIStatus *		outStatus )
{
	*outStatus = PAPISuccess;
	if ( inDaemon->mSessionId != 0 && inListenerId != 0 )
	{
		PAPIMessage *		theRequest;
		PAPIMessage *		theResponse;
		PAPIListenerList *	theListenerList =
			( PAPIListenerList * )inListenerId;
		const char *		theComponentName = theListenerList->mComponentName;

		if ( theComponentName == 0 )
		{
			return;
		}

		theRequest = newMessage( PAPIReqRemoveListener );
		if ( theRequest == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			return;
		}
		addParam( theRequest, PAPIParamSessionId, inDaemon->mSessionId );	
		addParam( theRequest, PAPIParamComponentName, theComponentName );
		theResponse = sendRequest( inDaemon->mConnection,theRequest,outStatus );	
		deleteMessage( theRequest );
		deleteMessage( theResponse );

		if ( *outStatus == PAPISuccess )
		{
			connectionRemoveListener( inDaemon->mConnection,
							  		  inListenerId,
							  		  outStatus );
		}
	}
}

void deleteDaemon( PAPIDaemon * inDaemon )
{
	if ( inDaemon != 0 )
	{
		if ( inDaemon->mUserId != 0 )
		{
			free( ( void *)inDaemon->mUserId );
		}
		if ( inDaemon->mEntities != 0 )
		{
            papiFreeEntityList( inDaemon->mEntities );
		}
		if ( inDaemon->mSessionId != 0 )
		{
			destroySession( inDaemon );
			free( ( void * )inDaemon->mSessionId );
		}
		deleteConnection( inDaemon->mConnection );
		free( inDaemon );
	}
}

static void fillDaemonEntities( PAPIDaemon * inDaemon )
{
    PAPIEntityList *theEntity = inDaemon->mEntities;

    while ( theEntity != 0 )
    {
        switch ( theEntity->entityType )
        {
            case PAPIEntityUser:
                if ( theEntity->entityId == 0 ) 
                {
                    theEntity->entityId = strdup(inDaemon->mUserId);
                }
                break ;
            default:
                break ;
        }
        theEntity = theEntity->next;
    }
}

PAPIDaemon * newDaemon(	const PAPIEntityList *	inEntities,
						PAPIConnectionListener	inListener,
						void *					inUserData,
						PAPIListener			inInternalListener,
						void *					inInternalListenerData,
						PAPIStatus *			outStatus )
{
	PAPIDaemon * theDaemon = ( PAPIDaemon *)malloc( sizeof( PAPIDaemon ) );
	if ( theDaemon == 0 )
	{
		*outStatus = PAPIOutOfMemoryFailure;
		return 0;
	}
	memset( theDaemon, 0, sizeof( PAPIDaemon ) );
	theDaemon->mUserId = getUserName();
	if ( theDaemon->mUserId == 0 )
	{
		*outStatus = PAPIUnknownFailure;
		deleteDaemon( theDaemon );
		return 0;
	}
	if ( inEntities != 0 )
	{
		theDaemon->mEntities = papiCopyEntityList( inEntities );
		if ( theDaemon->mEntities == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			deleteDaemon( theDaemon );
			return 0;
		}
        fillDaemonEntities( theDaemon );
	}
		

	theDaemon->mConnection = newConnection( inListener,
											inUserData,
											inInternalListener,
											inInternalListenerData,
											outStatus );
	if ( *outStatus == PAPISuccess )
	{
		daemonReconnect( theDaemon, outStatus );
	}
	if ( *outStatus != PAPISuccess && *outStatus != PAPIConnectionFailure )
	{
		deleteDaemon( theDaemon ); 
		theDaemon = 0;
	}
	return theDaemon;
}

PAPIMessage * createCreateSessionRequest( const char *	         inUserId,
										  const PAPIEntityList * inEntities,
										  const char *	         inCredentials )
{
	PAPIMessage * theRequest = newMessage( PAPIReqCreateSessionExt );
	if ( theRequest != 0 )
	{
        const PAPIEntityList * entity = inEntities;

		addParam( theRequest, PAPIParamUserId,		inUserId );
		addParam( theRequest, PAPIParamCredentials,	inCredentials );
        while ( entity != 0 )
        {
            addParam( theRequest, PAPIParamEntityType, 
                      getEntityTypeString( entity->entityType ) );
		    addParam( theRequest, PAPIParamEntityId, entity->entityId );
            entity = entity->next;
        }
	}
	return theRequest;
}

void createSession( PAPIDaemon * inDaemon,
					PAPIStatus * outStatus )
{
	int				shouldContinue	= 1;
	char *			theCreds		= 0;
	void *			theSASLContext	= 0;

	connectionSetSASLAuthHandler( inDaemon->mConnection, 0, 0 );
	while ( shouldContinue == 1 )
	{
		PAPIMessage * theResponse;
		PAPIMessage * theRequest =
			createCreateSessionRequest( inDaemon->mUserId,
										inDaemon->mEntities,
										theCreds );
		if ( theCreds != 0 )
		{
			free( ( void * )theCreds );
		}
		if ( theRequest == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			connectionSetSASLAuthHandler( inDaemon->mConnection,
										  saslAuthHandler,
										  inDaemon );
			return;
		}
		theResponse = sendRequest(inDaemon->mConnection, theRequest, outStatus);
		deleteMessage( theRequest );
		if ( *outStatus == PAPISuccess )
		{
			PAPIStringList * theValues;
			switch( theResponse->mName )
			{
				case PAPIRespSuccess:
					theValues = getParamValue( theResponse, PAPIParamSessionId);
					if ( theValues != 0 )
					{
						inDaemon->mSessionId = strdup( theValues->data );
						deleteStringList( theValues );
					}
					shouldContinue = 0;
					break;

				case PAPIRespSuccessContinueLocalAuth:
				case PAPIRespSuccessContinueSASLAuth:
					theValues = getParamValue( theResponse, PAPIParamMessage );
					if ( theValues != 0 )
					{
						theCreds =
						 theResponse->mName == PAPIRespSuccessContinueLocalAuth?
							( char * )getLocalCreds( theValues->data,
													 outStatus ) :
							( char * )getSASLCreds(	theValues->data,
													outStatus );
						if ( *outStatus != PAPISuccess )
						{
							theCreds = 0;
							*outStatus = PAPISuccess;
						}
						deleteStringList( theValues );		
					}
					else
					{
						*outStatus = PAPIUnknownFailure;
						shouldContinue = 0;
					}
					break;

				case PAPIRespConnectionFailure:
				case PAPIRespAuthenticationFailure:
					*outStatus = PAPISuccess;
					shouldContinue = 0;
					break;

				default:
					*outStatus = PAPIUnknownFailure;
					shouldContinue = 0;
					break;
			}
		}
		else
		{
			shouldContinue = 0;
		}
		deleteMessage( theResponse );
	}
	connectionSetSASLAuthHandler( inDaemon->mConnection,
								  saslAuthHandler,
								  inDaemon );
}

void daemonReaddListener( PAPIDaemon *	inDaemon,
						  const char *	inComponentName,
						  PAPIStatus *	outStatus )
{
	if ( inDaemon->mSessionId != 0 )
	{
		PAPIMessage *	theRequest;
		PAPIMessage *	theResponse;

		theRequest = newMessage( PAPIReqAddListener );
		if ( theRequest == 0 )
		{
			*outStatus = PAPIOutOfMemoryFailure;
			return;
		}
		addParam( theRequest, PAPIParamSessionId, inDaemon->mSessionId );	
		addParam( theRequest, PAPIParamComponentName, inComponentName );
		theResponse = sendRequest( inDaemon->mConnection,theRequest,outStatus );	
		deleteMessage( theRequest );
		deleteMessage( theResponse );
	}
	else
	{
		*outStatus = PAPIAuthenticationFailure;
	}
}

void destroySession( PAPIDaemon * inDaemon )
{
	if ( inDaemon->mSessionId != 0 )
	{
		PAPIMessage *	theRequest;
		PAPIMessage *	theResponse;
		PAPIStatus		theStatus;

		theRequest = newMessage( PAPIReqDestroySession );
		if ( theRequest == 0 )
		{
			return;
		}
		addParam( theRequest, PAPIParamSessionId, inDaemon->mSessionId );	
		theResponse = sendRequest( inDaemon->mConnection,theRequest,&theStatus);	
		deleteMessage( theRequest );
		deleteMessage( theResponse );
    }
}

const char * getLocalCreds( const char *	inLocalChallengeFile,
							PAPIStatus *	outStatus )
{
	char * theCredentials = ( char * )malloc( sLocalCredentialsBufferSize );
	if ( theCredentials == 0 )
	{
		*outStatus = PAPIOutOfMemoryFailure;
		return theCredentials;
	}
	if ( inLocalChallengeFile != 0 )
	{
		FILE * theFile = fopen( inLocalChallengeFile, "r" );
		if ( theFile != 0 )
		{
			fgets( theCredentials,
				   sLocalCredentialsBufferSize,
				   theFile );
			fclose( theFile );
		}
		else
		{
			*outStatus = PAPIUnknownFailure;
		}
	}
	else
	{
		*outStatus = PAPIUnknownFailure;
	}
	return theCredentials;
}

void * saslAuthHandler( void * inData )
{
	PAPIAuthHandlerData *	theData		= ( PAPIAuthHandlerData * )inData;
	PAPIDaemon *			theDaemon	= ( PAPIDaemon * )theData->mUserData;
	PAPIStringList *		theValues	= getParamValue( theData->mChallenge,
														 PAPIParamMessage );
	deleteMessage( theData->mChallenge );
	if ( theValues != 0 )
	{
		PAPIStatus   	theStatus;
		const char *	theCreds	=
			getSASLCreds( theValues->data, &theStatus );
		PAPIMessage *	theRequest	=
			createCreateSessionRequest( theDaemon->mUserId,
										theDaemon->mEntities,
										theCreds );
		deleteStringList( theValues );
		if ( theCreds != 0 )
		{
			free( ( void * )theCreds );
		}
		if ( theRequest != 0 )
		{
			PAPIMessage * theResponse =
				sendRequest( theDaemon->mConnection, theRequest,&theStatus);
			deleteMessage( theResponse );
			deleteMessage( theRequest );
		}
	}
	connectionSetSASLAuthHandler( theDaemon->mConnection,
								  saslAuthHandler,
								  theDaemon );
	return 0;
}
