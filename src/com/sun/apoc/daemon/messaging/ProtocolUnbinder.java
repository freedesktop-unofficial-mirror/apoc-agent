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

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transport.*;

import java.io.*;
import java.nio.*;

class ProtocolUnbinder extends InputStream implements ConfigEventListener
{
	private ClientChannel	    mClientChannel;
	private final ByteBuffer    mInputBuffer	= ByteBuffer.allocate( 1024 );
	private int				    mContentLength;
	private final StringBuffer	mContentLengthBuffer	=
		new StringBuffer( sContentLengthValueSize );
	private long			    mAvailableByteCount;

	private static int			sMaxRequestLengthMinimum= 4096;
	private static Integer		sMaxRequestLength		=
		new Integer( sMaxRequestLengthMinimum );
	private static final byte[]	sContentLengthHeader	=
		APOCSymbols.sContentLength.getBytes();
	private static final int	sContentLengthValueSize = 5;

	static
	{
		setMaxRequestLength();
	}

	public ProtocolUnbinder()
	{
		DaemonConfig.addConfigEventListener( this );
	}

	public void setClientChannel( final ClientChannel inClientChannel )
	{
		mClientChannel		= inClientChannel;
		mAvailableByteCount	= 0;
	}

	public void onConfigEvent()
	{
		setMaxRequestLength();
	}

	public int read()
		throws IOException
	{
		int theByte = -1;
		if ( mContentLength-- > 0 )
		{
			theByte = ( int )getByte();
		}
		return theByte;
	}

	public int read( final byte[] outBytes, final int inOffset, final int inByteCount )
		throws IOException
	{
		int theByteCount = 0;
		while ( mContentLength > 0 && theByteCount < inByteCount )
		{
			outBytes[ inOffset + theByteCount++ ] = getByte();
			mContentLength--;
		}
		return theByteCount > 0 ? theByteCount : -1;
	}

	public InputStream unbind()
		throws APOCException, IOException
	{
		mContentLength = getContentLength();
		if ( mContentLength == 0 || mContentLength > getMaxRequestLength() )
		{
			throw new APOCException( APOCSymbols.sSymRespInvalidRequest );
		}
		return this;
	}

	private byte getByte()
		throws IOException
	{
		if ( mAvailableByteCount == 0 )
		{
			mInputBuffer.clear();
			mAvailableByteCount = mClientChannel.read( mInputBuffer );
			mInputBuffer.flip();
		}
		mAvailableByteCount--;
		return mInputBuffer.get();
	}

	private int getContentLength()
		throws IOException
	{
		return hasContentHeader() ? readContentLength() : 0;
	}

	private int getMaxRequestLength()
	{
		synchronized( sMaxRequestLength )
		{
			return sMaxRequestLength.intValue();
		}
	}

	private boolean hasContentHeader()
		throws IOException
	{
		boolean hasContentHeader = true;
		for ( int theIndex = 0;
			  theIndex < sContentLengthHeader.length;
			   theIndex ++ )
		{
			if ( getByte() != sContentLengthHeader[ theIndex ] )
			{
				hasContentHeader = false;
				break;
			}
		}
		return hasContentHeader;	
	}

	private int readContentLength()
		throws IOException
	{
		
		mContentLengthBuffer.delete( 0, sContentLengthValueSize );
		byte theByte = 0;
		for ( int theIndex = 0 ;
			  theIndex < sContentLengthValueSize ;
			  theIndex ++ )
		{
			if ( ( theByte = getByte() ) == '\r' )
			{
				break;
			}
			else
			{
				mContentLengthBuffer.append( ( char )theByte );
			}
		}
		if ( theByte != '\r' )
		{
			getByte(); // skip '\r'
		}
		getByte(); // skip '\n'
		return Integer.parseInt( mContentLengthBuffer.toString() );
	}

	private static void setMaxRequestLength()
	{
		synchronized( sMaxRequestLength )
		{
			int theMaxRequestLength =
				DaemonConfig.getIntProperty( DaemonConfig.sMaxRequestSize );
			if ( theMaxRequestLength < sMaxRequestLengthMinimum )
			{
				String theRequestSizes[] =
				{
					String.valueOf( theMaxRequestLength ),
					String.valueOf( sMaxRequestLengthMinimum ),
					String.valueOf( sMaxRequestLengthMinimum )
				};
				APOCLogger.warning( "Punb001", theRequestSizes );
				theMaxRequestLength = sMaxRequestLengthMinimum;
			}
			sMaxRequestLength = new Integer( theMaxRequestLength );
		}
	}
}
