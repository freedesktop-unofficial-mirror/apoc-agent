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

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.misc.*;

import com.sun.apoc.spi.*;
import com.sun.apoc.spi.entities.*;
import com.sun.apoc.spi.environment.*;
import com.sun.apoc.spi.profiles.*;

import java.io.*;
import java.util.*;
import javax.security.auth.callback.*;

public class PolicyBackendFactory
{
	public static int				sTypeHost		= 0;
	public static int				sTypeUser		= 1;
	public static final String		sLocalHostName	= "localHost";
	public static final String		sLocalUserName	= "localUsers";
	public static final Properties	sLocalProperties= new Properties();

	private HashMap						mPolicyBackends = new HashMap();
	private HashMap						mPolicyMgrs		= new HashMap();
	private static PolicyBackendFactory	sInstance;
	private static final String			sLocalPolicyDir		=
		new StringBuffer( "file://" )
			.append(  DaemonConfig.getStringProperty( DaemonConfig.sDataDir ) )
			.append( "/" )
			.append( "Policies/" )
			.toString();

	static
	{
		sLocalProperties.setProperty(
			EnvironmentConstants.URL_KEY, sLocalPolicyDir );
        String sourcesList = 
                (String) ((Properties) DaemonConfig.getLocalConfig()).get(
                                            EnvironmentConstants.SOURCES_KEY) ;
        
        if (sourcesList != null) {
            sLocalProperties.setProperty(EnvironmentConstants.SOURCES_KEY, 
                                         sourcesList);
        }
		try
		{
			new File (
				new java.net.URI( sLocalPolicyDir + "/assignments" ) ).mkdirs();
			new File (
				new java.net.URI( sLocalPolicyDir + "/profiles" ) ).mkdirs();
			new File (
				new java.net.URI(
					sLocalPolicyDir + "/entities.txt" ) ).createNewFile();
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "PolicyBackendFactory", "init", theException );
		}
	}

	public void closePolicyBackend( final PolicyBackend inPolicyBackend )
	{
		synchronized( mPolicyBackends )
		{
			if ( inPolicyBackend.release()  == 0 )
			{
				inPolicyBackend.close();
				mPolicyBackends.remove(
					createPolicyBackendKey( inPolicyBackend.getName() ) );
			}
		}
	}

	public void closePolicyMgr( final PolicyManager inPolicyMgr )
	{
		synchronized( mPolicyMgrs )
		{
			final RefCountedPolicyMgr thePolicyMgr =
				( RefCountedPolicyMgr )inPolicyMgr;
			if ( thePolicyMgr.release() == 0 )
			{
				try
				{
					thePolicyMgr.close();
				}
				catch( SPIException theException ){}
				mPolicyMgrs.remove(
					createPolicyMgrKey( thePolicyMgr.mName ) );
			}
		}
	}

	public static PolicyBackendFactory getInstance()
	{
		if ( sInstance == null )
		{
			sInstance = new PolicyBackendFactory();
		}
		return sInstance;
	}

	public PolicyBackend openPolicyBackend(
							final Name				inName,
							final CallbackHandler	inHandler )
		throws APOCException
	{
		final String	theKey		= createPolicyBackendKey( inName );
		PolicyBackend	theBackend	= null;
		synchronized( mPolicyBackends )
		{
			theBackend = ( PolicyBackend )mPolicyBackends.get( theKey );
			if ( theBackend == null )
			{
				final Properties theProperties = inName.isLocal() ?
					sLocalProperties :
					( Properties )(
						( Properties )DaemonConfig.getLocalConfig() ).clone();
				if ( inHandler != null )
				{
					theProperties.put( EnvironmentConstants.LDAP_AUTH_CBH,
								       inHandler );
				}
                theBackend = new PolicyBackend( inName, theProperties );
				mPolicyBackends.put( theKey, theBackend );
			}
			if ( theBackend != null )
			{
				theBackend.acquire();
			}
		}
		return theBackend;
	}

    public static String [] getSources() 
    {
        return EnvironmentMgr.getSources( DaemonConfig.getLocalConfig() );
    }

	public PolicyManager openPolicyMgr( final Properties	inProperties,
									    final Name			inName )
		throws APOCException
	{
		final String		theKey			= createPolicyMgrKey( inName );
		RefCountedPolicyMgr	thePolicyMgr	= null;
		synchronized( mPolicyMgrs )
		{
			try
			{
				thePolicyMgr = 
					( RefCountedPolicyMgr )mPolicyMgrs.get( theKey );
				if ( thePolicyMgr == null )
				{
					thePolicyMgr =
						new RefCountedPolicyMgr( 
							inName,
							inProperties );
					mPolicyMgrs.put( theKey, thePolicyMgr );
				}
				if ( thePolicyMgr != null )
				{
					thePolicyMgr.acquire();
				}
			}
			catch( Exception theException )
			{
				throw new APOCException( theException );
			}
		}
		return thePolicyMgr;
	}

	private PolicyBackendFactory()
	{
	}

	private String createPolicyBackendKey( final Name inName )
	{
		return inName.toString();
	}

	private String createPolicyMgrKey( final Name inName )
	{
		return inName.getUserName() + Name.sSep + inName.getLocation();
	}

	class RefCountedPolicyMgr extends PolicyManager
	{
		private int			    mRefCount = 0;
		private Name		    mName;

		RefCountedPolicyMgr( final Name inName, 
                             final Hashtable inProperties ) throws SPIException
		{
            super( inProperties );
			mName		= inName;	
		}

		int acquire() { return ++ mRefCount; }
		int release() { return -- mRefCount; }
	}
}
