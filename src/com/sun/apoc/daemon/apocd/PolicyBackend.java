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
import com.sun.apoc.spi.policies.*;
import com.sun.apoc.spi.profiles.*;

import java.io.*;
import java.net.*;
import java.util.*;
import javax.security.auth.callback.*;

public class PolicyBackend
{
	protected static final PolicyInfo[]	sPolicyInfoModel= new PolicyInfo[ 0 ];
	protected static final Policy[]		sPolicyModel	= new Policy[ 0 ];
	protected static final Profile[]	sProfileModel	= new Profile[ 0 ];

	protected Name		    mName;
	protected PolicyManager	mPolicyMgr;
	protected Entity	    mEntity;
	protected Profile[]	    mProfiles;
	private	  int		    mRefCount				= 0;
	private	Object		    mLock					= new Object();
	private boolean		    mProfilesInitialised	= false;

	public synchronized int		acquire()		{ return ++ mRefCount; }
	public synchronized int		release()		{ return -- mRefCount; }

	public PolicyBackend( final Name		inName,
						  final Properties	inProperties )
		throws APOCException
	{
		mName = inName;
		try
		{
			mPolicyMgr	=
				PolicyBackendFactory
					.getInstance()
						.openPolicyMgr( inProperties, mName );
		}
		catch( Exception theException )
		{
			throw new APOCException( theException );
		}
	}	

	public void close()
	{
		PolicyBackendFactory.getInstance().closePolicyMgr( mPolicyMgr );
	}

    public String toString() { return mName.toString() ; }

	public synchronized HashSet getComponentList()
		throws APOCException
	{
		try
		{
			initProfiles();
			HashSet theComponentList = null;
			if ( mProfiles != null && mProfiles.length > 0 )
			{
				theComponentList = new HashSet();
				for ( int theIndex = 0;
				      theIndex < mProfiles.length;
				      theIndex ++ )
				{
					Iterator theIterator = mProfiles[ theIndex ].getPolicies();
					if ( theIterator != null )
					{
						while ( theIterator.hasNext() )
						{
                            Policy policy = (Policy) theIterator.next() ;

                            theComponentList.add(policy.getId()) ;
						}
					}
				}
			}
			return theComponentList;
		}
		catch( SPIException theException )	
		{
			throw new APOCException( theException );
		}
	}

	public synchronized PolicyInfo[][] getLayeredPolicies(
											final Set	inComponentList,
										    boolean		inTimeStampOnly )
		throws APOCException
	{
		try
		{
			initProfiles();
			PolicyInfo[][] thePolicies = null;

			if ( inComponentList != null	&&
		         mProfiles != null			&&
		         mProfiles.length > 0 )
			{
				int theIndex;
				int theComponentCount = inComponentList.size();
				if ( theComponentCount > 0 )
				{
					final Hashtable theTable			=
						new Hashtable( theComponentCount );
					final ArrayList theComponentList	=
						new ArrayList( inComponentList );
					for ( theIndex = 0;
				          theIndex < theComponentCount;
				          ++ theIndex )	
					{
						theTable.put( theComponentList.get( theIndex ),
					                  new ArrayList() );
					}
					for ( theIndex = 0;
				          theIndex < mProfiles.length;
				          ++ theIndex )
					{
						final Iterator theIterator =
							inTimeStampOnly ?
							mProfiles[ theIndex ].getPolicyInfos(
								inComponentList.iterator() ) :
							mProfiles[ theIndex ].getPolicies(
								inComponentList.iterator() );
						while ( theIterator.hasNext() )
						{
							final PolicyInfo	thePolicy	=
								( PolicyInfo )theIterator.next();
							( ( ArrayList )theTable.get( thePolicy.getId() ) )
									.add( thePolicy );
						}
					}	
					thePolicies = inTimeStampOnly ?
						new PolicyInfo[ theComponentCount ][] :
						new Policy[ theComponentCount ][];
					for ( theIndex = 0;
				          theIndex < theComponentCount;
				          ++ theIndex )
					{
						ArrayList theList =
							( ArrayList )theTable.get(
								theComponentList.get( theIndex ) );
						theList.trimToSize();
						if ( theList.size() > 0 )
						{
							thePolicies[ theIndex ] =
								inTimeStampOnly ?
								( PolicyInfo[] )theList.
									toArray( sPolicyInfoModel ) :
								( Policy[] )theList.toArray( sPolicyModel );
						}
						else
						{
							thePolicies[ theIndex ] = null;
						}
					}
				}
			}
			return thePolicies;
		}
		catch( SPIException theException )
		{
			throw new APOCException( theException );	
		}
	}

