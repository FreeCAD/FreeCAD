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
#include "GeoFeatureGroupExtension.h"
#include <Base/Console.h>

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
        getExtendedObject()->getDocument()->removeObject(obj->getNameInDocument());
        return nullptr;
    }
    addObject(obj);
    return obj;
}

std::vector<DocumentObject*> GroupExtension::addObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> vec = {obj};
    return addObjects(vec);
}

std::vector< DocumentObject* > GroupExtension::addObjects(std::vector< DocumentObject* > objs) {
    
    std::vector<DocumentObject*> added;
    std::vector<DocumentObject*> grp = Group.getValues();
    for(auto obj : objs) {
            
        if(!allowObject(obj))
            continue;
        
        if (hasObject(obj))
            continue;
            
        //only one group per object. Note that it is allowed to be in a group and geofeaturegroup. However,
        //getGroupOfObject() returns only normal groups, no GeoFeatureGroups. Hence this works.
        auto *group = App::GroupExtension::getGroupOfObject(obj);
        if(group && group != getExtendedObject())
            group->getExtensionByType<App::GroupExtension>()->removeObject(obj);
        
        //if we are in a geofeaturegroup we need to ensure the object is too
        auto geogrp = GeoFeatureGroupExtension::getGroupOfObject(getExtendedObject());
        auto objgrp = GeoFeatureGroupExtension::getGroupOfObject(obj);
        if( geogrp != objgrp ) {
            //what to do depends on if we are in  geofeature group or not
            if(geogrp)
                geogrp->getExtensionByType<GeoFeatureGroupExtension>()->addObject(obj);
            else 
                objgrp->getExtensionByType<GeoFeatureGroupExtension>()->removeObject(obj);
        }
        
        grp.push_back(obj);
        added.push_back(obj);
    }
    
    Group.setValues(grp);
    
    return added;
}

std::vector< DocumentObject* > GroupExtension::setObjects(std::vector< DocumentObject* > obj) {

    Group.setValues(std::vector< DocumentObject* > ());
    return addObjects(obj);
}

std::vector<DocumentObject*> GroupExtension::removeObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> vec = {obj};
    return removeObjects(vec);
}

std::vector< DocumentObject* > GroupExtension::removeObjects(std::vector< DocumentObject* > objs) {

    const std::vector<DocumentObject*> & grp = Group.getValues();
    std::vector<DocumentObject*> newGrp = grp;
    std::vector<DocumentObject*> removed;

    std::vector<DocumentObject*>::iterator end = newGrp.end();
    for(auto obj : objs) {       
       auto res = std::remove(newGrp.begin(), end, obj);
       if(res != end) {
           end = res;
           removed.push_back(obj);
       }
    }
    
    newGrp.erase(end, newGrp.end());
    if (grp.size() != newGrp.size()) {
        Group.setValues (newGrp);
    }
    
    return removed;
}

void GroupExtension::removeObjectsFromDocument()
{
#if 1
    while (Group.getSize() > 0) {
        // Remove the objects step by step because it can happen
        // that an object is part of several groups and thus a
        // double destruction could be possible
        const std::vector<DocumentObject*> & grp = Group.getValues();
        removeObjectFromDocument(grp.front());
    }
#else
    const std::vector<DocumentObject*> & grp = Group.getValues();
    // Use set so iterate on each linked object exactly one time (in case of multiple links to the same document)
    std::set<DocumentObject*> grpSet (grp.begin(), grp.end());

    for (std::set<DocumentObject*>::iterator it = grpSet.begin(); it != grpSet.end(); ++it) {
        removeObjectFromDocument(*it);
    }
#endif
}

