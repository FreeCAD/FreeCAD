/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "DocumentObjectGroup.h"
#include "DocumentObjectGroupPy.h"
#include "GroupExtensionPy.h"
#include "Document.h"
#include "FeaturePythonPyImp.h"

using namespace App;

EXTENSION_PROPERTY_SOURCE(App::GroupExtension, App::DocumentObjectExtension)

GroupExtension::GroupExtension()
{
    initExtensionType(GroupExtension::getExtensionClassTypeId());
    
    EXTENSION_ADD_PROPERTY_TYPE(Group,(0),"Base",(App::PropertyType)(Prop_Output),"List of referenced objects");
}

GroupExtension::~GroupExtension()
{
}

DocumentObject* GroupExtension::addObject(const char* sType, const char* pObjectName)
{
    DocumentObject* obj = getExtendedObject()->getDocument()->addObject(sType, pObjectName);
    if(!allowObject(obj)) {
        getExtendedObject()->getDocument()->remObject(obj->getNameInDocument());
        return nullptr;
    }
    if (obj) addObject(obj);
    return obj;
}

void GroupExtension::addObject(DocumentObject* obj)
{
    if(!allowObject(obj))
        return;
    
    //only one group per object
    auto *group = App::GroupExtension::getGroupOfObject(obj);
    if(group && group != getExtendedObject())
        group->getExtensionByType<App::GroupExtension>()->removeObject(obj);
    
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Group.getValues();
        grp.push_back(obj);
        Group.setValues(grp);
    }
}

void GroupExtension::removeObject(DocumentObject* obj)
{
    const std::vector<DocumentObject*> & grp = Group.getValues();
    std::vector<DocumentObject*> newGrp;

    std::remove_copy (grp.begin(), grp.end(), std::back_inserter (newGrp), obj);
    if (grp.size() != newGrp.size()) {
        Group.setValues (newGrp);
    }
}

void GroupExtension::removeObjectsFromDocument()
{
    const std::vector<DocumentObject*> & grp = Group.getValues();
    // Use set so iterate on each linked object exactly one time (in case of multiple links to the same document)
    std::set<DocumentObject*> grpSet (grp.begin(), grp.end());

    for (std::set<DocumentObject*>::iterator it = grpSet.begin(); it != grpSet.end(); ++it) {
        removeObjectFromDocument(*it);
    }
}

void GroupExtension::removeObjectFromDocument(DocumentObject* obj)
{
    // remove all children
    if (obj->hasExtension(GroupExtension::getExtensionClassTypeId())) {
        GroupExtension *grp = static_cast<GroupExtension*>(obj->getExtension(GroupExtension::getExtensionClassTypeId()));

        // recursive call to remove all subgroups
        grp->removeObjectsFromDocument();
    }

    getExtendedObject()->getDocument()->remObject(obj->getNameInDocument());
}

DocumentObject *GroupExtension::getObject(const char *Name) const
{
    DocumentObject* obj = getExtendedObject()->getDocument()->getObject(Name);
    if (obj && hasObject(obj))
        return obj;
    return 0;
}

bool GroupExtension::hasObject(const DocumentObject* obj, bool recursive) const
{
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj) {
            return true;
        } else if ( recursive && (*it)->hasExtension(GroupExtension::getExtensionClassTypeId()) ) {
            App::GroupExtension *subGroup = static_cast<App::GroupExtension *> (
                                    (*it)->getExtension(GroupExtension::getExtensionClassTypeId()));

            if (subGroup->hasObject (obj, recursive)) {
                return true;
            }
        }
    }

    return false;
}

bool GroupExtension::isChildOf(const GroupExtension* group) const
{
    const std::vector<DocumentObject*>& grp = group->Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == getExtendedObject())
            return true;
        if ((*it)->hasExtension(GroupExtension::getExtensionClassTypeId())) {
            if (this->isChildOf(static_cast<GroupExtension*>((*it)->getExtension(GroupExtension::getExtensionClassTypeId()))))

                return true;
        }
    }

    return false;
}

std::vector<DocumentObject*> GroupExtension::getObjects() const
{
    return Group.getValues();
}

std::vector<DocumentObject*> GroupExtension::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> type;
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type.push_back(*it);
    }

    return type;
}

int GroupExtension::countObjectsOfType(const Base::Type& typeId) const
{
    int type=0;
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type++;
    }

    return type;
}

DocumentObject* GroupExtension::getGroupOfObject(const DocumentObject* obj)
{
    const Document* doc = obj->getDocument();
    std::vector<DocumentObject*> grps = doc->getObjectsWithExtension(GroupExtension::getExtensionClassTypeId());
    for (std::vector<DocumentObject*>::const_iterator it = grps.begin(); it != grps.end(); ++it) {
        GroupExtension* grp = (*it)->getExtensionByType<GroupExtension>();
        if (grp->hasObject(obj))
            return *it;
    }

    return 0;
}

PyObject* GroupExtension::getExtensionPyObject(void) {
    
    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        auto grp = new GroupExtensionPy(this);
        ExtensionPythonObject = Py::Object(grp,true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}


namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GroupExtensionPython, App::GroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GroupExtension>>;
}
