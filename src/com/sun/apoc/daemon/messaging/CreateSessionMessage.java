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
package com.sun.apoc.daemon.messaging;

import com.sun.apoc.daemon.misc.*;

public class CreateSessionMessage extends Message 
                                  implements CredentialsProviderMessage
{
	private String	mUserId;
	private String	mEntityId;
	private String	mCredentials;
	private int[]	mExpectedSymbols= { APOCSymbols.sSymParamUserId,
										APOCSymbols.sSymUnknown };
	private boolean	mIsValid		= false;

	public CreateSessionMessage()
	{
		super( APOCSymbols.sSymReqCreateSession );
	}

	public boolean isValid()
	{
		return mIsValid;
	}

	public String getUserId()		{ return mUserId; }
	public String getEntityId() 	{ return mEntityId; }
	public String getCredentials()	{ return mCredentials; }

	protected void addParameter( final String inName, final String inValue )
		throws APOCException
	{
		switch ( APOCSymbols.getProtocolSymbol( inName ) )
		{
			case APOCSymbols.sSymParamUserId:
				addUserId( inValue );
				break;

			case APOCSymbols.sSymParamEntityId:
				addEntityId( inValue );
				break;

			case APOCSymbols.sSymParamCredentials:
				addCredentials( inValue );
				break;

			default:
				throw new APOCException();
		}
	}	

	private void addUserId( final String inUserId )
		throws APOCException
	{
		checkSymbol( APOCSymbols.sSymParamUserId );
		mUserId = inUserId;
		setCheckSymbols( APOCSymbols.sSymParamEntityId,
						 APOCSymbols.sSymParamCredentials );
		mIsValid = true;
	}

	private void addEntityId( final String inEntityId )
		throws APOCException
	{
		checkSymbol( APOCSymbols.sSymParamEntityId );
		mEntityId = inEntityId;
		setCheckSymbols( APOCSymbols.sSymParamCredentials );
	}

	private void addCredentials( final String inCredentials )
		throws APOCException
	{
		checkSymbol( APOCSymbols.sSymParamCredentials );
		mCredentials = inCredentials;
		setCheckSymbols( APOCSymbols.sSymUnknown );
	}
	
	private void checkSymbol( final int inSymbol )
		throws APOCException
	{
		if ( mExpectedSymbols[ 0 ] == APOCSymbols.sSymUnknown ||
		 	 ( mExpectedSymbols[ 0 ] != inSymbol &&
		       mExpectedSymbols[ 1 ] != inSymbol ) )
		{
			throw new APOCException();
		}
	}

	private void setCheckSymbols( final int inSymbol )
	{
		setCheckSymbols( inSymbol,
						 APOCSymbols.sSymUnknown );
	}

	private void setCheckSymbols( final int inSymbol1,
								  final int inSymbol2 )
	{
		mExpectedSymbols[ 0 ] = inSymbol1;
		mExpectedSymbols[ 1 ] = inSymbol2;
	}
}
