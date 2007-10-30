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
#include "papiLayerList.h"

#include <stdlib.h>
#include <string.h>

static PAPILayerList *		newLayerList( void );

PAPILayerList * addLayerList( PAPILayerList **		ioPrevious,
							  const char *			inData,
							  const char *		    inTimestamp )
{
	PAPILayerList * theLayerList = ioPrevious[ 0 ];

	if ( inData != 0 )
	{
		if ( ( theLayerList = newLayerList() ) != 0 )
		{
			theLayerList->data		= ( unsigned char * )strdup( inData );
			theLayerList->size		= strlen( inData );
			theLayerList->timestamp	= strdup( inTimestamp );
			if ( ioPrevious[ 0 ] == 0 )
			{
				ioPrevious[ 0 ] = theLayerList;
			}
			else
			{
				PAPILayerList * theTmpList = ioPrevious[ 0 ];
				while ( theTmpList->next != 0 )
				{
					theTmpList = theTmpList->next;
				}
				theTmpList->next = theLayerList;
			}
		}
	}
	return theLayerList;
}

void deleteLayerList( PAPILayerList * inLayerList )
{
	if ( inLayerList != 0 )
	{
		if ( inLayerList->next != 0 )
		{
			deleteLayerList( inLayerList->next );
		}
		if ( inLayerList->data != 0 )
		{
			free( ( void *)inLayerList->data );
		}
		if ( inLayerList->timestamp != 0 )
		{
			free( ( void *)inLayerList->timestamp );
		}
		free( inLayerList );
	}
}

PAPILayerList * newLayerList( void )
{
	PAPILayerList * theLayerList =
		( PAPILayerList * )malloc( sizeof( PAPILayerList ) );
	if ( theLayerList != 0 )
	{
		theLayerList->data = 0;
		theLayerList->size = 0;
		theLayerList->next = 0;
	}
	return theLayerList;
}
