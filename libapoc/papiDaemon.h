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
/* papiDaemon.h */

#ifndef PAPIDAEMON_H_
#define PAPIDAEMON_H_

#include "papiConnection.h"
#include "papiTypes.h"
#include "papiInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PAPIDaemon
{
	const char *			mUserId;
	PAPIEntityList *		mEntities;
	PAPIConnection *		mConnection;
	const char *			mSessionId;
	PAPIConnectionListener	mConnectionListener;
	void *					mUserData;
} PAPIDaemon;

PAPIListenerId	daemonAddListener(		PAPIDaemon *	inDaemon,
										const char *	inComponentName,
										PAPIListener	inListener,
										void *			inUserData,
										PAPIStatus *	outStatus );

PAPIStringList *daemonList(				PAPIDaemon *	inDaemon,
										PAPIStatus *	outStatus );

PAPIStringList *daemonRead(				PAPIDaemon *	inDaemon,
										const char *	inComponentName,
										PAPIStatus *	outStatus );

void			daemonReconnect(		PAPIDaemon *	inDaemon,
										PAPIStatus *	outStatus );

void			daemonRemoveListener(	PAPIDaemon *	inDaemon,
										PAPIListenerId 	inListenerId,
										PAPIStatus *	outStatus );

void			deleteDaemon(			PAPIDaemon *	inDaemon );

PAPIDaemon *	newDaemon(				const PAPIEntityList *	inEntities,
										PAPIConnectionListener	inListener,
										void *			inUserData,
										PAPIListener	inInternalListener,
										void *			inInternalListenerData,
										PAPIStatus *	outStatus );

#ifdef __cplusplus
}
#endif

#endif /* PAPIDAEMON_H_ */
