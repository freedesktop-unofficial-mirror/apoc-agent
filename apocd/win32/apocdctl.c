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

#include <papi/papiUtils.h>

#include <stdlib.h>
#include <stdio.h>

#ifndef WNT
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#endif

typedef unsigned char	daemonOp;

static int			authenticate( PAPISocket inSocket );

static int			doDisable( int inQuite );

static int			doEnable( void );

static int			doGeneric( daemonOp inOp, int inQuiet );

static int			doIsEnabled( void );

static int			doOp( daemonOp inOp );

static int			doServiceOp( void * inService, int inOp );

#ifndef WNT
static int			doStart( int inQuite );
#else
static int			doStart( int inQuite, HANDLE * outHandle );
#endif

static int			doStop( void );

static int			doRestart( void );

static void *		getService( void );

static int			getResponse( int inSocket, char * outResponse );

static void			invalidArg( const char * inArg );

static void			putEnv( void );

static int			sendRequest( int					inSocket,
							 	 const unsigned char *	inRequest );

static daemonOp		stringToDaemonOp( const char *	inString );

static void			usage( void );

static void			writeInitialMessage( daemonOp inOp );

static void			writeFinalMessage( daemonOp inOp, int inStatus );

#ifndef WNT

static int			doInetdDisable( void );

static int			doInetdEnable( void );

static int			doInetdStart( void );

static pid_t		getPid( void );

static const char *	getPidFile( void );

static int			isInetdRunning( void );

static void			killPid( void );

static void			putPid( void );

static const char *		sPidFile			= ".pid";

#ifdef LINUX

#define SCRIPT_DISABLE	"#!/bin/sh\n				\
if [ -f /etc/inetd.conf ] ; then\n					\
 grep -v apocd /etc/inetd.conf > /tmp/inetd.conf\n	\
 cp /tmp/inetd.conf /etc/inetd.conf\n				\
 rm -f /tmp/inetd.conf\n							\
 /etc/init.d/inetd status > /dev/null 2>&1\n		\
 if [ $? -eq 0 ] ; then\n							\
  /etc/init.d/inetd restart\n						\
 fi\n												\
fi\n												\
exit 0\n"

#define SCRIPT_ENABLE	"#!/bin/sh\n				\
if [ -f /etc/inetd.conf ] ; then\n					\
 grep -v apocd /etc/inetd.conf > /tmp/inetd.conf\n	\
 cp /tmp/inetd.conf /etc/inetd.conf\n				\
 rm -f /tmp/inetd.conf\n							\
 echo \"apocd dgram udp wait root /usr/lib/apoc/apocd apocd inetdStart\" >> /etc/inetd.conf\n\
 /etc/init.d/inetd status > /dev/null 2>&1\n		\
 if [ $? -eq 0 ] ; then\n							\
  /etc/init.d/inetd restart\n						\
 fi\n												\
fi\n												\
exit 0\n"

#else

#define SCRIPT_DISABLE	"#!/bin/sh\n				\
if [ -f /etc/inetd.conf ] ; then\n					\
 grep -v apocd /etc/inetd.conf > /tmp/inetd.conf\n	\
 cp /tmp/inetd.conf /etc/inetd.conf\n				\
 rm -f /tmp/inetd.conf\n							\
 pkill -HUP inetd > /dev/null 2>&1\n				\
fi\n												\
exit 0\n"

#define SCRIPT_ENABLE	"#!/bin/sh\n				\
if [ -f /etc/inetd.conf ] ; then\n					\
 grep -v apocd /etc/inetd.conf > /tmp/inetd.conf\n	\
 cp /tmp/inetd.conf /etc/inetd.conf\n				\
 rm -f /tmp/inetd.conf\n							\
 echo \"apocd dgram udp wait root /usr/lib/apoc/apocd apocd inetdStart\" >> /etc/inetd.conf\n\
 pkill -HUP inetd > /dev/null 2>&1\n				\
fi\n												\
exit 0\n"

#endif

#else

#include <windows.h>

typedef WINADVAPI BOOL ( WINAPI *ChangeServiceConfig2Func )(	SC_HANDLE, DWORD, LPVOID );

typedef WINADVAPI BOOL ( WINAPI *CloseServiceHandleFunc )(	SC_HANDLE );

typedef WINADVAPI BOOL ( WINAPI *ControlServiceFunc )(	SC_HANDLE, DWORD, LPSERVICE_STATUS );

typedef WINADVAPI SC_HANDLE ( WINAPI *CreateServiceFunc )(	SC_HANDLE,
											LPCTSTR,
											LPCTSTR,
											DWORD,
											DWORD,
											DWORD,
											DWORD,
											LPCTSTR,
											LPCTSTR,
											LPDWORD,
											LPCTSTR,
											LPCTSTR,
											LPCTSTR );

typedef WINADVAPI BOOL ( WINAPI *DeleteServiceFunc )(		SC_HANDLE );

