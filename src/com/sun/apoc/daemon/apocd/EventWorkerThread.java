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
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;
import com.sun.apoc.daemon.transaction.*;

import java.io.*;
import java.util.*;

class EventWorkerThread extends Thread
{
	private final   EventQueue	mQueue;
	private final   Messenger	mMessenger	= new Messenger();
	private final   Object		mStateLock	= new Object();
	private			long		mTimestamp	= System.currentTimeMillis();
	private 		int			mState		= sStateIdle;

	private static	int			sStateIdle			= 0;
	private static	int			sStateBusy			= 1;
	private static	int			sStateTerminating	= 2;
	private static	long		sIdleTimeout		=
		( long )DaemonConfig.
			getIntProperty( DaemonConfig.sThreadTimeToLive ) * 60000;

	public EventWorkerThread( final EventQueue inQueue )
		throws APOCException
	{
		setName( "EventWorker" );
		setDaemon( true );
		mQueue = inQueue;
	}

	public boolean isAvailable()
	{
		return privgetState() == sStateIdle;
	}

	public void run()
	{
		while ( ! ( privgetState() == sStateTerminating ) )
		{
			final EventHandler theEventHandler = mQueue.get();
			if ( theEventHandler != null )
			{
				setState( sStateBusy );
				theEventHandler.handleEvent( mMessenger );
				setState( sStateIdle );
			}
		}
	}

	public boolean terminate( boolean inIfIdle )
	{
		boolean isTerminated = true;
		if ( ! inIfIdle )
		{
			setState( sStateTerminating );
		}
		else
		{
			long theIdleTime = 
				privgetState() == sStateIdle ?
					System.currentTimeMillis() - mTimestamp : 0;
			if ( theIdleTime > sIdleTimeout )
			{
				setState( sStateTerminating );
			}
			else
			{
				isTerminated = false;
			}
		}
		return isTerminated;
	}

	private synchronized int privgetState() { return mState; }

	private synchronized void setState( int inState )
	{
		if ( mState != sStateTerminating )
		{
			mTimestamp	= inState == sStateTerminating ?
				0 : System.currentTimeMillis();
			mState		= inState;
		}
	}
}
