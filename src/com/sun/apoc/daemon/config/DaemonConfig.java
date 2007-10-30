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
package com.sun.apoc.daemon.config;

import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.spi.*;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.logging.*;
import java.util.zip.*;

public class DaemonConfig implements ConfigEventListener
{
	// Public bootstrap properties
	public static final String	sDaemonPort			= "DaemonPort";
	public static final String	sDataDir			= "DataDir";

	// Non bootstrap
	public static final String	sMaxClientThreads				=
		"MaxClientThreads";
	public static final String	sMaxClientConnections			=
		"MaxClientConnections";
	public static final String	sMaxRequestSize					=
		"MaxRequestSize";
	public static final String	sConfigCDInterval				=
		"DaemonChangeDetectionInterval";
	public static final String	sChangeDetectionInterval		=
		"ChangeDetectionInterval";
	public static final String	sGarbageCollectionInterval		=
		"GarbageCollectionInterval";
	public static final String	sTimeToLive						=
		"TimeToLive";
	public static final String	sLogLevel						=
		"LogLevel";
	public static final String	sIdleThreadDetectionInterval	=
		"IdleThreadDetectionInterval";
	public static final String	sThreadTimeToLive				=
		"ThreadTimeToLive";
	public static final String	sConnectionReadTimeout			=
		"ConnectionReadTimeout";
	public static final String  sInitialChangeDetectionDelay	=
		"InitialChangeDetectionDelay";
	public static final String	sApplyLocalPolicy				=
		"ApplyLocalPolicy";
	public static final String	sMaxDbLocks						=
		"MaxDatabaseLocks";
	private static DaemonConfig	sInstance;

	private static final String	sAPOCJarFile			= "apocd.jar";
	private static final String	sDefaultsPropertiesFile	= "defaults.properties";
	public static final String	sOSPropertiesFile		= "os.properties";
	public static final String	sApocPropertiesFile		= "apocd.properties";
	public static final String	sJProxyPropertiesFile	= "policymgr.properties";
	private static final String[] sLocalPropertiesFiles =
		{ sOSPropertiesFile, sApocPropertiesFile, sJProxyPropertiesFile };
	private static String		sDaemonInstallDir;
	private static String		sDaemonPropertiesDir;

	private Properties		      mDefaultProperties;
	private RemoteConfig	      mRemoteConfig;
	private LocalConfig		      mLocalConfig;
	private final HashSet	      mConfigEventListeners		= new HashSet();
	private String			      mHostIdentifier;
	private final DaemonTimerTask mConfigLoader				=
		new DaemonTimerTask( new ConfigLoader() );

	public static void addConfigEventListener(
		final ConfigEventListener inConfigListener )
	{
		synchronized( sInstance.mConfigEventListeners )
		{
			sInstance.mConfigEventListeners.add( inConfigListener );
		}
	}

	static public String getHostIdentifier()
	{
		return sInstance.mHostIdentifier;
	}

	public static DaemonConfig getInstance()
	{
		return sInstance;
	}

	public static boolean getBooleanProperty( final String inPropertyName )
	{
		return getBooleanProperty( inPropertyName, null );
	}

	public static boolean getBooleanProperty( final String inPropertyName,
											  final String inDefaultValue )
	{
		return Boolean.valueOf(
			getStringProperty( inPropertyName, inDefaultValue ).trim() ).
				booleanValue();
	}

	public static float getFloatProperty( final String inPropertyName )
	{
		return getFloatProperty( inPropertyName, null );
	}

	public static float getFloatProperty( final String inPropertyName,
										  final String inDefaultValue )
	{
		return Float.parseFloat( 
			getStringProperty( inPropertyName, inDefaultValue ).trim() ); 
	}

	public static int getIntProperty( final String inPropertyName )
	{
		return getIntProperty( inPropertyName, null );
	}

	public static int getIntProperty( final String inPropertyName,
									  final String inDefaultValue )
	{
		return Integer.parseInt(
			getStringProperty( inPropertyName, inDefaultValue ).trim() );
	}	

	public static LocalConfig getLocalConfig()
	{
		return sInstance.mLocalConfig;
	}

	public static String getStringProperty( final String inPropertyName )
	{
		return getStringProperty( inPropertyName, null );
	}

	public static String getStringProperty( final String inPropertyName,
											final String inDefaultValue )
	{
		return sInstance.mLocalConfig.getProperty( inPropertyName,
												   inDefaultValue );
	}

	public static void init( final String[] inArgs )
		throws IOException, APOCException
	{
		sInstance = new DaemonConfig( inArgs );
		reload();
		addConfigEventListener( sInstance );
		sInstance.pollForChanges();
	}

	public static void log()
	{
		if ( APOCLogger.isLoggable( Level.CONFIG )	&&
			 sInstance != null						&&
			 sInstance.mLocalConfig != null )
		{
			final StringBuffer theBuffer = new StringBuffer( "" );
			final Enumeration theEnumeration =
				sInstance.mLocalConfig.propertyNames();
			while ( theEnumeration.hasMoreElements() )
			{
				final String theName = ( String )theEnumeration.nextElement();
				theBuffer.append( " " )
						 .append( theName )
						 .append( " = " )
						 .append(
							theName.compareTo( "Password" ) == 0 ?
								obscure(
									DaemonConfig.getStringProperty( theName ) ):
								DaemonConfig.getStringProperty( theName ) )
						 .append( "\n" );
			}
			APOCLogger.config( "Dcfg001", theBuffer.toString() );
		}
	}

