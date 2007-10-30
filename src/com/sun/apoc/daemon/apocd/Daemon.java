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

import com.sun.apoc.daemon.admin.*;
import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.localdatabase.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transport.*;

import java.io.*;
import java.util.*;

public class Daemon implements ConfigEventListener, AdminOpHandler
{
	private ChannelManager			mChannelManager;
	private ClientManager			mClientManager;
	private AdminManager			mAdminManager;
	private final DaemonTimerTask	mGarbageCollector =
		new DaemonTimerTask( new GarbageCollector() );

    public static void main( final String[] inArgs )
    {
		try
		{
        	new Daemon( inArgs ).run();
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "Daemon", "main", theException );
		}
    }

    public Daemon( final String[] inArgs )
		throws APOCException, IOException, InterruptedException
    {
		if ( inArgs.length != 2 )
		{
			usage();
			System.exit( 1 );
		}
		init( inArgs );
    }

	public void onConfigEvent()
	{
		startGarbageCollectionTimer();
		DaemonConfig.log();
	}

	public int onStop()
	{
		mChannelManager.terminate();
		return 0;
	}

	public int onStatus()
	{
		return 0;
	}

	public int onReload()
	{
		int theRC;
		try
		{
			DaemonConfig.reload();
			DaemonConfig.log();
			theRC = 0;
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "Daemon", "onReload", theException );
			theRC = 1;
		}
		return theRC;
	}

	public int onChangeDetect()
	{
		mClientManager.changeDetect();
		return 0;
	}

    public void run()
		throws APOCException, InterruptedException
    {
		APOCLogger.info( "Dmon001" );

		startGarbageCollectionTimer();
		mChannelManager = new ChannelManager();
		mClientManager	= new ClientManager( mChannelManager );
		verifyConfig();
		mChannelManager.start();
		DaemonConfig.addConfigEventListener( this );
		mAdminManager	= new AdminManager( this );
		mAdminManager.start();
		mChannelManager.join();

		mClientManager.terminate();
		mChannelManager.closeChannels();
		APOCLogger.info( "Dmon002" );
    }

	private void usage()
	{
		System.err.println( "Usage: apocd <daemon installation directory>" );
	}

	private void init( final String[] inArgs )
		throws IOException, APOCException
	{
		System.loadLibrary( "FileAccess" );
		initConfig( inArgs );
		initAuthDir();
	}

	private void initAuthDir()
		throws APOCException
	{
		if ( ! APOCAuthenticator.sAuthDir.exists() )
		{
			if ( ! APOCAuthenticator.sAuthDir.mkdirs() )
			{
				throw new APOCException();
			}
		}
		else
		{
			if ( ! APOCAuthenticator.sAuthDir.isDirectory() )
			{
				throw new APOCException();
			}
		}
		FileAccess.chmod( APOCAuthenticator.sAuthDir.getAbsolutePath(), 0755 );
		FileAccess.chmod( APOCAuthenticator.sAuthDir.getParent(), 0755 );
	}

	private void initConfig( final String[] inArgs )
		throws IOException, APOCException
	{
		DaemonConfig.init( inArgs );
		DaemonConfig.log();
	}

	private void startGarbageCollectionTimer()
	{
		final long theMinuteDelay	=
			DaemonConfig.getIntProperty(
				DaemonConfig.sGarbageCollectionInterval );
		if ( mGarbageCollector.setPeriod( 60000 * theMinuteDelay ) )
		{
			if ( mGarbageCollector.getPeriod() > 0 )
			{
				APOCLogger.fine( "Dmon003", String.valueOf( theMinuteDelay ) );
			}
			else
			{
				APOCLogger.fine( "Dmon004" );
			}
		}
	}

	private void verifyConfig()
		throws APOCException
	{
		LocalDatabaseFactory.getInstance();
	}

	class GarbageCollector implements Runnable
	{
		public void run()
		{
			mClientManager.onGarbageCollect();
		}
	}
}
