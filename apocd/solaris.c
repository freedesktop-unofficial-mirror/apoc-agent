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

#include <libscf.h>
#include <stdlib.h>
#include <string.h>

static const char * sInstanceName   = "svc:/network/apocd/udp:default";

static Status osEnabler( int inEnable, int inOneTime, int inQuiet );

Status osDaemonDisable( void )
{
	return osEnabler( 0, 0, 0 );
}

Status osDaemonEnable( void )
{
	return osEnabler( 1, 0, 0 );
}

Status osDaemonIsEnabled( int inQuiet )
{
	Status			isEnabled = sStatusNotEnabled;
	scf_handle_t *	theHandle = scf_handle_create( SCF_VERSION );
	if (inQuiet == 0) { writeInitialMessage( sOpIsEnabled ); }
	if ( theHandle != 0 )
	{
		if ( scf_handle_bind( theHandle ) == 0 )
		{
			scf_instance_t * theInstance = scf_instance_create( theHandle );
			if ( theInstance != 0 )
			{
				if ( scf_handle_decode_fmri(
						theHandle,
						sInstanceName,
						0,
						0,
						theInstance,
						0,
						0,
						SCF_DECODE_FMRI_EXACT ) != -1 )
				{
					scf_handle_t * theInstanceHandle =
						scf_instance_handle( theInstance );
					if ( theInstanceHandle != 0 )
					{
						uint8_t					theEnabled;
						scf_propertygroup_t *	theGroup	=
							scf_pg_create( theInstanceHandle );
						scf_property_t *		theProp		=
							scf_property_create( theInstanceHandle );
						scf_value_t *			theValue	=
							scf_value_create( theInstanceHandle );
						if ( theGroup != 0 && theProp != 0 && theValue != 0 )
						{
							if ( scf_instance_get_pg( theInstance, 
							                          SCF_PG_GENERAL,
							                          theGroup ) == 0		&&
							     scf_pg_get_property( theGroup,
							                          SCF_PROPERTY_ENABLED,
							                          theProp ) == 0		&&
							     scf_property_get_value( theProp,
							                             theValue ) == 0	&&
							     scf_value_get_boolean( theValue,	
							                            &theEnabled ) == 0 )
								{
							     	isEnabled = theEnabled == 1 ?
										sStatusEnabled : sStatusNotEnabled;	
								}
						}
						scf_pg_destroy( theGroup );
						scf_property_destroy( theProp );
						scf_value_destroy( theValue );
					}
				}
				scf_instance_destroy( theInstance );
			}
		}
		scf_handle_destroy( theHandle );
	}
	if (inQuiet == 0) { writeFinalMessage( sOpIsEnabled, isEnabled ); }
	return isEnabled;
}

Status osDaemonStart( void )
{
	Status theRC;
	writeInitialMessage( sOpStart );
	osEnabler( 1, 1, 1 );
	theRC = daemonWaitStatus( sStatusOK, 30 );
	writeFinalMessage( sOpStart, theRC );
	return theRC;
}

Status osDaemonStop( void )
{
	Status theRC;
	writeInitialMessage( sOpStop );
	osEnabler( 0, 1, 1 );
	theRC = daemonWaitStatus( sStatusNotRunning, 20 ) == sStatusNotRunning ?
		sStatusOK : sErrorGeneric;
	writeFinalMessage( sOpStop, theRC );
	return theRC;
}

Status osDaemonSvcStart( void )
{
	return daemonStart();
}

Status osDaemonSvcStop( void )
{
	return daemonStop();
}

Status osEnabler( int inEnable, int inOneTime, int inQuiet )
{
	Status		theStatus;
	DaemonOp	theOp		= inEnable == 1 ? sOpEnable : sOpDisable;
	int			theFlags	= inOneTime == 1 ? SMF_TEMPORARY : 0;

	if ( inQuiet == 0 )
	{
		writeInitialMessage( theOp );
	}
	if ( theOp == sOpEnable )
	{
		theStatus =
			smf_enable_instance( sInstanceName, theFlags ) == SCF_SUCCESS ?
				sStatusOK : sErrorGeneric;
	}
	else
	{
		theStatus =
			smf_disable_instance( sInstanceName, theFlags ) == SCF_SUCCESS ?
				sStatusOK : sErrorGeneric;
	}
	if ( inQuiet == 0 )
	{
		writeFinalMessage(  theOp, theStatus );
	}
	return theStatus;
}
