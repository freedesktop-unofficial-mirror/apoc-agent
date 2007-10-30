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

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef WNT
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

typedef struct PAPIConfigList
{
	int						mName;
	const char *			mValue;
	struct PAPIConfigList *	mNext;
} PAPIConfigList;

typedef enum
{
	PAPIDaemonInstallDir = 0,
	PAPIDaemonPort,
	PAPIDaemonDataDir,
	PAPITransactionTimeout,
	PAPIUnknown
} PAPIConfigName;

static PAPIConfigList * addConfigList(		PAPIConfigList **	inPrevious,
									   		int					inName,
									   		char *				inValue) ;

static void				deleteConfigList(	PAPIConfigList *	inList );

static unsigned char	findNextPair(		FILE *				inFile,
											int *				outName,
											char **				outValue );

static int				mapToName(			const char *		inName );

static void				loadConfig(			void );

static void				loadConfigFile(		const char *		inFileName,
											PAPIConfigList **	inoutList );

static int				configLock(			void );

static PAPIConfigList *	newConfigList(		void );

static int				skipComments(		FILE *				inFile );

static int				configUnlock(		void );

static int				papiConnect(		PAPISocket			inSocket,
						                    struct sockaddr *	inRemote );

static const char *	sDaemonInstallDir		= 0;
static const char *	sDaemonLibraryDir		= 0;
static const char *	sDaemonPropertiesDir	= 0;
static int			sDaemonPort				= -1;
static const char *	sDataDir				= 0;
static int			sTransactionTimeout		= -1;

static int			sDefaultDaemonPort		= 3809;
static int			sDefaultTransactionTimeout = 10000;
static int			sMinimumTransactionTimeout = 1000;

static PAPIMutex *	sConfigMutex		= 0;


static const char 			sB64[ 64 ] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define ISWHITESPACE(inChar)    ((inChar) == ' ' || (inChar) == '\t')
#define ISRETURN(inChar)        ((inChar) == '\r' || (inChar) == '\n')

int base64_encode( const void *	inBuffer,
				   int			inBufferSize,
				   char **		outB64Buffer )
{
    char *s, *p;
    int i;
    int c;
    const unsigned char *q;

    p = s = (char *) malloc( inBufferSize * 4 / 3 + 4);
    if (p == NULL)
	return -1;
    q = (const unsigned char *) inBuffer;
    i = 0;
    for (i = 0; i <  inBufferSize;) {
	c = q[i++];
	c *= 256;
	if (i <  inBufferSize)
	    c += q[i];
	i++;
	c *= 256;
	if (i <  inBufferSize)
	    c += q[i];
	i++;
	p[0] = ( char )sB64[(c & 0x00fc0000) >> 18];
	p[1] = ( char )sB64[(c & 0x0003f000) >> 12];
	p[2] = ( char )sB64[(c & 0x00000fc0) >> 6];
	p[3] = ( char )sB64[(c & 0x0000003f) >> 0];
	if (i >  inBufferSize)
	    p[3] = '=';
	if (i >  inBufferSize + 1)
	    p[2] = '=';
	p += 4;
    }
    *p = 0;
    *outB64Buffer = s;
    return strlen(s);
}

const char * getDaemonDataDir( void )
{
	if ( configLock() == 0 )
	{
		if ( sDataDir == 0 )
		{
			loadConfig();
		}
		configUnlock();
	}
	return sDataDir;
}

const char * getDaemonInstallDir( void )
{
	if ( sDaemonInstallDir == 0 )
	{
		sDaemonInstallDir = getInstallDir();
	}
	return sDaemonInstallDir;
}

const char * getDaemonLibraryDir( void )
{
	if ( sDaemonLibraryDir == 0 )
	{
		sDaemonLibraryDir = getLibraryDir();
	}
	return sDaemonLibraryDir;
}

const char * getDaemonPropertiesDir( void )
{
	if ( sDaemonPropertiesDir == 0 )
	{
		sDaemonPropertiesDir = getPropertiesDir();
	}
	return sDaemonPropertiesDir;
}

int getDaemonPort( void )
{
	if ( configLock() == 0 )
	{
		if ( sDaemonPort == -1 )
		{
			loadConfig();
			if ( sDaemonPort == -1 )
			{
				sDaemonPort = sDefaultDaemonPort;
			}
		}
		configUnlock();
	}
	return sDaemonPort;
}

