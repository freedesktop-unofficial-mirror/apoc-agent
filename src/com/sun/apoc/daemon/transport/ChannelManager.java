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
package com.sun.apoc.daemon.transport;

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.misc.*;

import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.util.*;
import java.util.logging.*;

public class ChannelManager extends Thread implements ConfigEventListener
{
	private Selector			mSelector;
	private ServerSocketChannel	mChannel;
	private final HashSet		mRegisteringChannels			=
		new HashSet();
	private final HashSet		mDeRegisteringChannels			=
		new HashSet();
	private final Object		mLock							= new Object();
	private int					mMaxClientChannels				=
		DaemonConfig.getIntProperty( DaemonConfig.sMaxClientConnections );
	private final HashSet		mClientChannelEventListeners	= new HashSet();
	private int					mChannelCount					= 0;
	private boolean				mTerminate						= false;
	private static final byte	sReject							= 0;
	private static final byte[]	sAcceptByte						= { 1 };
	private static final ByteBuffer sAccept	=
		( ByteBuffer )ByteBuffer.wrap( sAcceptByte );

	public ChannelManager()
		throws APOCException
	{
		try
		{
			initThread();
			initChannel();
			DaemonConfig.addConfigEventListener( this );
		}
		catch( IOException theException )
		{
			throw new APOCException(
				APOCSymbols.sSymCreateServerChannelFailed,
				theException );
		}
	}

	public void addClientChannelEventListener(
		final ClientChannelEventListener inListener )
	{
		if ( inListener != null )
		{
			synchronized( mClientChannelEventListeners )
			{
				mClientChannelEventListeners.add( inListener );
			}
		}
	}

	public void closeClientChannels()
	{
		final Set theKeys = mSelector.keys();
		if ( theKeys != null )
		{
			final Iterator theIterator = theKeys.iterator();
			while ( theIterator.hasNext() )
			{
				final SelectionKey theKey =
					( SelectionKey )theIterator.next();
				if ( theKey.isValid() )
				{
					try
					{
						theKey.channel().close();
					}	
					catch( IOException theException )
					{}
				}
			}
		}	
	}

	public void closeChannels()
	{
		closeClientChannels();
		closeServerChannel();
	}

	public void onConfigEvent()
	{
		setMaxClientChannels();
	}

	public void removeClientChannelEventListener(
		final ClientChannelEventListener inListener )
	{
		if ( inListener != null )
		{
			synchronized( mClientChannelEventListeners )
			{
				mClientChannelEventListeners.remove( inListener );
			}
		}
	}

	public void run()
	{
		APOCLogger.fine( "Chmr001" );
		while ( ! shouldTerminate() )
		{
			try
			{
				selectChannels();
				registerChannels();	
			}
			catch( Exception theException )
			{
				APOCLogger.throwing( "ChannelManager", "run", theException );
				break;
			}
		}
		APOCLogger.fine( "Chmr003" );
	}

	public void terminate()
	{
		synchronized( mLock )
		{
			mTerminate = true;
		}
		mSelector.wakeup();
	}

	void closeClientChannel( final ClientChannel inChannel )
	{
		inChannel.close();
		notifyClosed( inChannel );
		synchronized( mLock )
		{
			-- mChannelCount;
		}
	}

	void registerForSelection( final ClientChannel inChannel,boolean inRegister)
	{
		final HashSet theSet = inRegister ?
			mRegisteringChannels : mDeRegisteringChannels;
		synchronized( theSet )
		{
			theSet. add( inChannel.getSocketChannel().keyFor( mSelector ) );
			mSelector.wakeup();
			try
			{
				theSet.wait();
			}
			catch( Exception theException )
			{}
		}
	}

