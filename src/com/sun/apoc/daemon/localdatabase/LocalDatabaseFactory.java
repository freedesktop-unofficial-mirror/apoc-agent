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

import com.sun.apoc.daemon.config.*;
import com.sun.apoc.daemon.misc.*;

import java.io.*;
import java.util.*;

public class LocalDatabaseFactory extends DbEnv
{
	private static          LocalDatabaseFactory	sInstance;
	private static final    HashMap		            sDatabases	=
		new HashMap( DaemonConfig.getIntProperty(
						DaemonConfig.sMaxClientConnections ) );
	private static final String			            sLDBHome	=
		new StringBuffer(
				DaemonConfig.getStringProperty( DaemonConfig.sDataDir ) )
			.append( File.separatorChar )
			.append( "LocalDB" )
			.toString();
    private static final Name                       sDbListDbName = 
        new Name(null, "DbListType", "DbList", true) ;

		
	private static final int			sDbEnvFlags = Db.DB_CREATE		|
													  Db.DB_INIT_CDB 	|
													  Db.DB_INIT_MPOOL 	|
													  Db.DB_PRIVATE;

	private static final int			sDbEnvMode	=  0600;
	private static final String			s__dbPrefix	=  "__db.";

	private LocalDatabaseFactory()
		throws APOCException, DbException
	{
		super( 0 );
		createDBHome();
		initLocks();
		openEnv();
	}

