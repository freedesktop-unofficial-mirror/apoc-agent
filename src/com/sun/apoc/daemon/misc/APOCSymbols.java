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
package com.sun.apoc.daemon.misc;

import java.util.*;

public class APOCSymbols
{
	public static final int		sSymUnknown				= 0;


	// Protocol Stuff
	// Binding
	public static final String	sContentLength			= "CONTENT-LENGTH:";

	public static final int		sSymContentLength		= 1;

	// Message Names - Requests
	public static final String	sReqAddListener			= "addListener";
	public static final String	sReqCreateSession		= "createSession";
	public static final String	sReqDestroySession		= "destroySession";
	public static final String	sReqList				= "list";
	public static final String	sReqRead				= "read";
	public static final String	sReqRemoveListener		= "removeListener";
	public static final String	sReqCreateSessionExt	= "createSessionExt";

	public static final int		sSymReqAddListener		= 11;
	public static final int		sSymReqCreateSession	= 12;
	public static final int		sSymReqDestroySession	= 13;
	public static final int		sSymReqList				= 14;
	public static final int		sSymReqRead				= 15;
	public static final int		sSymReqRemoveListener	= 16;
	public static final int		sSymReqCreateSessionExt	= 17;

	// Message Names - Responses
	public static final String	sRespSuccess					= "success";
	public static final String	sRespInvalidRequest				=
		"invalidRequest";
	public static final String	sRespConnectionFailure			=
		"connectionFailure";
	public static final String	sRespAuthenticationFailure		=
		"authenticationFailure";
	public static final String	sRespInvalidSessionId			=
		"invalidSessionId";
	public static final String	sRespNoSuchComponent			=
		"noSuchComponent";
	public static final String	sRespUnknownFailure				=
		"unknownFailure";
	public static final String	sRespSuccessContinueLocalAuth	=
		"successContinueLocalAuth";
	public static final String	sRespSuccessContinueSASLAuth	=
		"successContinueSASLAuth";

	public static final int		sSymRespSuccess					= 20;
	public static final int		sSymRespInvalidRequest			= 21;
	public static final int		sSymRespConnectionFailure		= 22;
	public static final int		sSymRespAuthenticationFailure	= 23;
	public static final int		sSymRespInvalidSessionId		= 24;
	public static final int		sSymRespNoSuchComponent			= 25;
	public static final int		sSymRespUnknownFailure			= 26;
	public static final int		sSymRespSuccessContinueLocalAuth= 27;
	public static final int		sSymRespSuccessContinueSASLAuth	= 28;

	// Message Names - Notification
	public static final String	sAddNotification	= "addNotification";
	public static final String	sRemoveNotification	= "removeNotification";
	public static final String	sModifyNotification	= "modifyNotification";

	public static final int		sSymAddNotification		= 30;
	public static final int		sSymRemoveNotification	= 31;
	public static final int		sSymModifyNotification	= 32;

	// Message Parameters
	public static final String	sParamUserId			= "userId";
	public static final String	sParamEntityId			= "entityId";
	public static final String	sParamCredentials		= "credentials";
	public static final String	sParamSessionId			= "sessionId";
	public static final String	sParamComponentName		= "componentName";
	public static final String	sParamClientData		= "clientData";
	public static final String	sParamMessage			= "message";
	public static final String	sParamVersion			= "version";
	public static final String	sParamServer			= "server";
	public static final String	sParamAuthType			= "authType";
	public static final String	sParamLayer				= "layer";
	public static final String	sParamEntityType		= "entityType";

	public static final int		sSymParamUserId			= 40;
	public static final int		sSymParamEntityId		= 41;
	public static final int		sSymParamCredentials	= 42;
	public static final int		sSymParamSessionId		= 43;
	public static final int		sSymParamComponentName	= 44;
	public static final int		sSymParamClientData		= 45;
	public static final int		sSymParamMessage		= 46;
	public static final int		sSymParamVersion		= 47;
	public static final int		sSymParamServer			= 48;
	public static final int		sSymParamAuthType		= 49;
	public static final int		sSymParamLayer			= 50;
	public static final int		sSymParamEntityType		= 51;

	// Misc
	public static final String	sDBCreateFailed					=
		"Failed to create a db handle";
	public static final String	sDBEnvCreateFailed				=
		"Failed to create a db environment handle";
	public static final String	sCreateServerChannelFailed		=
		"Failed to create a server channel";
	
