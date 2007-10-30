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
#include "papiListenerList.h"

#include <stdlib.h>
#include <string.h>

static PAPIListenerList *	newListenerList( void );

PAPIListenerList * addListenerList( PAPIListenerList **		ioPrevious,
									const char *			inComponentName,
									PAPIListener 			inListener,
									void *					inUserData )
{
	PAPIListenerList * theListenerList = ioPrevious[ 0 ];

	if ( inComponentName != 0 && inListener != 0 )
	{
		if ( ( theListenerList = newListenerList() ) != 0 )
		{
			theListenerList->mComponentName	= strdup( inComponentName );
			theListenerList->mListener		= inListener;
			theListenerList->mUserData		= inUserData;
			if ( ioPrevious[ 0 ] == 0 )
			{
				ioPrevious[ 0 ] = theListenerList;
			}
			else
			{
				PAPIListenerList * theTmpList = ioPrevious[ 0 ];
				while ( theTmpList->mNext != 0 )
				{
					theTmpList = theTmpList->mNext;
				}
				theTmpList->mNext = theListenerList;
			}
		}
	}
	return theListenerList;
}

void removeListenerList( PAPIListenerList **    ioPreviousList,
                         PAPIListenerList *     inList )
{
	if ( inList != 0 && ioPreviousList != 0 )
	{
		PAPIListenerList * thePreviousList	= 0;
		PAPIListenerList * theNextList		= ioPreviousList[ 0 ];
		while ( theNextList != 0 )
		{
			if ( theNextList == inList )
			{
				if ( theNextList == ioPreviousList[ 0 ] )
				{
					ioPreviousList[ 0 ] = ioPreviousList[ 0 ]->mNext;
				}
				else
				{
					thePreviousList->mNext = theNextList->mNext;
				}
				inList->mNext = 0;
				deleteListenerList( inList );
				break;
			}
			thePreviousList = theNextList;
			theNextList		= theNextList->mNext;
		}
	}
}

void deleteListenerList( PAPIListenerList * inListenerList )
{
	if ( inListenerList != 0 )
	{
		if ( inListenerList->mNext != 0 )
		{
			deleteListenerList( inListenerList->mNext );
		}
		if ( inListenerList->mComponentName != 0 )
		{
			free( ( void *)inListenerList->mComponentName );
		}
		free( inListenerList );
	}
}

PAPIListenerList * newListenerList( void )
{
	PAPIListenerList * theListenerList =
		( PAPIListenerList * )malloc( sizeof( PAPIListenerList ) );
	if ( theListenerList != 0 )
	{
		theListenerList->mComponentName	= 0; 
		theListenerList->mListener		= 0; 
		theListenerList->mUserData		= 0; 
		theListenerList->mNext			= 0; 
	}
	return theListenerList;
}
