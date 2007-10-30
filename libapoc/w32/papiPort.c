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

#include <lmcons.h>
#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <stdlib.h>
#include <malloc.h>
#include <shlobj.h>

typedef WINADVAPI SC_HANDLE ( WINAPI *OpenSCManagerFunc )
	(  LPCTSTR, LPCTSTR, DWORD );

typedef WINADVAPI BOOL ( WINAPI * EnumServicesStatusFunc )
	( SC_HANDLE, DWORD, DWORD, LPENUM_SERVICE_STATUS, DWORD, LPDWORD, LPDWORD,
      LPDWORD );

static PSecurityFunctionTable 	getSecurityInterface( int * outMaxTokenSize );

static int						isRegistryEnabled( void );

static int						isServiceEnabled( void );

static int						loadServiceFuncs( void );

static void						writeGSSAPIAuthId(
									PSecurityFunctionTable	inSecurityInterface,
									CredHandle *			inCredentials,
									CtxtHandle *			inContext,
									SecBuffer *				outBuffer,
									PAPIStatus *			outStatus );

static void						encodeGSSAPITokens( SecBuffer * inBuffers,
													int			inBufferCount,
													char **		outB64Buffer );

#define INSTALLDIR				"\\Sun\\apoc\\lib"

const char * convertDataDir( const char * inDataDir )
{
	char * theDataDir = 0;
	if ( inDataDir != 0 )
	{
		theDataDir = ( char * )malloc( strlen( inDataDir ) + 1 );
		if ( theDataDir != 0 )
		{
			char * theToken = strtok( inDataDir, "\\" );
			theDataDir[ 0 ] = 0;
			while ( theToken != 0 )
			{
				strcat( theDataDir, theToken );
				theToken = strtok( 0, "\\" );
				if ( theToken != 0 )
				{
					strcat( theDataDir, "\\" );
				}
			}
		}
	}
	return theDataDir;
}

int createThread( PAPIThreadFunc	inThreadFunc,
				  void *			inArg )
{
	int		theThreadId;
	HANDLE	theThread =
		CreateThread( NULL,
					  0,
					  ( LPTHREAD_START_ROUTINE )inThreadFunc,
					  inArg,
					  0,
					  ( LPDWORD )&theThreadId );
	return theThread != 0 ? 0 : -1;
}

const char * getInstallDir( void )
{
	static char * sInstallDir = 0;
	if ( sInstallDir == 0 )
	{
		char theProgramFiles[ MAX_PATH ];
		if ( SHGetFolderPath( NULL,
						CSIDL_PROGRAM_FILES,
						NULL,
						SHGFP_TYPE_CURRENT,
						( LPTSTR )&theProgramFiles ) == S_OK )
		{
			char 	theShortPath[ MAX_PATH ];
			DWORD	theSize = MAX_PATH;
			theSize =GetShortPathName( theProgramFiles, &theShortPath, theSize);
			sInstallDir = malloc( theSize					+
								  strlen( INSTALLDIR )		+
								  1 );
			if ( sInstallDir != 0 )
			{
				sprintf( sInstallDir, "%s%s", theShortPath, INSTALLDIR );
			}
		}
	}
	return sInstallDir;
}

const char * getLibraryDir( void )
{
	return getInstallDir();
}

const char * getPropertiesDir( void )
{
	return getInstallDir();
}

