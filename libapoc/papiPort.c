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
#include "papiUtils.h"
#include "config.h"

#include <gssapi.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef SOLARIS
#include <libscf.h>
#endif

static void					initGSSAPIContext( const char *		inServiceName,
											   gss_ctx_id_t *		outContext,
											   gss_buffer_desc *	outToken,	
											   PAPIStatus *			outStatus );

static int					isDaemonInitdEnabled( void );

static void					writeGSSAPIAuthId( gss_ctx_id_t *		outContext,
											   gss_buffer_desc *	outToken,	
											   PAPIStatus *			outStatus );

const char *				getGSSAPISrcName(  gss_ctx_id_t *		inContext );

static void					getTargetService(  const char *		inServiceName,
											   gss_name_t *		outService,
											   PAPIStatus *		outStatus );

static void					encodeGSSAPITokens( gss_buffer_desc *	inBuffers,
								    int			inBufferCnt,
								    char **		outB64 );

static const char * sInstallDir		= APOC_INSTALLDIR;
static const char * sLibraryDir		= APOC_LIBRARYDIR;
static const char * sPropertiesDir	= APOC_PROPDIR;


PAPIThread createThread( PAPIThreadFunc	inThreadFunc, void * inArg )
{
	pthread_attr_t	theAttr;
	pthread_t *		theThread = ( pthread_t * )malloc( sizeof( pthread_t ) );

	pthread_attr_init( &theAttr );
	pthread_attr_setdetachstate( &theAttr, PTHREAD_CREATE_DETACHED );
	if ( pthread_create( theThread, &theAttr, inThreadFunc, inArg ) != 0 )
	{
		free( ( void * )theThread );
		theThread = 0;
	}
	return theThread;
}

void destroyThread( PAPIThread inThread )
{
	pthread_cancel( *inThread );
}

void deleteMutex( PAPIMutex * inMutex )
{
	if ( inMutex != 0 )
	{
		pthread_mutex_destroy( inMutex );
		free( ( void * )inMutex );
	}
}

const char * getSASLCreds( const char * inServiceName, PAPIStatus * outStatus )
{
	char *			theCredentials = 0;
	gss_ctx_id_t	theContextHandle	= GSS_C_NO_CONTEXT;
	gss_buffer_desc	theOutputTokens[ 3 ];

	theOutputTokens[ 0 ].length =
	theOutputTokens[ 1 ].length =
	theOutputTokens[ 2 ].length = 0;
	initGSSAPIContext( inServiceName,
					   &theContextHandle,
					   &theOutputTokens[ 0 ],
					   outStatus );
	if ( *outStatus == PAPISuccess )
	{
		writeGSSAPIAuthId( &theContextHandle, &theOutputTokens[ 2 ],outStatus );
		if ( *outStatus == PAPISuccess )
		{
			OM_uint32 theMinorStatus;
			encodeGSSAPITokens( theOutputTokens,
								3,
								&theCredentials );
			gss_release_buffer( &theMinorStatus, &theOutputTokens[ 0 ] );
			gss_release_buffer( &theMinorStatus, &theOutputTokens[ 2 ] );
			gss_delete_sec_context( &theMinorStatus,
									&theContextHandle,
									0 );
		}
	}
	return theCredentials;
}

const char * getInstallDir( void )
{
	return sInstallDir;
}

const char * getLibraryDir( void )
{
	return sLibraryDir;
}

const char * getPropertiesDir( void )
{
	return sPropertiesDir;
}

const char * getUserName( void )
{
/*
 * no longer using cuserid here as it truncates user name at 8 characters
 */
	struct passwd * thePasswd = getpwuid( geteuid() );
	return thePasswd != 0 && thePasswd->pw_name != 0 ?
		strdup( thePasswd->pw_name ) : 0;
}

int isDaemonEnabled( void )
{
	int isDaemonEnabled = 0;
#ifdef SOLARIS
	isDaemonEnabled = isServiceEnabled();
#else
	isDaemonEnabled = isDaemonInitdEnabled();
#endif
	return isDaemonEnabled;
}

