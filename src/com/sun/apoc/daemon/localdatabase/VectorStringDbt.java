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
package com.sun.apoc.daemon.localdatabase;

import com.sleepycat.db.*;

import java.util.*;

class VectorStringDbt extends Dbt
{
	VectorStringDbt()
    {
    	super() ;
        set_flags(Db.DB_DBT_MALLOC) ;
    }

    VectorStringDbt(final Vector inValue)
	{
    	super() ;
        fillData(inValue) ;
        set_flags(Db.DB_DBT_MALLOC) ;
    }

    private void fillData(final Vector inValue)
    {
    	Iterator theIterator = inValue.iterator() ;
        int theTotalLength = 0 ;

        while (theIterator.hasNext()) 
        {
        	theTotalLength += ((String) theIterator.next()).length() ;
            ++ theTotalLength ; // The intermediate 0
        }
        byte [] theData = new byte [theTotalLength] ;
        int theCurrentIndex = 0 ;
            
        theIterator = inValue.iterator() ;
        while (theIterator.hasNext())
        {
        	byte [] theSource = ((String) theIterator.next()).getBytes() ;

            for (int i = 0 ; i < theSource.length ; ++ i)
            {
            	theData [theCurrentIndex ++] = theSource [i] ;
            }
            theData [theCurrentIndex ++] = 0 ;
         }
         set_data(theData) ;
         set_size(theTotalLength) ;
	}

    Vector retrieveData()
    {
    	Vector theData = null ;
        int theSize = get_size() ;

        if (theSize > 0)
        {
        	int theStart = 0 ;
            int theEnd = 0 ;
            byte [] theArray = get_data() ;

            theData = new Vector() ;
            do
            {
            	while (theEnd < theSize && theArray [theEnd] != 0) 
                {
                	++ theEnd ; 
                }
                theData.add(new String(theArray, theStart, theEnd - theStart)) ;
                ++ theEnd ; 
                theStart = theEnd ;
            } while (theEnd < theSize) ;
		}
        return theData ;
	}
}
