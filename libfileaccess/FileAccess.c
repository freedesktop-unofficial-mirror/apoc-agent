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
#include "FileAccess.h"

#ifndef WNT

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>

#else

#include <windows.h>
#include <aclapi.h>

static BOOL addACE(			PACL			inACL,
							DWORD			inMask,
							PSID			inSid );

static int	chmod(			const char *	inFile,
							int			 	inMode );

static int	chown(			const char *	inFile,
							const char *	inUser );

static BOOL initSid(		PSID_IDENTIFIER_AUTHORITY	inAuthority,
					 		DWORD						inSubAuthority,
					 		PSID *						outSid );

static BOOL getSID(			const char *	inOwner,
							PSID *		 	outSID );

static void modeToAccessMask( int			inMode,
							DWORD *			outOwnerMask,
							DWORD *			outGroupMask,
							DWORD *			outOthersMask );


static BOOL setOwner(		PSID			inSID,
							const char *	inFile );

static BOOL setPrivelege(	LPCTSTR			inPrivelege );

static BOOL setPriveleges(	LPCTSTR *		inPriveleges,
							int		 		inCount );

static BOOL setSecurity(	const char *	inFileName,
							PACL		 	inDACL );

int S_IRWXU=0700;
int S_IRUSR=0400;
int S_IWUSR=0200;
int S_IXUSR=0100;
int S_IRWXG=0070;
int S_IRGRP=0040;
int S_IWGRP=0020;
int S_IXGRP=0010;
int S_IRWXO=0007;
int S_IROTH=0004;
int S_IWOTH=0002;
int S_IXOTH=0001;

#endif

JNIEXPORT int JNICALL
Java_com_sun_apoc_daemon_misc_FileAccess_chmod
				   ( JNIEnv *	inEnv,
				  	 jclass		inClass,
				  	 jstring	inFileName,
				  	 jint		inMode )
{
	int				theResult;
	const char *	theFileName	=
		( *inEnv )->GetStringUTFChars( inEnv, inFileName, 0);	

#ifndef WNT

	theResult = chmod( theFileName, ( mode_t )inMode );

#else

	theResult = chmod( theFileName, inMode );

#endif

	( *inEnv )->ReleaseStringUTFChars( inEnv, inFileName, theFileName );
	return theResult;
}

JNIEXPORT int JNICALL
Java_com_sun_apoc_daemon_misc_FileAccess_chown
				   ( JNIEnv *	inEnv,
				  	 jclass		inClass,
				  	 jstring	inFileName,
				  	 jstring	inUserName )
{
	int				theResult;
	const char *	theFileName =
		( *inEnv )->GetStringUTFChars( inEnv, inFileName, 0 );
	const char *	theUserName =
		( *inEnv )->GetStringUTFChars( inEnv, inUserName, 0 );
#ifndef WNT

	struct passwd *	thePasswd	= getpwnam( theUserName );
	if ( thePasswd != 0 )
	{
		theResult	= chown( theFileName,
							 thePasswd->pw_uid,
							 thePasswd->pw_gid );
	}
	else
	{
		theResult	= 1;
	}

#else
	theResult = chown( theFileName, theUserName );

#endif
	( *inEnv )->ReleaseStringUTFChars( inEnv, inFileName, theFileName );
	( *inEnv )->ReleaseStringUTFChars( inEnv, inUserName, theUserName );
	return theResult;
}

#ifdef WNT

BOOL addACE( PACL inACL, DWORD inMask, PSID inSid )
{
	return inMask != 0 ?
		AddAccessAllowedAce( inACL, ACL_REVISION, inMask, inSid ) :
		AddAccessDeniedAce( inACL, ACL_REVISION, GENERIC_ALL, inSid );
}

