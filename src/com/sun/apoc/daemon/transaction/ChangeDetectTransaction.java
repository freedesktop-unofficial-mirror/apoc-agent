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
package com.sun.apoc.daemon.transaction;

import com.sun.apoc.daemon.apocd.*;
import com.sun.apoc.daemon.localdatabase.*;
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;

import java.util.*;
import java.nio.*;

public class ChangeDetectTransaction extends Transaction
{
	private ClientManager			mClientManager;
	private Cache					mCache;
    private UpdateAggregator        mAggregator ;
	private static final String[]	sStringModel	= new String[ 0 ];

	public ChangeDetectTransaction( final ClientManager	   inClientManager,
	                                final Cache			   inCache,
                                    final UpdateAggregator inAggregator )
	{
		super( null, null);
		mClientManager	= inClientManager;
		mCache			= inCache;
        mAggregator     = inAggregator ;
	}

	protected void executeTransaction()
	{
		Session theSession = null;
		try
		{
			APOCLogger.finer( "Cdtn001" );
			//
			// getLockedSession will try to lock 1 of the Session instances
			// associated with database to be refreshed. If successful,
			// the returned Session instance should be safe to use ( i.e. it
			// wont be closed by someone else ) for the duration of the
			// transaction. If unsuccessful, all relevant Session objects
			// are already closed or closing so we can skip the change
			// detection
			//
			theSession = getLockedSession();
			if ( theSession != null )
			{
				final ArrayList	theResults = mCache.changeDetect();
                Session [] theSessions = mCache.getSessions();
                int nbResults = theResults.size() ;

				if ( nbResults > 0 )
				{
                    for (int theIndex = 0 ; theIndex < nbResults ; ++ theIndex)
                    {
                        mAggregator.addUpdate(
                                        theSessions,
                                        (UpdateItem) theResults.get(theIndex)) ;
                    }
				}
			}
			APOCLogger.finer( "Cdtn002" );
		}
		catch( Exception theException )
		{
			APOCLogger.throwing( "ChangeDetectTransaction",
			                     "execute", 
			                     theException );
		}
		if ( theSession != null )
		{
			theSession.unlock();
		}	
	}

	protected boolean needSession() { return false; }

	private Session getLockedSession()
	{
		Session		theSession	= null;
		Session[]	theSessions	= mCache.getSessions();
		if ( theSessions != null )
		{
			for ( int theIndex = 0; theIndex < theSessions.length; ++ theIndex )
			{
				if ( theSessions[ theIndex ].lock() )
				{
					theSession = theSessions[ theIndex ];
					break;
				}
			}
		}
		return theSession;
	}

}
