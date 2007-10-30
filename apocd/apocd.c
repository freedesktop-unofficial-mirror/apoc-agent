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

#include "apocd.h"
#include "config.h"

#include <papiUtils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

static int		authenticate( PAPISocket inSocket );
static Status	daemonChangeDetect( void );
static Status	daemonReload( void );
static Status	daemonRequest( DaemonOp inOp, int inQuiet );
static Status	daemonRestart( void );
static Status	doOp( DaemonOp inOp );
static int		getDaemonAdminPort( void );
static int		getResponse( int inSocket, char * outResponse );
static void		invalidArg( const char * inArg );
static Status	javaStart( int inWait );
static void		putEnv( void );
static int		sendRequest( int inSocket, const unsigned char * inRequest );
static DaemonOp	stringToDaemonOp( const char * inString );
static void		usage( void );

static const char *		sArgChangeDetect	= "change-detect";
static const char *		sArgDisable			= "disable";
static const char *		sArgEnable			= "enable";
static const char *		sArgIsEnabled		= "is-enabled";
static const char *		sArgReload			= "reload";		
static const char *		sArgRestart			= "restart";
static const char *		sArgStart			= "start";
static const char *		sArgStatus			= "status";
static const char *		sArgStop			= "stop";
static const char *		sArgsvcStart		= "svcStart";
static const char *		sArgsvcStop			= "svcStop";

int main( int inArgc, char ** inArgv )
{
	Status theRC;

	if ( inArgc == 2 )
	{
		DaemonOp theOp = stringToDaemonOp( inArgv[ 1 ] );
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

Status daemonStart( void )
{
	Status theRC = daemonStatus( 1 );
	writeInitialMessage( sOpStart );
	if ( theRC == sStatusNotRunning )
	{
#ifndef LINUX
		theRC = javaStart( 0 );
		if ( theRC == sStatusOK )
		{
			theRC = daemonWaitStatus( sStatusOK, 30 );
		}
#else
		if ( fork() == 0 )
		{
			while ( 1 )
			{
				theRC = javaStart( 1 );
				if ( theRC == sStatusOK )
				{
					break;
				}
				if ( osDaemonShouldRestart() == 0 )
				{
					theRC = sErrorGeneric;
					break;
				}
			}
			return theRC;
		}
		else
		{
			theRC = daemonWaitStatus( sStatusOK, 30 );
		}
#endif
	}
	writeFinalMessage( sOpStart, theRC );
	return theRC;
}

Status daemonStatus( int inQuiet )
{
	return daemonRequest( sOpStatus, inQuiet ) == sStatusOK ?
		sStatusOK : sStatusNotRunning;
}

Status daemonStop( void )
{
	Status		theRC;
	writeInitialMessage( sOpStop );
	daemonRequest( sOpStop, 1 );
	theRC =
		daemonWaitStatus( sStatusNotRunning, 20 ) == sStatusNotRunning ?
			sStatusOK : sErrorGeneric;
	writeFinalMessage( sOpStop, theRC );
	return theRC;
}

Status daemonWaitStatus( Status inStatus, int inMaxIterations )
{
	Status	theRC;
	int		theIteration = 0;

	while ( ( theRC = daemonStatus( 1 ) ) != inStatus &&
	        ++ theIteration <= inMaxIterations )
	{
		papiSleep( 1 );
	}
	return theRC;
}

void writeInitialMessage( DaemonOp inOp )
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
	fflush( stdout );
}

void writeAlreadyMessage(DaemonOp inOp)
{
    const char *theMessage = NULL ;

    switch (inOp) {
        case sOpStart:
            theMessage = "Already started" ;
            break ;
        case sOpStop:
            theMessage = "Already stopped" ;
            break ;
        case sOpEnable:
            theMessage = "Already enabled" ;
            break ;
        case sOpDisable:
            theMessage = "Already disabled" ;
            break ;
        default:
            break ;
    }
    if (theMessage != NULL) {
        printf("%s\n", theMessage) ;
    }
}

void writeFinalMessage( DaemonOp inOp, Status inStatus )
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
	fflush( stdout );
}

int authenticate( PAPISocket inSocket )
{
	static int sLocalCredentialsBufferSize = 21;

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
				}
			}
			break;
		}	
	}
	return theRC;
}

