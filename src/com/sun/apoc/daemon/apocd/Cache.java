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
import com.sun.apoc.daemon.localdatabase.*;
import com.sun.apoc.daemon.misc.*;

import com.sun.apoc.spi.*;
import com.sun.apoc.spi.policies.*;

import java.util.*;
import javax.security.auth.callback.*;


public class Cache implements ConfigEventListener
{
	private String					mUserName;
    private String                  mEntityType;
	private String					mEntityId;
	private CallbackHandler			mHandler;
	private DataSource		        mDataSources[]	= null; 
    private final HashSet           mSessions = new HashSet();
	private int						mRefCount		= 0;
	private boolean					mIsLocalPolicyEnabled =
		DaemonConfig.getBooleanProperty( DaemonConfig.sApplyLocalPolicy );
	private static final String[]	sStringModel	= new String[ 0 ];

	public Cache( final String					inUserName,
	              final String 				    inEntityType,
                  final String                  inEntityId,
	              final SaslAuthCallbackHandler inHandler )
	{
		mUserName		= inUserName;
        mEntityType     = inEntityType;
        mEntityId       = inEntityId;
		mHandler		= inHandler;
		createDataSources();
		inHandler.setReconnectMode();
		DaemonConfig.addConfigEventListener( this );
	}

	public synchronized int acquire( final Session inSession )
    {
        mSessions.add( inSession );
        return ++ mRefCount; 
    }
	public synchronized int release( final Session inSession )
    {
        mSessions.remove( inSession );
        return -- mRefCount;
    }
    public synchronized Session [] getSessions()
    {
        return (Session []) mSessions.toArray(new Session [0]);
    }
	public synchronized ArrayList changeDetect()
	{
        for ( int theIndex = 0 ; theIndex < mDataSources.length ; ++ theIndex ) 
        {
            mDataSources[ theIndex ].open(mHandler);
        }
		ArrayList	theResults					= new ArrayList();
		HashSet		theOriginalList				= null;
		int			theOnlineDataSourceCount	= 0;

        for ( int theIndex = 0 ; theIndex < mDataSources.length ; ++ theIndex )
        {
            if ( mDataSources[ theIndex ].isOnline()) 
            {
                ++ theOnlineDataSourceCount;
            }
        }
		if ( theOnlineDataSourceCount > 1 )
		{
			theOriginalList = list();	
		}
		for ( int theIndex = 0 ; theIndex < mDataSources.length ; theIndex ++ )
		{
			final ArrayList theResult = 
                                    changeDetect( mDataSources[ theIndex ] );
			if ( theResult != null )
			{
				theResults.addAll( theResult );
			}
		}
		if ( theOnlineDataSourceCount > 1 )
		{
			theResults = mergeUpdateResults( theResults, theOriginalList );
		}
		return theResults;
	}

	public synchronized void close()
	{
		for ( int theIndex = 0; theIndex < mDataSources.length ; theIndex ++ )
		{
			mDataSources[ theIndex ].close();
		}
	}

    public String getEntityType() { return mEntityType; }

	public String getEntityId() { return mEntityId; }

    public boolean isOutOfDate() 
    {
        long lastChangeDetect = Timestamp.getMillis(getLastChangeDetect()) ;

        if (lastChangeDetect == 0) { return true ; }
        return lastChangeDetect + 
               (long) (DaemonConfig.getFloatProperty(
                           DaemonConfig.sChangeDetectionInterval) * 60000) <
               Timestamp.getMillis(Timestamp.getTimestamp()) ;
    }
    
	public synchronized String getLastChangeDetect()
	{
		String theResult = null;
		for ( int theIndex = 0; theIndex < mDataSources.length; theIndex ++ )
		{
			final LocalDatabase theDatabase =
				mDataSources[ theIndex ].getLocalDatabase();
			if ( theDatabase != null )
			{
				try
				{
					theDatabase.beginTransaction();
					final String theTimestamp=theDatabase.getLastChangeDetect();
					if ( theTimestamp.compareTo( Timestamp.sEpoch ) == 0 )
					{
						break;
					}
					else if ( theResult == null )
					{
						theResult = theTimestamp;
					}
					else if ( Timestamp.isNewer( theResult, theTimestamp ) )
					{
						theResult = theTimestamp;
					}
				}
				catch( APOCException theException )
				{
					APOCLogger.throwing( "Cache",
								 	 	"getLastChangeDetect",
								 	 	theException );
				}
				finally
				{
					try
					{
						theDatabase.endTransaction();
					}
					catch( APOCException theException )
					{
						APOCLogger.throwing( "Cache",
									 	 	"getLastChangeDetect",
									 	 	theException );
					}
				}	
			}	
		}
		return theResult != null ? theResult : Timestamp.sEpoch;
	}