const char * getTimestamp( void )
{
	char *		theTimestamp	= 0;	
	time_t		theTime			= time( 0 );
	struct tm *	theGMTime		= gmtime( &theTime );
	if ( theTime != 0 )
	{
		theTimestamp = ( char * )malloc( 16 );
		if ( theTimestamp != 0 )
		{
			snprintf( theTimestamp,
					  16,
					  "%d%.2d%.2d%.2d%.2d%.2dZ",
					  theGMTime->tm_year	+ 1900,
					  theGMTime->tm_mon		+ 1,
					  theGMTime->tm_mday,
					  theGMTime->tm_hour,
					  theGMTime->tm_min,
					  theGMTime->tm_sec );
		}
	}
	return theTimestamp;
}

int getTransactionTimeout( void )
{
	if ( configLock() == 0 )
	{
		if ( sTransactionTimeout == -1 )
		{
			loadConfig();
			if ( sTransactionTimeout == -1 )
			{
				sTransactionTimeout = sDefaultTransactionTimeout;
			}
			else
			if ( sTransactionTimeout < sMinimumTransactionTimeout )
			{
				sTransactionTimeout = sMinimumTransactionTimeout;
			}
		}
		configUnlock();
	}
	return sTransactionTimeout;
}

PAPIConfigList * addConfigList( PAPIConfigList **	inPrevious,
								int					inName,
								char *		        inValue)
{
	PAPIConfigList * theConfigList = newConfigList();
	if ( theConfigList != 0 )
	{
		theConfigList->mName	= inName;
		theConfigList->mValue	= inValue ;
		theConfigList->mNext	= 0;
		if ( inPrevious[ 0 ] == 0 )
		{
			inPrevious[ 0 ] = theConfigList;
		}
		else
		{
			PAPIConfigList * theTmpList = inPrevious[ 0 ];
			while ( theTmpList->mNext != 0 )
			{
				theTmpList = theTmpList->mNext;
			}
			theTmpList->mNext = theConfigList;
		}
	}
	return theConfigList;
}

void deleteConfigList( PAPIConfigList * inList )
{
	if ( inList != 0 )
	{
		if ( inList->mNext != 0 )
		{
			deleteConfigList( inList->mNext );
		}
		if ( inList->mValue )
		{
			free( ( void * )inList->mValue );
		}
		free( inList );
	}
}

unsigned char findNextPair(FILE *inFile, int *outName, char **outValue)
{
    char theString [1024] ;
    int theChar = -1 ;
    int theStringLength = 0 ;

    *outName = PAPIUnknown ;
    *outValue = 0 ;
    /* Get rid of comments */
    theChar = skipComments(inFile) ;
    /* Find name */
    while (theChar != EOF)
    {
        /* Skip initial whitespace */
        if (theStringLength == 0 && ISWHITESPACE(theChar)) { continue ; }
        if (theChar == '\\')
        {   /* Drop escape character */
            theChar = fgetc(inFile) ;
        }
        else if (theChar == '=')
        {   /* Skip initial whitespace for possible value */
            do { theChar = fgetc(inFile) ; } while (ISWHITESPACE(theChar)) ;
            break ;
        }
        /* If we encounter a line break, something's wrong */
        if (ISRETURN(theChar)) 
        {
            theStringLength = 0 ;
            break ;
        }    
        /* Else copy into the current string */
        theString [theStringLength ++] = theChar ; 
        theChar = fgetc(inFile) ;
    }
    /* Remove trailing whitespace */
    while (theStringLength != 0 && 
            ISWHITESPACE(theString [theStringLength - 1]))
    {
        -- theStringLength ;
    }
    /* Terminate string */
    theString [theStringLength] = 0 ;
    *outName = mapToName(theString) ;
    if (*outName == PAPIUnknown)
    {   /* If uncomprehensible name, move to next line */
        while (!ISRETURN(theChar) && theChar != EOF)
        {
            theChar = fgetc(inFile) ; 
        }
        return theChar != EOF ;
    }
    theStringLength = 0 ;
    /* Find corresponding value */
    while (theChar != EOF)
    {
        /* Skip initial whitespace */
        if (theStringLength == 0 && ISWHITESPACE(theChar)) { continue ; }
        if (theChar == '\\') 
        {   /* Drop escape character */
            theChar = fgetc(inFile) ; 
        }
        if (ISRETURN(theChar))
        {   /* If end of line is reached, remove trailing whitespace */
            while (theStringLength != 0 && 
                    ISWHITESPACE(theString [theStringLength - 1]))
            {
                -- theStringLength ;
            }
            break ; 
        }
        /* Else copy into the current string */
        theString [theStringLength ++] = theChar ;
        theChar = fgetc(inFile) ;
    }
    /* Terminate and duplicate value */
    theString [theStringLength] = 0 ;
    *outValue = strdup(theString) ;
    return theChar != EOF ;
}

