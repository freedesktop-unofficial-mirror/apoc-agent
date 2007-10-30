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
#include "papiDatabase.h"
#include "papiInternal.h"
#include "papiUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void connectionListener(	PAPIConnectionState	inState,
								void *				inUserData);

static int	componentExists(	PAPIDatabase *		inDatabase,
								const char *		inComponentName );

static void componentsListener(	const PAPIEvent *	inEvent,
								void *				inUserData );

static void * onConnect(		void *				inDatabase );

static void * onDisconnect(		void *				inDatabase );

#define TIMESTAMPSIZE 16


PAPIListenerId addListenerToDatabase( PAPIDatabase *	inDatabase,
									  const char *	inComponentName,
									  PAPIListener	inListener,
									  void *		inUserData,
									  PAPIStatus *	outStatus )
{
	return daemonAddListener( inDatabase->mDaemon,
							  inComponentName,
							  inListener,
							  inUserData,
							  outStatus );
}

void deleteDatabase( PAPIDatabase * inDatabase )
{
	if ( inDatabase != 0 )
	{
		deleteStringList( inDatabase->mComponents );
		deleteMutex( inDatabase->mComponentsMutex );
		if ( inDatabase->mDaemon != 0 )
		{
			deleteDaemon( inDatabase->mDaemon );
		}
		deleteMutex( inDatabase->mConnectionStateMutex );
		free( ( void *)inDatabase );
	}
}

PAPIConnectionState getConnectionState( PAPIDatabase * inDatabase )
{
	PAPIConnectionState theState = PAPIDisconnected;
	if ( lockMutex( inDatabase->mConnectionStateMutex ) == 0 )
	{
		theState = inDatabase->mConnectionState;
		unlockMutex( inDatabase->mConnectionStateMutex );
	}
	return theState;
}

PAPILayerList * readLayersFromDatabase( PAPIDatabase *	inDatabase,
									    const char *	inComponentName,
										PAPIStatus *	outStatus )
{
	PAPILayerList * theLayerList = 0;
	if ( componentExists( inDatabase, inComponentName ) != 0 )
	{
		PAPIStringList * theTmpList =
			daemonRead( inDatabase->mDaemon, inComponentName, outStatus );
		PAPIStringList * theList = theTmpList;
		while ( theTmpList != 0 )
		{
			if ( theTmpList->data != 0 &&
			     strlen( theTmpList->data ) >= TIMESTAMPSIZE )
			{
				char theTimestamp[ TIMESTAMPSIZE ];
				snprintf( theTimestamp, TIMESTAMPSIZE, "%s", theTmpList->data);
#ifdef WNT
				theTimestamp[ TIMESTAMPSIZE - 1 ] = 0;
#endif
				addLayerList( &theLayerList,
							  theTmpList->data + TIMESTAMPSIZE - 1,
							  theTimestamp );
				theTmpList = theTmpList->next;
			}
		}
		deleteStringList( theList );
	}
	else
	{
		*outStatus = PAPINoSuchComponentFailure;
	}
	return theLayerList;
}

PAPIStringList * readNamesFromDatabase( PAPIDatabase *	inDatabase,
										const char *	inFilter )
{
	PAPIStringList * theList = 0;
	if ( lockMutex( inDatabase->mComponentsMutex ) == 0 )
	{
		theList = copyStringList( inDatabase->mComponents );
		unlockMutex( inDatabase->mComponentsMutex );
	}
	return theList;
}

void removeListenerFromDatabase( PAPIDatabase *	inDatabase,
								 PAPIListenerId	inListenerId,
								 PAPIStatus *	outStatus )
{
	daemonRemoveListener( inDatabase->mDaemon, inListenerId, outStatus );
}