void GroupExtension::removeObjectFromDocument(DocumentObject* obj)
{
    // check that object is not invalid
    if (!obj || !obj->getNameInDocument())
        return;

    // remove all children
    if (obj->hasExtension(GroupExtension::getExtensionClassTypeId())) {
        GroupExtension *grp = static_cast<GroupExtension*>(obj->getExtension(GroupExtension::getExtensionClassTypeId()));

        // recursive call to remove all subgroups
        grp->removeObjectsFromDocument();
    }

    getExtendedObject()->getDocument()->removeObject(obj->getNameInDocument());
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

    if(obj == getExtendedObject())
        return false;

    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (auto child : grp) {

        if(!child)
            continue;

        if (child == obj) {
            return true;
        } else if (child == getExtendedObject()) {
            Base::RuntimeError("Cyclic dependencies detected: Search cannot be performed");
        } else if ( recursive && child->hasExtension(GroupExtension::getExtensionClassTypeId()) ) {

            App::GroupExtension *subGroup = static_cast<App::GroupExtension *> (
                                    child->getExtension(GroupExtension::getExtensionClassTypeId()));
            std::vector<const GroupExtension*> history;
            history.push_back(this);

            if (subGroup->recursiveHasObject (obj, subGroup, history)) {
                return true;
            }
        }
    }

    return false;
}

bool GroupExtension::recursiveHasObject(const DocumentObject* obj, const GroupExtension* group, 
                                        std::vector< const GroupExtension* > history) const {

    //the purpose is to prevent infinite recursion when groups form a cyclic graph. To do this 
    //we store every group we processed on the current leave of the tree, and if we reach an 
    //already processed group we know that it not really is a tree but a cycle.
    history.push_back(this);

    //we use hasObject with out recursion to allow override in derived classes
    if(group->hasObject(obj, false))
        return true;

    //we checked for the searched object already with hasObject and did not find it, now we need to
    //do the same for all subgroups
    for (auto child : group->Group.getValues()) {

        if(!child)
            continue;

        if ( child->hasExtension(GroupExtension::getExtensionClassTypeId()) ) {

            auto ext = child->getExtensionByType<GroupExtension>();
            
            if(std::find(history.begin(), history.end(), ext) != history.end())
                Base::RuntimeError("Cyclic dependencies detected: Search cannot be performed");

            if (recursiveHasObject(obj, ext, history)) {
                return true;
            }
        }
    }
    return false;
}


bool GroupExtension::isChildOf(const GroupExtension* group, bool recursive) const
{
    return group->hasObject(getExtendedObject(), recursive);
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
    //note that we return here only Groups, but nothing derived from it, e.g. no GeoFeatureGroups. 
    //That is important as there are clear differences between groups/geofeature groups (e.g. an object
    //can be in only one group, and only one geofeaturegroup, however, it can be in both at the same time)
    auto list = obj->getInList();
    for (auto obj : list) {
        if(obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false))
            return obj;
    }

    return nullptr;
}

PyObject* GroupExtension::getExtensionPyObject(void) {

    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        auto grp = new GroupExtensionPy(this);
        ExtensionPythonObject = Py::Object(grp,true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

void GroupExtension::extensionOnChanged(const Property* p) {

    //objects are only allowed in a single group. Note that this check must only be done for normal
    //groups, not any derived classes
    if((this->getExtensionTypeId() == GroupExtension::getExtensionClassTypeId()) &&
       (strcmp(p->getName(), "Group")==0)) {

        if(!getExtendedObject()->getDocument()->isPerformingTransaction()) {
            
            bool error = false;
            auto corrected = Group.getValues();
            for(auto obj : Group.getValues()) {

                //we have already set the obj into the group, so in a case of multiple groups getGroupOfObject
                //would return anyone of it and hence it is possible that we miss an error. We need a custom check
                auto list = obj->getInList();
                for (auto in : list) {
                    if(in->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false) &&
                        in != getExtendedObject()) {
                        error = true;
                        corrected.erase(std::remove(corrected.begin(), corrected.end(), obj), corrected.end());
                    }
                }
            }

            //if an error was found we need to correct the values and inform the user
            if(error) {
                Group.setValues(corrected);
                throw Base::RuntimeError("Object can only be in a single Group");
            }
        }
    }

    App::Extension::extensionOnChanged(p);
}


namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GroupExtensionPython, App::GroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GroupExtension>>;
}