	private void acceptableKey( final SelectionKey inKey )
	{
		SocketChannel theSocketChannel = null;
		try
		{
			theSocketChannel =
				( ( ServerSocketChannel )inKey.channel() ).accept();
			if ( ! testMaxChannels() && theSocketChannel != null )
			{
				try
				{
					theSocketChannel.socket()
						.getOutputStream().write( sReject );
					theSocketChannel.close();
				}
				catch( Exception theException ){}
				return;
			}
			if ( theSocketChannel.
					socket().
						getInetAddress().
							isLoopbackAddress() )
			{
				theSocketChannel.configureBlocking( false );
				final SelectionKey theClientKey =
					theSocketChannel.register(
						mSelector,
						SelectionKey.OP_READ );
				ClientChannel theClientChannel =
					new ClientChannel( theSocketChannel, this );
				theClientKey.attach( theClientChannel );
				synchronized( mLock )
				{
					++ mChannelCount;
				}
				theClientChannel.write( sAccept );
				sAccept.flip();
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
				catch( Exception ignore )
				{}
			}
			APOCLogger.throwing( "ChannelManager",
			                     "acceptableKey",
			                     theException );
		}
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
			APOCLogger.throwing( "ClientManager", "close", theException );
		}
	}

	private int getMaxClientChannels()
	{
		synchronized( mLock )
		{
			return mMaxClientChannels;
		}
	}

	private void initChannel()
		throws IOException
	{
		mSelector	= Selector.open();
		mChannel	= ServerSocketChannel.open();
		mChannel.configureBlocking( false );
		mChannel.socket().bind(
			new InetSocketAddress( 
				DaemonConfig.getIntProperty( DaemonConfig.sDaemonPort ) ) );
		mChannel.register( mSelector, SelectionKey.OP_ACCEPT );
	}

	private void initThread()
	{
		setName( "ChannelManager" );
		setDaemon( true );
	}

	private void notifyClosed( final ClientChannel inClientChannel )
	{
		final Iterator theIterator = mClientChannelEventListeners.iterator();
		while ( theIterator.hasNext() )
		{	
			( ( ClientChannelEventListener )theIterator.next() ).onClosed(
															inClientChannel );	
		}
	}

	private void notifyPending( final ClientChannel inClientChannel )
	{
		final Iterator theIterator = mClientChannelEventListeners.iterator();
		while ( theIterator.hasNext() )
		{	
			( ( ClientChannelEventListener )theIterator.next() ).onPending(
															inClientChannel );	
		}
	}

	private void readableKey( final SelectionKey inKey )
	{
		inKey.interestOps(
			inKey.interestOps() & 
			( ~ SelectionKey.OP_READ ) );
		notifyPending( ( ClientChannel )inKey.attachment() );
	}

	private void registerChannels()
	{
		synchronized( mRegisteringChannels )
		{
			if ( mRegisteringChannels.size() > 0 )
			{
				final Iterator theIterator = mRegisteringChannels.iterator();
				while ( theIterator.hasNext() )
				{
					final SelectionKey theKey =
						( SelectionKey )theIterator.next();
					theKey.interestOps( theKey.interestOps() |
					                    SelectionKey.OP_READ );
				}
				mRegisteringChannels.clear();
				mRegisteringChannels.notifyAll();
			}
		}	
		synchronized( mDeRegisteringChannels )
		{
			if ( mDeRegisteringChannels.size() > 0 )
			{
				final Iterator theIterator = mDeRegisteringChannels.iterator();
				while ( theIterator.hasNext() )
				{
					final SelectionKey theKey =
						( SelectionKey )theIterator.next();
					theKey.interestOps( theKey.interestOps() &
				                        ( ~ SelectionKey.OP_READ ) );
				}
			}
			mDeRegisteringChannels.clear();
			mDeRegisteringChannels.notifyAll();
		}
	}

	private void selectChannels()
		throws IOException
	{
		if ( mSelector.select() > 0 )
		{
			final Iterator theIterator = mSelector.selectedKeys().iterator();
			while ( theIterator.hasNext() )
			{
				final SelectionKey theKey = ( SelectionKey )theIterator.next();
				theIterator.remove();
				if ( theKey.isAcceptable() )
				{
					acceptableKey( theKey );
				}
				if ( theKey.isReadable() )
				{
					readableKey( theKey );
				}
			}
		}
	}

	private void setMaxClientChannels()
	{
		synchronized( mLock )
		{
			mMaxClientChannels =
				DaemonConfig.getIntProperty(
					DaemonConfig.sMaxClientConnections );
		}
		mSelector.wakeup();
	}

	private boolean shouldTerminate()
	{
		synchronized( mLock )
		{
			return mTerminate;
		}
	}
				
	private boolean testMaxChannels()
	{
		boolean	bMoreChannelsAllowed	= false;
		int		theMaxClientChannels	= getMaxClientChannels();
		if ( theMaxClientChannels > 0 )
		{
			synchronized( mLock )
			{
				bMoreChannelsAllowed = theMaxClientChannels > mChannelCount;
			}
		}
		if ( ! bMoreChannelsAllowed )
		{
			APOCLogger.warning( "Chmr002",
		                    	String.valueOf( theMaxClientChannels ) );
		}
		return bMoreChannelsAllowed;	
	}
}