int isNewerThanWindows98( void )
{
	if ( GetVersion() < 0x80000000 )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

PAPIMutex * newMutex( void )
{
	PAPIMutex * theMutex =
		( PAPIMutex * )malloc( sizeof( PAPIMutex ) );
	if ( theMutex != 0 )
	{
		InitializeCriticalSection( theMutex );
	}
	return theMutex;
}

void deleteMutex( PAPIMutex * inMutex )
{
	if ( inMutex != 0 )
	{
		DeleteCriticalSection( inMutex );
		free( ( void *)inMutex );
	}
}

const char * getSASLCreds( const char * inServiceName, PAPIStatus * outStatus )
{
	char *					theCredentials;
	static int				sMaxTokenSize;
	PSecurityFunctionTable	theSecurityInterface	=
		getSecurityInterface( &sMaxTokenSize );

	*outStatus = PAPIAuthenticationFailure;
	if ( theSecurityInterface != 0 )
	{
		if ( inServiceName != 0 )
		{
			CredHandle		theCredHandle;
			TimeStamp		theCredLifetime;
			SECURITY_STATUS	theStatus =
				( theSecurityInterface->AcquireCredentialsHandle )
			(
				0,
          		"Kerberos",
          		SECPKG_CRED_OUTBOUND,
          		NULL,
          		NULL,
          		NULL,
          		NULL,
          		&theCredHandle,
          		&theCredLifetime	
			);
			if ( theStatus == SEC_E_OK )
			{
				CtxtHandle		theContextHandle;
				SecBufferDesc	theOutBufferDesc;
				SecBuffer		theOutBuffer[ 3 ];
				ULONG			theAttrs;

				theOutBuffer[ 1 ].cbBuffer		=
				theOutBuffer[ 2 ].cbBuffer		= 0;
				theOutBufferDesc.ulVersion		= SECBUFFER_VERSION;
				theOutBufferDesc.cBuffers		= 1;
				theOutBufferDesc.pBuffers		= &theOutBuffer[ 0 ];
				theOutBuffer[ 0 ].BufferType	= SECBUFFER_TOKEN;
				theOutBuffer[ 0 ].cbBuffer		= sMaxTokenSize;
				theOutBuffer[ 0 ].pvBuffer		=
					( PBYTE )malloc( sMaxTokenSize );

				theStatus =
					( theSecurityInterface->InitializeSecurityContext )
					  (
						&theCredHandle,
						NULL,
						( SEC_CHAR * )inServiceName,
						ISC_REQ_DELEGATE,
						0,
						SECURITY_NATIVE_DREP,
						NULL,
						0,
						&theContextHandle,
						&theOutBufferDesc,
						&theAttrs,
						&theCredLifetime
					  );
				if ( theStatus == SEC_E_OK				||
					 theStatus == SEC_I_CONTINUE_NEEDED	||
					 theStatus == SEC_I_COMPLETE_NEEDED	||
					 theStatus == SEC_I_COMPLETE_AND_CONTINUE )
				{
					writeGSSAPIAuthId( theSecurityInterface,
									   &theCredHandle,
									   &theContextHandle,
									   &theOutBuffer[ 2 ],
									   outStatus );
					if ( *outStatus == PAPISuccess )
					{
						encodeGSSAPITokens( theOutBuffer,
											3,
											&theCredentials );
					}
					FreeContextBuffer( &theContextHandle );
				}
				FreeCredentialsHandle( &theCredHandle );
			}
		}
	}
	return theCredentials;
}

const char * getUserName( void )
{
	DWORD	theBufferSize	= UNLEN + 1;
	LPTSTR	theBuffer		= ( LPTSTR )malloc( ( size_t )theBufferSize );
	
	GetUserName( theBuffer, &theBufferSize );
	return theBuffer;
}

int isDaemonEnabled( void )
{
	return isNewerThanWindows98() == 1 ?
		isServiceEnabled() : isRegistryEnabled();
}

int lockMutex( PAPIMutex * inMutex )
{
	int theRC = -1;
	if ( inMutex != 0 )
	{
		EnterCriticalSection( inMutex );
		theRC = 0;
	}
	return theRC;
}

int socketPoll( PAPISocket inSocket, int inMillis )
{
	fd_set theReadSet;

	FD_ZERO( &theReadSet );
	FD_SET( inSocket, &theReadSet );
	if ( inMillis != -1 )
	{
		struct timeval theTimeout;
		theTimeout.tv_sec   = 0;
		theTimeout.tv_usec  = inMillis * 1000;
		return select( 1, &theReadSet, 0, 0, &theTimeout );
	}
	else
	{
		return select( 1, &theReadSet, 0, 0, 0 );
	}
}

int startDaemon( void )
{
	int					theRC		= 1;
	static const char *	theFormat	= "%s%sbin%sapocd start";
	static char *		theCommand	= 0;

	if ( isNewerThanWindows98() == 0 && isDaemonEnabled() == 1 )
	{
		if ( theCommand == 0 )
		{
			const char * theDaemonInstallDir = getDaemonInstallDir();
			char * theCP = ( char * )theDaemonInstallDir +
			               strlen( theDaemonInstallDir ) - 1;
			while ( theCP-- != theDaemonInstallDir )
			{
				if ( *theCP == '\\' )
				{
					*theCP = 0;
					break;
				}
			}
			theCommand =
				( char * )malloc( strlen( theDaemonInstallDir )	+
								  ( strlen( FILESEP ) * 2 )		+
								  strlen( theFormat ) + 1 );
			if ( theCommand != 0 )
			{
				sprintf( theCommand,
			 		 	theFormat,
			 		 	theDaemonInstallDir,
			 		 	FILESEP,
			 		 	FILESEP );
			}
		}
		if ( theCommand != 0 )
		{
			STARTUPINFO			theStartupInfo;
			PROCESS_INFORMATION	theProcessInfo;	

			ZeroMemory( &theStartupInfo, sizeof( theStartupInfo ) );
			theStartupInfo.dwFlags		= STARTF_USESHOWWINDOW;
			theStartupInfo.wShowWindow	= SW_HIDE;
			theStartupInfo.cb			= sizeof( theStartupInfo );
			ZeroMemory( &theProcessInfo, sizeof( theProcessInfo ) );
			if ( CreateProcess( 0,
								theCommand,
								0,
								0,
								FALSE,
								DETACHED_PROCESS,
								0,
								0,
								&theStartupInfo,
								&theProcessInfo ) != 0 )
			{
				theRC = 0;
				CloseHandle( theProcessInfo.hProcess );
				CloseHandle( theProcessInfo.hThread );
			}
		}
	}
	return theRC;
}

int unlockMutex( PAPIMutex * inMutex )
{
	int theRC = -1;
	if ( inMutex != 0 )
	{
		LeaveCriticalSection( inMutex );
		theRC = 0;
	}
	return theRC;
}

void encodeGSSAPITokens( SecBuffer *	inBuffers,
						 int			inBufferCount,
						 char **		outB64Buffer )
{
	void *	theBuffer;
	char *	theTmpPtr;	
	int		theBufferSize = 0;
	int		theIndex;

	for ( theIndex = 0; theIndex < inBufferCount; ++ theIndex )
	{
		theBufferSize += inBuffers[ theIndex ].cbBuffer + 5;
	}

	theBuffer = malloc( theBufferSize );
	theTmpPtr = ( char *)theBuffer;
	for ( theIndex = 0; theIndex < inBufferCount; ++ theIndex )
	{
		/* Write the token size */
		snprintf( theTmpPtr, 6, "%05d", inBuffers[ theIndex ].cbBuffer );
		theTmpPtr[ 5 ] = 0;
		theTmpPtr += 5;

		/* Write the token */
		memcpy( ( void *)theTmpPtr,
				inBuffers[ theIndex ].pvBuffer,
				inBuffers[ theIndex ].cbBuffer );
		theTmpPtr += inBuffers[ theIndex ].cbBuffer;
	}
	base64_encode( theBuffer, theBufferSize, outB64Buffer );
	free( ( void * )theBuffer );
}

PSecurityFunctionTable getSecurityInterface( int * outMaxTokenSize )
{
	static PSecurityFunctionTable	theSecurityInterface	= 0;
	static int						isInitialised			= 0;

	if ( isInitialised == 0 )
	{
		isInitialised = 1;
		theSecurityInterface = InitSecurityInterface();
		if ( theSecurityInterface != 0 )
		{
			PSecPkgInfo thePkgInfo;
			if ( ( theSecurityInterface->QuerySecurityPackageInfo )
				   (
					"Kerberos",
					&thePkgInfo
				   ) != SEC_E_OK )
			{
				theSecurityInterface = 0;
			}
			*outMaxTokenSize = thePkgInfo->cbMaxToken;
		}
	}
	return theSecurityInterface;
}

int isRegistryEnabled( void )
{
	int		theRC = 0;
	HKEY	theKey;	
	if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
	                   APOCENABLEDKEY,
	                   0,
	                   KEY_QUERY_VALUE,
	                   &theKey ) == ERROR_SUCCESS )
	{
		RegCloseKey( theKey );
		theRC = 1;
	}
	return theRC;
}

