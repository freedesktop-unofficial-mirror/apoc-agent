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
package com.sun.apoc.daemon.transaction;

import com.sun.apoc.daemon.apocd.*;
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;

import java.util.*;

class ReadTransaction extends Transaction
{
	public ReadTransaction( final Client inClient, final Message inRequest )
	{
		super( inClient, inRequest );
	}

	protected void executeTransaction()
	{
		APOCLogger.finer( "Rdtn001" );
		StringBuffer		theBuffer			= null;
		final String		theComponentName	=
			( String )( ( ReadMessage )mRequest )
				.getComponentNames().elementAt( 0 );
        final Cache [] theCaches = mSession.getCaches();
        final ArrayList theResults = new ArrayList();
        
        for (int theIndex = 0 ; theIndex < theCaches.length ; ++ theIndex)
        {
            theResults.addAll(theCaches [theIndex].read(theComponentName));
        }
		if ( theResults.size() > 0 )
		{
			theBuffer = new StringBuffer();
			final Iterator theIterator = theResults.iterator();
			while ( theIterator.hasNext() )
			{
				CacheReadResult theResult = (CacheReadResult)theIterator.next();
				theBuffer.append( "<" )
						 .append( APOCSymbols.sParamLayer )
						 .append( "><![CDATA[" )
					 	 .append( theResult.mTimestamp )
					 	 .append( theResult.mData )
					 	 .append( "]]></" )
						 .append( APOCSymbols.sParamLayer )
						 .append( ">" );
			}
		}
		APOCLogger.finer( "Rdtn002" );
		setResponse( APOCSymbols.sSymRespSuccess,
					 mRequest.getSessionId(),
					 theBuffer != null ? theBuffer.toString() : "" );
	}
}