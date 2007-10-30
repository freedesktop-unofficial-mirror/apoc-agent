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
package com.sun.apoc.daemon.apocd ;

import com.sun.apoc.daemon.localdatabase.UpdateItem ;

import java.util.ArrayList ;
import java.util.HashMap ;
import java.util.Iterator ;
import java.util.Map ;

public class UpdateAggregator {
    // Key = session, value = updates
    private Map mUpdates = new HashMap() ;

    public synchronized void addUpdate(Session [] aSessions, 
                                       UpdateItem aUpdate) {
        for (int i = 0 ; i < aSessions.length ; ++ i) {
            Session session = aSessions [i] ;

            if (!mUpdates.containsKey(session)) {
                ArrayList updateList = new ArrayList() ;

                updateList.add(aUpdate) ;
                mUpdates.put(session, updateList) ;
            }
            else {
                ArrayList updateList = (ArrayList) mUpdates.get(session) ;

                for (int j = 0 ; j < updateList.size() ; ++ j) {
                    UpdateItem update = (UpdateItem) updateList.get(j) ;

                    if (update.getComponentName().equals(
                                                aUpdate.getComponentName())) {
                        if (update.getUpdateType() != aUpdate.getUpdateType()) {
                            update.setUpdateType(UpdateItem.MODIFY) ;
                        }
                        return ;
                    }
                }
                updateList.add(aUpdate) ;
            }
        }
    }
    public void sendNotifications() {
        Iterator updates = mUpdates.entrySet().iterator() ;

        while (updates.hasNext()) {
            Map.Entry update = (Map.Entry) updates.next();
            Session session = (Session) update.getKey() ;
            ArrayList updateList = (ArrayList) update.getValue() ;
            int nbUpdates = updateList.size() ;

            for (int i = 0 ; i < nbUpdates ; ++ i) {
                session.sendNotification((UpdateItem) updateList.get(i)) ;
            }
        }
    }
}