	public String getUserId() { return mUserName; }

	public synchronized boolean hasDatabase( final Database inDatabase )
	{
		boolean haveLocalDatabase = false;
		for ( int theIndex = 0; theIndex < mDataSources.length; theIndex ++ )
		{
			if ( mDataSources[ theIndex ].getLocalDatabase() == inDatabase )
			{
				haveLocalDatabase = true;
				break;
			}
		}
		return haveLocalDatabase;
	}

	public synchronized HashSet list()
	{
		HashSet theComponents = new HashSet();
		for ( int theIndex = 0; theIndex < mDataSources.length; theIndex ++ )
		{
			final HashSet theSet = list ( mDataSources[ theIndex ] );
			if ( theSet != null )
			{
				theComponents.addAll( theSet );
			}
		}
		return theComponents;
	}

	public synchronized void onConfigEvent()
	{
		boolean isLocalPolicyEnabled =
			DaemonConfig.getBooleanProperty( DaemonConfig.sApplyLocalPolicy );
		if ( isLocalPolicyEnabled != mIsLocalPolicyEnabled )
		{
			mIsLocalPolicyEnabled = isLocalPolicyEnabled;
			if ( mIsLocalPolicyEnabled )
			{
				enableLocalPolicy();
			}
			else
			{
				disableLocalPolicy();
			}
		}
	}

	public synchronized ArrayList read( final String inComponentName )
	{
		final ArrayList theResults = new ArrayList();
		for ( int theIndex = 0; theIndex < mDataSources.length; theIndex ++ )
		{
			final ArrayList theResult =
				read( mDataSources[ theIndex ], inComponentName );
			if ( theResult != null )
			{
				theResults.addAll( theResult );
			}
		}
		return theResults;
	}

	public synchronized String toString()
	{
		final StringBuffer theBuffer = new StringBuffer();
		for ( int theIndex = 0; theIndex < mDataSources.length; theIndex ++ )
		{
			theBuffer.append( " dataSource" + ( theIndex + 1 ) )
			         .append( "\n " )
					 .append( mDataSources[ theIndex ].toString() )
			         .append( "\n" );
		}
		return theBuffer.toString();
	}

	private ArrayList changeDetect( final DataSource inDataSource )
	{
		ArrayList theResult = null;
		if ( inDataSource.isOnline() )
		{
			final LocalDatabase theDatabase = inDataSource.getLocalDatabase();
			try
			{
				theDatabase.beginTransaction();
				switch( inDataSource.getState() )
				{
					case DataSource.sStateEnabled:
						theResult = updateLocalDatabase( inDataSource );
						break;
				
					case DataSource.sStateDisabling:
						theResult = new ArrayList();
						addRemovalItems( theResult,
					                     theDatabase.getLocalComponentList() );
						break;
					case DataSource.sStateEnabling:
						theResult = new ArrayList();
						addAdditionItems( theResult,
										  inDataSource,
					                      theDatabase.getLocalComponentList() );
						inDataSource.setState( DataSource.sStateEnabled );
						break;

					default:
						break;
				}
			}
			catch( Exception theException )
			{
				APOCLogger.throwing( "Cache", "changeDetect", theException );
			}
			finally
			{
				try
				{	
					theDatabase.endTransaction();
				}
				catch( APOCException theException )
				{
					APOCLogger.throwing( "Cache",
					                     "changeDetect",
					                     theException );
				}
			}
			if ( inDataSource.mState == DataSource.sStateDisabling )
			{
				inDataSource.close();
			}
		}
		return theResult;
	}