int chmod( const char * inFileName, int inMode )
{
	#define sizeofACE( inACE ) \
		sizeof( ACCESS_ALLOWED_ACE ) - sizeof( DWORD ) + GetLengthSid( inACE )
	SID_IDENTIFIER_AUTHORITY	theCreator	= SECURITY_CREATOR_SID_AUTHORITY;
	PSID						theOwner	= 0;
	PSID						theGroup	= 0;
	PSID						theOthers	= 0;
	PSID						theAdmin	= 0;
	DWORD						theOwnerMask;
	DWORD						theGroupMask;
	DWORD						theOthersMask;
	DWORD						theDACLSize;
	PACL						theDACL		= 0;
	int							theResult	= 1;

	if ( GetVersion() < 0x80000000 )
	{
		getSID( "Administrators", &theAdmin );
		modeToAccessMask( inMode, &theOwnerMask, &theGroupMask, &theOthersMask);
		if ( initSid( &theCreator, SECURITY_CREATOR_OWNER_RID, &theOwner )!=0 &&
			 initSid( &theCreator, SECURITY_CREATOR_GROUP_RID, &theGroup )!=0 )
		{
			getSID( "Users", &theOthers );
			theDACLSize =	sizeof( ACL )			+
							sizeofACE( theOwner )	+
							sizeofACE( theGroup )	+
							sizeofACE( theOthers )	+
							sizeofACE( theAdmin );
			theDACL		=   ( PACL ) malloc( theDACLSize );
			if ( theDACL != 0 )
			{
				InitializeAcl( theDACL, theDACLSize, ACL_REVISION );
				if ( addACE( theDACL, GENERIC_ALL,  theAdmin  ) != 0 &&
				     addACE( theDACL, GENERIC_ALL,  theOwner  ) != 0 &&
				     addACE( theDACL, theGroupMask,  theGroup  ) != 0 &&
				     addACE( theDACL, theOthersMask, theOthers ) != 0 ) 
				{
					theResult = setSecurity( inFileName, theDACL ) != 0 ? 0 : 1;
				}
				free( ( void *)theDACL );
			}
		}
		if ( theOwner  != 0 ) { FreeSid( theOwner  ); }
		if ( theGroup  != 0 ) { FreeSid( theGroup  ); }
	}
	else
	{
		theResult = _chmod( inFileName, inMode );
	}
	return theResult;
}

int chown( const char * inFileName, const char * inUserName )
{
	int		theResult			= 1;
	PSID	theSID;
	LPCTSTR	thePriveleges[ 4 ]	=
		{
			SE_TAKE_OWNERSHIP_NAME,
			SE_RESTORE_NAME,
			SE_BACKUP_NAME,
			SE_CHANGE_NOTIFY_NAME
		};

	if ( GetVersion() < 0x80000000 )
	{
		if ( setPriveleges( thePriveleges, 4 ) )
		{
			if ( getSID( inUserName, &theSID ) )
			{
				theResult = setOwner( theSID, inFileName ) ? 0 : 1;
			}
		}
	}
	else
	{
		theResult = 0;
	}
	return theResult;	
}

BOOL initSid( PSID_IDENTIFIER_AUTHORITY		inAuthority,
			  DWORD							inSubAuthority,
			  PSID *						outSid )
{
	return AllocateAndInitializeSid(
                inAuthority,
                1,
                inSubAuthority,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                outSid );
}

