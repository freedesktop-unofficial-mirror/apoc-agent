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

import com.sun.apoc.daemon.misc.*;

import java.util.*;
import javax.security.auth.callback.*;

public class CacheFactory
{
	private final HashMap				mCaches		= new HashMap();
	private static final CacheFactory	sInstance	= new CacheFactory();
	private static final Cache[]		sCacheModel	= new Cache[ 0 ];

	private synchronized void closeCache( final Cache inCache, 
                                          final Session inSession )
	{
		if ( inCache.release( inSession ) == 0 )
		{
			final Cache theCache =
				( Cache )mCaches.remove(
					makeKey( inCache.getUserId(), 
                             inCache.getEntityType(), 
                             inCache.getEntityId() ) );
			if ( theCache != null )
			{
				theCache.close();
			}
		}
	}

    public void closeCaches( final Cache[] inCaches, final Session inSession )
    {
        for ( int theIndex = 0 ; theIndex < inCaches.length ; ++ theIndex )
        {
            closeCache( inCaches[ theIndex ], inSession );
        }
    }

	public synchronized Cache[] getCaches()
	{
		Cache[] theCaches = null;
		if ( mCaches.size() > 0 )
		{
			theCaches = ( Cache[] )mCaches.values().toArray( sCacheModel );
		}
		return theCaches;
	}

	public static CacheFactory getInstance() { return sInstance; }

    public synchronized Cache [] openCaches(
                                    final String                  inUserId,
                                    final Hashtable               inEntities,
                                    final Session                 inSession,
                                    final SaslAuthCallbackHandler inHandler )
        throws APOCException
    {
        String [] theSources = PolicyBackendFactory.getInstance().getSources();
        ArrayList theCaches = new ArrayList();

        for ( int theIndex = 0 ; theIndex < theSources.length ; ++ theIndex )
        {
            String theEntity = 
                            (String) inEntities.get( theSources[ theIndex ] );
            Cache theCache = getCache( inUserId, theSources[ theIndex ], 
                                       theEntity, inHandler );

            theCache.acquire( inSession );
            theCaches.add( theCache );
        }
        return (Cache []) theCaches.toArray( sCacheModel );
    }

	private CacheFactory(){}

    private Cache getCache(final String inUserId, 
                           final String inEntityType,
                           final String inEntityId, 
                           final SaslAuthCallbackHandler inHandler)
        throws APOCException
    {
        Cache retCode = getExistingCache(inUserId, inEntityType, inEntityId) ;

        if (retCode == null) 
        {
            retCode = createNewCache(inUserId, inEntityType, 
                                     inEntityId, inHandler) ;
        }
        return retCode ;
    }
	private Cache createNewCache( final String					inUserId,
                                  final String                  inEntityType,
								  final String					inEntityId,
								  final SaslAuthCallbackHandler inHandler )
		throws APOCException
	{
		Cache theCache = new Cache( inUserId, inEntityType, 
                                    inEntityId, inHandler );
		mCaches.put( makeKey( inUserId, inEntityType, inEntityId ), theCache );
		return theCache;
	}

	private Cache getExistingCache( final String inUserId,
                                    final String inEntityType,
									final String inEntityId )
	{
		return ( Cache )mCaches.get( makeKey( inUserId, inEntityType, 
                                              inEntityId ) );
	}

	private String makeKey( final String inUserId, final String inEntityType,
                            final String inEntityId )
	{
        return inUserId + "/" + inEntityType + "/" + 
                ( inEntityId != null ? inEntityId : "" );
	}
}