	public void deleteDatabase( final Database inDatabase )
	{
		final Name theName = inDatabase.getName();
		synchronized ( sDatabases )
		{
			if ( ! sDatabases.containsKey( createDatabaseKey( theName ) ) )
			{
				try
				{
					final HashSet theDBs = removeDbFromDbList( theName );
					if ( theDBs == null || theDBs.size() == 0 )
					{
						new File(getLocalDatabaseFilePath(theName)).delete() ;
					}
				}
				catch( Exception theException )
				{}
			}
		}
	}
	public static LocalDatabaseFactory getInstance()
		throws APOCException
	{
		if ( sInstance == null )
		{
			try
			{
				sInstance = new LocalDatabaseFactory();
                Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
                        public void run() {
                            try {
                                Iterator databases = 
                                                sDatabases.values().iterator() ;

                                while (databases.hasNext()) {
                                    ((Database) databases.next()).close(0) ;
                                }
                                sInstance.close(0) ;
                            }
                            catch (DbException ignored) {} 
                        }
                    })) ;
                
			}
			catch ( DbException theException )
			{
				throw new APOCException( theException );
			}
		}
		return sInstance;
	}

	public Object[] getDatabases()
	{
		Object[] theDatabases = null;
		final String[] theDBFileNames = new File( sLDBHome ).list();
		if ( theDBFileNames != null )
		{
			final ArrayList theDBList = new ArrayList( theDBFileNames.length );
			for ( int theIndex = 0;
				  theIndex < theDBFileNames.length;
				  ++ theIndex )
			{
				if ( theDBFileNames[ theIndex ].startsWith( s__dbPrefix ) )
				{
					continue;
				}
				String[] theNames =
					getDatabaseNames( theDBFileNames[ theIndex ] );
				if ( theNames != null )
				{
					for ( int theNamesIndex = 0; 
						  theNamesIndex < theNames.length;
						  ++ theNamesIndex )
					{
						synchronized( sDatabases )
						{
							// No need to include LocalDatabase instances
							// ( i.e. databases with client connections )
							// in return array
							Database theDatabase =
								( Database )sDatabases.
									get( theNames[ theNamesIndex ] );
							if ( theDatabase == null )
							{
								try
								{
									theDatabase = 
                                        Database.createDatabase(
                                                theNames [ theNamesIndex ] ) ;
									theDBList.add( theDatabase );
								}
								catch( Exception theException)
								{}
							}
						}
					}
				}
			}	
			theDatabases = theDBList.toArray();
		}
		return theDatabases;
	}

	public String[] getDatabaseNames( final String inDBFileName )
	{
		String[] theDatabaseNames = null;
		try
		{
			final HashSet theDBNames = getDBList( inDBFileName );	
			if ( theDBNames != null && theDBNames.size() > 0 )
			{
				theDatabaseNames =
					( String[] )theDBNames.toArray( new String[ 0 ] );	
			}
		}
		catch( Exception theException )
		{}
		return theDatabaseNames;
	}

	public Object[] openLocalDatabases()
	{
		Object[] theLocalDatabases = null;
		if ( ! sDatabases.isEmpty() )
		{
			theLocalDatabases = sDatabases.values().toArray();
		}
		return theLocalDatabases;
	}

	public LocalDatabase openLocalDatabase( final Name inName )
		throws APOCException
	{
		final String	theKey				= createDatabaseKey( inName );
		LocalDatabase	theLocalDatabase	= null;
		synchronized( sDatabases )
		{
			theLocalDatabase = ( LocalDatabase )sDatabases.get( theKey );
			if ( theLocalDatabase == null )
			{
				theLocalDatabase = createNewLocalDatabase( inName );
			}
			if ( theLocalDatabase != null )
			{
				theLocalDatabase.acquire();
			}
		}
		return theLocalDatabase;
	}

	public void closeLocalDatabase( final LocalDatabase inLocalDatabase )
	{
		synchronized( sDatabases )
		{
			if ( inLocalDatabase.release() == 0 )
			{
				sDatabases.remove(
					createDatabaseKey(
						inLocalDatabase.getName() ) );
			}
		}
	}

	private void addDbToDbList( final Name inName )
		throws APOCException, DbException, FileNotFoundException	
	{
		final Database	theDatabase = 
                new Database(sDbListDbName, getLocalDatabaseFileName(inName)) ;
		HashSet theDBList			= theDatabase.getDBList();
		if ( theDBList == null )
		{
			theDBList = new HashSet( 1 );
		}
		theDBList.add( createDatabaseKey(inName) );
		theDatabase.beginTransaction();
		theDatabase.putDBList( theDBList );
		theDatabase.endTransaction();
		theDatabase.close( 0 );
	}

	private static String createDatabaseKey( final Name inName )
	{
        return Database.getDatabaseName(inName);
	}

	private void createDBHome()
		throws APOCException
	{
		final File theFile = new File( sLDBHome );
		if ( ! theFile.exists() )
		{
			if ( ! theFile.mkdirs() )
			{
				throw new APOCException( APOCSymbols.sSymDBEnvCreateFailed );
			}
		}
		else
		{
			if ( ! theFile.isDirectory() )
			{
				throw new APOCException( APOCSymbols.sSymDBEnvCreateFailed );
			}
		}
		FileAccess.chmod( sLDBHome, 0700 );
		FileAccess.chmod( theFile.getParent(), 0755 );
	}

	private LocalDatabase createNewLocalDatabase( final Name inName )
		throws APOCException
	{
		return createNewLocalDatabase( inName, true );
	}

	private LocalDatabase createNewLocalDatabase( final Name	inName,
												  boolean		inDeleteRetry )
		throws APOCException
	{
		ensureNonZeroLengthDBFile( inName );
		try
		{
			final LocalDatabase theLocalDatabase = new LocalDatabase( inName );
			addDbToDbList( inName );
			sDatabases.put( createDatabaseKey( inName ), theLocalDatabase );
			return theLocalDatabase;
		}
		catch ( Exception theException )
		{
			if ( inDeleteRetry )
			{
				new File( getLocalDatabaseFilePath(inName)).delete();
				return createNewLocalDatabase( inName, false );
			}
			else
			{
				throw new APOCException( theException );
			}
		}
	}

	private void ensureNonZeroLengthDBFile( final Name inName )
		throws APOCException
	{
		File theFile = new File( getLocalDatabaseTmpFilePath( inName ) );
		try
		{
			if ( theFile.exists() )
			{
				theFile.delete();
			}
		}
		catch( Exception theException )
		{
			throw new APOCException( theException );
		}

		theFile = new File ( getLocalDatabaseFilePath( inName ) );
		try
		{
			if ( theFile.exists() && theFile.length() == 0 )
			{
				theFile.delete();
			}
		}
		catch( Exception theException )
		{
			throw new APOCException( theException );
		}
	}

	/*
	 * 6417522 - default bdb values ( 1000 )for max lockers etc. too small
	 *           Adjusting the max. number of lockers dynamically ( based
	 *           roughly on MaxClientConnections ) would require the ability
	 *           to close & reopen the db environment and all database
	 *           dynamically. That's not terribly easy ... at least for our
	 *           current implementation.
	 *
     *           As an alternative and considering that additional locks etc.
	 *           are really quite cheap ( in terms of memory usge ), have
	 *           decided that a practical solution to running out of locks etc.
	 *           is to set the max at a ( most likely ) unreachable value.
	 *           The number of locks required per user connection seems to
	 *           vary dependent on the platform but 12 is a reasonable
	 *           generalisation.
	 *           For the moment, I'm using a default value of 12000 for max.
	 *           locks. This would allow approx. 1000 users to connect. It
	 *           also has no significant impact on memory use.
	 */
	private void initLocks()
		throws DbException
	{
		int theCount = DaemonConfig.getIntProperty( DaemonConfig.sMaxDbLocks );
        setLockMaxLockers( theCount );
        setLockMaxLocks( theCount );
        setLockMaxObjects( theCount );
	}

	private void openEnv()
		throws APOCException
	{
		try
		{
			new DbEnv( 0 ).remove( sLDBHome, Db.DB_FORCE );
			open( sLDBHome, sDbEnvFlags, sDbEnvMode );
		}
		catch( DbException theException )
		{
			APOCLogger.throwing( "LocalDatabaseFactory",
								 "openEnv",
								 theException );
			throw new APOCException( theException );	
		}
		catch( FileNotFoundException theException )
		{
			APOCLogger.throwing( "LocalDatabaseFactory",
								 "openEnv",
								 theException );
			throw new APOCException( theException );	
		}
	}

	private HashSet getDBList( final String inDBFileName )
		throws APOCException, DbException, FileNotFoundException	
	{
		final Database	theDatabase	= new Database(sDbListDbName, 
                                                   inDBFileName) ;
		final HashSet	theList		= theDatabase.getDBList();
		theDatabase.close( 0 );
		return theList;
	}

    public static String getLocalDatabaseFileName( final Name inName )
    {
        return new StringBuffer(inName.getEntityType())
                        .append(".")
                        .append(inName.getEntityName()).toString();
    }

	private static String getLocalDatabaseFilePath( final Name inName )
	{
		return new StringBuffer( sLDBHome )
					.append( File.separatorChar )
                    .append(getLocalDatabaseFileName(inName) )
					.toString();
	}

	private static String getLocalDatabaseTmpFilePath( final Name inName )
	{
		return new StringBuffer( sLDBHome )
					.append( File.separatorChar )
					.append( s__dbPrefix )
                    .append( getLocalDatabaseFileName( inName ) )
					.toString();
	}
    
	private HashSet removeDbFromDbList( final Name inName )
		throws APOCException, DbException, FileNotFoundException	
	{
		final Database	theDatabase =
            new Database( sDbListDbName, getLocalDatabaseFileName( inName ) );
		/*
		 * 6417504 - ensure Db handle is closed to avoid leak
		 */
		HashSet theDBList			= null;
		try
		{
			theDBList = theDatabase.getDBList();
			if ( theDBList != null )
			{
				theDBList.remove( inName.toString() );
				theDatabase.beginTransaction();
				theDatabase.putDBList( theDBList );
				theDatabase.endTransaction();
			}
		}
		finally
		{
			theDatabase.close( 0 );
		}
		return theDBList;
	}
}
