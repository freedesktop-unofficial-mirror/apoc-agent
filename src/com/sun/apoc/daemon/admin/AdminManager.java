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
package com.sun.apoc.daemon.admin;

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transport.*;

import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.util.*;

public class AdminManager extends Thread
{
	private static final byte[]		sSuccess		= { 0 };
	private static final byte[]		sFailure		= { 1 };
	private static final byte		sOpChangeDetect	= ( byte )1;
	private static final byte		sOpReload		= ( byte )6;
	private static final byte		sOpStatus		= ( byte )9;
	private static final byte		sOpStop			= ( byte )10;

	private static final byte[] sAcceptByte			= { 1 };
    private static final ByteBuffer sAccept			=
		( ByteBuffer )ByteBuffer.wrap( sAcceptByte );

	private Selector			mSelector;
	private ServerSocketChannel	mChannel;
	private AdminOpHandler		mAdminOpHandler;
	private final Object		mTerminateLock	= new Object();
	private boolean				mTerminate		= false;

	public AdminManager( final AdminOpHandler inAdminOpHandler )
		throws APOCException
	{
		setName( "AdminManager" );
		mAdminOpHandler	= inAdminOpHandler;
		initChannel();
	}

	public void run()
	{
		APOCLogger.fine( "Admr001" );
		while ( ! shouldTerminate() )
		{
			try
			{
				final ClientChannel theChannel = acceptClientChannel();
				if ( theChannel != null )
				{
					if ( authenticate( theChannel ) )
					{
						try
						{
							handleRequest( theChannel );
						}
						catch( Exception theException )
						{
							APOCLogger.throwing( "AdminManager",
							                     "run",
							                     theException );
						}
					}
					theChannel.close();
				}
			}
			catch ( IOException theException )
			{
				APOCLogger.throwing( "AdminManager", "run", theException );
				break;
			}
		}
		closeServerChannel();
		APOCLogger.fine( "Admr002" );
	}

	public void terminate()
	{
		synchronized( mTerminateLock )
		{
			mTerminate = true;
		}
	}

	private ClientChannel acceptClientChannel()
		throws IOException
	{
		ClientChannel theChannel = null;
		if ( mSelector.select() > 0 )
		{
			final Iterator theIterator = mSelector.selectedKeys().iterator();
			while ( theIterator.hasNext() )
			{
				final SelectionKey theKey = ( SelectionKey )theIterator.next();
				theIterator.remove();
				if ( theKey.isAcceptable() )
				{
					SocketChannel theSocketChannel = null;
					try
					{
						theSocketChannel =
							(( ServerSocketChannel )theKey.channel() ).accept();
						if ( theSocketChannel.
								socket().
									getInetAddress().
										isLoopbackAddress() )
						{
							theChannel =
								new ClientChannel( theSocketChannel, null);
							theChannel.write( sAccept );
							sAccept.flip();
							break;
						}
					}
					catch( IOException theException )
					{
						if ( theSocketChannel != null )
						{
							try
							{
								theSocketChannel.close();
							}
							catch( IOException ignore ){}
						}
						APOCLogger.throwing( "AdminManager", 
						                     "acceptClientChannel",
						                     theException );
					}
				}	
			}
		}
		return theChannel;
	}

	private boolean authenticate( final ClientChannel inChannel )
	{
		boolean				isAuthenticated;
		APOCAuthenticator	theAuthenticator = null;
		try
		{
			theAuthenticator	= new APOCAuthenticator( null );
			byte[] theChallenge = theAuthenticator.getChallenge();
			inChannel.write(
				( ByteBuffer )ByteBuffer.allocate( theChallenge.length + 1 )
					.put( theChallenge )
					.put( ( byte )0 )
					.flip() );
			ByteBuffer theAuthBuffer =
				ByteBuffer.wrap( 
					new byte[ APOCAuthenticator.getAuthBufferSize() ] );
			while ( inChannel.read( theAuthBuffer ) == 0 ){}
			theAuthenticator.processResponse( theAuthBuffer.array() );
			inChannel.write(
				( ByteBuffer )ByteBuffer.allocate( 1 ).put( sSuccess ).flip() );
			isAuthenticated = true;
		}
		catch( Exception theException )
		{
			inChannel.write(
				( ByteBuffer )ByteBuffer.allocate( 1 ).put( sFailure ).flip() );
			isAuthenticated = false;
		}
		finally
		{
			if ( theAuthenticator != null )
			{
				theAuthenticator.cleanup();
			}
		}
		return isAuthenticated;
	}

	private void closeServerChannel()
	{
		try
		{
			mSelector.close();
			mChannel.close();
		}
		catch( IOException theException )
		{
			APOCLogger.throwing( "AdminManager",
			                     "closeServerChannel",
			                     theException );
		}
	}

	private void handleRequest( final ClientChannel inChannel )
		throws Exception
	{
		int theRC;
		ByteBuffer theBuffer = ByteBuffer.wrap( new byte[ 1 ] );
		while ( inChannel.read( theBuffer ) != 1 ){}
		switch( theBuffer.array()[ 0 ] )
		{
			case sOpStop:
				theRC = mAdminOpHandler.onStop();
				terminate();
				break;

			case sOpStatus:
				theRC = mAdminOpHandler.onStatus();
				break;

			case sOpReload:
				theRC = mAdminOpHandler.onReload();
				break;

			case sOpChangeDetect:
				theRC = mAdminOpHandler.onChangeDetect();
				break;

			default:
				theRC = -1;
				break;
		}
		theBuffer = ByteBuffer.wrap( theRC == 0 ? sSuccess : sFailure );
		inChannel.write( theBuffer );
	}

	private void initChannel()
		throws APOCException
	{
		try
		{
			mSelector	= Selector.open();
			mChannel	= ServerSocketChannel.open();
			mChannel.configureBlocking( false );
			mChannel.socket().bind( null );
			mChannel.register( mSelector, SelectionKey.OP_ACCEPT );
			/*
			 * 6417539 - Need to use IANA registered port
			 *           Use a PortMapper instance to let admin. clients
			 *           know which port the agent is using.
			 */
			new PortMapper(
			 ( ( InetSocketAddress )mChannel.socket().getLocalSocketAddress() )
				.getPort() ).start();
		}
		catch( IOException theException )
		{
			throw new APOCException( theException );
		}
	}

	private boolean shouldTerminate()
	{
		synchronized( mTerminateLock )
		{
			return mTerminate;
		}
	}
}
