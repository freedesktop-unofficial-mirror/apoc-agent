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
import com.sun.apoc.spi.cfgtree.*;
import com.sun.apoc.spi.cfgtree.property.*;
import com.sun.apoc.spi.cfgtree.policynode.*;
import com.sun.apoc.spi.cfgtree.readwrite.*;
import com.sun.apoc.spi.entities.*;
import com.sun.apoc.spi.environment.*;
import com.sun.apoc.spi.policies.*;
import com.sun.apoc.spi.profiles.*;

import java.util.*;

class RemoteConfig extends Properties
{
	private static final String		sParentNodeName = "com.sun.apoc.apocd";

	public void load()
	{
		clear();
		PolicyManager thePolicyMgr = null;
		try
		{
			final Properties theProperties =
				( Properties )DaemonConfig.getLocalConfig().clone();
			theProperties.put( EnvironmentConstants.LDAP_AUTH_TYPE_KEY,
			                   EnvironmentConstants.LDAP_AUTH_TYPE_ANONYMOUS );
			thePolicyMgr = new PolicyManager( theProperties );
			load( thePolicyMgr );
		}
		catch ( Exception theException )
		{}
		finally
		{
			if ( thePolicyMgr != null )
			{
				try
				{
					thePolicyMgr.close();
				}
				catch( Exception theException )
				{
					APOCLogger.throwing( "RemoteConfig", "load", theException );
				}
			}
		}
	}

	public void setDefaults( final Properties inProperties )
	{
		defaults = inProperties;
	}

	private String getPropertyValue( final PolicyNode	inParentNode,
									 final String		inPropertyName )
	{
		String theValue	= null;
		if ( inParentNode != null )
		{
			final Property theProperty =
				inParentNode.getProperty( inPropertyName );
			if ( theProperty != null )
			{
				try
				{
					theValue = theProperty.getValue();
				}
				catch( SPIException theException ){}
			}
		}
		return theValue;
	}

	private void load( final PolicyManager inPolicyMgr )
		throws SPIException
	{
		final Entity	theDomain	= 
            inPolicyMgr.getRootEntity( EnvironmentConstants.HOST_SOURCE );
		final Iterator	theIterator	= ((Domain) theDomain).findEntities(
										DaemonConfig.getHostIdentifier(),
										true );
		if ( theIterator != null && theIterator.hasNext() )
		{
			final Entity	theEntity	= ( Entity )theIterator.next();
			final Iterator	theProfiles	= theEntity.getLayeredProfiles();
			if ( theProfiles != null )
			{
				final ArrayList thePolicies = new ArrayList();
				while ( theProfiles.hasNext() )
				{
					final Profile	theProfile	= ( Profile )theProfiles.next();
					final Policy	thePolicy	=
										theProfile.getPolicy( sParentNodeName);
					if ( thePolicy != null )
					{
						thePolicies.add( thePolicy );
					}
				}
				if ( thePolicies.size() > 0 )
				{
					final PolicyTree theTree =
						new ReadWritePolicyTreeFactoryImpl()
							.getPolicyTree(	thePolicies.iterator() );
					if ( theTree != null )
					{
						final PolicyNode theParentNode =
							theTree.getNode( sParentNodeName );
						if ( theParentNode != null )
						{
							setProp( DaemonConfig.sMaxClientThreads,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sMaxClientThreads ) );
							setProp( DaemonConfig.sMaxClientConnections,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sMaxClientConnections ) );
							setProp( DaemonConfig.sMaxRequestSize,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sMaxRequestSize ) );
							setProp( DaemonConfig.sConfigCDInterval,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sConfigCDInterval ) );
							setProp( DaemonConfig.sChangeDetectionInterval, 
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sChangeDetectionInterval ) );
							setProp( DaemonConfig.sGarbageCollectionInterval,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sGarbageCollectionInterval ) );
							setProp( DaemonConfig.sTimeToLive,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sTimeToLive ) );
							setProp( DaemonConfig.sLogLevel,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sLogLevel ) );
							setProp( DaemonConfig.sApplyLocalPolicy,
				 					getPropertyValue(
										theParentNode,
										DaemonConfig.sApplyLocalPolicy ) );
						}
					}
				}
			}
		}
	}

	private void setProp( final String inPropertyName,
					  final String inPropertyValue )
	{
		if ( inPropertyValue != null )
		{
			setProperty( inPropertyName, inPropertyValue );
		}
	}
}
