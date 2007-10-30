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

#ifndef _APOCD_H_
#define _APOCD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	sOpUndefined,
	sOpChangeDetect,
	sOpDisable,
	sOpEnable,
	sDEPRECATED_OpInetdStart,
	sOpIsEnabled,
	sOpReload,
	sOpRestart,
	sOpStart,
	sOpStatus,
	sOpStop,
	sOpSvcStart,
	sOpSvcStop
} DaemonOp;

typedef enum
{
	sStatusOK			= 0,
	sStatusEnabled		= 0,
	sStatusRunning		= 0,
	sErrorGeneric		= 1,	
	sStatusNotEnabled	= 1,	
	sErrorInvalidArg,
	sStatusNotRunning,
	sErrorNotRunning,
	sErrorAuthentication
} Status;

extern Status daemonStart( void );
extern Status daemonStatus( int inQuiet );
extern Status daemonStop( void );
extern Status daemonWaitStatus( Status inStatus, int inMaxIterations );

extern Status osDaemonDisable( void );
extern Status osDaemonEnable( void );
extern Status osDaemonIsEnabled( int inQuiet );
extern Status osDaemonStart( void );
extern Status osDaemonStop( void );
extern Status osDaemonSvcStart( void );
extern Status osDaemonSvcStop( void );
#ifdef LINUX
extern int osDaemonShouldRestart( void );
#endif

extern void writeInitialMessage( DaemonOp inOp );
extern void writeFinalMessage( DaemonOp inOp, Status inStatus );


#ifdef __cplusplus
}
#endif

#endif /* APOCD_H_ */