	private void createDataSources()
	{
        mDataSources = new DataSource [2];
        mDataSources[ 0 ] = new DataSource( mUserName, 
                                            mEntityType,
                                            mEntityId,
                                            true );
        mDataSources[ 1 ] = new DataSource( mUserName, 
                                            mEntityType,
                                            mEntityId,
                                            false );
		if ( mIsLocalPolicyEnabled )
		{
			mDataSources[ 0 ].open( mHandler );
		}
		mDataSources[ 1 ].open( mHandler );
	}

	private void disableLocalPolicy()
	{
		mDataSources[ 0 ].setState( DataSource.sStateDisabling );
	}

	private void enableLocalPolicy()
	{
		mDataSources[ 0 ].setState( DataSource.sStateEnabling );
	}

	private HashSet list( final DataSource inDataSource )
	{
		HashSet				theSet		= null;
		if ( inDataSource.getState() == DataSource.sStateEnabled )
		{
			final LocalDatabase	theDatabase	= inDataSource.getLocalDatabase();
			if ( theDatabase != null )
			{
				try
				{
					theDatabase.beginTransaction();
					theSet = theDatabase.getRemoteComponentList();
				}
				catch ( APOCException theException )
				{
					APOCLogger.throwing( "Cache", "list", theException );
				}
				finally
				{
					try
					{
						theDatabase.endTransaction();
					}
					catch( APOCException theException )
					{
						APOCLogger.throwing( "Cache", "list", theException );
					}
				}
			}
		}
		return theSet;
	}

	private ArrayList mergeUpdateResults( final ArrayList	inResults,
										  final HashSet		inOriginalList )
	{
		ArrayList theResult = inResults;
		if ( inResults.size() > 0	&&
		     inOriginalList != null &&
		     inOriginalList.size() > 0 )
		{
			final HashSet	theNewList				= list();
			final HashSet	theUpdatedComponents	= new HashSet();
			final Iterator	theIterator				= inResults.iterator();
			theResult = new ArrayList();
			while ( theIterator.hasNext() )
			{
				final UpdateItem	theItem = ( UpdateItem )theIterator.next();
				final String		theName	= theItem.getComponentName();
				if ( theUpdatedComponents.add( theName ) )
				{
					if ( ( theItem.getUpdateType() == UpdateItem.ADD &&
						   inOriginalList.contains( theName ) ) ||
					     ( theItem.getUpdateType() == UpdateItem.REMOVE ) &&
						   theNewList.contains( theName ) ) 
					{
						theItem.setUpdateType( UpdateItem.MODIFY );
					}
					theResult.add( theItem );
				}
			}
		}
		return theResult;
	}

	private ArrayList read( final DataSource	inDataSource,
	                        final String		inComponentName )
	{
		final ArrayList		theResult	= new ArrayList();
		final LocalDatabase	theDatabase	= inDataSource.getLocalDatabase();
		if ( theDatabase != null )
		{
			try
			{
				theDatabase.beginTransaction();
				boolean haveComponent =
					theDatabase.hasComponent( inComponentName );
				if ( ! haveComponent && inDataSource.isOnline() )
				{
					final HashSet theSet = theDatabase.getRemoteComponentList();
					if ( theSet != null && theSet.contains( inComponentName ) )
					{
						final Policy[] thePolicies =
							inDataSource.
								getPolicyBackend().
									getLayeredPolicies(
										inComponentName,	
										false );
						if ( thePolicies != null )
						{
							theDatabase.
								update(
									new UpdateItem(
										UpdateItem.ADD,
										inComponentName,
										thePolicies ),
									Timestamp.getTimestamp() );
							haveComponent = true;
						}
					}
				}
				if ( haveComponent )
				{
					final Vector theLayerIds =
						theDatabase.getLayerIds( inComponentName );
					for ( int theIndex = 0;
					      theIndex < theLayerIds.size();
					      ++ theIndex )
					{
						final String theLayerId			=
							( String )theLayerIds.elementAt( theIndex );
						theResult.add(
							new CacheReadResult(
									theDatabase.
										getLastModified( theLayerId,
										                 inComponentName ),
								theDatabase.
									getLayer( inComponentName, theLayerId ) ) );
					}	
				}
			}
			catch ( APOCException theException )
			{
				APOCLogger.throwing( "Cache", "read", theException );
			}
			finally
			{
				try
				{
					theDatabase.endTransaction();
				}
				catch( APOCException theException )
				{
					APOCLogger.throwing( "Cache", "read", theException );
				}
			}
		}
		return theResult;
	}