typedef WINADVAPI SC_HANDLE ( WINAPI *OpenSCManagerFunc )(	LPCTSTR, LPCTSTR, DWORD );

typedef WINADVAPI SC_HANDLE ( WINAPI *OpenServiceFunc )(		SC_HANDLE, LPCTSTR, DWORD );

typedef WINADVAPI BOOL ( WINAPI *SetServiceStatusFunc )(		SERVICE_STATUS_HANDLE, LPSERVICE_STATUS );

typedef WINADVAPI SERVICE_STATUS_HANDLE ( WINAPI *RegisterServiceCtrlHandlerFunc )( LPCTSTR, LPHANDLER_FUNCTION );

typedef WINADVAPI BOOL ( WINAPI *StartServiceFunc )( SC_HANDLE, DWORD, LPCTSTR * );

typedef WINADVAPI BOOL ( WINAPI *StartServiceCtrlDispatcherFunc )(	CONST LPSERVICE_TABLE_ENTRY );

ChangeServiceConfig2Func		fChangeServiceConfig2;
CloseServiceHandleFunc			fCloseServiceHandle;
ControlServiceFunc				fControlService;
CreateServiceFunc				fCreateService;
DeleteServiceFunc				fDeleteService;
OpenSCManagerFunc				fOpenSCManager;
OpenServiceFunc					fOpenService;
RegisterServiceCtrlHandlerFunc	fRegisterServiceCtrlHandler;
SetServiceStatusFunc			fSetServiceStatus;
StartServiceFunc				fStartService;
StartServiceCtrlDispatcherFunc	fStartServiceCtrlDispatcher;

static int				doRegistryDisable(	void );

static int				doRegistryEnable(	void );

static int				doServiceDisable(	void );

static int				doServiceEnable(	void );

static DWORD			getServiceStatus(	void );

static int				loadServiceFuncs(	void );

static void				serviceCtrlHandler(	DWORD inOpCode );

static void				serviceMain(		DWORD inArgc, LPTSTR * inArgv );

static void				serviceRestartLoop(	HANDLE inHandle );

static void				setServiceStatus(	DWORD inStatus );

static const char *		sServiceProgram		= "apocd.exe";

static const char *		sServiceName		= "apocd";

static const char *		sServiceDisplayName	= "Configuration Agent";

static const char *		sServiceDescription = "Maintains locally cached policy data";

SERVICE_STATUS_HANDLE	gServiceStatusHandle;

SERVICE_STATUS			gServiceStatus;

PAPIMutex *				gServiceStatusMutex;

#endif

static const char *		sArgChangeDetect	= "change-detect";
static const char *		sArgDisable			= "disable";
static const char *		sArgEnable			= "enable";
static const char *		sArgInetdStart		= "inetdStart";
static const char *		sArgIsEnabled		= "is-enabled";
static const char *		sArgReload			= "reload";		
static const char *		sArgRestart			= "restart";
static const char *		sArgStart			= "start";
static const char *		sArgStatus			= "status";
static const char *		sArgStop			= "stop";

static unsigned char	sOpUndefined		= 0;
static unsigned char	sOpChangeDetect		= 1;
static unsigned char	sOpDisable			= 2;
static unsigned char	sOpEnable			= 3;
static unsigned char	sOpInetdStart		= 4;
static unsigned char	sOpIsEnabled		= 5;
static unsigned char	sOpReload			= 6;
static unsigned char	sOpRestart			= 7;
static unsigned char	sOpStart			= 8;
static unsigned char	sOpStatus			= 9;
static unsigned char	sOpStop				= 10;

static int				sStatusOK					= 0;
static int				sStatusEnabled				= 0;
static int				sErrorGeneric				= 1;
static int				sStatusNotEnabled			= 1;
static int				sErrorInvalidArg			= 2;
static int				sStatusNotRunning			= 3;
static int				sStatusEnabledWithWarning	= 4;
static int				sErrorNotRunning			= 7;
static int				sErrorAuthentication		= 8;

static int				sLocalCredentialsBufferSize	= 21;

int main( int inArgc, char ** inArgv )
{
	int	theRC;
	int	newerThanWindows98 = 0;

#ifdef WNT
	if ( isNewerThanWindows98() == 1 )
	{
		newerThanWindows98 = 1;
		loadServiceFuncs();
		gServiceStatusMutex = newMutex();
	}
	if ( inArgc == 1 && newerThanWindows98 == 1 )
	{
		SERVICE_TABLE_ENTRY theServiceTable[] =
    	{
        	{ ( char *)sServiceName, ( LPSERVICE_MAIN_FUNCTION )serviceMain },
        	{ NULL,             NULL }
    	};
		fStartServiceCtrlDispatcher( theServiceTable );
	}
#endif

	if ( inArgc == 2 )
	{
		daemonOp theOp = stringToDaemonOp( inArgv[ 1 ] );
		if ( theOp == sOpUndefined )
		{
			invalidArg( inArgv[ 1 ] );
			theRC = sErrorInvalidArg;
		}
		else
		{
			theRC = doOp( theOp );
		}
	}	
	else
	{
		usage();
		theRC = sErrorInvalidArg;
	}
	return theRC;
}