int isServiceEnabled( void )
{
	int		theRC		= 0;
	HMODULE	theModule	= GetModuleHandle( "advapi32" );

	if ( theModule != 0 )
	{
		OpenSCManagerFunc		fOpenSCManager		=
			( OpenSCManagerFunc )GetProcAddress( theModule, "OpenSCManagerA" );
		EnumServicesStatusFunc	fEnumServicesStatus	=
			( EnumServicesStatusFunc )GetProcAddress( theModule,
		                                              "EnumServicesStatusA" );
		if ( fOpenSCManager != 0 && fEnumServicesStatus != 0 )
		{
			SC_HANDLE theSCManager =
				fOpenSCManager( 0, 0, SC_MANAGER_ENUMERATE_SERVICE );
			if ( theSCManager != 0 )
			{
				DWORD theBytes;
				DWORD theCount;
				DWORD theHandle = 0;

				if ( fEnumServicesStatus(
						theSCManager,
						SERVICE_WIN32,
						SERVICE_STATE_ALL,
						0,
						0,
						&theBytes,
						&theCount,
						&theHandle ) == 0				&&
				     GetLastError() == ERROR_MORE_DATA	&&
				     theBytes > 0 )
				{
					ENUM_SERVICE_STATUS * theServices =
						( ENUM_SERVICE_STATUS * )malloc( theBytes );
					if ( theServices != 0 )
					{
						DWORD theIndex;

						fEnumServicesStatus(
							theSCManager,
							SERVICE_WIN32,
							SERVICE_STATE_ALL,
							theServices,
							theBytes,
							&theBytes,
							&theCount,
							&theHandle );
						for ( theIndex = 0; theIndex < theCount; ++ theIndex )
						{
							if ( strcmp( theServices[ theIndex ].lpServiceName,
										 APOCSERVICENAME ) == 0 )
							{
								theRC = 1;
								break;
							}
						}
					}
				}
				CloseServiceHandle( theSCManager );
			}
		}
	}
	return theRC;
}