#ifdef SOLARIS
int isServiceEnabled( void )
{
	static const char *	sInstanceName		= "svc:/network/apocd/udp:default";
	int					isServiceEnabled	= 0;
	char *				theState			= smf_get_state( sInstanceName );
	if ( theState != 0 )
	{
		if ( strcmp( theState, SCF_STATE_STRING_DISABLED ) != 0 )
		{
			isServiceEnabled = 1;
		}
		free( ( void * )theState );
	}
	return isServiceEnabled;
}
#else
int isDaemonInitdEnabled( void )
{
	static char *	theCommand	= "/sbin/chkconfig -c apocd";
	FILE *			theStream;
	int				theRC		= 0;

	theStream = papiPopen( theCommand, "w" );
	if ( theStream != 0 )
	{
		theRC = papiPclose( theStream ) == 0 ? 1 : 0;
	}

	return theRC;
}
#endif

int lockMutex( PAPIMutex * inMutex )
{
	return pthread_mutex_lock( inMutex );
}

PAPIMutex * newMutex( void )
{
	pthread_mutex_t * theMutex =
		( pthread_mutex_t * )malloc( sizeof( pthread_mutex_t ) );
	if ( theMutex != 0 )
	{
		pthread_mutex_init( theMutex, 0 );
	}
	return theMutex;
}

int socketPoll( PAPISocket inSocket, int inMillis )
{
	struct pollfd	thePollFD;
	int				theRC;

	thePollFD.fd 		= inSocket;
	thePollFD.events	= POLLIN;
	theRC				= poll( &thePollFD, 1, inMillis );
	if ( theRC == 1 && thePollFD.revents & POLLIN != POLLIN ) 
	{
		theRC = 0;
	}
	else if ( theRC == -1 && thePollFD.revents & POLLHUP == POLLHUP )
	{
		theRC = 0;
	}
		
	return theRC;
}

int startDaemon( void )
{
	int theRC		= 1;

	/*
	 * apoc2.0: daemon startup via inetd no longer supported
	 *
	if ( isDaemonEnabled() == 1 )
	{
		int theSocket = getConnectedSocket( getDaemonPort(), SOCK_DGRAM, 0 );
		if ( theSocket != INVALID_SOCKET )
		{
			if ( send( theSocket, ( void *)"\n", 1, 0 ) != -1 )
			{
				theRC = 0;
			}
			close( theSocket );
		}
	}
	 */
	return theRC;
}

int unlockMutex( PAPIMutex * inMutex )
{
	return pthread_mutex_unlock( inMutex );
}

void encodeGSSAPITokens( gss_buffer_desc *	inBuffers,
						 int				inBufferCnt,
						 char **			outB64Buffer )
{
	void *	theBuffer;
	char *	theTmpPtr;
	int		theBufferSize = 0;
	int		theIndex;

	for ( theIndex = 0; theIndex < inBufferCnt; ++ theIndex )
	{
		theBufferSize += inBuffers[ theIndex ].length + 5;
	}	

	theBuffer = malloc( theBufferSize );	
	theTmpPtr = ( char *)theBuffer;
	for ( theIndex = 0; theIndex < inBufferCnt; ++ theIndex )
	{
		/* Write the token size */
		snprintf( theTmpPtr, 6, "%05d", inBuffers[ theIndex ].length );
		theTmpPtr += 5;
			
		/* Write the token */
		memcpy( ( void *)theTmpPtr,
				inBuffers[ theIndex ].value,
				inBuffers[ theIndex ].length);
		theTmpPtr += inBuffers[ theIndex ].length;	
	}
	base64_encode( theBuffer, theBufferSize, outB64Buffer );
	free( ( void *)theBuffer );
}

void initGSSAPIContext( const char *		inServiceName,
						gss_ctx_id_t * 		outContext,
						gss_buffer_desc *	outToken,
						PAPIStatus *		outStatus )
{
	gss_name_t theServiceName;

	getTargetService( inServiceName, &theServiceName, outStatus );
	if ( *outStatus == PAPISuccess )
	{
		OM_uint32		theMajorStatus;
		OM_uint32		theMinorStatus;
		gss_buffer_desc	theServiceNameBuf;
		OM_uint32		theReqFlags = GSS_C_DELEG_FLAG;
		OM_uint32		theRetFlags;
		gss_OID			theMech;
#ifdef SOLARIS
		gss_OID_desc	theMechArr =
			{ 9, "\052\206\110\206\367\022\001\002\002"};
		theMech = &theMechArr;
#else
		theMech = GSS_KRB5_MECHANISM;
#endif
		
		outToken->length = 0;
		theMajorStatus = gss_init_sec_context( &theMinorStatus,
											   GSS_C_NO_CREDENTIAL,
											   outContext,
										  	   theServiceName,
											   GSS_C_NO_OID,
											   0,
											   0,
											   NULL,
											   GSS_C_NO_BUFFER,
											   0,
											   outToken,
											   &theRetFlags,
											   0 );
		if ( theMajorStatus == GSS_S_COMPLETE )
		{
			*outStatus = PAPISuccess;
		}
		else
		{
			*outStatus = PAPIAuthenticationFailure;
		}
	}
}