int authenticate( PAPISocket inSocket )
{
	int		theRC = sErrorAuthentication;
	char	theChallenge[ BUFSIZ ];
	int		theIndex = 0;

	theChallenge[ 0 ] = 0;
	while ( recv( inSocket, &theChallenge[ theIndex ++ ], 1, 0 ) != -1 )
	{
		if ( theChallenge[ theIndex - 1 ] == 0 )
		{
			if ( theChallenge[ 0 ] != 0 )
			{
				FILE * theFile = fopen( theChallenge, "r" );
				if ( theFile != 0 )
				{
					char * theCredentials =
						( char * )malloc( sLocalCredentialsBufferSize );
					if ( theCredentials != 0 )
					{
						if ( fgets( theCredentials,
									sLocalCredentialsBufferSize,
									theFile ) != 0 )
						{
							if ( send( inSocket,
								   	theCredentials,
								   	sLocalCredentialsBufferSize - 1,
								   	0 ) == ( sLocalCredentialsBufferSize - 1 ) )
							{
								char theResponse;
								if ( getResponse(inSocket,&theResponse)!= -1 &&
							         theResponse == 0 )
								{
									theRC = sStatusOK;
								}
							}
						}	
						free( ( void * )theCredentials );
					}
					fclose( theFile );
#ifdef WNT
					remove( theChallenge );
#endif
				}
			}
			break;
		}	
	}
	return theRC;
}

int doGeneric( daemonOp inOp, int inQuiet )
{
	int					theRC;
	PAPISocket			theSocket;
	PAPIConnectResult	theConnectResult =
		getConnectedSocket( getDaemonAdminPort(), SOCK_STREAM, 0, &theSocket );

	if ( inQuiet != 1 )
	{
		writeInitialMessage( inOp );
	}
	if ( theConnectResult == PAPIConnectSuccess )
	{
		if ( ( theRC = authenticate( theSocket ) ) == sStatusOK )
		{
			if ( ( theRC = sendRequest( theSocket, &inOp ) ) != -1 )
			{
				char theResponse;
				theRC = getResponse( theSocket, &theResponse );
			}
			if ( theRC == -1 )
			{
				theRC = inOp == sOpStatus ?
					sStatusNotRunning : sErrorNotRunning;
			}
			else
			{
				theRC = sStatusOK;
			}
		}
		papiClose( theSocket );
	}
	else
	{
		theRC = sStatusNotRunning;
	}
	if ( inQuiet != 1 )
	{
		writeFinalMessage( inOp, theRC );
	}
	return theRC;
}

int doIsEnabled()
{
	int theRC = isDaemonEnabled() == 1 ? sStatusEnabled : sStatusNotEnabled;
	writeInitialMessage( sOpIsEnabled );
	writeFinalMessage( sOpIsEnabled, theRC );
	return theRC;
}

int doOp( daemonOp inOp )
{
	int		theRC;

	if ( inOp == sOpDisable )
	{
		theRC = doDisable( 0 );
	}
	else if ( inOp == sOpEnable )
	{
		theRC = doEnable();
	}
#ifndef WNT
	else if ( inOp == sOpInetdStart )
	{
		theRC = doInetdStart();
	}
#endif
	else if ( inOp == sOpIsEnabled )
	{
		theRC = doIsEnabled();
	}
	else if ( inOp == sOpStart || inOp == sOpStop || inOp == sOpRestart )
	{
		void * theService = getService();
		if ( theService != 0 )
		{
			theRC = doServiceOp( theService, inOp );
		}
		else
		{
			if ( inOp == sOpStart )
			{
#ifndef WNT
				theRC = doStart( 0 );
#else
				theRC = doStart( 0, 0 );
#endif
			}
			else if ( inOp == sOpStop )
			{
				theRC = doStop();
			}
			else if ( inOp == sOpRestart )
			{
				theRC = doRestart();
			}
		}
	}
	else
	{
		theRC = doGeneric( inOp, 0 );
	}
	return theRC;
}

int doServiceOp( void * inService, int inOp )
{
	int theRC = sStatusOK;
#ifdef WNT
	if ( inOp == sOpStop || inOp == sOpRestart )
	{
		SERVICE_STATUS	theServiceStatus;
		BOOL			theStatus;

		writeInitialMessage( sOpStop );
		theStatus =
		 fControlService( inService, SERVICE_CONTROL_STOP, &theServiceStatus );
		if ( theStatus == 0 &&
             GetLastError() != ERROR_SERVICE_NOT_ACTIVE )
		{
			theRC = sErrorGeneric;
		}
		writeFinalMessage( sOpStop, theRC );
	}
	if ( inOp == sOpStart || ( inOp == sOpRestart && theRC == sStatusOK ) )
	{
		BOOL theStatus;

		writeInitialMessage( sOpStart );
		theStatus = fStartService( inService, 0, NULL );
		if ( theStatus == 0 &&
		     GetLastError() != ERROR_SERVICE_ALREADY_RUNNING ) 
		{
			theRC = sErrorGeneric;
		}
		writeFinalMessage( sOpStart, theRC );
	}
#endif
	return theRC;
}