	private void addDownloadItems( final ArrayList	outResult,
								   final DataSource	inDataSource,
	                               final HashSet	inAddComponents,
	                               final HashSet	inModifyComponents )
		throws APOCException
	{
		if ( inAddComponents != null && inModifyComponents != null )
		{
			final HashSet	theSet			= ( HashSet)inAddComponents.clone();
			int				theAddCount		= theSet.size();
			theSet.addAll( inModifyComponents );
			final Policy[][]	thePolicies	=
				( Policy[][] )inDataSource.getPolicyBackend().
					getLayeredPolicies( theSet, false );
			final Iterator		theIterator	= theSet.iterator();
			int					theIndex	= 0;
			while ( theIterator.hasNext() )
			{
				final String theComponent = ( String )theIterator.next();
				outResult.add(
					new UpdateItem( inAddComponents.contains( theComponent ) ?
										UpdateItem.ADD : UpdateItem.MODIFY,
									theComponent,
									thePolicies[ theIndex ++ ] ) );
			}
			
		}
		else
		if ( inAddComponents != null )
		{
			addAdditionItems( outResult, inDataSource, inAddComponents );
		}
		else if ( inModifyComponents != null )
		{
			addModificationItems( outResult, inDataSource, inModifyComponents );
		}
			
	}

	private void addAdditionItems( final ArrayList	outResult,
								   final DataSource	inDataSource,
	                               final HashSet	inComponents )
		throws APOCException
	{
		if ( inComponents != null )
		{
			final Policy[][] thePolicies =
				( Policy[][] )inDataSource.
					getPolicyBackend().
						getLayeredPolicies( inComponents, false );
			final Iterator		theIterator		= inComponents.iterator();
			int					theIndex		= 0;
			while ( theIterator.hasNext() )
			{
				outResult.add(
					new UpdateItem( UpdateItem.ADD,
									( String )theIterator.next(),
									thePolicies[ theIndex ++ ] ) );
			}
		}
	}

	private void addModificationItems( final ArrayList	outResult,
									   final DataSource	inDataSource,
	                                   final HashSet	inComponents )
		throws APOCException
	{
		if ( inComponents != null )
		{
			final Policy[][] thePolicies =
				( Policy[][] )inDataSource.
					getPolicyBackend().
						getLayeredPolicies( inComponents, false );
			final Iterator		theIterator		= inComponents.iterator();
			int					theIndex		= 0;
			while ( theIterator.hasNext() )
			{
				outResult.add(
					new UpdateItem( UpdateItem.MODIFY,
									( String )theIterator.next(),
									thePolicies[ theIndex ++ ] ) );
			}
		}
	}

	private void addRemovalItems( final ArrayList	outResult,
								  final HashSet		inComponents )
		throws APOCException
	{
		if ( inComponents != null )
		{
			final Iterator theIterator = inComponents.iterator();
			while ( theIterator.hasNext() )
			{
				outResult.add(
					new UpdateItem( UpdateItem.REMOVE,
									( String )theIterator.next(),
									null ) );
			}
		}
	}

	private int compareLayers( final DataSource		inDataSource,
							   final String			inComponentName,
	                           final PolicyInfo[]	inPolicies )
		throws APOCException
	{
		int					theRC		= 0;
		final LocalDatabase	theDatabase	= inDataSource.getLocalDatabase();
		final Vector		theLayerIds	=
			theDatabase.getLayerIds( inComponentName );
		if ( theLayerIds.size() != inPolicies.length )
		{
			theRC = 1;
		}
		else
		{
			final String theComponentName = inPolicies[ 0 ].getId();
			for ( int theIndex = 0 ; theIndex < inPolicies.length; ++ theIndex)
			{
				final String theLayerId =
					( String )theLayerIds.elementAt( theIndex );


				if (  theLayerId.compareTo( 
						String.valueOf(
							inPolicies[ theIndex ].getProfileId().hashCode() ) )
								!= 0
				||
					  Timestamp.getTimestamp(
						inPolicies[ theIndex ].getLastModified() ).compareTo(
							theDatabase.
								getLastModified( theLayerId, theComponentName ))
							!= 0 )
				{
						theRC = 1;
						break;
				}
			}
		}
		return theRC;
	}

