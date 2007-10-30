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

#include "papiEntityList.h"

#include <stdlib.h>
#include <string.h>

PAPIEntityList *papiAddEntityItem(PAPIEntityList *aEntities,
                                  PAPIEntityType aEntityType,
                                  const char *aEntityId)
{
    PAPIEntityList *retCode = aEntities ;

    if (aEntityId != NULL) 
    {
        PAPIEntityList *last = retCode ;
        PAPIEntityList *newItem = malloc(sizeof(PAPIEntityList)) ;

        newItem->entityType = aEntityType ;
        newItem->entityId = strdup(aEntityId) ;
        newItem->next = NULL ;
        while (last != NULL && last->next != NULL) { last = last->next ; }
        if (last != NULL) { last->next = newItem ; }
        else { retCode = newItem ; }
    }
    return retCode ;
}

PAPIEntityList *papiCopyEntityList(const PAPIEntityList *aEntities) 
{
    PAPIEntityList *retCode = NULL ;

    if (aEntities != NULL)
    {
        retCode = malloc(sizeof(PAPIEntityList)) ;
        retCode->entityType = aEntities->entityType ;
        if (aEntities->entityId != NULL) 
        {
            retCode->entityId = strdup(aEntities->entityId) ;
        }
        else { retCode->entityId = NULL ; }
        retCode->next = papiCopyEntityList(aEntities->next) ;
    }
    return retCode ;
}

void papiFreeEntityList(PAPIEntityList *aEntities)
{
    if (aEntities != NULL) 
    {
        if (aEntities->entityId != NULL) 
        {
            free((void *) aEntities->entityId) ;
        }
        if (aEntities->next != NULL) { papiFreeEntityList(aEntities->next) ; }
        free(aEntities) ;
    }
}