	public synchronized Policy[] getLayeredPolicies(
											final String	inComponentName,
										    boolean			inTimeStampOnly )
		throws APOCException
	{
		try
		{
			Policy[] thePolicies = null;

			if ( inComponentName != null )
			{
				if ( mProfiles != null && mProfiles.length > 0 )
				{
					ArrayList theList = new ArrayList();
					for ( int theIndex = 0;
					      theIndex < mProfiles.length;
					      ++ theIndex )
					{
						Policy thePolicy = mProfiles[ theIndex ].
											getPolicy( inComponentName );
						if ( thePolicy != null )
						{
							theList.add( thePolicy );
						}
					}
					theList.trimToSize();
					if ( theList.size() > 0 )
					{
						thePolicies =
							( Policy[] )theList.toArray( sPolicyModel );
					}
				}
			}
			return thePolicies;
		}
		catch( SPIException theException )
		{
			throw new APOCException( theException );
		}
	}

	public Name getName() { return mName; }

	public synchronized void refreshProfiles()
		throws SPIException
	{
		if ( mName.isLocal() )
		{
			refreshLocalProfiles();
		}
		else
		{
			refreshGlobalProfiles();
		}
	}

	private Entity findEntity()
    {
        Entity theEntity = null;

        try 
        {
            final Node theRoot = 
                    ( Node ) mPolicyMgr.getRootEntity( mName.getEntityType() );
            final Iterator theIterator = 
                            theRoot.findEntities( mName.getEntityName(), true );
            if ( theIterator != null && theIterator.hasNext() )
            {
                theEntity = ( Entity ) theIterator.next();
            }
        }
        catch ( Exception theException ){}
        return theEntity;
    }

	private void initProfiles()
		throws SPIException
	{
		// There are two special cases to handle here
		// 1. Creation of a first ever session for a user.
		//    In this case, we need to write the remote component list to bdb
		//    and we need profiles to figure out the list
		// 2. Reading a component for the first time
		//    If a component isn't yet in bdb, we'll need the profiles to
		//    get the appropriate layers
		synchronized( mLock )
		{
			if ( ! mProfilesInitialised )
			{
				refreshProfiles();
				mProfilesInitialised = true;
			}
		}
	}

	private void refreshGlobalProfiles()
		throws SPIException
	{
		mProfiles = null;
		if ( mEntity == null )
		{
			mEntity = findEntity();
		}
		if ( mEntity != null )
		{
			final Iterator theIterator = mEntity.getLayeredProfiles();
			if ( theIterator != null )
			{
				final ArrayList theProfiles = new ArrayList();
				while ( theIterator.hasNext() )
				{
					theProfiles.add( theIterator.next() );
				}
				if ( mEntity instanceof User )
				{
					final Profile theProfile =
						( ( User ) mEntity ).getUserProfile();
					if ( theProfile != null )
					{
						theProfiles.add( theProfile );	
					}
				}
				if ( theProfiles.size() > 0 )
				{
					mProfiles =
						( Profile[] )theProfiles.toArray( sProfileModel );
				}
			}
		}
	}

	private void refreshLocalProfiles()
		throws SPIException
	{
		mProfiles = null;
		final ProfileProvider	theProvider = 
                        mPolicyMgr.getProfileProvider( mName.getEntityType() );
		final Iterator			theIterator	= theProvider.getAllProfiles();

		if ( theIterator != null )
		{
			final ArrayList theProfiles = new ArrayList();
			while ( theIterator.hasNext() )
			{
				final Profile theProfile = ( Profile )theIterator.next();
				final Applicability theApp	= theProfile.getApplicability();
				if ( theApp.equals( Applicability.ALL ) ||
				     theApp.equals( mName.getEntityType() ) )
				{
					theProfiles.add( theProfile );
				}
			}
			if ( theProfiles.size() > 0 )
			{
				mProfiles =
					( Profile[] )theProfiles.toArray( sProfileModel );
			}
		}
	}
}
