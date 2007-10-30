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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "papiMessage.h"
#include "papiInternal.h"

static PAPIParamNameList * newParamNameList( PAPIParamName inParamName );

static void deleteParamNameList( PAPIParamNameList * inParamNameList );

static PAPIParamNameList * addParamNameList( PAPIParamNameList **ioPreviousList,
											 PAPIParamName inParamName );

static const char *sMessageNames[] =
{
	"success",
	"invalidRequest",
	"connectionFailure",
	"authenticationFailure",
	"invalidSessionId",
	"noSuchComponent",
	"successContinueLocalAuth",
	"successContinueSASLAuth",

	"addNotification",
	"removeNotification",
	"modifyNotification",

	"createSession",
	"read",
	"list",
	"destroySession",
	"addListener",
	"removeListener",
    "createSessionExt"
};

static const char *sParamNames[] =
{
	"sessionId",
	"userId",
	"entityId",
	"credentials",
	"componentName",
	"clientData",
	"message",
	"layer",
    "entityType"
};

/* Must be aligned with PAPIEntityType enumeration in papiTypes.h AND
   must be the same strings as the SOURCE ones in 
   spi/environment/EnvironmentConstants. */
static const char *sEntityTypes[] = 
{
    "HOST",
    "USER"
};

PAPIMessage * newMessage( PAPIMessageName inMessageName )
{
	PAPIMessage * theMessage = ( PAPIMessage * )malloc( sizeof( PAPIMessage ) );
	if ( theMessage != 0 )
	{
		theMessage->mName			= inMessageName;
		theMessage->mParamNames		= 0;
		theMessage->mParamValues	= 0;
	}
	return theMessage;
}

void deleteMessage( PAPIMessage * inMessage )
{
	if ( inMessage != 0 )
	{
		deleteParamNameList( inMessage->mParamNames );
		deleteStringList( inMessage->mParamValues );
		free( ( void * )inMessage );
	}
}

void addParam( PAPIMessage *	inMessage,
			   PAPIParamName	inParamName,
			   const char *		inParamValue )
{
	if ( inMessage != 0 )
	{
		addParamNameList( &inMessage->mParamNames,	inParamName );
		addStringList( &inMessage->mParamValues,	inParamValue );
	}
}

char * messageToString( PAPIMessage * inMessage )
{
	int		theStringSize;
	char *	theString		= 0;
	if ( inMessage != 0 )
	{
		PAPIParamNameList *	theNameList		= inMessage->mParamNames;
		PAPIStringList *	theValueList	= inMessage->mParamValues;
		theStringSize =
			( 2 * strlen( sMessageNames[ ( int )inMessage->mName ] ) ) + 5;
		while ( theNameList != 0 )
		{
			theStringSize +=
				( 2 * strlen( sParamNames[ ( int )theNameList->mName ] ) ) + 5;
			if ( theValueList->data != 0 )
			{
				theStringSize += strlen( theValueList->data );
			}
			theNameList		= theNameList->mNext;
			theValueList	= theValueList->next;
		}


		if ( theStringSize > 0 )
		{
			theNameList		= inMessage->mParamNames;
			theValueList	= inMessage->mParamValues;
			theString		= ( char * )malloc( theStringSize + 1 );
			theString[ 0 ]	= 0;

			sprintf( theString, "<%s>", sMessageNames[( int )inMessage->mName]);
			while ( theNameList != 0 )
			{
				strcat( theString, "<" );
				strcat( theString, sParamNames[ ( int )theNameList->mName ] );
				strcat( theString, ">" );

				if ( theValueList->data != 0 )
				{
					strcat( theString, theValueList->data );
				}

				strcat( theString, "</" );
				strcat( theString, sParamNames[ ( int )theNameList->mName] );
				strcat( theString, ">" );

				theNameList		= theNameList->mNext;
				theValueList	= theValueList->next;
			}
			strcat( theString, "</" );
			strcat( theString, sMessageNames[ ( int )inMessage->mName ] );
			strcat( theString, ">" );
		}
	}
	return theString;
}

PAPIStringList * getParamValue( PAPIMessage *	inMessage,
								PAPIParamName	inParamName )
{
	PAPIStringList * theValue = 0;
	if ( inMessage != 0	)
	{
		PAPIParamNameList *	theNames	= inMessage->mParamNames;
		PAPIStringList *	theValues	= inMessage->mParamValues;
		while ( theNames != 0 )
		{
			if ( theNames->mName == inParamName ) 
			{
				addStringList( &theValue, theValues->data );
			}
			theNames	= theNames->mNext;
			theValues	= theValues->next;
		}
	}
	return theValue;
}

PAPIParamNameList * newParamNameList( PAPIParamName inParamName )
{
	PAPIParamNameList * theList =
		( PAPIParamNameList * )malloc( sizeof( PAPIParamNameList ) );
	if ( theList != 0 )
	{
		theList->mName = inParamName;
		theList->mNext = 0;
	}
	return theList;
}

PAPIMessageName stringToMessageName( const char * inMessageName )
{
	PAPIMessageName theName = PAPIMessageNameUnused;
	if ( inMessageName != 0 )
	{
		int theIndex;
		for ( theIndex = 0; theIndex < PAPIMessageNameUnused; theIndex ++ )
		{
			if ( strcmp( inMessageName, sMessageNames[ theIndex ] ) == 0 )
			{
				theName = ( PAPIMessageName )theIndex;
				break;
			}
		}
	}
	return theName;
}

PAPIParamName stringToParamName(   const char * inParamName )
{
	PAPIParamName theName = PAPIParamNameUnused;
	if ( inParamName != 0 )
	{
		int theIndex;
		for ( theIndex = 0; theIndex < PAPIParamNameUnused; theIndex ++ )
		{
			if ( strcmp( inParamName, sParamNames[ theIndex ] ) == 0 )
			{
				theName = ( PAPIParamName )theIndex;
				break;
			}
		}
	}
	return theName;
}

static void deleteParamNameList( PAPIParamNameList * inParamNameList )
{
	if ( inParamNameList != 0 )
	{
		if ( inParamNameList->mNext != 0 )
		{
			deleteParamNameList( inParamNameList->mNext );
		}
		free( ( void * )inParamNameList );
	}
}

static PAPIParamNameList * addParamNameList( PAPIParamNameList **ioPrevious,
		                                     PAPIParamName       inParamName )
{
	PAPIParamNameList * theList = newParamNameList( inParamName );
	if ( theList != 0 )
	{
		if ( ioPrevious[ 0 ] == 0 )
		{
			ioPrevious[ 0 ] = theList;
		}
		else
		{
			PAPIParamNameList * theTmpList = ioPrevious[ 0 ];
			while ( theTmpList->mNext != 0 )
			{
				theTmpList = theTmpList->mNext;
			}
			theTmpList->mNext = theList;
		}
	}
	return theList;
}

const char * getEntityTypeString( PAPIEntityType inType )
{
    return sEntityTypes[ inType ];
}