PAPIConnectResult getConnectedSocket(
	int inPort, int inType, int inRetryCount, PAPISocket * outSocket )
{
	static char			sRejected = 0;
	static char			sAccepted = 1;
	static const char *	sLocalHostName = "127.0.0.1";
	PAPIConnectResult	theResult;
	PAPISocket			theSocket;
	struct sockaddr_in	theDaemonAddr;
	struct hostent *	theHostEnt;
	int					bResue = 1;
#ifdef WNT
	static int			isWSAInitialised = 0;
	WSADATA				theWSAData;

	if ( isWSAInitialised == 0 )
	{
		WSAStartup( MAKEWORD( 2, 2 ), &theWSAData );
		isWSAInitialised = 1;
	}
#endif

	*outSocket = INVALID_SOCKET;

	theHostEnt					= gethostbyname( sLocalHostName );
	theDaemonAddr.sin_family	= AF_INET;
	theDaemonAddr.sin_port		= htons( inPort );
	theDaemonAddr.sin_addr		= *( (struct in_addr * )theHostEnt->h_addr );

	do
	{
		memset( ( void * )&( theDaemonAddr.sin_zero ), 0, 8 );
		theResult = PAPIConnectFailed;
		if ( ( theSocket = socket( AF_INET, inType, 0 ) ) != INVALID_SOCKET	&&
		     papiConnect( theSocket,
		                  ( struct sockaddr * )&theDaemonAddr ) == 0 )
		{
			char theInitialByte;
			if ( socketPoll( theSocket, getTransactionTimeout() )	!= 1 ||
			     recv( theSocket, &theInitialByte, 1, 0 )			!= 1 ||
			     ( theInitialByte != sAccepted && theInitialByte != sRejected ))
			{
				papiClose( theSocket );
				theResult = PAPIConnectWrongService;	
				break;
			}
			else
			{
				theResult = theInitialByte == sAccepted ?
					PAPIConnectSuccess : PAPIConnectRejected;
				if ( theResult == PAPIConnectSuccess )
				{
					*outSocket = theSocket;
					break;
				}
			}
		}
		papiClose( theSocket );
		if ( inRetryCount > 0 )
		{
			papiSleep( 1 );
		}
	} while ( --inRetryCount > 0 );
	return theResult;
}

int mapToName(const char *inName)
{
    if (strcmp(inName, "DaemonPort") == 0)
		{ return PAPIDaemonPort ; }
    else if (strcmp(inName, "DataDir") == 0)
		{ return PAPIDaemonDataDir ; }
    else if (strcmp(inName, "DaemonInstallDir") == 0)
		{ return PAPIDaemonInstallDir ; }
	else if (strcmp(inName, "TransactionTimeout") == 0)
		{ return PAPITransactionTimeout ; }
    else return PAPIUnknown ; 
}

void loadConfig( void )
{
	static const char *	sPropertiesFile	= "/apocd.properties";
    static const char * sOsPropertiesFile = "/os.properties" ;
	PAPIConfigList *	theConfigList	= 0 ;
	PAPIConfigList *	theTmpList = 0 ;
	char *				thePropertiesFileName = 0;
	int					theSize;
	int					prefixLength;
	
	if ( sDaemonPropertiesDir == 0 )
	{
		sDaemonPropertiesDir = getPropertiesDir();
	}
    prefixLength = strlen( sDaemonPropertiesDir );
	theSize = prefixLength + strlen( sPropertiesFile )	+ 1;
	thePropertiesFileName = ( char * )malloc( theSize );
	snprintf( thePropertiesFileName,
			  theSize,
			  "%s%s", 
			  sDaemonPropertiesDir,
			  sPropertiesFile );
	loadConfigFile( thePropertiesFileName, &theConfigList );
	free( ( void * )thePropertiesFileName );

	theSize = prefixLength + strlen( sOsPropertiesFile ) + 1;
	thePropertiesFileName = ( char * )malloc( theSize );
	snprintf( thePropertiesFileName, 
			  theSize,
			  "%s%s",
			  sDaemonPropertiesDir,
			 sOsPropertiesFile );
    loadConfigFile( thePropertiesFileName, &theConfigList );
    free( ( void *)thePropertiesFileName );

	theTmpList = theConfigList;
	while ( theTmpList != 0 )
	{
		switch( theTmpList->mName )
		{
			case PAPIDaemonPort:
				sDaemonPort = atoi( theTmpList->mValue );
				break;

			case PAPIDaemonDataDir:
#ifndef WNT
				sDataDir = strdup( theTmpList->mValue );
#else
				sDataDir = convertDataDir( theTmpList->mValue );
#endif
				break;

			case PAPITransactionTimeout:
				sTransactionTimeout = 1000 * atoi( theTmpList->mValue );
				break;

			default:
				break;
		}
		theTmpList = theTmpList->mNext;
	}
	deleteConfigList( theConfigList );
}