#ifndef WNT
int doStart( int inQuiet )
#else
int doStart( int inQuiet, HANDLE * outHandle )
#endif
{
	static char * 	theFormat	= 
	"java -Djava.library.path=%s -cp %s%sapocd.jar%s%s%sdb.jar%s%s%sldapjdk.jar%s%s%sspi.jar com.sun.apoc.daemon.apocd.Daemon %s %s";
	static char * 	theCommand	= 0;
	int				theRC		= sStatusOK;

	if ( inQuiet != 1 )
	{
		writeInitialMessage( sOpStart );
	}
	if ( isDaemonEnabled() == 1 )
	{
		putEnv();
		if ( theCommand == 0 )
		{
			const char * theDaemonInstallDir	= getDaemonInstallDir();
			const char * theDaemonLibraryDir	= getDaemonLibraryDir();
			const char * theDaemonPropertiesDir	= getDaemonPropertiesDir();
			theCommand = 
				( char * )malloc( ( strlen( theDaemonInstallDir ) * 5 )	+
							  	  strlen( theDaemonLibraryDir )			+
							  	  strlen( theDaemonPropertiesDir )		+
							  	  ( strlen( FILESEP ) * 4 )				+
							      ( strlen( PATHSEP ) * 3 )				+
							      strlen( theFormat ) + 1 );
			if ( theCommand != 0 )
			{
				sprintf( theCommand,
					 	 theFormat,	
					     theDaemonLibraryDir,
					     theDaemonInstallDir, FILESEP, PATHSEP,
					     theDaemonInstallDir, FILESEP, PATHSEP,
					     theDaemonInstallDir, FILESEP, PATHSEP,
					     theDaemonInstallDir, FILESEP,
					     theDaemonInstallDir,
						 theDaemonPropertiesDir );
			}
		}
		if ( theCommand != 0 )
		{	
#ifndef WNT
			if ( papiPopen( theCommand, "w" ) == 0 )
			{
				theRC = sErrorGeneric;
			}
#else
			STARTUPINFO			theStartupInfo;
			PROCESS_INFORMATION	theProcessInfo;

		
			ZeroMemory( &theStartupInfo, sizeof( theStartupInfo ) );
			theStartupInfo.dwFlags		= STARTF_USESHOWWINDOW;
			theStartupInfo.wShowWindow	= SW_HIDE;
        	theStartupInfo.cb = sizeof( theStartupInfo );
        	ZeroMemory( &theProcessInfo, sizeof( theProcessInfo ) ); 
			if ( CreateProcess( 0,
						        theCommand,
						        0,
						        0,
						        FALSE,
						        0,
						        0,
						        0,
						        &theStartupInfo,
						        &theProcessInfo ) == 0 )
			{
				theRC = sErrorGeneric;	
			}
			else if ( outHandle != 0 )
			{
				*outHandle = theProcessInfo.hProcess;
			}
#endif
		}
		else
		{
			theRC = sErrorGeneric;
		}
	}
	else
	{
		theRC = sErrorGeneric;
	}
	if ( inQuiet != 1 )
	{
		writeFinalMessage( sOpStart, theRC );
	}
	return theRC;
}

int doStop( void )
{
	int theRC;
	writeInitialMessage( sOpStop );
	theRC = doGeneric( sOpStatus, 1 );
	if ( theRC != sErrorAuthentication )
	{
		if ( theRC == sStatusNotRunning )
		{
			theRC = sStatusOK;
		}
		else
		{
			theRC = doGeneric( sOpStop, 1 );
		}
	}
	writeFinalMessage( sOpStop, theRC );
#ifndef WNT
	if ( theRC == sStatusOK )
	{
		killPid();
	}
#endif
	return theRC;
}

void * getService( void )
{
#ifndef WNT
	return 0;
#else
	if ( isNewerThanWindows98() == 0 )
	{
		return 0;
	}
	else
	{
		SC_HANDLE theService		= NULL;
		SC_HANDLE theServiceManager	=
			fOpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
		if ( theServiceManager != NULL )
		{
			theService = fOpenService( theServiceManager,
									   sServiceName,
									   GENERIC_EXECUTE );
		}
		return theService;
	}
#endif
}

int doRestart( void )
{
	doGeneric( sOpStop, 0 );
#ifndef WNT
	return doStart( 0 );
#else
	return doStart( 0, 0 );
#endif
}