Status daemonChangeDetect( void )
{
	return daemonRequest( sOpChangeDetect, 0 );
}

Status daemonReload( void )
{
	return daemonRequest( sOpReload, 0 );
}

Status daemonRequest( DaemonOp inOp, int inQuiet )
{
	Status				theRC				= sErrorNotRunning;
	PAPISocket			theSocket;
	PAPIConnectResult	theConnectResult	=
		getConnectedSocket( getDaemonAdminPort(), SOCK_STREAM, 0, &theSocket );
	if ( inQuiet == 0 )
	{
		writeInitialMessage( inOp );
	}
	if ( theConnectResult == PAPIConnectSuccess )
	{
		if ( ( theRC = authenticate( theSocket ) ) == sStatusOK )
		{
			unsigned char theOp = ( unsigned char )inOp;
			if ( sendRequest( theSocket, &theOp ) != -1 )
			{
				char theResponse;
				if ( getResponse( theSocket, &theResponse ) == -1 )
				{
					theRC = sErrorNotRunning;
				}
				else if ( theResponse == 0 )
				{
					theRC = sStatusOK;
				}
				else
				{
					theRC = sErrorGeneric;
				}
			}
		}
		papiClose( theSocket );
	}
	if ( inQuiet == 0 )
	{
		writeFinalMessage( inOp, theRC );
	}
	return theRC;
}

Status daemonRestart( void )
{
	Status theRC = osDaemonStop();
	if ( theRC == sStatusOK )
	{
		theRC = osDaemonStart();
	}
	return theRC;
}

static int checkAlreadyDone(DaemonOp inOp) 
{
    int retCode = 0 ;

    switch (inOp) {
        case sOpStart:
            if (daemonStatus(1) == sStatusRunning) { retCode = 1 ; }
            break ;
        case sOpStop:
            if (daemonStatus(1) == sStatusNotRunning) { retCode = 1 ; }
            break ;
        case sOpEnable:
            if (osDaemonIsEnabled(1) == sStatusEnabled) { retCode = 1 ; }
            break ;
        case sOpDisable:
            if (osDaemonIsEnabled(1) == sStatusNotEnabled) { retCode = 1 ; }
            break ;
        default:
            break ;
    }
    return retCode ;
}

Status doOp( DaemonOp inOp )
{
	Status theStatus = sErrorGeneric;

    if (checkAlreadyDone(inOp)) { 
        writeInitialMessage(inOp) ;
        writeAlreadyMessage(inOp) ;
        return sStatusOK ; 
    }
	/*
	 * Non os specific operations
	 */
	if ( inOp == sOpChangeDetect )
	{
		theStatus = daemonChangeDetect();
	}
	else if ( inOp == sOpReload )
	{
		theStatus = daemonReload();
	}
	else if ( inOp == sOpRestart )
	{
		theStatus = daemonRestart();
	}
	else if ( inOp == sOpStatus )
	{
		theStatus = daemonStatus( 0 );
	}

	/*
	 * os specific operations
	 */
	else if ( inOp == sOpDisable )
	{
		theStatus = osDaemonDisable();
	}
	else if ( inOp == sOpEnable )
	{
		theStatus = osDaemonEnable();
	}
	else if ( inOp == sOpIsEnabled )
	{
		theStatus = osDaemonIsEnabled(0);
	}
	else if ( inOp == sOpStart )
	{
		theStatus = osDaemonStart();
	}
	else if ( inOp == sOpStop )
	{
		theStatus = osDaemonStop();
	}
	else if ( inOp == sOpSvcStart )
	{
		theStatus = osDaemonSvcStart();
	}
	else if ( inOp == sOpSvcStop )
	{
		theStatus = osDaemonSvcStop();
	}
	return theStatus;
}

/*
 * 6417539 - Need to use IANA registered port
 *
 * IANA has registered 3809 for us. We use a udp connection on this port to
 * the daemon to ask what port is currently set as the admin port
 */
