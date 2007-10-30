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
import com.sun.apoc.daemon.messaging.*;
import com.sun.apoc.daemon.misc.*;

import java.util.*;

public abstract class Transaction
{
	protected final Client	mClient;
	protected       Session	mSession;
	protected final Message	mRequest;
	protected       Message	mResponse;
	private			Thread	mThread;
	private			Object	mThreadLock = new Object();
	private			long	mStartTime;

	public Transaction( final Client inClient, final Message inRequest )
	{
		mClient		= inClient;
		mRequest	= inRequest;
		if ( inClient != null && inRequest != null )
		{
			mSession = inClient.getSession( mRequest.getSessionId() );
		}
	}

	public Message execute()
	{
		if ( ! needSession() || mSession != null )
		{
			try
			{
				beginTransaction();
				executeTransaction();
				endTransaction();
			}
			catch( Exception theException )
			{
				APOCLogger.throwing( "Transaction", "execute", theException );
                setResponse( APOCSymbols.sSymRespUnknownFailure );
			}
		}
		else
		{
			setResponse( APOCSymbols.sSymRespInvalidSessionId );
		}
		return mResponse;
	}

	public long getDuration()
	{
		return System.currentTimeMillis() - mStartTime;
	}

	public void interrupt()
	{
		synchronized( mThreadLock )
		{
			if ( mThread != null )
			{
				mThread.interrupt();
				mThread = null;
			}
		}
	}

	protected boolean needSession()
	{
		return true;
	}

	protected void beginTransaction()
	{
		mStartTime = System.currentTimeMillis();
		setThread( Thread.currentThread() );
		TransactionFactory.getInstance().addTransaction( this );
	}
	protected abstract void executeTransaction();
	protected void endTransaction()
	{
		setThread( null );
		TransactionFactory.getInstance().removeTransaction( this );
	}

	protected void setResponse( final int inResponseType )
	{
		setResponse( inResponseType, mRequest.getSessionId(), null );
	}

	protected void setResponse( final int		inResponseType,
							    final String	inSessionId )
	{
		setResponse( inResponseType, inSessionId, null );
	}	

	protected void setResponse( final int		inResponseType,
							    final String	inSessionId,
							    final String	inResponseMessage )
	{
		mResponse = MessageFactory.createResponse( inResponseType,
												   inSessionId,	
												   inResponseMessage );
	}	

	private void setThread( final Thread inThread )
	{
		synchronized( mThreadLock )
		{
			mThread = inThread;
		}
	}
}