int getResponse( int inSocket, char * outResponse )
{
	return recv( inSocket,
			     outResponse,
			     1,
			     0 );
}

void invalidArg( const char * inOp )
{
	if ( inOp != 0 )
	{
		fprintf( stderr, "Invalid argument '%s'\n", inOp );
	}
	usage();
}

void putEnv( void )
{
	char *	theJavaHome			= getenv( "JAVA_HOME" );

	if ( theJavaHome != 0 )
	{
		char * theJavaPath = ( char * )malloc( strlen( theJavaHome )	+
											   4						+
											   strlen( FILESEP ) );
		if ( theJavaPath != 0 )
		{
			char *	thePath = getenv( "PATH" );
			char *	theNewPath;
			sprintf( theJavaPath, "%s%sbin", theJavaHome, FILESEP );
			theNewPath = ( char * )malloc( strlen( theJavaPath )	+
										   strlen( PATHSEP )		+	
										   strlen( thePath )		+	
										   6 );
			if ( theNewPath != 0 )
			{
				sprintf( theNewPath,
						 "PATH=%s%s%s",
						 theJavaPath,
						 PATHSEP,
						 thePath );
				putenv( theNewPath );
				free( ( void * )theJavaPath );
			}
		}
	}
	papiUmask( 022 );
}

int sendRequest( int					inSocket,
				 const unsigned char *	inRequest )
{
	return send( inSocket, inRequest, 1, 0 );
}

daemonOp stringToDaemonOp( const char * inString )
{
	daemonOp theDaemonOp = sOpUndefined;
	if ( strcmp( inString, sArgChangeDetect ) == 0 )
	{
		theDaemonOp = sOpChangeDetect;
	}
	else if ( strcmp( inString, sArgDisable ) == 0 )
	{
		theDaemonOp = sOpDisable;
	}
	else if ( strcmp( inString, sArgEnable ) == 0 )
	{
		theDaemonOp = sOpEnable;
	}
	else if ( strcmp( inString, sArgInetdStart ) == 0 )
	{
		theDaemonOp = sOpInetdStart;
	}
	else if ( strcmp( inString, sArgIsEnabled ) == 0 )
	{
		theDaemonOp = sOpIsEnabled;
	}
	else if ( strcmp( inString, sArgReload ) == 0 )
	{
		theDaemonOp = sOpReload;
	}
	else
	if ( strcmp( inString, sArgRestart ) == 0 )
	{
		theDaemonOp = sOpRestart;
	}
	else
	if ( strcmp( inString, sArgStart ) == 0 )
	{
		theDaemonOp = sOpStart;
	}
	else if ( strcmp( inString, sArgStatus ) == 0 )
	{
		theDaemonOp = sOpStatus;
	}
	else
	if ( strcmp( inString, sArgStop ) == 0 )
	{
		theDaemonOp = sOpStop;
	}
	return theDaemonOp;
}

void usage( void )
{
	fprintf( stderr,
			 "%s",
			 "Usage: apocdctl change-detect | disable | enable | is-enabled | reload | restart | start | status | stop\n" );
}

void writeInitialMessage( daemonOp inOp )
{
	if ( inOp == sOpChangeDetect )
	{
		printf( "Requesting a Configuration Agent change detection ... " );
	}
	else if ( inOp == sOpDisable )
	{
		printf( "Disabling Configuration Agent ... ");
	}
	else if ( inOp == sOpEnable )
	{
		printf( "Enabling Configuration Agent ... ");
	}
	else if ( inOp == sOpIsEnabled )
	{
		printf( "Checking Configuration Agent enabled status ... " );
	}
	else if ( inOp == sOpReload )
	{
		printf( "Reloading Configuration Agent ... " );
	}
	else if ( inOp == sOpStatus )
	{
		printf( "Checking Configuration Agent status ... " );
	}
	else if ( inOp == sOpStart )
	{
		printf( "Starting Configuration Agent ... " );
	}
	else if ( inOp == sOpStop )
	{
		printf( "Stopping Configuration Agent ... " );
	}
}

void writeFinalMessage( daemonOp inOp, int inStatus )
{
	if ( inStatus == sErrorAuthentication )
	{
		printf( "Failed\n" );
	}
	else if ( inOp == sOpIsEnabled )
	{
		printf( "%s\n", inStatus == sStatusEnabled ? "Enabled" : "Not enabled");
	}
	else if ( inOp == sOpStatus )
	{
		printf( "%s\n", inStatus == sStatusOK ? "Running" : "Not running" );
	}
	else if ( inOp == sOpStop )
	{
		printf( inStatus == sStatusOK || inStatus == sErrorNotRunning ?
				"OK\n" : "Failed\n" );
	}
	else
	{
		printf( inStatus == sStatusOK ? "OK\n" : "Failed\n" );
	}
}

#ifndef WNT

