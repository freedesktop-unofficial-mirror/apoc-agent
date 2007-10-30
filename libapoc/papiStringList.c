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
#include "papiStringList.h"

#include <stdlib.h>
#include <string.h>

static PAPIStringList *		newStringList( void );

PAPIStringList * addStringList( PAPIStringList **	ioPrevious,
								const char *		inData )
{
	PAPIStringList * theStringList = newStringList();
	if ( theStringList != 0 )
	{
		theStringList->data = inData == 0 ? 0 : strdup( inData );
		if ( ioPrevious[ 0 ] == 0 )
		{
			ioPrevious[ 0 ] = theStringList;
		}
		else
		{
			PAPIStringList * theTmpList = ioPrevious[ 0 ];
			while ( theTmpList->next != 0 )
			{
				theTmpList = theTmpList->next;
			}
			theTmpList->next = theStringList;
		}
	}
	return theStringList;
}

int containsString( PAPIStringList * inList, const char * inString )
{
	int isContained = 0;
	if ( inList != 0 && inString != 0 )
	{
		while ( inList != 0 )
		{
			if ( strcmp( inList->data, inString ) == 0 )
			{
				isContained = 1;
				break;
			}
			inList = inList->next;
		}
	}
	return isContained;
}

PAPIStringList * copyStringList( PAPIStringList * inList )
{
	PAPIStringList * theList = 0;
	while ( inList != 0 )
	{
		addStringList( &theList, inList->data );
		inList = inList->next;
	}
	return theList;
}

void deleteStringList( PAPIStringList * inStringList )
{
	if ( inStringList != 0 )
	{
		if ( inStringList->next != 0 )
		{
			deleteStringList( inStringList->next );
		}
		if ( inStringList->data != 0 )
		{
			free( ( void *)inStringList->data );
		}
		free( inStringList );
	}
}

PAPIStringList * removeStringList( PAPIStringList **	ioStringList,
								   const char *			inString )
{
	PAPIStringList * theList = 0;
	if ( ioStringList != 0 && inString != 0 )
	{
		theList = ioStringList[ 0 ];
		if ( theList != 0 )
		{
			PAPIStringList * thePrevList = theList;
			while ( theList != 0 )
			{
				if ( strcmp( theList->data, inString ) == 0 )
				{
					if ( theList == ioStringList[ 0 ] )
					{
						ioStringList[ 0 ] = ioStringList[ 0 ]->next;
					}
					else
					{
						thePrevList->next = theList->next;
					}
					theList->next = 0;
					break;
				}
				thePrevList = theList;
				theList = theList->next;	
			}
		}
	}
	return theList;
}
 
PAPIStringList * strToStringList( const char * inString )
{
	PAPIStringList * theList = 0;
	if ( inString != 0 )
	{
		PAPIStringList *	theTmpList;
		const char *		theString = strdup( inString );
		char *				theToken = strtok( ( char *)theString, " " );
		while ( theToken != 0 )
		{	
			theTmpList	= addStringList( &theList, theToken );
			if ( theList == 0 )
			{
				theList = theTmpList;
			}
			theToken	= strtok( NULL, " " );
		}
		free( ( void * )theString );
	}
	return theList;
}

PAPIStringList * newStringList( void )
{
	PAPIStringList * theStringList =
		( PAPIStringList * )malloc( sizeof( PAPIStringList ) );
	if ( theStringList != 0 )
	{
		theStringList->data = 0;
		theStringList->next = 0;
	}
	return theStringList;
}