int getDaemonAdminPort( void )
{
	static char			sByte			= 1;
	static const char *	sLocalHostName	= "127.0.0.1";

	int					thePort			= -1;
	PAPISocket			theSocket;
	char				thePortString[ 6 ];
	struct sockaddr_in	theDaemonAddr;
	struct hostent *	theHostEnt; 
	int					theCount;

	theHostEnt					= gethostbyname( sLocalHostName );
	theDaemonAddr.sin_family	= AF_INET;
	theDaemonAddr.sin_port		= htons( getDaemonPort() );
	theDaemonAddr.sin_addr		= *( (struct in_addr * )theHostEnt->h_addr );
	memset( ( void * )&( theDaemonAddr.sin_zero ), 0, 8 );
	if ( ( theSocket = socket( AF_INET, SOCK_DGRAM, 0 ) ) != INVALID_SOCKET &&
	     connect( theSocket,
		          ( struct sockaddr * )&theDaemonAddr,
		          sizeof( struct sockaddr ) ) == 0 &&
	     send( theSocket, &sByte, 1, 0 ) == 1 &&
		 ( theCount = recv( theSocket, thePortString, 6, 0 ) ) > 1 &&
		 thePortString[ theCount - 1 ] == 0 )
	{
		thePort = atoi( thePortString );
	}
	papiClose( theSocket );
	return thePort;
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

Status javaStart( int inWait )
{
	static char * 	theFormat	= 
	"java -Djava.library.path=%s%s%s -cp %s%sapocd.jar com.sun.apoc.daemon.apocd.Daemon %s %s";
	static char * 	theCommand	= 0;
	Status			theRC		= sStatusOK;

	putEnv();
	if ( theCommand == 0 )
	{
		const char * theDaemonInstallDir	= getDaemonInstallDir();
		const char * theDaemonLibraryDir	= getDaemonLibraryDir();
		const char * theDaemonPropertiesDir	= getDaemonPropertiesDir();
		theCommand = 
			( char * )malloc( ( strlen( theDaemonInstallDir ) * 2 )	+
					strlen( theDaemonLibraryDir )		+
					strlen( PATHSEP )			+
					strlen( DBJAVA_LIBDIR )			+
					strlen( theDaemonPropertiesDir )	+
					strlen( FILESEP )			+
					( strlen( theFormat ) + 6 ));

		if ( theCommand != 0 )
		{
			sprintf( theCommand,
					theFormat,	
					theDaemonLibraryDir, PATHSEP,
					DBJAVA_LIBDIR,
					theDaemonInstallDir, FILESEP,
					theDaemonInstallDir,
					theDaemonPropertiesDir );
		}
	}
	if ( theCommand != 0 )
	{	
		FILE * theStream = papiPopen( theCommand, "r" );
		if ( theStream == 0 )
		{
			theRC = sErrorGeneric;
		}
		else if ( inWait == 1 )
		{
			theRC = papiPclose( theStream ) == 0 ? sStatusOK : sErrorGeneric;
		}
	}
	else
	{
		theRC = sErrorGeneric;
	}
	return theRC;
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

DaemonOp stringToDaemonOp( const char * inString )
{
	DaemonOp theDaemonOp = sOpUndefined;
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
	else if ( strcmp( inString, sArgIsEnabled ) == 0 )
	{
		theDaemonOp = sOpIsEnabled;
	}
	else if ( strcmp( inString, sArgReload ) == 0 )
	{
		theDaemonOp = sOpReload;
	}
	else if ( strcmp( inString, sArgRestart ) == 0 )
	{
		theDaemonOp = sOpRestart;
	}
	else if ( strcmp( inString, sArgStart ) == 0 )
	{
		theDaemonOp = sOpStart;
	}
	else if ( strcmp( inString, sArgStatus ) == 0 )
	{
		theDaemonOp = sOpStatus;
	}
	else if ( strcmp( inString, sArgStop ) == 0 )
	{
		theDaemonOp = sOpStop;
	}
	else if ( strcmp( inString, sArgsvcStart ) == 0 )
	{
		theDaemonOp = sOpSvcStart;
	}
	else if ( strcmp( inString, sArgsvcStop ) == 0 )
	{
		theDaemonOp = sOpSvcStop;
	}
	return theDaemonOp;
}

void usage( void )
{
	fprintf( stderr,
			 "%s",
			 "Usage: apocd change-detect | disable | enable | is-enabled | reload | restart | start | status | stop\n" );
}
