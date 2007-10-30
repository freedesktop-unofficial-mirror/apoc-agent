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

public class TransactionFactory
{

	private static TransactionFactory	sInstance;
	private static final Transaction[]	sTransactionModel		=
		new Transaction[ 0 ];
	//private static long					sTransactionTimeout		= 30000;
	private static long					sTransactionTimeout		= 10000;
	private final HashSet				mTransactions			= new HashSet();
	private final DaemonTimerTask		mTransactionInterrupter	=
		new DaemonTimerTask( new TransactionInterrupter() );

	public void addTransaction( final Transaction inTransaction )
	{
		synchronized( mTransactions )
		{
			if ( mTransactions.size() == 0 )
			{
				mTransactionInterrupter.setPeriod( sTransactionTimeout );
			}
			mTransactions.add( inTransaction );
		}
	}

    public ChangeDetectTransaction createChangeDetectTransaction(
                                    final ClientManager     inClientManager,
                                    final Cache             inCache,
                                    final UpdateAggregator  inAggregator )
    {
        return new ChangeDetectTransaction( inClientManager, 
                                            inCache, inAggregator );
    }

	public GarbageCollectTransaction createGarbageCollectTransaction(
							final Database inDatabase )
	{
		return new GarbageCollectTransaction( inDatabase );
	}

	public Transaction createTransaction( final Client	      inClient,
										  final Message	      inMessage )
	{
		Transaction	theTransaction	= null;
		switch ( inMessage.getName() )
		{
			case APOCSymbols.sSymReqRead:
				theTransaction = new ReadTransaction( inClient, inMessage );
				break;

			case APOCSymbols.sSymReqAddListener:
				theTransaction =
					new AddListenerTransaction( inClient, inMessage );
				break;

			case APOCSymbols.sSymReqCreateSession:
				theTransaction =
					new CreateSessionTransaction( inClient, inMessage );	
				break;

			case APOCSymbols.sSymReqCreateSessionExt:
				theTransaction =
					new CreateSessionExtTransaction( inClient, inMessage );	
				break;

			case APOCSymbols.sSymReqDestroySession:
				theTransaction =
					new DestroySessionTransaction( inClient, inMessage );
				break;

			case APOCSymbols.sSymReqRemoveListener:
				theTransaction =
					new RemoveListenerTransaction( inClient, inMessage );
				break;

			case APOCSymbols.sSymReqList:
				theTransaction =
					new ListTransaction( inClient, inMessage );
				break;

			default:
				break;
		}
		return theTransaction;
	}

	public static TransactionFactory getInstance()
	{
		if ( sInstance == null )
		{
			sInstance = new TransactionFactory();
		}
		return sInstance;
	}

	public void removeTransaction( final Transaction inTransaction )
	{
		synchronized( mTransactions )
		{
			if ( mTransactions.size() == 1 )
			{
				mTransactionInterrupter.cancel();
			}
			mTransactions.remove( inTransaction );
		}
	}

	private TransactionFactory(){}

	class TransactionInterrupter implements Runnable
	{
		public void run()
		{
			synchronized( mTransactions )
			{
				Transaction[] theTransactions = null;
				if ( mTransactions.size() > 0 )
				{
					theTransactions =
					 ( Transaction[] )mTransactions.toArray( sTransactionModel);
				}
				if ( theTransactions != null && theTransactions.length > 0 )
				{
					for ( int theIndex = 0;
					      theIndex < theTransactions.length;
					      ++ theIndex )
					{
						if ( theTransactions[ theIndex ].getDuration() >
								sTransactionTimeout )
						{
							theTransactions[ theIndex ].interrupt();
							removeTransaction( theTransactions[ theIndex ] );
						}
					}
				}
			}
		}
	}
}
