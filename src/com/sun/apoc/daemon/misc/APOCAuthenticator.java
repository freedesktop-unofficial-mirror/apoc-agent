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

public class APOCAuthenticator
{
	public static final File sAuthDir =
		new File(
			new StringBuffer(
				DaemonConfig.getStringProperty( DaemonConfig.sDataDir ) )
					.append( File.separatorChar )
					.append( ".auth" )
					.toString() );

	private File	mFile;
	private String	mFileName;
	private byte[]	mResponse;

	private static final Random	sRandom			= new Random();
	private static int			sAuthBufferSize	= 20;

	public APOCAuthenticator( final String inUserId )
		throws APOCException
	{
		final String theUserId =
			inUserId != null ? inUserId : "<Daemon administrator>";
		APOCLogger.finest( "Auth001", theUserId );
		try
		{
			createAuthFile( inUserId );
			createAuthToken();
		}
		catch( Exception theException )
		{
			cleanup();
			APOCLogger.finest( "Auth003" );
			throw new APOCException( theException );
		}
	}

	public void cleanup()
	{
		try
		{
			if ( mFile != null )
			{
				mFile.delete();
				mFile = null;
			}
		}
		catch( Exception theException )
		{}
	}

	public static int getAuthBufferSize() { return sAuthBufferSize; }

	public byte[] getChallenge()
	{
		return mFileName.getBytes();
	}

	public void processResponse( final byte[] inResponse )
		throws APOCException
	{
		try
		{
			if ( inResponse == null || inResponse.length != sAuthBufferSize )
			{
				APOCLogger.finest( "Auth003" );
				throw new APOCException();
			}
			for ( int theIndex = 0; theIndex < sAuthBufferSize; theIndex ++ )
			{
				if ( mResponse[ theIndex ] != inResponse[ theIndex ] )
				{
					APOCLogger.finest( "Auth003" );
					throw new APOCException();
				}
			}
			APOCLogger.finest( "Auth002" );
		}
		finally
		{
			cleanup();
		}	
	}

	private void createAuthFile( final String inUserId )
		throws Exception
	{
		long thePrefix = sRandom.nextLong();
		thePrefix = thePrefix < 0 ? -thePrefix : thePrefix;
		mFile =
			File.createTempFile( String.valueOf( thePrefix ), null, sAuthDir );
		mFile.deleteOnExit();
		mFileName = mFile.getAbsolutePath();
		if ( inUserId != null )
		{
			FileAccess.chown( mFileName, inUserId );
		}
		FileAccess.chmod( mFileName, 0600 );
	}

	private void createAuthToken()
		throws Exception
	{
		final FileOutputStream	theOutputStream	=
			new FileOutputStream( mFileName );
		long					theValue		= sRandom.nextLong();
		theValue = theValue < 0 ? -theValue : theValue;
		final StringBuffer		theStringBuffer =
			new StringBuffer().append( theValue );
		int theLength = theStringBuffer.length();
		if ( theLength > sAuthBufferSize )
		{
			theStringBuffer.delete( sAuthBufferSize + 1, theLength );
		}
		else
		if ( theLength < sAuthBufferSize )
		{
			int theExtraCharacters = sAuthBufferSize - theLength;
			for ( int theIndex = 0; theIndex < theExtraCharacters; theIndex ++ )
			{
				theStringBuffer.append( ( int )0 );
			}
		}
		mResponse = theStringBuffer.toString().getBytes();
		theOutputStream.write( mResponse );
		theOutputStream.close();
	}
	
}
