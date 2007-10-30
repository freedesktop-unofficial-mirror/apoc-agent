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
import com.sun.apoc.daemon.transport.*;
import java.util.*;

public class Client
{
	private final ClientChannel	mChannel;
	private final HashMap		mSessions = new HashMap( 1 ); // norm is 1 session
	private final HashSet		mSessionListeners = new HashSet( 1 ); 

	public Client( final ClientChannel inChannel )
	{
		mChannel = inChannel;
	}

	public ClientChannel getClientChannel()	{ return mChannel; }

	// Changing this from a simple accessor because when change detection
	// and session closure occur around the same time, the toArray() call
	// below ( originally located in ClientManager ) can get confused.
	public Object[] getSessions()		
	{
		synchronized( mSessions )
		{
			return mSessions.values().toArray();
		}
	}

	public Session getSession( final String inSessionId )
	{
		synchronized( mSessions )
		{
				return inSessionId != null ?
						( Session )mSessions.get( inSessionId ):
						null;
		}
	}

	public void addSession( final Session inSession )
	{
		synchronized( mSessions )
		{
			mSessions.put( inSession.getSessionId(), inSession );
		}	
		notifySessionCreated( inSession );
	}

	public void addSessionListener( final SessionListener inSessionListener )
	{
		synchronized( mSessionListeners )
		{
			mSessionListeners.add( inSessionListener );
		}
	}

	public void closeSession( final Session inSession )
	{
		synchronized( mSessions )
		{
			mSessions.remove( inSession.getSessionId() );
		}
		inSession.close();
		notifySessionDestroyed( inSession );
	}

	synchronized public void close()
	{
		final Iterator theIterator = mSessions.keySet().iterator();
		while (  theIterator.hasNext() )
		{
			final Session theSession = 
				( Session )mSessions.get( theIterator.next() );
			theSession.close();
			notifySessionDestroyed( theSession );
		}
		mSessions.clear();
	}

	public void removeSessionListener( final SessionListener inSessionListener )
	{
		synchronized( mSessionListeners )
		{
			mSessionListeners.remove( inSessionListener );
		}
	}

	private void notifySessionCreated( final Session inSession )
	{
		synchronized( mSessionListeners )
		{
			final Iterator theIterator = mSessionListeners.iterator();
			while ( theIterator.hasNext() )
			{
				( ( SessionListener )theIterator.next() )
					.onSessionCreate( inSession );
			}
		}
	}

	private void notifySessionDestroyed( final Session inSession )
	{
		synchronized( mSessionListeners )
		{
			final Iterator theIterator = mSessionListeners.iterator();
			while ( theIterator.hasNext() )
			{
				( ( SessionListener )theIterator.next() )
					.onSessionDestroy( inSession );
			}
		}
	}
}