	private void createUpdateItems( final ArrayList		outResult,
	                                final DataSource	inDataSource,
									final HashSet		inRemoteComponents )
		throws APOCException
	{
		final HashSet	theLocalComponents	=
			inDataSource.getLocalDatabase().getLocalComponentList();
		final HashSet	theAdditions		=
			diff( inRemoteComponents, theLocalComponents );
		final HashSet	theRemovals			=
			diff( theLocalComponents, inRemoteComponents );
		final HashSet	theModifications	=
			getModifications( inDataSource,
		                      diff( theLocalComponents, theRemovals ) );
		addDownloadItems( outResult,
						  inDataSource,
						  theAdditions,
						  theModifications );
		addRemovalItems( outResult, theRemovals );
	}

	private HashSet diff( final HashSet inSet1, final HashSet inSet2 )
	{
		HashSet theResult = null;
		if ( inSet2 == null )
		{
			theResult = inSet1;
		}
		else
		if ( inSet1 != null )
		{
			theResult = ( HashSet )inSet1.clone();
			theResult.removeAll( inSet2 );
			if ( theResult.size() == 0 )
			{
				theResult = null;
			}
		}
		return theResult;
	}

	private HashSet getModifications( final DataSource	inDataSource,
	                                  final HashSet		inSet )
		throws APOCException
	{
		HashSet theModifications = null;
		if ( inSet != null )
		{
			final PolicyInfo[][]	thePolicies		=
				inDataSource.
					getPolicyBackend().
						getLayeredPolicies( inSet, true );
			final String[]		theComponents	=
				( String[] )inSet.toArray( sStringModel );
			theModifications = new HashSet();
			for ( int theIndex = 0 ; theIndex < thePolicies.length; ++ theIndex)
			{
				if ( isUpdated( inDataSource,
				                theComponents[ theIndex ],
				                thePolicies[ theIndex ] ) )
				{
					theModifications.add( theComponents[ theIndex ] );
				}
			}
			if ( theModifications.size() == 0 )
			{
				theModifications = null;
			}
		}
		return theModifications;
	}

	private boolean isUpdated(	final DataSource	inDataSource,
	                            final String		inComponent,
								final PolicyInfo[]	inPolicies )
		throws APOCException
	{
		boolean isUpdated = false;
		if ( inPolicies == null )
		{
			isUpdated = true;
		}
		else
		{
			isUpdated = compareLayers( inDataSource, inComponent, inPolicies )
								== 0 ? false : true;
		}
		return isUpdated;
	}

	private ArrayList updateLocalDatabase( final DataSource inDataSource )
		throws APOCException
	{
		final ArrayList theResult = new ArrayList();
		try
		{
			final PolicyBackend theBackend = inDataSource.getPolicyBackend();
			theBackend.refreshProfiles();
			final HashSet theRemoteComponents = theBackend.getComponentList();
			createUpdateItems( theResult, inDataSource, theRemoteComponents );
			theResult.trimToSize();
			final String		theTimestamp= Timestamp.getTimestamp();
			final LocalDatabase	theDatabase	= inDataSource.getLocalDatabase();
			theDatabase.update( theResult, theTimestamp );
			theDatabase.putLastChangeDetect( theTimestamp );
			theDatabase.putRemoteComponentList(
				theRemoteComponents != null ?
					theRemoteComponents : new HashSet() );
		}
		catch( SPIException theException )
		{
			throw new APOCException( theException );	
		}
		return theResult;
	}

	private static class DataSource
	{
		protected LocalDatabase	mLocalDatabase;
		protected PolicyBackend	mPolicyBackend;
		protected Name			mName;
		protected int			mState			= sStateDisabled;
		protected boolean		mInitialised	= false;

