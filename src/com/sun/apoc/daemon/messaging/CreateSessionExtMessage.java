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

package com.sun.apoc.daemon.messaging ;

import java.util.Hashtable ;

import com.sun.apoc.daemon.apocd.Client ;

import com.sun.apoc.daemon.misc.APOCException ;
import com.sun.apoc.daemon.misc.APOCSymbols ;

public class CreateSessionExtMessage extends Message 
                                     implements CredentialsProviderMessage {
    private String mUserId = null ;
    private String mCredentials = null ;
    private Hashtable mEntities = new Hashtable() ;
    private int mExpectedSymbol = APOCSymbols.sSymParamUserId ;
    private String mLastEntityType = null ;
    private boolean mIsValid = false ;

    public CreateSessionExtMessage() {
        super(APOCSymbols.sSymReqCreateSessionExt) ;
    }
    public boolean isValid() { return mIsValid ; }
    public String getUserId() { return mUserId ; }
    public String getCredentials() { return mCredentials ; }
    public Hashtable getEntities() { return mEntities ; }

    protected void addParameter(final String aName, 
                                final String aValue) throws APOCException {
        switch (APOCSymbols.getProtocolSymbol(aName)) {
            case APOCSymbols.sSymParamUserId:
                setUserId(aValue) ;
                break ;
            case APOCSymbols.sSymParamCredentials:
                setCredentials(aValue) ;
                break ;
            case APOCSymbols.sSymParamEntityType:
                setEntityType(aValue) ;
                break ;
            case APOCSymbols.sSymParamEntityId:
                setEntityId(aValue) ;
        }
    }
    private void setUserId(final String aUserId) throws APOCException {
        checkState(APOCSymbols.sSymParamUserId) ;
        mUserId = aUserId ;
        mExpectedSymbol = APOCSymbols.sSymParamCredentials ;
    }
    private void setCredentials(
                            final String aCredentials) throws APOCException {
        checkState(APOCSymbols.sSymParamCredentials) ;
        mCredentials = aCredentials ;
        mExpectedSymbol = APOCSymbols.sSymParamEntityType ;
    }
    private void setEntityType(final String aEntityType) throws APOCException {
        try { checkState(APOCSymbols.sSymParamEntityType) ; }
        catch (APOCException exception) {
            // Allow for skipping credentials
            if (mExpectedSymbol != APOCSymbols.sSymParamCredentials) {
                throw exception ;
            }
        }
        if (mEntities.contains(aEntityType)) { throw new APOCException() ; }
        mLastEntityType = aEntityType ;
        mExpectedSymbol = APOCSymbols.sSymParamEntityId ;
    }
    private void setEntityId(final String aEntityId) throws APOCException {
        checkState(APOCSymbols.sSymParamEntityId) ;
        mEntities.put(mLastEntityType, aEntityId) ;
        mIsValid = true ;
        mExpectedSymbol = APOCSymbols.sSymParamEntityType ;
    }
    private void checkState(final int aSymbol) throws APOCException {
        if (aSymbol != mExpectedSymbol) { throw new APOCException() ; }
    }
}

