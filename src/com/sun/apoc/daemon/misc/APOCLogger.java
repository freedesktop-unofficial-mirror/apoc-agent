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
package com.sun.apoc.daemon.misc;

import com.sun.apoc.daemon.config.*;

import java.io.*;
import java.util.*;
import java.util.logging.*;

public class APOCLogger
{
	private static final	String	sLoggerName	= "com.sun.apoc.daemon";
	private static final	String	sLogDir		= "/Logs";
	private static final	String	sLogFile	= "/apocd.log";
	private static final	Logger	sLogger		=
		ConfigurableLogger.getLogger( sLoggerName, sLoggerName );
	private static final	int		sLimit		= 1000000;
	private static final	int		sCount		= 5;

	static
	{
		try
		{
			final Handler	theHandler	=
				createHandler(
					DaemonConfig.getStringProperty( DaemonConfig.sDataDir ) );
			final Level		theLevel	=
				Level.parse(
					DaemonConfig.getStringProperty( DaemonConfig.sLogLevel ) );
			theHandler.setLevel( theLevel );
			theHandler.setFormatter( new SimpleFormatter() );

			sLogger.addHandler( theHandler );
			sLogger.setLevel( theLevel );
		}
		catch( Exception theException )
		{
			theException.printStackTrace();
		}
	}

	public static void config( final String inKey )
	{
		sLogger.log( Level.CONFIG, inKey );
	}

	public static void config( final String inKey, final Object inParam )
	{
		sLogger.log( Level.CONFIG, inKey, inParam );
	}

	public static void fine( final String inKey )
	{
		sLogger.log( Level.FINE, inKey );
	}

	public static void fine( final String inKey, final Object inParam )
	{
		sLogger.log( Level.FINE, inKey, inParam );
	}

	public static void finer( final String inKey )
	{
		sLogger.log( Level.FINER, inKey );
	}

	public static void finer( final String inKey, final Object inParam )
	{
		sLogger.log( Level.FINER, inKey, inParam );
	}

	public static void finest( final String inKey )
	{
		sLogger.log( Level.FINEST, inKey );
	}

	public static void finest( final String inKey, final Object inParam )
	{
		sLogger.log( Level.FINEST, inKey, inParam );
	}

	public static boolean isLoggable( final Level inLevel )
	{
		return sLogger.isLoggable( inLevel );
	}

	public static void info( final String inKey )
	{
		sLogger.log( Level.INFO, inKey );
	}

	public static void info( final String inKey, final Object inParam )
	{
		sLogger.log( Level.INFO, inKey, inParam );
	}

	public static void severe( final String inKey )
	{
		sLogger.log( Level.SEVERE, inKey );
	}

	public static void severe( final String inKey, final Object inParam )
	{
		sLogger.log( Level.SEVERE, inKey, inParam );
	}

	public static void throwing( final String		inClass,
								 final String		inMethod,
								 final Throwable	inThrowable )
	{
		sLogger.throwing( inClass, inMethod, inThrowable );
	}

	public static void warning( final String inKey )
	{
		sLogger.log( Level.WARNING, inKey );
	}

	public static void warning( final String inKey, final Object inParam )
	{
		sLogger.log( Level.WARNING, inKey, inParam );
	}

	public static void warning( final String inKey, final Object[] inParams )
	{
		sLogger.log( Level.WARNING, inKey, inParams );
	}

	private static Handler createHandler( final String inDataDir )
	{
		Handler	 theHandler;
		if ( needFileHandler() && inDataDir != null )
		{
			final String theLogDir	=
				new StringBuffer( inDataDir ).append( sLogDir ).toString();
			createLogDir( theLogDir );
			try
			{	
				theHandler	= 
					new FileHandler( 
						new StringBuffer( theLogDir )
							.append( sLogFile )
							.toString(),
						sLimit,
						sCount,
						true );
			}
			catch( Exception theException )
			{
				System.err.println("Warning:Failed to create log file handler");
				theHandler = new ConsoleHandler();
			}
		}
		else
		{
			theHandler = new ConsoleHandler();
		}
		return theHandler;
	}

	private static void createLogDir( final String inLogDir )
	{
		final File theFile = new File( inLogDir );
		if ( ! theFile.exists() )
		{
			if ( ! theFile.mkdirs() )
			{
				System.err.println( "Warning: Cannot create Log directory '" +
									inLogDir + "'" );
			}
		}
		else
		{
			if ( ! theFile.isDirectory() )
			{
				System.err.println( "Warning: Log directory '"	+
									inLogDir					+
									"' is actually a file" );
			}
		}
	}

	private static boolean needFileHandler()
	{
		boolean needsFileHandler = false;
		final String theHandler = DaemonConfig.getStringProperty( "handlers" );
		if ( theHandler == null ||
			 theHandler.compareTo( "java.util.logging.FileHandler" ) == 0 )
		{
			needsFileHandler = true;
		}
		return needsFileHandler;
	}
}
