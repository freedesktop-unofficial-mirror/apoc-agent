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

public class Name
{
	// Location
	private static int	sLocationLocal		= 0;
	private static int	sLocationGlobal		= 1;

	// Applicability
	private static int	sApplicabilityHost	= 2; 
	private static int	sApplicabilityUser	= 3;

	// Entities
	private static int	sEntityLocalHost	= 4;
	private static int	sEntityLocalUser	= 5;

	private static int	sUndefined			= 6;

	private static final String	sLocationLocalStr		= "local";
	private static final String	sLocationGlobalStr		= "global";
	private static final String	sApplicabilityHostStr	= "host";
	private static final String	sApplicabilityUserStr	= "user";
	public  static final String	sEntityLocalHostStr		= "localHost";
	public  static final String	sEntityLocalUserStr		= "localUser";

	public static final String	sSep					= "/";

	public static final String	sNames[]		=
	{
			sLocationLocalStr,
			sLocationGlobalStr,
			sApplicabilityHostStr,
			sApplicabilityUserStr,
			sEntityLocalHostStr,
			sEntityLocalUserStr
	};

	private String	mUserName;
    private String  mEntityType;
	private String	mEntityName;
	private boolean	mIsLocal;

	public Name( final String	inUserName,
                 final String   inEntityType,
				 final String	inEntityName,
				 boolean		inIsLocal )
	{
		mUserName		= inUserName;
        mEntityType     = inEntityType;
		mEntityName		= inEntityName;
		mIsLocal		= inIsLocal;
	}
    public Name( final String   inUserName,
                 final String   inEntityType,
				 final String	inEntityName,
				 final String	inIsLocal )
    {
        mUserName = inUserName ;
        mEntityType = inEntityType ;
        mEntityName = inEntityName ;
        mIsLocal = inIsLocal.compareTo(sLocationLocalStr) == 0 ;
    }

	public String	getUserName() 		{ return mUserName; }

    public String   getEntityType()     { return mEntityType; }

	public String	getEntityName() 	{ return mEntityName; }

	public String getLocation()
	{
		return sNames[ getLocationAsInt() ];
	}

	public int getLocationAsInt()
	{
		return mIsLocal ? sLocationLocal : sLocationGlobal;
	}

	public boolean	isLocal()			{ return mIsLocal; }
	public boolean	isGlobal()			{ return ! isLocal(); }

    public String toString() 
    {
        return new StringBuffer(mUserName)
                        .append(sSep)
                        .append(mEntityType)
                        .append(sSep)
                        .append(mEntityName)
                        .append(sSep)
                        .append(getLocation())
                        .toString() ;
    }
}