	public void onConfigEvent()
	{
		pollForChanges();
	}

	public static void reload()
		throws IOException
	{
		reload( false );
	}

	public static void reload( boolean inRemoteOnly )
		throws IOException
	{
		final Set theOriginalValues = sInstance.getAllValues();
		if ( ! inRemoteOnly )
		{
			sInstance.mLocalConfig.load( sLocalPropertiesFiles );
			sInstance.setHostIdentifier();
		}
		sInstance.mRemoteConfig.load();
		if ( ! sInstance.getAllValues().equals( theOriginalValues ) )
		{
			sInstance.configLoaded();
		}
	}

	public static void removeConfigEventListener(
		final ConfigEventListener inConfigListener)
	{
		synchronized( sInstance.mConfigEventListeners )
		{
			sInstance.mConfigEventListeners.remove( inConfigListener );
		}
	}

	private DaemonConfig( final String[] inArgs )
		throws IOException
	{
		sDaemonInstallDir		= inArgs[ 0 ];
		sDaemonPropertiesDir	= inArgs[ 1 ];
		createDefaults();
		mLocalConfig			= new LocalConfig( sDaemonPropertiesDir );
		mRemoteConfig			= new RemoteConfig();
		mLocalConfig.setDefaults( mRemoteConfig );
		mRemoteConfig.setDefaults( mDefaultProperties );
	}

	private void configLoaded()
	{
		Object[] theConfigEventListeners;
		synchronized( mConfigEventListeners )
		{
			theConfigEventListeners = mConfigEventListeners.toArray();
		}
		if ( theConfigEventListeners != null )
		{
			for ( int theIndex = 0;
				  theIndex < theConfigEventListeners.length;
				  ++ theIndex )
			{
				( ( ConfigEventListener )theConfigEventListeners[ theIndex ] )
											.onConfigEvent();
			}
		}
	}

	private void createDefaults()
		throws IOException
	{
		ZipFile theZipFile =
			new ZipFile(
				new StringBuffer( sDaemonInstallDir )
						.append( File.separatorChar )
						.append( sAPOCJarFile )
						.toString() );

		String thePath = new String( "com/sun/apoc/daemon/" );

		mDefaultProperties = new Properties();
		mDefaultProperties.load(
			theZipFile.getInputStream(
				theZipFile.getEntry( 
					new StringBuffer( thePath )
						.append( sDefaultsPropertiesFile )
							.toString() ) ) );

		mDefaultProperties.load(
			theZipFile.getInputStream(
				theZipFile.getEntry(
					new StringBuffer( thePath )
						.append( sOSPropertiesFile )
							.toString() ) ) );
	}

	private Set getAllValues()
	{
		final HashSet theSet = new HashSet();
		if ( mLocalConfig != null )
		{
			final Enumeration theEnumeration = mLocalConfig.propertyNames();
			while ( theEnumeration.hasMoreElements() )
			{
				theSet.add(
					mLocalConfig.getProperty(
						( String )theEnumeration.nextElement() ) );
			}
		}
		return theSet;
	}

    /**
      * Check if the address can be used to identify the machine from
      * an external point of view. For the moment limited to checking 
      * that it's not the loopback address.
      */
    private static boolean isAcceptableAddress(InetAddress inAddress) {
        return !inAddress.isLoopbackAddress() ;
    }

	private String guessHostIdentifer()
	{
		String theHostIdentifier = null;
		try
		{
			Enumeration theInterfaces = NetworkInterface.getNetworkInterfaces();
			if ( theInterfaces != null )
			{
				while ( theInterfaces.hasMoreElements() )
				{
					NetworkInterface	theInterface	=
						( NetworkInterface )theInterfaces.nextElement();
					Enumeration			theInetAddresses=
						theInterface.getInetAddresses();
					if ( theInetAddresses != null )
					{
						while ( theInetAddresses.hasMoreElements() )
						{
							InetAddress	theAddress		=
								( InetAddress )theInetAddresses.nextElement();
                            if (isAcceptableAddress(theAddress))
							{
								theHostIdentifier = theAddress.getHostAddress();
								break;
							}
						}
					}	
				}	
			}
		}
		catch( Exception theException ){}
		return theHostIdentifier;
	}

	private static String obscure( final String inString )
	{
		String theString = null;
		if ( inString != null )
		{
			theString = inString.replaceAll( "\\p{Alnum}", "*" );
		}
		return theString;
	}

	private void pollForChanges()
	{
		mConfigLoader.setPeriod( 60000 * getIntProperty( sConfigCDInterval ) );
	}

	private void setHostIdentifier()
	{
		try
		{
			if ( mLocalConfig.getProperty( "HostIdentifierType", "Hostname" )
					.compareTo( "IPAddress" ) == 0 )
			{
                InetAddress theLocalHost = InetAddress.getLocalHost() ;
                if (isAcceptableAddress(theLocalHost)) 
                {
                    mHostIdentifier = theLocalHost.getHostAddress() ;
                }
                else
				{
					mHostIdentifier = guessHostIdentifer();
				}
			}
			else
			{
				mHostIdentifier = InetAddress.getLocalHost().getHostName();
			}
		}
		catch( UnknownHostException theException )
		{
			mHostIdentifier = null;
		}
	}

	class ConfigLoader implements Runnable
	{
		public void run()
		{
			try
			{
				sInstance.reload( true );
			}
			catch( IOException theException )
			{}
		}
	}
}
