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

#include <papiUtils.h>

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <syslog.h>

#define SCRIPT_DISABLE	"#!/bin/sh\n	\
/sbin/chkconfig apocd off\n				\
exit $?\n"

#define SCRIPT_ENABLE	"#!/bin/sh\n	\
/sbin/chkconfig apocd on\n				\
exit $?\n"

#define SCRIPT_ISENABLED "#!/bin/sh\n	\
/sbin/chkconfig -c apocd\n				\
exit $?\n"

static int execute( const char * inCommand );

Status osDaemonDisable( void )
{
	Status theRC;
	writeInitialMessage( sOpDisable );
	theRC = execute( SCRIPT_DISABLE ) == 0 ? sStatusOK : sErrorGeneric;
	writeFinalMessage( sOpDisable, theRC );
	if ( theRC == sStatusOK && daemonStatus( 1 ) == sStatusOK )
	{
		theRC = daemonStop();
	}
	return theRC;
}

Status osDaemonEnable( void )
{
	Status theRC;
	writeInitialMessage( sOpEnable );
	theRC = execute( SCRIPT_ENABLE ) == 0 ? sStatusOK : sErrorGeneric;
	writeFinalMessage( sOpEnable, theRC );
	if ( theRC == sStatusOK && daemonStatus( 1 ) != sStatusOK )
	{
		theRC = daemonStart();
	}
	return theRC;
}

Status osDaemonIsEnabled( inQuiet )
{
	Status theRC;
	if (inQuiet == 0) { writeInitialMessage( sOpIsEnabled ); }
	theRC = execute( SCRIPT_ISENABLED ) == 0 ?
		sStatusEnabled : sStatusNotEnabled;
    if (inQuiet == 0) { writeFinalMessage( sOpIsEnabled, theRC ); }
	return theRC;
}

int osDaemonShouldRestart( void )
{
	static int		sFailureIndex	= 0;
	static time_t	sFailureTimes[ 4 ];
	time_t			theFailureTime	= time( 0 );
	int				bShouldRestart	= 1;

	if ( sFailureIndex == 4 )
	{
		if ( theFailureTime - sFailureTimes[ 0 ] < 60 )
		{
			bShouldRestart = 0;
		}
		else
		{
			sFailureTimes[ 0 ] = sFailureTimes[ 1 ];
			sFailureTimes[ 1 ] = sFailureTimes[ 2 ];
			sFailureTimes[ 2 ] = sFailureTimes[ 3 ];
			sFailureTimes[ 3 ] = theFailureTime;
		}
	}	
	else
	{
		sFailureTimes[ sFailureIndex ++ ] = theFailureTime;
	}
	if ( bShouldRestart == 0 )
	{
		syslog( LOG_WARNING, "Too many Configuration Agent failures in the last 60 seconds. Not restarting." );
	}
	return bShouldRestart;
}

Status osDaemonStart( void )
{
	return daemonStart();
}

Status osDaemonStop( void )
{
	return daemonStop();
}

Status osDaemonSvcStart( void )
{
	close( 0 ); close( 1 ); close( 2 );
	return daemonStart();
}

Status osDaemonSvcStop( void )
{
    return daemonStop();
}


int execute( const char * inCommand )
{
	int		theRC		= -1;
	char *	theFileName	= tmpnam( 0 );
	FILE *	theFile		= fopen( theFileName, "w" );

	if ( theFile != 0 )
	{
		char theBuf[ BUFSIZ ];

		fprintf( theFile, "%s", inCommand );
		fclose( theFile );
		chmod( theFileName, 0700 );

		theFile = papiPopen( theFileName, "r" );
		if ( theFile != 0 )
		{
			while ( 1 )
			{
				if ( fgets( theBuf, BUFSIZ, theFile ) == 0 )
				{
					break;
				}
			}
			theRC = papiPclose( theFile );
			unlink( theFileName );
		}
	}
	return theRC;
}
