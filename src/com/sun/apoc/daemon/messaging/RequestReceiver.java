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
import com.sun.apoc.daemon.transport.*;

import java.io.*;

class RequestReceiver
{
	private ProtocolUnbinder		mUnbinder;
	private ProtocolDeserialiser	mDeserialiser;

	public RequestReceiver()
		throws APOCException
	{
		try
		{
			mUnbinder		= new ProtocolUnbinder();
			mDeserialiser	= new ProtocolDeserialiser();
		}
		catch ( org.xml.sax.SAXException theException )
		{
			throw new APOCException( theException );
		}
		catch ( javax.xml.parsers.ParserConfigurationException theException )
		{
			throw new APOCException( theException );
		}
	}

	public Message receiveRequest( final ClientChannel inClientChannel )
		throws APOCException
	{
		Message theMessage;
		try
		{
			mUnbinder.setClientChannel( inClientChannel );
			mDeserialiser.setInputStream( mUnbinder.unbind() );
			theMessage = mDeserialiser.deserialise();
			if ( ! theMessage.isValid() )
			{
				throw new APOCException( APOCSymbols.sSymRespInvalidRequest );
			}
		}
		catch ( org.xml.sax.SAXException theException )
		{
			APOCLogger.throwing( "RequestReceiver",
								 "receiveRequest",
								 theException );
			throw new APOCException( theException );
		}
		catch ( IOException theException )
		{
			throw new APOCException( theException );
		}
		return theMessage;
	}
}