void loadConfigFile(const char *inFileName, PAPIConfigList **inoutConfigList)
{
    FILE *theFile = 0 ;
    int theName = 0 ;
    char *theValue = 0 ;

    if (inoutConfigList == 0 || inFileName == 0) { return ; }
    theFile = fopen(inFileName, "r") ;
    if (theFile == 0) { return ; }
    while (findNextPair(theFile, &theName, &theValue))
    {
        if (theName != PAPIUnknown) 
        { 
            addConfigList(inoutConfigList, theName, theValue) ;
        }
    }
    fclose(theFile) ;
}

int configLock( void )
{
	int theRC = -1;
	if ( sConfigMutex == 0 )
	{
		sConfigMutex = newMutex();
	}
	if ( sConfigMutex != 0 )
	{
		theRC = lockMutex( sConfigMutex );
	}
	return theRC;
}

PAPIConfigList * newConfigList( void )
{
	PAPIConfigList * theConfigList =
		( PAPIConfigList * )malloc( sizeof( PAPIConfigList ) );
	if ( theConfigList != 0 )
	{
		theConfigList->mName	= PAPIUnknown;
		theConfigList->mValue	= 0;
		theConfigList->mNext	= 0;
	}
	return theConfigList;
}

int skipComments(FILE *inFile)
{
    int theChar = -1 ;

    do
    {
        theChar = fgetc(inFile) ;
        if (ISRETURN(theChar) || ISWHITESPACE(theChar)) { continue ; }
        if (theChar == '/')
        {
            theChar = fgetc(inFile) ;
            if (theChar != '*') { ungetc(theChar, inFile) ; }
            else
            {
                do
                {
                    theChar = fgetc(inFile) ;
                    if (theChar == '*')
                    {
                        theChar = fgetc(inFile) ;
                        if (theChar == '/') { break ; }
                    }
                } while (theChar != EOF) ;
            }
        }
        else
        {
            ungetc(theChar, inFile) ;
            break ; 
        }
    } while (theChar != EOF) ;
    return fgetc(inFile) ;
}

int configUnlock( void )
{
	int theRC = -1;
	if ( sConfigMutex != 0 )
	{
		theRC = unlockMutex( sConfigMutex );
	}
	return theRC;
}

static int papiConnect( PAPISocket inSocket, struct sockaddr * inRemote )
{
	int	theRC	= -1;
	int bReuse	= 1;
	int theSocketFlags;
	if ( setsockopt( inSocket,
					 SOL_SOCKET,
					 SO_REUSEADDR,
					 &bReuse,
	                 sizeof( bReuse ) ) == -1 )
	{
		return theRC;
	}
#ifdef SOLARIS
	theSocketFlags	= fcntl( inSocket, F_GETFL, 0);
	if ( theSocketFlags >= 0 &&
	     fcntl( inSocket, F_SETFL, theSocketFlags | O_NONBLOCK ) != -1 )
	{
		theRC = connect( inSocket, inRemote, sizeof( struct sockaddr ) );
		fcntl( inSocket, F_SETFL, theSocketFlags );
	}
#else
	theRC = connect( inSocket, inRemote, sizeof( struct sockaddr ) );
#endif
	if ( theRC == 0 )
	{
		/*
		 * Nasty problem on Linux where connecting to daemon port sometimes
		 * works even when daemon is not listening. The following is an
		 * attempt to detect this
		 */
		struct sockaddr_in	theLocal;
		socklen_t			theSize = sizeof( theLocal );
		if ( getsockname( inSocket,
		                  ( struct sockaddr * )&theLocal,
		                  &theSize ) == 0 )
		{
			if ( ntohs( theLocal.sin_port ) ==
			     ntohs( ( ( struct sockaddr_in * )&inRemote )->sin_port ) )
			{
				theRC = -1;
			}
		}
		else
		{
			theRC = -1;
		}
	}
	return theRC;
}