const char * getGSSAPISrcName( gss_ctx_id_t * inContext )
{
	char *		theGSSAPISrcName = 0;
	OM_uint32	theMajorStatus;
	OM_uint32	theMinorStatus;
	gss_name_t	theSrcName;

	theMajorStatus = gss_inquire_context( &theMinorStatus,
										  *inContext,
										  &theSrcName,
										  0,
										  0,
										  0,
										  0,
										  0,
										  0 );
	if ( theMajorStatus == GSS_S_COMPLETE )
	{
		gss_buffer_desc theSrcNameBuffer;
		theMajorStatus = gss_display_name( &theMinorStatus,
										   theSrcName,
										   &theSrcNameBuffer,
										   0 );
		if ( theMajorStatus == GSS_S_COMPLETE )
		{
			int theSize			= theSrcNameBuffer.length + 1;
			theGSSAPISrcName	= ( char * )malloc( theSize );
			if ( theGSSAPISrcName != 0 )
			{
				snprintf( theGSSAPISrcName,
						  theSize,
						  "%s",
						  ( char *)theSrcNameBuffer.value );
			}
			gss_release_buffer( &theMinorStatus, &theSrcNameBuffer );	
		}
	}	
	return theGSSAPISrcName;
}

void getTargetService( const char *	inServiceName,
					   gss_name_t *	outServiceName,
					   PAPIStatus *	outStatus )
{
	if ( inServiceName != 0 )
	{
		OM_uint32		theMajorStatus;
		OM_uint32		theMinorStatus;
		gss_buffer_desc	theServiceNameBuf;
		OM_uint32		theReqFlags = GSS_C_DELEG_FLAG;
		OM_uint32		theRetFlags;

		theServiceNameBuf.length	= strlen( inServiceName );
		theServiceNameBuf.value		= ( char * )inServiceName;
		theMajorStatus				=
			gss_import_name( &theMinorStatus,
							 &theServiceNameBuf,
			 				 GSS_C_NO_OID,
			 				 outServiceName ); 
		*outStatus = theMajorStatus == GSS_S_COMPLETE ?
			PAPISuccess : PAPIAuthenticationFailure;
	}
}

void writeGSSAPIAuthId( gss_ctx_id_t *		inContext,
						gss_buffer_desc *	outToken,
						PAPIStatus *		outStatus )
{
	/*
	 * We no longer write the ldap authorisation id as LDAP servers
	 * can't agree on the format.
	 * See 6275775
	 */
	OM_uint32		theMajorStatus;
	OM_uint32		theMinorStatus;
	gss_buffer_desc	theAuthzIdBuffer;
	int				theConfState;

	*outStatus = PAPIAuthenticationFailure;
	theAuthzIdBuffer.length	= 4;
	theAuthzIdBuffer.value	= malloc( theAuthzIdBuffer.length );
	if ( theAuthzIdBuffer.value != 0 )
	{
		char * theCP	= ( char *)theAuthzIdBuffer.value;
		/* security layer */
		theCP[ 0 ]		= ( char )1;
		/* message size */
		theCP[ 1 ]		= ( char )1;
		theCP[ 2 ] = theCP[ 3 ] = ( char )0;
		
		theMajorStatus = gss_wrap( &theMinorStatus,
								   *inContext,
								   0,
								   GSS_C_QOP_DEFAULT,
								   &theAuthzIdBuffer,
								   &theConfState,
								   outToken );
		free( theAuthzIdBuffer.value );
		if ( theMajorStatus == GSS_S_COMPLETE )
		{
			*outStatus = PAPISuccess;
		}
	}
}
