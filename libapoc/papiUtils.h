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
#ifndef PAPIUTILS_H_
#define PAPIUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "papiTypes.h"

#include <sys/types.h>
#include <sys/stat.h>

typedef enum
{
	PAPIConnectSuccess,
	PAPIConnectRejected,
	PAPIConnectFailed,
	PAPIConnectWrongService
} PAPIConnectResult;

#ifndef WNT
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
typedef pthread_mutex_t	PAPIMutex;
typedef int				PAPISocket;
typedef pthread_t *		PAPIThread;

#define FILESEP			"/"
#define PATHSEP			":"
#define papiClose		close
#define papiSleep		sleep
#define papiPclose		pclose
#define papiPopen		popen
#define papiUmask		umask
#define INVALID_SOCKET	-1
#define SOCKET_ERROR	-1

#ifdef SOLARIS
int isServiceEnabled( void );
#endif

#else
#include <windows.h>
#include <io.h>
#include <stdio.h>

typedef CRITICAL_SECTION	PAPIMutex;
typedef SOCKET				PAPISocket;
typedef HANDLE				PAPIThread;
typedef int					socklen_t;

#define FILESEP		"\\\\"
#define PATHSEP		";"
#define snprintf	_snprintf
#define papiClose	closesocket
#define papiSleep( inSeconds )	Sleep( inSeconds * 1000 )
#define papiPclose	_pclose
#define papiPopen	_popen
#define papiUmask	_umask

#define APOCENABLEDKEY	"Software\\Sun Microsystems\\apoc\\1.0\\Enabled"

#define APOCSERVICENAME	"apocd"

const char *	convertDataDir( const char * inDir );

int				isNewerThanWindows98( void );

#endif


typedef  void * ( *PAPIThreadFunc )( void * inArg );

int						base64_encode( 		const void *		inBuffer,
									   		int					inBufferSize,
									   		char **				outB64Buffer );

PAPIThread				createThread(		PAPIThreadFunc		inThreadFunc,
											void *				inArg );

void					destroyThread(		PAPIThread			inThread );

void					deleteMutex(		PAPIMutex *			inMutex );

PAPIConnectResult		getConnectedSocket( int					inPort,
											int					inType,
											int					inRetryCount,
											PAPISocket *		outSocket );

const char *			getDaemonDataDir(	void );

const char *			getDaemonInstallDir(void );

const char *			getDaemonLibraryDir(void );

const char *			getDaemonPropertiesDir(void );

int						getDaemonPort(		void );

const char *			getInstallDir(		void );

const char *			getLibraryDir(		void );

const char *			getPropertiesDir(	void );

const char *			getSASLCreds(		const char *		inServiceName,
											PAPIStatus *		outStatus );

const char *			getTimestamp(		void );

int						getTransactionTimeout( void );

const char *			getUserName(		void );

int						isDaemonEnabled(	void );

int						lockMutex(			PAPIMutex *			inMutex );

int						socketPoll(			PAPISocket			inSocket,
											int					inMillis );

int						startDaemon(		void );

PAPIMutex *				newMutex(			void );

int						unlockMutex(		PAPIMutex *			inMutex );
#ifdef __cplusplus
}
#endif

#endif /* PAPIUTILS_H_ */
