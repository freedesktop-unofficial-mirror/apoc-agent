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

import com.sun.apoc.daemon.misc.*;

import java.io.*;
import java.util.*;

public class Database extends Db
{
	protected	final Name			mName;
	private 	Dbc					mDbc;

	private static final int		sDbType				= Db.DB_BTREE;
	private static final int		sDbFlags			= Db.DB_CREATE;
	private static final int		sDbMode				= 0600;
	private static final String		sValueEpoch			= "19700101000000Z";

	public static final String	sKeyLastChangeDetect	= "lastChangeDetect";
	public static final String	sKeyLastModified		= "lastModified/";
	public static final String	sKeyLastRead			= "lastRead";
	public static final String	sKeyLayerIds			= "layerIds/";
	public static final String	sKeyDBs					= "dbs";
	public static final String	sKeyRemoteComponentList	=
		"remoteComponentList";
	public static final String	sKeyLocalComponentList	=
		"localComponentList";

	public Database( final Name inName )
		throws APOCException, DbException, FileNotFoundException
	{
		super( LocalDatabaseFactory.getInstance(), 0 );
		mName	= inName;
		openDatabase( LocalDatabaseFactory.getLocalDatabaseFileName(mName),
			          getDatabaseName(mName));
	}

    public Database(final Name inName, final String inFileName) 
        throws APOCException, DbException, FileNotFoundException
    {
        super(LocalDatabaseFactory.getInstance(), 0) ;
        mName = inName ;
        openDatabase(inFileName, getDatabaseName(mName)) ;
    }
    private void openDatabase(final String inFileName, 
                              final String inDatabaseName)
        throws APOCException, DbException, FileNotFoundException
    {
        open(null, inFileName, inDatabaseName, sDbType, sDbFlags, sDbMode) ;
    }

	public Name getName() { return mName; }

    public static String getDatabaseName(Name aName)
    {
        return new StringBuffer(aName.getEntityType()).append(Name.sSep)
                        .append(aName.getEntityName()).append(Name.sSep)
                        .append(aName.getLocation()).toString() ;
    }

    public static Database createDatabase(String aName) 
        throws APOCException, DbException, FileNotFoundException
    {
        String [] components = aName.split(Name.sSep) ;

        if (components != null && components.length > 2) 
        {
            return new Database(new Name(null, 
                                         components [0], 
                                         components [1],
                                         components [2])) ;
        }
        throw new APOCException() ;
    }

    public String toString() { return getDatabaseName(mName) ; }

	public void beginTransaction()
		throws APOCException
	{
		try
		{
			final Dbc theCursor = cursor( null, Db.DB_WRITECURSOR );
			mDbc = theCursor;
		}
		catch( DbException theException )
		{
			throw new APOCException( theException );
		}
	}

	public void endTransaction()
		throws APOCException
	{
		if ( mDbc != null )
		{
			try
			{
				final Dbc theCursor = mDbc;
				mDbc = null;
				theCursor.close();
			}
			catch( DbException theException )
			{
				throw new APOCException( theException );
			}
		}
	}

	public int get( String inKey, final Dbt outValue )
		throws APOCException
	{
		return get( new StringDbt( inKey ), outValue );
	}

	public int get( final Dbt inKey, final Dbt outValue )
		throws APOCException
	{
		try
		{
			return get( null, inKey, outValue, 0 );
		}
		catch( DbException theException )
		{
			throw new APOCException( theException );
		}
	}

	public HashSet getDBList()
		throws APOCException
	{
		final SetDbt theSetDbt = new SetDbt();
		get( sKeyDBs, theSetDbt );
		return theSetDbt.getApocData();
	}

	public String getLastChangeDetect()
		throws APOCException
	{
		final StringDbt theValue = new StringDbt();
		get( sKeyLastChangeDetect, theValue );
		if ( theValue.get_size() > 0 )
		{
			return theValue.getApocData();
		}
		else
		{
			return sValueEpoch;
		}
	}