void writeGSSAPIAuthId( PSecurityFunctionTable	inSecurityInterface,
						CredHandle *			inCredentials,				
						CtxtHandle *			inContext,
						SecBuffer *				outBuffer,
						PAPIStatus *			outStatus )
{
	SecPkgContext_Sizes		theContextSizes;
	SecPkgContext_Names		theSourceName;
	SECURITY_STATUS			theStatus =
		( inSecurityInterface->QueryCredentialsAttributes )
		  (
			inCredentials,
			SECPKG_CRED_ATTR_NAMES,
			( PVOID )&theSourceName
		  );
	*outStatus = PAPIAuthenticationFailure;
	if ( theStatus == SEC_E_OK )
	{
		theStatus = ( inSecurityInterface->QueryContextAttributes )
					  (	
						inContext,
						SECPKG_ATTR_SIZES,
						( PVOID )&theContextSizes
					  );
		if ( theStatus == SEC_E_OK )
		{
			SecBufferDesc	theBufferDesc;
			SecBuffer		theBuffers[ 3 ];
			int				theSourceNameSize = strlen(theSourceName.sUserName);
			char *			theCP;
			BYTE *			theOutput		  =
				( PBYTE )malloc( theContextSizes.cbSecurityTrailer	+
								 theSourceNameSize					+
								 4									+
								 sizeof( DWORD ) );

			theBufferDesc.ulVersion		= SECBUFFER_VERSION;
			theBufferDesc.cBuffers		= 3;
			theBufferDesc.pBuffers		= theBuffers;

			theBuffers[ 0 ].cbBuffer	= theContextSizes.cbSecurityTrailer;
			theBuffers[ 0 ].BufferType	= SECBUFFER_TOKEN;
			theBuffers[ 0 ].pvBuffer	= theOutput + sizeof( DWORD );

			
			theBuffers[ 1 ].cbBuffer	= theSourceNameSize + 4;
			theBuffers[ 1 ].BufferType	= SECBUFFER_DATA;
			theBuffers[ 1 ].pvBuffer	= malloc( theBuffers[ 1 ].cbBuffer );
			theCP = ( char * )theBuffers[ 1 ].pvBuffer;
			theCP[ 0 ] = 1;
			theCP[ 1 ] = theCP[ 2 ] = theCP[ 3 ] = ( char )0;
			memcpy( theCP + 4,
					theSourceName.sUserName,
					theSourceNameSize );

			theBuffers[ 2 ].cbBuffer	= theContextSizes.cbBlockSize;
			theBuffers[ 2 ].BufferType	= SECBUFFER_PADDING;
			theBuffers[ 2 ].pvBuffer	= malloc( theBuffers[ 2 ].cbBuffer );

			theStatus = inSecurityInterface->EncryptMessage( inContext,
										KERB_WRAP_NO_ENCRYPT,
										&theBufferDesc,
										0 );
			if ( theStatus == SEC_E_OK )
			{
				outBuffer->cbBuffer = theBuffers[ 0 ].cbBuffer +
									  theBuffers[ 1 ].cbBuffer + 
									  theBuffers[ 2 ].cbBuffer;
				outBuffer->pvBuffer = malloc( outBuffer->cbBuffer );
				if ( outBuffer->pvBuffer != 0 )
				{
					theCP = ( char * )outBuffer->pvBuffer;
					memcpy( theCP,
							theBuffers[ 0 ].pvBuffer,
							theBuffers[ 0 ].cbBuffer );
					memcpy( theCP + theBuffers[ 0 ].cbBuffer,
							theBuffers[ 1 ].pvBuffer,
							theBuffers[ 1 ].cbBuffer );
					memcpy( theCP						+
							theBuffers[ 0 ].cbBuffer	+
							theBuffers[ 1 ].cbBuffer,
							theBuffers[ 2 ].pvBuffer,
							theBuffers[ 2 ].cbBuffer );
					*outStatus = PAPISuccess;
				}
				else
				{
					*outStatus = PAPIOutOfMemoryFailure;
				}
			}
		}
	}
	else
	{
		*outStatus = PAPIAuthenticationFailure;
	}
}
