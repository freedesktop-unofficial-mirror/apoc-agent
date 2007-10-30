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

#include "papiSAXParserContext.h"

static int  saxReader( void * inIOContext, char * inBuffer, int inLen );

static int  saxCloser( void * inIOContext );

static void papiStartElement( void *			inUserData,
						  const xmlChar *	inName,
						  const xmlChar **	inAttrs ); 
static void papiCharacters(	  void *			inUserData,
						  const xmlChar *	inChars,
						  int				inLen );

static xmlSAXHandler sSAXHandler =
	{
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		papiStartElement,	
		0,	
		0,	
		papiCharacters,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		0,	
		1
	};

PAPISAXParserContext  newSAXParserContext( PAPIConnection *		inConnection,
										   PAPISAXUserData *	inUserData )
{
	PAPISAXParserContext 	theSAXParserContext;
	PAPISAXIOContext *		theIOContext =
		( PAPISAXIOContext * )malloc( sizeof( PAPISAXIOContext ) );
	if ( theIOContext != 0 )
	{
		theIOContext->mContentLength	= -1;
		theIOContext->mConnection		= inConnection;
	
		theSAXParserContext = 
				xmlCreateIOParserCtxt( &sSAXHandler,
								   	   ( void *) inUserData,
								       saxReader,
								       saxCloser,
								       ( void *)theIOContext,
								       XML_CHAR_ENCODING_NONE );
		
	}
	return theSAXParserContext;
}

void deleteSAXParserContext( PAPISAXParserContext inParserContext )
{
	xmlFreeParserCtxt( inParserContext );
}

int parse( PAPISAXParserContext inParserContext )
{
	return xmlParseDocument( inParserContext );
}

void papiStartElement( void *            inUserData,
                          const xmlChar *   inName,
                          const xmlChar **  inAttrs )
{
	PAPISAXUserData * theUserData = ( PAPISAXUserData * )inUserData;
	if ( theUserData != 0 && theUserData->mMessage == 0 )
	{
		theUserData->mMessage = newMessage(
			stringToMessageName( ( const char * )inName ) );
	}
	else
	{
		theUserData->mCurrentParamName = 
			stringToParamName( ( const char * )inName );
	}
}

void papiCharacters(   void *            inUserData,
                          const xmlChar *   inChars,
                          int               inLen )
{
	PAPISAXUserData *  theUserData = ( PAPISAXUserData * )inUserData;
	if ( theUserData != 0 && theUserData->mMessage != 0 )
	{
		char * theValue = ( char * )malloc( inLen + 1 );
		if ( theValue != 0 )
		{
			snprintf( theValue, inLen + 1, "%s", inChars );
			theValue[ inLen ] = 0;
			addParam( theUserData->mMessage,
					  theUserData->mCurrentParamName,
					  theValue );
			free( ( void *)theValue );
			theUserData->mCurrentParamName = PAPIParamNameUnused;
		}
	}
}

int saxReader( void * inIOContext, char * inBuffer, int inLen )
{
	int					theByteCount = EOF;
	PAPISAXIOContext *	theIOContext = ( PAPISAXIOContext * )inIOContext;

	if ( theIOContext->mContentLength == -1 )
	{
		theIOContext->mContentLength =
			readContentLength( theIOContext->mConnection );
	}
	if ( theIOContext->mContentLength > 0 )
	{
		int theLen = inLen < theIOContext->mContentLength ? 
						inLen : theIOContext->mContentLength;
		theByteCount = readBytes( theIOContext->mConnection,
								  ( void *)inBuffer,
								  theLen );
		if ( theByteCount > 0 )
		{
			theIOContext->mContentLength -= theByteCount;
		}
	}
	return theByteCount == -1 ? 0 : theByteCount;	
}

int saxCloser( void * inIOContext )
{
	free( inIOContext );
    return 0;
}