int doDisable( int inQuite )
{
	int theRC;
	if ( inQuite != 1 )
	{
		writeInitialMessage( sOpDisable );
	}
#ifdef SOLARIS
	if ( haveService() != 0 )
	{
		theRC = doServiceDisable();
	}
	else
	{
		theRC = doInetdDisable();
	}
#else
	theRC = doInetdDisable();
#endif
	if ( inQuite != 1 )
	{
		writeFinalMessage( sOpDisable, theRC );
	}
	doStop();
	return theRC;
}

int doInetdDisable()
{
	char *	theFileName = tmpnam( 0 );
	FILE *	theFile;
	int		theRC		= sErrorGeneric;

	theFile  = fopen( theFileName, "w" );
	if ( theFile != 0 )
	{
		FILE *	theStream;
		char	theBuf[ BUFSIZ ];

		fprintf( theFile, "%s", SCRIPT_DISABLE );
		fclose( theFile );
		chmod( theFileName, 0700 );
		theStream = papiPopen( theFileName, "r" );
		if ( theStream != 0 )
		{
			while ( 1 )
			{
				if ( fgets( theBuf, BUFSIZ, theStream ) == 0 )
				{
					break;
				}
			}
			theRC = papiPclose( theStream );
		}
		unlink( theFileName );
	}
	return theRC;
}

int doEnable( void )
{
	int theRC;

	writeInitialMessage( sOpEnable );
#ifdef SOLARIS
	if ( haveService() != 0 )
	{
		theRC = doServiceEnable();
	}
	else
	{
		theRC = doInetdEnable();
	}
#else
	theRC = doInetdEnable();
#endif
	writeFinalMessage( sOpEnable, theRC );
    if ( theRC == sStatusOK && isInetdRunning() != 1 )
    {
        fprintf( stderr,
				 "%s\n",
				 "Warning: inetd is not running and must be started if you want the" );
		fprintf( stderr,
				 "%s\n",
		         "         Configuration Agent to start automatically." );
        theRC = sStatusEnabledWithWarning;
    }
	return theRC;
}

int doInetdEnable( void )
{
	char *	theFileName = tmpnam( 0 );
	FILE *	theFile;
	int		theRC		= sErrorGeneric;

	theFile  = fopen( theFileName, "w" );
	if ( theFile != 0 )
	{
		FILE *	theStream;
		char	theBuf[ BUFSIZ ];

		fprintf( theFile, "%s", SCRIPT_ENABLE );
		fclose( theFile );
		chmod( theFileName, 0700 );
		theStream = papiPopen( theFileName, "r" );
		if ( theStream != 0 )
		{
			while ( 1 )
			{
				if ( fgets( theBuf, BUFSIZ, theStream ) == 0 )
				{
					break;
				}
			}
			theRC = papiPclose( theStream );
		}
		unlink( theFileName );
	}
	return theRC;
}

int doInetdStart( void )
{
	if ( doStart( 1 ) == sStatusOK )
	{
		putPid();
		while ( 1 )
		{
			getchar();
		}
	}
	return 0;
}

pid_t getPid( void )
{
	pid_t			thePid		= 0;
	const char *	thePidFile	= getPidFile();
	if ( thePidFile != 0 )
	{
		FILE * thePidStream = fopen( thePidFile, "r" );
		if ( thePidStream != 0 )
		{
			fscanf( thePidStream, "%d", &thePid );
			fclose( thePidStream );
		}
		free( ( void * )thePidFile );
	}
	return thePid;
}

const char * getPidFile( void )
{
	const char *	thePidDir	= getDaemonInstallDir();
	char *			thePidFile	=
		( char * )malloc( strlen( thePidDir )		+
						  ( 2 * strlen( FILESEP ) )	+
						  strlen( sPidFile )		+
						  4 );
	if ( thePidFile != 0 )
	{
		sprintf( thePidFile,
				 "%s%s%s",
				 thePidDir,
				 FILESEP,
				 sPidFile );
	}
	return thePidFile;
}

int isInetdRunning( void )
{
	int					theRC		= 0;
	static const char * theCommand	= "/bin/ps -e | /bin/grep inetd ";
	FILE *				theStream	= papiPopen( theCommand, "r" );

	if ( theStream != 0 )
	{
		char theBuffer[ BUFSIZ ];

		while ( fgets( theBuffer, BUFSIZ, theStream ) != 0 ){}
		theRC = papiPclose( theStream ) == 0 ? 1 : 0;
	}
	return theRC;
}

void killPid( void )
{
	pid_t thePid = getPid();
	if ( thePid > 0 )
	{
		kill( thePid, SIGINT );
	}
	unlink( getPidFile() );
}

void putPid( void )
{
	const char * thePidFile = getPidFile();
	if ( thePidFile != 0 )
	{
		FILE * thePidStream = fopen( thePidFile, "w" );
		if ( thePidStream != 0 )
		{
			chmod( thePidFile, 0600 );
			fprintf( thePidStream, "%d", getpid() );
			fclose( thePidStream );
		}
		free( ( void * )thePidFile );
	}
}