PAPIDatabase * newDatabase( const PAPIEntityList *	inEntities,
							PAPIConnectionListener	inConnectionListener,
							void *					inUserData,
							PAPIStatus *			outStatus )
{
	PAPIDatabase *	theDatabase =
		( PAPIDatabase *)malloc( sizeof( PAPIDatabase ) );

	if ( theDatabase == 0 )
	{
		*outStatus = PAPIOutOfMemoryFailure;
		return 0;
	}
	memset( theDatabase, 0, sizeof( PAPIDatabase ) );
	theDatabase->mConnectionListener	= inConnectionListener;
	theDatabase->mUserData				= inUserData;
	theDatabase->mConnectionState		= PAPIDisconnected;
	theDatabase->mConnectionStateMutex	= newMutex();
	if ( lockMutex( theDatabase->mConnectionStateMutex ) == 0 )
	{
		theDatabase->mDaemon			= newDaemon( inEntities,
													 connectionListener,
													 theDatabase,
													 componentsListener,
													 theDatabase,
													 outStatus );
		if ( *outStatus != PAPISuccess &&
		     ( *outStatus != PAPIConnectionFailure || isDaemonEnabled() == 0 ) )
		{
			unlockMutex( theDatabase->mConnectionStateMutex );
			deleteDatabase( theDatabase );
			theDatabase = 0;
		}
		else 
		{
			if ( *outStatus == PAPISuccess )
			{
				theDatabase->mComponents		=
					daemonList( theDatabase->mDaemon, outStatus );
				theDatabase->mConnectionState = PAPIConnected;
			}
			theDatabase->mComponentsMutex = newMutex();
			unlockMutex( theDatabase->mConnectionStateMutex );
		}		
	}
	return theDatabase;
}

void connectionListener( PAPIConnectionState inState, void * inUserData )
{
	createThread( inState == PAPIConnected ? onConnect : onDisconnect,
				  inUserData );
}

int componentExists( PAPIDatabase *	inDatabase, const char * inComponentName )
{
	int haveComponent = 0;
	if ( lockMutex( inDatabase->mComponentsMutex ) == 0 )
	{
		haveComponent =
			containsString( inDatabase->mComponents, inComponentName );	
		unlockMutex( inDatabase->mComponentsMutex );
	}
	return haveComponent;
}

void componentsListener( const PAPIEvent * inEvent, void * inUserData )
{
	PAPIDatabase * theDatabase = ( PAPIDatabase * )inUserData;
	if ( lockMutex( theDatabase->mComponentsMutex ) == 0 )
	{
		if ( inEvent->eventType == PAPIComponentAdd )
		{
			addStringList( &theDatabase->mComponents,
						   ( const char * )inEvent->componentName );
		}
		else
		{
			deleteStringList(
				removeStringList( &theDatabase->mComponents,
							  ( const char * )inEvent->componentName ) );
		}
		unlockMutex( theDatabase->mComponentsMutex );
	}
}

void * onConnect( void * inDatabase )
{
	PAPIStatus		theStatus;
	PAPIDatabase *	theDatabase = ( PAPIDatabase * )inDatabase;
	if ( lockMutex( theDatabase->mConnectionStateMutex ) == 0 )
	{
		daemonReconnect( theDatabase->mDaemon,
						 &theStatus );
		if ( theStatus == PAPISuccess )
		{
			theDatabase->mConnectionState = PAPIConnected;
			if ( lockMutex( theDatabase->mComponentsMutex ) == 0 )
			{
				deleteStringList( theDatabase->mComponents );
				theDatabase->mComponents =
					daemonList( theDatabase->mDaemon, &theStatus );
				unlockMutex( theDatabase->mComponentsMutex );
			}
			unlockMutex( theDatabase->mConnectionStateMutex );
			if ( theDatabase->mConnectionListener != 0 )
			{
				theDatabase->mConnectionListener( PAPIConnected,
												  theDatabase->mUserData );
			}
		}
		else
		{
			unlockMutex( theDatabase->mConnectionStateMutex );
		}
	}
	return 0;
}

void * onDisconnect( void * inDatabase )
{
	PAPIDatabase * theDatabase = ( PAPIDatabase * )inDatabase;
	if ( lockMutex( theDatabase->mConnectionStateMutex ) == 0 )
	{
		theDatabase->mConnectionState = PAPIDisconnected;
		unlockMutex( theDatabase->mConnectionStateMutex );
		if ( theDatabase->mConnectionListener != 0 )
		{
			theDatabase->mConnectionListener( PAPIDisconnected,
											  theDatabase->mUserData );
		}
	}
	return 0;
}