BOOL getSID( const char * inOwner, PSID * outSID )
{
	BOOL			theRC			= FALSE;
	DWORD			theSIDSize		= 128;
	LPTSTR			theDomain		= NULL;
	DWORD			theDomainSize	= 16;
	SID_NAME_USE	theUse;

	*outSID		= ( PSID )HeapAlloc( GetProcessHeap(), 0, theSIDSize );
	theDomain	=
		( LPTSTR )HeapAlloc(GetProcessHeap(), 0, theDomainSize *sizeof( TCHAR));
	if ( *outSID != 0 )
	{
		while ( TRUE )
		{
			if ( ! LookupAccountName( NULL,
							 		  inOwner,
								 	  *outSID,
								 	  &theSIDSize,
								 	  theDomain,
								 	  &theDomainSize,
								 	  &theUse ) )
			{
				DWORD theError = GetLastError();
				if ( theError == ERROR_INSUFFICIENT_BUFFER )
				{
					*outSID = ( PSID )HeapReAlloc( 
										GetProcessHeap(),
										0,			
										*outSID,
										theSIDSize );

					if ( *outSID == 0 )
					{
						break;
					}

					theDomain = ( LPTSTR )HeapReAlloc(
											GetProcessHeap(),
											0,
											theDomain,
											theDomainSize *sizeof( TCHAR) );
					if ( theDomain == 0 )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				theRC = TRUE;
				break;
			}
		}
		if ( theDomain != 0 )
		{
			HeapFree( GetProcessHeap(), 0, theDomain );
		}
		if ( theRC == FALSE && *outSID != 0 )
		{
			HeapFree( GetProcessHeap(), 0, *outSID );
		}
	}
	return theRC;
}

void modeToAccessMask( int		inMode,
					   DWORD *	outOwnerMask,
					   DWORD *	outGroupMask,
					   DWORD *	outOthersMask )
{
	*outOwnerMask = *outGroupMask = *outOthersMask = 0;

	if ( ( inMode & S_IRUSR ) == S_IRUSR )
	{
		*outOwnerMask |= GENERIC_READ;
	}
	if ( ( inMode & S_IWUSR ) == S_IWUSR )
	{
		*outOwnerMask |= GENERIC_WRITE;
	}
	if ( ( inMode & S_IXUSR ) == S_IXUSR )
	{
		*outOwnerMask |= GENERIC_EXECUTE;
	}

	if ( ( inMode & S_IRGRP ) == S_IRGRP )
	{
		*outGroupMask |= GENERIC_READ;
	}
	if ( ( inMode & S_IWGRP ) == S_IWGRP )
	{
		*outGroupMask |= GENERIC_WRITE;
	}
	if ( ( inMode & S_IXGRP ) == S_IXGRP )
	{
		*outGroupMask |= GENERIC_EXECUTE;
	}

	if ( ( inMode & S_IROTH ) == S_IROTH )
	{
		*outOthersMask |= GENERIC_READ;
	}
	if ( ( inMode & S_IWOTH ) == S_IWOTH )
	{
		*outOthersMask |= GENERIC_WRITE;
	}
	if ( ( inMode & S_IXOTH ) == S_IXOTH )
	{
		*outOthersMask |= GENERIC_EXECUTE;
	}
}

BOOL setOwner( PSID * inSID, const char * inFileName )
{
	BOOL				theRC = FALSE;
	SECURITY_DESCRIPTOR	theDescriptor;

	if ( InitializeSecurityDescriptor( &theDescriptor,
									   SECURITY_DESCRIPTOR_REVISION ) )
	{
		if ( SetSecurityDescriptorOwner( &theDescriptor,
										 inSID,
										 FALSE ) )
		{
			if ( SetFileSecurity(
					inFileName,
					( SECURITY_INFORMATION )( OWNER_SECURITY_INFORMATION ),
					&theDescriptor ) )
			{
				theRC = TRUE;
			}
		}
	}
	return theRC;
}

BOOL setPrivelege( LPCTSTR inPrivelege )
{
	BOOL				theRC = FALSE;
	HANDLE				theToken;
	LUID				theLUID;
	TOKEN_PRIVILEGES	theTP;
	TOKEN_PRIVILEGES	thePreviousTP;
	DWORD				thePreviousTPSize = sizeof( TOKEN_PRIVILEGES );

	if ( OpenProcessToken(
			GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			&theToken ) )
	{
		if ( LookupPrivilegeValue( NULL, inPrivelege, &theLUID ) )
		{
			theTP.PrivilegeCount			= 1;
			theTP.Privileges[0].Luid		= theLUID;
			theTP.Privileges[0].Attributes	= 0;

			AdjustTokenPrivileges( theToken,
								   FALSE,
								   &theTP,
								   sizeof( TOKEN_PRIVILEGES ),
								   &thePreviousTP,
								   &thePreviousTPSize );

			thePreviousTP.PrivilegeCount			= 1;
			thePreviousTP.Privileges[0].Luid		= theLUID;
			thePreviousTP.Privileges[0].Attributes 	|= SE_PRIVILEGE_ENABLED;

			AdjustTokenPrivileges( theToken,
								   FALSE,
								   &thePreviousTP,
								   thePreviousTPSize,
								   NULL,
								   NULL );
			CloseHandle( theToken );
			theRC = TRUE;
		}
	}
	return theRC;
} 

BOOL setPriveleges( LPCTSTR * inPriveleges, int inCount )
{
	BOOL	theRC;
	int		theIndex;
	for ( theIndex = 0 ; theIndex < inCount; theIndex ++ )
	{
		if ( ( theRC = setPrivelege( inPriveleges[ theIndex ] ) ) == FALSE )
		{
			break;
		}
	}	
	return theRC;
}

BOOL setSecurity( const char * inFileName, PACL inDACL )
{
	BOOL				theRC = 0;
	SECURITY_DESCRIPTOR	theDescriptor;

	if ( InitializeSecurityDescriptor( &theDescriptor,
									   SECURITY_DESCRIPTOR_REVISION ) != 0 )
	{
		if ( SetSecurityDescriptorDacl( &theDescriptor,
										TRUE,
										inDACL,
										FALSE ) != 0 )
		{

			theRC = SetNamedSecurityInfo( ( char *)inFileName,
                SE_FILE_OBJECT,
                ( SECURITY_INFORMATION )( DACL_SECURITY_INFORMATION ),
				NULL,
				NULL,
				inDACL,
				NULL ) == ERROR_SUCCESS ? 1 : 0;
		}
	}
	return theRC;
}
#endif