#else

int doDisable( int inQuiet )
{
	int theRC;
	int	newerThanWindows98 = isNewerThanWindows98();

	if ( newerThanWindows98 == 1 )
	{
		doServiceOp( getService(), sOpStop );
		theRC = doServiceDisable();
	}
	else
	{
		theRC = doRegistryDisable();
		doStop();
	}
	return theRC;
}

int doRegistryDisable( void )
{
	int		theRC;
	LONG	theStatus;

	writeInitialMessage( sOpDisable );
	theStatus = RegDeleteKey( HKEY_LOCAL_MACHINE, APOCENABLEDKEY );
	theRC = theStatus == ERROR_SUCCESS || theStatus == ERROR_FILE_NOT_FOUND ?
		sStatusOK : sErrorGeneric;
	writeFinalMessage( sOpDisable, theRC );
	return theRC;
}

int doServiceDisable( void )
{
	int			theRC = sErrorGeneric;
	SC_HANDLE	theServiceManager;

	if ( isNewerThanWindows98() == 1 )
	{
		theServiceManager = fOpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
		writeInitialMessage( sOpDisable );
		if ( theServiceManager != 0 )
		{
			SC_HANDLE theService = fOpenService( theServiceManager,
											    sServiceName,
											    DELETE );
			if ( theService != 0 )
			{
				if ( fDeleteService( theService ) == TRUE ||
				 	 GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE )
				{
					theRC = sStatusOK;
				}
				fCloseServiceHandle( theService );	
			}
			else
			{
				if ( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST )
				{
					theRC = sStatusOK;
				}
			}
			fCloseServiceHandle( theServiceManager );
		}
		else
		{
			theRC = sStatusOK;
		}
	}
	writeFinalMessage( sOpDisable, theRC );
	return theRC;
}

int doEnable( void )
{
	int theRC;
	int	newerThanWindows98 = isNewerThanWindows98();

	writeInitialMessage( sOpEnable );
	theRC = newerThanWindows98 == 1 ?
					doServiceEnable() : doRegistryEnable();
	writeFinalMessage( sOpEnable, theRC );

	if ( newerThanWindows98 == 1 && theRC == sStatusOK )
	{
		theRC = doServiceOp( getService(), sOpStart );
	}
	return theRC;
}

int doServiceEnable( void )
{
	int		theRC				= sErrorGeneric;
	char *	theDaemonInstallDir	= getDaemonInstallDir();
	char *	theServiceProgram	=
		( char * )malloc( strlen( theDaemonInstallDir )	+
						  strlen( FILESEP )				+
					      strlen( sServiceProgram )		+
					      1 );
	if ( theServiceProgram != 0 )
	{
		SC_HANDLE theServiceManager;
		char * theCP = ( char * )theDaemonInstallDir	+
					   strlen( theDaemonInstallDir ) - 1;
		while ( theCP-- != theDaemonInstallDir )
		{
			if ( *theCP  == '\\' )
			{
				*( ++theCP ) = 0;
				break;
			}
		}
		sprintf( theServiceProgram,
				 "%sbin%s%s",
				 theDaemonInstallDir,
				 FILESEP,
				 sServiceProgram );
		theServiceManager = fOpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
		if ( theServiceManager != 0 )
		{
			SC_HANDLE theService =
				fCreateService( theServiceManager,
						       sServiceName,
						       sServiceDisplayName,
						       SERVICE_ALL_ACCESS,
						       SERVICE_WIN32_OWN_PROCESS,
						       SERVICE_AUTO_START,
						       SERVICE_ERROR_NORMAL,
						       theServiceProgram,
						       0,
						       0,
						       0,
						       0,
						       0 );
			if ( theService != 0 )
			{
				fChangeServiceConfig2( theService,
								      SERVICE_CONFIG_DESCRIPTION,
								      ( LPVOID )&sServiceDescription );
				theRC = sStatusOK;
				fCloseServiceHandle( theService );
			}
			else
			{
				if ( GetLastError() == ERROR_SERVICE_EXISTS )
				{
					theRC = sStatusOK;
				}
			}
			fCloseServiceHandle( theServiceManager );
		}
		free( ( void * )theServiceProgram );
	}
	return theRC;	
}

int doRegistryEnable( void )
{
	int		theRC = sErrorGeneric;
	HKEY	theKey;
	DWORD	theDisposition;

	if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
					APOCENABLEDKEY,
					0,
					0,
					0,
					KEY_WRITE,
					0,
					&theKey,
					&theDisposition ) == ERROR_SUCCESS )
	{
		RegCloseKey( theKey );
		theRC = sStatusOK;
	}
	return theRC;
}

