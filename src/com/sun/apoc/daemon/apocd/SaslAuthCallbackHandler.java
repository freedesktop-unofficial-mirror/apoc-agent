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
package com.sun.apoc.daemon.apocd;

import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transport.*;

import com.sun.apoc.spi.ldap.authentication.*;

import java.io.IOException;
import java.nio.*;
import javax.security.auth.callback.*;

class SaslAuthCallbackHandler implements CallbackHandler
{
	private Messenger		mMessenger		= new Messenger();
	private ClientChannel	mChannel;
	private ByteBuffer		mBuffer;	
	private boolean			mReconnectMode	= false;

	SaslAuthCallbackHandler( final Client inClient )
		throws APOCException
	{
		mChannel = inClient.getClientChannel();
		mMessenger.setClientChannel( mChannel );
	}

	public void handle( Callback[] inCallbacks )
		throws IOException, UnsupportedCallbackException
	{
		final LdapSaslGSSAPICallback theCallback =
			( LdapSaslGSSAPICallback )inCallbacks[ 0 ];
		if ( mBuffer == null )
		{
			createTokens( theCallback );	
		}
		theCallback.setResponse( getToken() );
	}

	public void setReconnectMode() { mReconnectMode = true; }

	private void createTokens( final LdapSaslGSSAPICallback inCallback )
	{
		try
		{
			if ( mReconnectMode )
			{
				mChannel.registerForSelection( false );
			}
			mMessenger.sendResponse(
				MessageFactory.createResponse(
					APOCSymbols.sSymRespSuccessContinueSASLAuth,
					null,
					new StringBuffer( inCallback.getServiceName() )
						.append( "/" )
						.append( inCallback.getHostname() )
						.toString() ),
				false );
			final CredentialsProviderMessage theMessage = 
				( CredentialsProviderMessage )mMessenger.receiveRequest(
													! mReconnectMode );

			if ( theMessage != null )
			{
				final String theCreds = theMessage.getCredentials();
				if ( theCreds != null )
				{
					mBuffer =
						ByteBuffer.wrap(
				 			base64ToByteArray( theCreds.toCharArray() ) );
				}
			}
			if ( mReconnectMode )
			{
				mMessenger.sendResponse(
					MessageFactory.createResponse(
						APOCSymbols.sSymRespSuccess, null, null ),
					true );
			}
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "SaslAuthCallbackHandler",
								 "createTokens",
								 theException );
		}
	}

	private byte[] getToken()
	{
		byte[] theToken = null;
		if ( mBuffer != null && mBuffer.hasRemaining() )
		{
			int theTokenSize = nextTokenSize();
			if ( theTokenSize > 0 )
			{
				theToken = new byte[ theTokenSize ];
				mBuffer = mBuffer.get( theToken );
			}
			if ( ! mBuffer.hasRemaining() )
			{
				mBuffer = null;
			}
		}
		return theToken;
	}

	private int nextTokenSize()
	{
		int theTokenSize = 0;
		try
		{
			if ( mBuffer.hasRemaining() )
			{
				final byte[] theTokenSizeBytes = new byte[ 5 ];
				mBuffer			= mBuffer.get( theTokenSizeBytes );	
				theTokenSize	=
					Integer.parseInt( new String( theTokenSizeBytes ) );
			}	
		}
		catch( Exception theException )
		{}
		return theTokenSize;
	}

	// Code borrowed from java.util.prefs.Base64
    private static final byte base64ToInt[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54,
        55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4,
        5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };

    private byte[] base64ToByteArray(char[] encoded) {
		byte[] alphaToInt = base64ToInt;
        int encodedLen = encoded.length;
        int numGroups = encodedLen/4;
        if (4*numGroups != encodedLen)
            throw new IllegalArgumentException(
                "String length must be a multiple of four.");
        int missingBytesInLastGroup = 0;
        int numFullGroups = numGroups;
        if (encodedLen != 0) {
            if (encoded[encodedLen-1] == '=') {
                missingBytesInLastGroup++;
                numFullGroups--;
            }
            if (encoded[encodedLen-2] == '=')
                missingBytesInLastGroup++;
        }
        byte[] result = new byte[3*numGroups - missingBytesInLastGroup];

        // Translate all full groups from base64 to byte array elements
        int inCursor = 0, outCursor = 0;
        for (int i=0; i<numFullGroups; i++) {
            int ch0 = base64toInt(encoded[inCursor++], alphaToInt);
            int ch1 = base64toInt(encoded[inCursor++], alphaToInt);
            int ch2 = base64toInt(encoded[inCursor++], alphaToInt);
            int ch3 = base64toInt(encoded[inCursor++], alphaToInt);
            result[outCursor++] = (byte) ((ch0 << 2) | (ch1 >> 4));
            result[outCursor++] = (byte) ((ch1 << 4) | (ch2 >> 2));
            result[outCursor++] = (byte) ((ch2 << 6) | ch3);
        }

        // Translate partial group, if present
        if (missingBytesInLastGroup != 0) {
            int ch0 = base64toInt(encoded[inCursor++], alphaToInt);
            int ch1 = base64toInt(encoded[inCursor++], alphaToInt);
            result[outCursor++] = (byte) ((ch0 << 2) | (ch1 >> 4));

            if (missingBytesInLastGroup == 1) {
                int ch2 = base64toInt(encoded[inCursor++], alphaToInt);
                result[outCursor++] = (byte) ((ch1 << 4) | (ch2 >> 2));
            }
        }
        return result;
    }

    private int base64toInt(char c, byte[] alphaToInt) {
        int result = alphaToInt[c];
        if (result < 0)
            throw new IllegalArgumentException("Illegal character " + c);
        return result;
    }
}
