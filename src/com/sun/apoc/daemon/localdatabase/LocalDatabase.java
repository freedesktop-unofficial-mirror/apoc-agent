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
package com.sun.apoc.daemon.localdatabase;

import com.sleepycat.db.*;

import com.sun.apoc.daemon.apocd.*;
import com.sun.apoc.daemon.misc.*;

import com.sun.apoc.spi.policies.*;

import java.io.*;
import java.nio.*;
import java.util.*;

public class LocalDatabase extends Database
{
	private HashSet			mLocalComponentList;
	private int				mRefCount				= 0;

	public LocalDatabase( final Name inName )
		throws APOCException, DbException, FileNotFoundException
	{
		super( inName );
		initLocalComponentList();
	}	

	synchronized public int acquire()    { return ++mRefCount; }
	synchronized public int release()
	{
		if ( --mRefCount == 0 )
		{
			try
			{
				beginTransaction();
				putLastRead();
				endTransaction();
				close( 0 );
			}
			catch( DbException theException )
			{}
			catch( APOCException theException )
			{}
		}
		return mRefCount;
	}

	public boolean hasComponent( final String inComponentName )
	{
		return mLocalComponentList.contains( inComponentName );
	}

	public void putLocalComponentList( final HashSet inLocalComponentList )
		throws APOCException
	{
		mLocalComponentList = inLocalComponentList;
		super.putLocalComponentList( mLocalComponentList );
	}

	public void update( final UpdateItem inUpdateItem, final String inTimestamp)
		throws APOCException
	{
		if ( inUpdateItem != null )
		{
			updateItem( inUpdateItem, new StringDbt( inTimestamp ) );
		}
	}

	public void update( final ArrayList inUpdateItems, final String inTimestamp)
		throws APOCException
	{
		int theSize;
		if ( inUpdateItems != null && ( theSize = inUpdateItems.size() ) > 0 ) 
		{
			StringDbt theTimestamp = new StringDbt( inTimestamp );
			for ( int theIndex = 0; theIndex < theSize; ++ theIndex )
			{
				updateItem( ( UpdateItem )inUpdateItems.get( theIndex ),
							theTimestamp );	
			}
		}
	}

	private void addLocalComponent( final String inComponentName )
		throws APOCException
	{
		mLocalComponentList.add( inComponentName );
		super.putLocalComponentList( mLocalComponentList );
	}

	private void initLocalComponentList()
		throws APOCException
	{
		if ( ( mLocalComponentList = super.getLocalComponentList() ) == null )
		{
			mLocalComponentList = new HashSet();
		}
	}

	private void putComponent( final String		inComponentName,
							   final Policy[]	inPolicies )
		throws APOCException
	{
		if ( inPolicies != null && inPolicies.length > 0 )
		{
			final Vector theLayerIds = new Vector( inPolicies.length );
			for ( int theIndex = 0; theIndex < inPolicies.length; theIndex ++ )
			{
				final String theLayerId =
					String.valueOf(
						inPolicies[ theIndex].getProfileId().hashCode() );
			
				String theKey	= new StringBuffer( theLayerId )
										.append( "/" )
                                    	.append( inComponentName )
										.toString();
				put( theKey, inPolicies[ theIndex ].getData() );

				theLayerIds.add( theLayerId );

				putLastModified( theLayerId,
								 inComponentName,
								 Timestamp.getTimestamp(
									inPolicies[ theIndex ].getLastModified() ));
			}
			putLayerIds( inComponentName, theLayerIds );
		}
	}

	private void removeLocalComponent( final String inComponentName )
		throws APOCException
	{
		mLocalComponentList.remove( inComponentName );
		super.putLocalComponentList( mLocalComponentList );
	}

	private void updateItem( final UpdateItem inUpdateItem )
		throws APOCException
	{
		updateItem( inUpdateItem, new StringDbt( Timestamp.getTimestamp() ) );
	}

	private void updateItem( final UpdateItem inUpdateItem,
							 final Dbt		  inTimestamp )
		throws APOCException
	{
		String		theComponentName	= inUpdateItem.getComponentName();
		Policy[]	thePolicies			= inUpdateItem.getPolicies();
		switch ( inUpdateItem.getUpdateType() )
		{
			case UpdateItem.ADD:
				putComponent( theComponentName, thePolicies );
				addLocalComponent( theComponentName );
				break;

			case UpdateItem.MODIFY:
				putComponent( theComponentName, thePolicies );
				break;

			case UpdateItem.REMOVE:
				removeLocalComponent( theComponentName );
				break;

			default:
				break;
		}
	}
}