DWORD getServiceStatus( void )
{
	DWORD theStatus = -1;
	if ( lockMutex( gServiceStatusMutex ) == 0 )
	{
		theStatus = gServiceStatus.dwCurrentState;
		unlockMutex( gServiceStatusMutex );	
	}
	return theStatus;
}

int loadServiceFuncs( void )
{
	HMODULE theModule = GetModuleHandle( "advapi32" );
	if ( theModule != NULL )
	{
		fChangeServiceConfig2 =
			( ChangeServiceConfig2Func )GetProcAddress(
				theModule, "ChangeServiceConfig2A" );
		fCloseServiceHandle =
			( CloseServiceHandleFunc )GetProcAddress(
				theModule, "CloseServiceHandle" );
		fControlService =
			( ControlServiceFunc )GetProcAddress(
				theModule, "ControlService" );
		fCreateService =
			( CreateServiceFunc )GetProcAddress(
				theModule, "CreateServiceA" );
		fDeleteService =
			( DeleteServiceFunc )GetProcAddress(
				theModule, "DeleteService" );
		fOpenSCManager =
			( OpenSCManagerFunc )GetProcAddress(
				theModule, "OpenSCManagerA" );
		fOpenService =
			( OpenServiceFunc )GetProcAddress(
				theModule, "OpenServiceA" );
		fRegisterServiceCtrlHandler =
			( RegisterServiceCtrlHandlerFunc )GetProcAddress(
				theModule, "RegisterServiceCtrlHandlerA" );
		fSetServiceStatus =
			( SetServiceStatusFunc )GetProcAddress(
				theModule, "SetServiceStatus" );
		fStartService =
			( StartServiceFunc )GetProcAddress(
				theModule, "StartServiceA" );
		fStartServiceCtrlDispatcher =
			( StartServiceCtrlDispatcherFunc )GetProcAddress(
				theModule, "StartServiceCtrlDispatcherA" );
	}
	return 1;	
}

void serviceCtrlHandler( DWORD inOpcode )
{
	switch( inOpcode )
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			setServiceStatus( SERVICE_STOPPED );
			doGeneric( sOpStop, 1 );
			break;

		case SERVICE_CONTROL_INTERROGATE:
			setServiceStatus( doGeneric( sOpStatus, 1 ) == 0 ?
								SERVICE_RUNNING : SERVICE_STOPPED );
			break;

		default:
			break;
	}
	gServiceStatus.dwWin32ExitCode	= NO_ERROR;
	gServiceStatus.dwCheckPoint		= 0;
	gServiceStatus.dwWaitHint		= 0;
	fSetServiceStatus( gServiceStatusHandle, &gServiceStatus );
}

void serviceMain( DWORD inArgc, LPTSTR * inArgv )
{
	HANDLE theHandle;
	gServiceStatusHandle =
		fRegisterServiceCtrlHandler(
			sServiceName,
			( LPHANDLER_FUNCTION )serviceCtrlHandler );

	gServiceStatus.dwServiceType				= SERVICE_WIN32_OWN_PROCESS;
	setServiceStatus( SERVICE_START_PENDING );
	gServiceStatus.dwControlsAccepted			= SERVICE_ACCEPT_STOP |
												  SERVICE_ACCEPT_SHUTDOWN;
	gServiceStatus.dwWin32ExitCode				= 0;
	gServiceStatus.dwServiceSpecificExitCode	= 0;
	gServiceStatus.dwCheckPoint					= 0;
	gServiceStatus.dwWaitHint					= 0;
	fSetServiceStatus( gServiceStatusHandle, &gServiceStatus );

	if ( doStart( 1, &theHandle ) == 0 )
	{
		setServiceStatus( SERVICE_RUNNING );
		gServiceStatus.dwWin32ExitCode	= NO_ERROR;
		gServiceStatus.dwCheckPoint		= 0;
		gServiceStatus.dwWaitHint		= 0;	
		fSetServiceStatus( gServiceStatusHandle, &gServiceStatus );
		serviceRestartLoop( theHandle );
	}
	else
	{
		setServiceStatus( SERVICE_STOPPED );
		gServiceStatus.dwWin32ExitCode	= ERROR_GEN_FAILURE;
		gServiceStatus.dwCheckPoint		= 0;
		gServiceStatus.dwWaitHint		= 0;
		fSetServiceStatus( gServiceStatusHandle, &gServiceStatus );
	}
}

void serviceRestartLoop( HANDLE inHandle )
{
	while ( 1 )
	{
		WaitForSingleObject( inHandle, INFINITE );
		Sleep( 5000 );
		if ( getServiceStatus() == SERVICE_RUNNING )
		{
			doStart( 1, &inHandle );
		}
		else
		{
			break;
		}
	}
}

void setServiceStatus( DWORD inStatus )
{
	if ( lockMutex( gServiceStatusMutex ) == 0 )
	{
		gServiceStatus.dwCurrentState = inStatus;
		unlockMutex( gServiceStatusMutex );
	}
}

#endif