	public String getLastModified( final String inLayerId,
								   final String inComponentName )
		throws APOCException
	{
		final StringDbt	theValue = new StringDbt();
		get( new StringBuffer( sKeyLastModified )
				.append( inLayerId )
				.append( "/" )
				.append( inComponentName )
				.toString(),
			 theValue );
		if ( theValue.get_size() > 0 )
		{
			return theValue.getApocData();
		}
		else
		{
			return sValueEpoch;
		}
	}

	public String getLastRead()
		throws APOCException
	{
		final StringDbt	theValue = new StringDbt();
		get( sKeyLastRead, theValue );
		if ( theValue.get_size() > 0 )
		{
			return theValue.getApocData();
		}
		else
		{
			return sValueEpoch;
		}
	}

	public String getLayer( final String inComponentName,
							final String inLayerId )
		throws APOCException
	{
		final String theKey	= new StringBuffer( inLayerId )
									.append( "/" )
									.append( inComponentName )
									.toString();
		StringDbt theValue	= new StringDbt();
		get( theKey, theValue );
		return theValue.getApocData();	
	}

	public Vector getLayerIds( final String inComponentName )
		throws APOCException
	{
		final VectorStringDbt theVectorDbt = new VectorStringDbt();
		get( new StringBuffer( sKeyLayerIds )
			 		.append( inComponentName )
				 	.toString(),
		     theVectorDbt );
		return theVectorDbt.retrieveData();
	}

	public HashSet getLocalComponentList()
		throws APOCException
	{
		final SetDbt theSetDbt = new SetDbt();
		get( sKeyLocalComponentList, theSetDbt );
		return theSetDbt.getApocData();
	}

	public HashSet getRemoteComponentList()
		throws APOCException
	{
		final SetDbt theSetDbt = new SetDbt();
		get( sKeyRemoteComponentList, theSetDbt );
		return theSetDbt.getApocData();
	}

	public int put( final Dbt inKey, final String inData )
		throws APOCException
	{
		return put( inKey, new StringDbt( inData ) );
	}

	public int put( final String inKey, final HashSet inData )
		throws APOCException
	{
		return put( new StringDbt( inKey ), new SetDbt( inData ) );
	}
	
	public int put( final Dbt inKey, final HashSet inData )
		throws APOCException
	{
		return put( inKey, new SetDbt( inData ) );
	}

    public int put(final String inKey, final Vector inData)
        throws APOCException
    {
        return put(new StringDbt(inKey), new VectorStringDbt(inData)) ;
    }

	public int put( final String inKey, final String inData )
		throws APOCException
	{
		return put( new StringDbt( inKey ), new StringDbt( inData ) );
	}

	public int put( final String inKey, final Dbt inData )
		throws APOCException
	{
		return put( new StringDbt( inKey ), inData );
	}

	public int put( final Dbt inKey, final Dbt inData )
		throws APOCException
	{
		try
		{
			return mDbc.put( inKey, inData, Db.DB_KEYFIRST );
		}
		catch( DbException theException )
		{
			throw new APOCException( theException );
		}
	}

	public void putDBList( final HashSet inDBList )
		throws APOCException
	{
		put( sKeyDBs, inDBList );
	}

	public void putLastChangeDetect( final String inTimestamp )
		throws APOCException
	{
		put( sKeyLastChangeDetect, inTimestamp );
	}

	public void putLastModified( final String	inLayerId,
								 final String	inComponentName,
								 final String	inTimestamp )
		throws APOCException
	{
		put( new StringBuffer( sKeyLastModified )
					.append( inLayerId )
					.append( "/" )
					.append( inComponentName )
					.toString(),
		     inTimestamp );
	}

	public void putLayerIds( final String	inComponentName,
                             final Vector	inLayerIds )
		throws APOCException
	{
		put ( new StringBuffer( sKeyLayerIds )
					.append( inComponentName )
					.toString(),
		      inLayerIds );
	}

	public void putRemoteComponentList( final HashSet inRemoteComponentList )
		throws APOCException
	{
		put( sKeyRemoteComponentList, inRemoteComponentList );
	}

	public void putLocalComponentList( final HashSet inComponentNames )
		throws APOCException
	{
		put( sKeyLocalComponentList, inComponentNames );
	}

	protected void putLastRead()
		throws APOCException
	{
		put( sKeyLastRead, Timestamp.getTimestamp() );
	}

}
