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

#ifndef PAPIMESSAGE_H_ 
#define PAPIMESSAGE_H_ 

#include "papiTypes.h"
#include "papiEntityList.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	PAPIRespSuccess = 0,
	PAPIRespInvalidRequest,
	PAPIRespConnectionFailure,
	PAPIRespAuthenticationFailure,
	PAPIRespInvalidSessionId,
	PAPIRespNoSuchComponent,
	PAPIRespSuccessContinueLocalAuth,
	PAPIRespSuccessContinueSASLAuth,

	PAPIaddNotification,
	PAPIremoveNotification,
	PAPImodifyNotification,

	PAPIReqCreateSession,
	PAPIReqRead,
	PAPIReqList,
	PAPIReqDestroySession,
	PAPIReqAddListener,
	PAPIReqRemoveListener,
    PAPIReqCreateSessionExt,

	PAPIMessageNameUnused

} PAPIMessageName;

typedef enum
{
	PAPIParamSessionId = 0,
	PAPIParamUserId,
	PAPIParamEntityId,
	PAPIParamCredentials,
	PAPIParamComponentName,
	PAPIParamClientData,
	PAPIParamMessage,
	PAPIParamLayer,
    PAPIParamEntityType,
	PAPIParamNameUnused
} PAPIParamName;

typedef struct PAPIParamNameList
{
	PAPIParamName				mName;
	struct PAPIParamNameList *	mNext;
} PAPIParamNameList;

typedef struct PAPIMessage
{
	PAPIMessageName		mName;
	PAPIParamNameList *	mParamNames;
	PAPIStringList *	mParamValues;
} PAPIMessage;

PAPIMessage *	newMessage( PAPIMessageName inMessageName );

void			deleteMessage( PAPIMessage * inMessage );

void			addParam( PAPIMessage *	inMessage,
						  PAPIParamName	inParamName,
						  const char *	inParamValue );

PAPIStringList *getParamValue( PAPIMessage *	inMessage,
							   PAPIParamName	inParamName );

char * messageToString( PAPIMessage * inMessage );

PAPIMessageName stringToMessageName( const char * inMessageName );
PAPIParamName   stringToParamName(	 const char * inParamName );

const char *getEntityTypeString( PAPIEntityType inType );

#define CONTENTLENGTH			"CONTENT-LENGTH:"
#define MAX_CONTENT_LENGTH_SIZE	23

#ifdef __cplusplus
}
#endif

#endif /* PAPIMESSAGE_H_ */
