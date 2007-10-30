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
#include "papi.h"
#include "papiDatabase.h"

static PAPIMutex * gMutex = 0;

static int lock( void )
{
    int theRC = -1;
    if ( gMutex == 0 )
    {
        gMutex = newMutex();
    }
    if ( gMutex != 0 )
    {
        theRC = lockMutex( gMutex );
    }
    return theRC;
}

static int unlock( void )
{
    int theRC = -1;
    if ( gMutex != 0 )
    {
        theRC = unlockMutex( gMutex );
    }
    return theRC;
}

PAPIListenerId addListener( const PAPI *	inPAPI,
							const char *	inComponentName,
							PAPIListener	inListener,
							void *			inUserData,
							PAPIStatus *	outStatus )
{
	PAPIStatus			theStatus;
	PAPIDatabase *		theDatabase	= ( PAPIDatabase * )inPAPI;
	PAPIListenerId		theListenerId;

	if ( theDatabase != 0 && inComponentName != 0 && inListener != 0 )
	{
		if ( getConnectionState( theDatabase ) == PAPIDisconnected )
		{
			theStatus = PAPIConnectionFailure;
		}
		else if ( lock() == 0 )
		{
			theListenerId = addListenerToDatabase( theDatabase,
												   inComponentName,
												   inListener,
												   inUserData,
												   &theStatus );
			unlock();
		}
	}
	else
	{
		theStatus = PAPIInvalidArg;
	}
	if ( outStatus != 0 )
	{
		*outStatus = theStatus;
	}
	return theListenerId;
}

void deinitialise( PAPI * inPAPI, PAPIStatus * outStatus )
{
	if ( lock() == 0 )
	{
		deleteDatabase( ( PAPIDatabase * )inPAPI );
		unlock();
	}
	if ( outStatus != 0 )
	{
		*outStatus = PAPISuccess;
	}
}

PAPI * initialise( const PAPIEntityList *	inEntities,
				   PAPIConnectionListener	inConnectionListener,
				   void *					inUserData,
				   PAPIStatus *				outStatus )
{
	PAPIStatus	theStatus;
	PAPI *		thePAPI	= 0;
	if ( lock() == 0 )
	{
		thePAPI = ( PAPI *)newDatabase( inEntities,
										inConnectionListener,
										inUserData,
										&theStatus );
		unlock();
	}
	if ( outStatus != 0 )
	{
		*outStatus = theStatus;
	}
	return thePAPI;
}

PAPIStringList * listComponentNames( const PAPI *	inPAPI,
								  	 const char *	inFilter,
								   	 PAPIStatus *	outStatus )
{
	PAPIStatus			theStatus;
	PAPIDatabase *		theDatabase	= ( PAPIDatabase * )inPAPI;
    PAPIStringList *	theNames	= 0;
	if ( theDatabase != 0 )
	{
		if ( getConnectionState( theDatabase ) == PAPIDisconnected )
		{
			theStatus = PAPIConnectionFailure;
		}
		else if ( lock() == 0 )
		{
			theNames	= readNamesFromDatabase( theDatabase, inFilter );
			theStatus	= PAPISuccess;
			unlock();
		}
	}
	else
	{
		theStatus = PAPIInvalidArg;
	}
	if ( outStatus != 0 )
	{
		*outStatus = theStatus;
	}
	return theNames;
}

PAPILayerList * readComponentLayers( const PAPI *	inPAPI,
									 const char *	inComponentName,
									 PAPIStatus *	outStatus )
{
	PAPIStatus		theStatus;
	PAPIDatabase *	theDatabase	= ( PAPIDatabase * )inPAPI;
	PAPILayerList *	theLayers	= 0;

	if ( theDatabase != 0 && inComponentName != 0 )
	{
		if ( getConnectionState( theDatabase ) == PAPIDisconnected )
		{
			theStatus = PAPIConnectionFailure;
		}
		else if ( lock() == 0 )
		{
			theLayers = readLayersFromDatabase( theDatabase,
												inComponentName,
												&theStatus );
			unlock();
		}
	}
	else
	{
		theStatus = PAPIInvalidArg;
	}
	if ( outStatus != 0 )
	{
		*outStatus = theStatus;
	}
	return theLayers;
}

void removeListener( const PAPI *	inPAPI,
					 PAPIListenerId	inListenerId,
					 PAPIStatus *	outStatus )
{
	PAPIStatus			theStatus;
	PAPIDatabase *		theDatabase	= ( PAPIDatabase * )inPAPI;

	if ( theDatabase != 0 && inListenerId != 0 )
	{
		if ( getConnectionState( theDatabase ) == PAPIDisconnected )
		{
			theStatus = PAPIConnectionFailure;
		}
		else if ( lock() == 0 )
		{
			removeListenerFromDatabase( theDatabase, inListenerId, &theStatus );
			unlock();
		}
	}
	else
	{
		theStatus = PAPIInvalidArg;
	}
	if ( outStatus != 0 )
	{
		*outStatus = theStatus;
	}
}