	public static final int		sSymDBCreateFailed				= 60;
	public static final int		sSymDBEnvCreateFailed			= 61;
	public static final int		sSymCreateServerChannelFailed	= 62;

	private static final Hashtable sProtocolSyms = new Hashtable();
	private static final Hashtable sProtocolStrs = new Hashtable();

	static
	{
		addEntries( sContentLength,
					new Integer( sSymContentLength ) );
		addEntries( sReqAddListener,
					new Integer( sSymReqAddListener ) );
		addEntries( sReqCreateSession,
					new Integer( sSymReqCreateSession ) );
		addEntries( sReqDestroySession,
					new Integer( sSymReqDestroySession ) );
		addEntries( sReqList,
					new Integer( sSymReqList ) );
		addEntries( sReqRead,
					new Integer( sSymReqRead ) );
		addEntries( sReqRemoveListener,
					new Integer( sSymReqRemoveListener ) );
		addEntries( sReqCreateSessionExt,
					new Integer( sSymReqCreateSessionExt ) );
		addEntries( sRespSuccess,
					new Integer( sSymRespSuccess ) );
		addEntries( sRespInvalidRequest,
					new Integer( sSymRespInvalidRequest ) );
		addEntries( sRespConnectionFailure,
					new Integer( sSymRespConnectionFailure ) );
		addEntries( sRespAuthenticationFailure,
					new Integer( sSymRespAuthenticationFailure ) );
		addEntries( sRespInvalidSessionId,
					new Integer( sSymRespInvalidSessionId ) );
		addEntries( sRespNoSuchComponent,
					new Integer( sSymRespNoSuchComponent ) );
		addEntries( sRespUnknownFailure,
					new Integer( sSymRespUnknownFailure ) );
		addEntries( sRespSuccessContinueLocalAuth,
					new Integer( sSymRespSuccessContinueLocalAuth ) );
		addEntries( sRespSuccessContinueSASLAuth,
					new Integer( sSymRespSuccessContinueSASLAuth ) );
		addEntries( sAddNotification,
					new Integer( sSymAddNotification ) );
		addEntries( sRemoveNotification,
					new Integer( sSymRemoveNotification ) );
		addEntries( sModifyNotification,
					new Integer( sSymModifyNotification ) );
		addEntries( sParamUserId,
					new Integer( sSymParamUserId ) );
		addEntries( sParamEntityId,
					new Integer( sSymParamEntityId ) );
		addEntries( sParamCredentials,
					new Integer( sSymParamCredentials ) );
		addEntries( sParamSessionId,
					new Integer( sSymParamSessionId ) );
		addEntries( sParamComponentName,
					new Integer( sSymParamComponentName ) );
		addEntries( sParamClientData,
					new Integer( sSymParamClientData ) );
		addEntries( sParamMessage,
					new Integer( sSymParamMessage ) );
		addEntries( sParamVersion,
					new Integer( sSymParamVersion ) );
		addEntries( sParamServer,
					new Integer( sSymParamServer ) );
		addEntries( sParamAuthType,
					new Integer( sSymParamAuthType ) );
		addEntries( sParamLayer,
					new Integer( sSymParamLayer ) );
		addEntries( sParamEntityType,
					new Integer( sSymParamEntityType ) );
		addEntries( sDBCreateFailed,
					new Integer( sSymDBCreateFailed ) );
		addEntries( sDBEnvCreateFailed,
					new Integer( sSymDBEnvCreateFailed ) );
		addEntries( sCreateServerChannelFailed,
					new Integer( sSymCreateServerChannelFailed ) );
	}

	private static void addEntries( final String	inProtocolString,
							   		final Integer	inProtocolSymbol )
	{
		sProtocolSyms.put( inProtocolString, inProtocolSymbol );
		sProtocolStrs.put( inProtocolSymbol, inProtocolString );
	}

	public static int getProtocolSymbol( final String inString )
	{
		int theSym = sSymUnknown;
		if ( inString != null )
		{
			final Integer theValue = ( Integer )sProtocolSyms.get( inString );
			if ( theValue != null )
			{
				theSym = theValue.intValue();
			}
		}
		return theSym;
	}

	public static String getProtocolString( final int inSymbol )
	{
		return ( String )sProtocolStrs.get( new Integer( inSymbol ) );
	}
}