		static final int		sStateEnabled	= 0;
		static final int		sStateDisabled	= 1;
		static final int		sStateEnabling	= 2;
		static final int		sStateDisabling	= 3;

        DataSource( final String inUserName,
                    final String inEntityType,
                    final String inEntityId,
                    final boolean inIsLocal )
        {
            mName = new Name( inUserName, inEntityType, inEntityId, inIsLocal );
        }

		void close()
		{
			closePolicyBackend();
			closeLocalDatabase();
		}

		synchronized LocalDatabase getLocalDatabase() { return mLocalDatabase; }

		synchronized PolicyBackend getPolicyBackend() { return mPolicyBackend; }

		synchronized int getState() { return mState; }

		synchronized boolean isOnline() {
			return mPolicyBackend != null && mLocalDatabase != null; }

		synchronized void open( final CallbackHandler inHandler )
		{
			boolean isLocal = mName.isLocal();
			if ( ! isLocal ||
			     DaemonConfig.getBooleanProperty(
					DaemonConfig.sApplyLocalPolicy ) )
			{
				if ( mLocalDatabase == null )
				{
					mLocalDatabase = openLocalDatabase();
				}
				try
				{
					if ( mLocalDatabase != null && mPolicyBackend == null &&
					     ( mInitialised	||
					       mLocalDatabase.getLastChangeDetect().compareTo(
					        Timestamp.sEpoch ) == 0 ) )
					{
						mPolicyBackend = openPolicyBackend( inHandler );
						if ( ! mInitialised && mPolicyBackend != null )
						{
							initialiseDatabase();
						}	
					}
				}
				catch( APOCException theException )
				{
					APOCLogger.throwing( "DataSource", "open", theException );
				}
				mInitialised = true;
				if ( ! isLocal || mState != sStateEnabling )
				    {
					mState = sStateEnabled;
				}
			}
		}

		synchronized void setState( int inState ) { mState = inState; }

		public String toString()
		{
			return new StringBuffer( "     Name: " )
				   .append( mName.toString() )
				   .append( "\n     Cache: " )
			       .append( mLocalDatabase )
			       .append( "\n   Backend: " )
			       .append( mPolicyBackend )
				   .toString();
		}

		protected void initialiseDatabase()
		{
			try
			{
				final HashSet theSet = mPolicyBackend.getComponentList();
				if ( theSet != null )
				{
					mLocalDatabase.beginTransaction();
					mLocalDatabase.putRemoteComponentList( theSet );
					mLocalDatabase.endTransaction();
				}
			}
			catch( Exception theException )
			{
				APOCLogger.throwing( "DataSource",
				                     "initialiseDatabase",
				                     theException);
			}
		}

		protected void closeLocalDatabase()
		{
			if ( mLocalDatabase != null )
			{
				try
				{
					LocalDatabaseFactory.getInstance().closeLocalDatabase(
						mLocalDatabase );
				}
				catch( Exception theException ){}
				finally { mLocalDatabase = null; }
			}
		}

		protected void closePolicyBackend()
		{
			if ( mPolicyBackend != null )
			{
				PolicyBackendFactory.getInstance().closePolicyBackend(
					mPolicyBackend );
				mPolicyBackend = null;
			}
		}

		protected LocalDatabase openLocalDatabase()
		{
			LocalDatabase theLocalDatabase = null;
			try
			{
				theLocalDatabase =
					LocalDatabaseFactory.getInstance().
						openLocalDatabase( mName );
			}
			catch( Exception theException )
			{
				APOCLogger.throwing( "DataSource",
			                  		 "openLocalDatabase",
		                              theException );
			}
			return theLocalDatabase;
		}

		protected PolicyBackend openPolicyBackend(
									final CallbackHandler	inHandler )
		{
			PolicyBackend theBackend = null;
			try
			{
				theBackend =
					PolicyBackendFactory
						.getInstance()
							.openPolicyBackend(	mName, inHandler );
			}
			catch( APOCException theException )
			{
				APOCLogger.throwing( "PolicyBackend",
									 "openPolicyBackend",
									 theException );
			}
			return theBackend;
		}
	}
}
