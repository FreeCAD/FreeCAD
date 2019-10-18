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
#include <Base/Tools.h>

FC_LOG_LEVEL_INIT("App",true,true);

using namespace App;

EXTENSION_PROPERTY_SOURCE(App::GroupExtension, App::DocumentObjectExtension)

GroupExtension::GroupExtension()
{
    initExtensionType(GroupExtension::getExtensionClassTypeId());
    
    EXTENSION_ADD_PROPERTY_TYPE(Group,(0),"Base",(App::PropertyType)(Prop_None),"List of referenced objects");

    EXTENSION_ADD_PROPERTY_TYPE(_GroupTouched, (false), "Base", 
            PropertyType(Prop_Hidden|Prop_Transient),0);

    static const char *ExportModeEnum[] = {"Disabled", "By Visibility", "Child Query", "Both", 0};
    ExportMode.setEnums(ExportModeEnum);
    ExportMode.setStatus(Property::Hidden,true);
    EXTENSION_ADD_PROPERTY_TYPE(ExportMode,(EXPORT_DISABLED),"Base",(App::PropertyType)(Prop_None),
            "Disabled: do not export any child.\n\n"
            "By Visibility: export all children with their current visibility. Note, depending\n"
            "on your exporter setting, invisible object may or may not be exported.\n\n"
            "Child Query: export only children whose property 'Enable Export' is set to True.\n"
            "and export them as visible object regardless of their actual visibility.\n\n"
            "Both: same as 'Child Query' but with their current visibility.");

}

GroupExtension::~GroupExtension()
{
}

void GroupExtension::checkParentGroup() {
    _checkParentGroup = true;
}

bool GroupExtension::queryChildExport(App::DocumentObject *obj) const {
    if(!obj || !obj->getNameInDocument())
        return false;
    switch(ExportMode.getValue()) {
    case EXPORT_DISABLED:
        return false;
    case EXPORT_BY_VISIBILITY:
        return true;
    default:
        break;
    }
    auto prop = obj->getPropertyByName("Group_EnableExport");
    if(prop && prop->getContainer()==obj) {
        if(!prop->isDerivedFrom(PropertyBool::getClassTypeId())) {
            if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Unexpected property type of " <<  prop->getFullName());
            return true;
        }
        return static_cast<PropertyBool*>(prop)->getValue();
    }
    prop = obj->addDynamicProperty("App::PropertyBool","Group_EnableExport","Group");
    static_cast<App::PropertyBool*>(prop)->setValue(true);
    return true;
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
    
    auto owner = getExtendedObject();
    auto inSet = owner->getInListEx(true);
    inSet.insert(owner);

    std::vector<DocumentObject*> added;
    std::vector<DocumentObject*> grp = Group.getValues();
    for(auto obj : objs) {
            
        if(inSet.count(obj) || !allowObject(obj) || hasObject(obj))
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
    
    Base::ObjectStatusLocker<Property::Status, Property> guard(Property::User3, &Group);
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
        Base::ObjectStatusLocker<Property::Status, Property> guard(Property::User3, &Group);
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

const std::vector<DocumentObject*> &GroupExtension::getObjects() const
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
    for (auto o : obj->getInList()) {
        if(o->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false))
            return o;
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

    auto owner = getExtendedObject();

    //objects are only allowed in a single group. Note that this check must only be done for normal
    //groups, not any derived classes
    if(p == &Group && !Group.testStatus(Property::User3)) {

        if(!owner->isRestoring() && !owner->getDocument()->isPerformingTransaction()) {

            std::unordered_set<App::DocumentObject*> objSet;

            int error = Group.removeIf( [&](App::DocumentObject *obj) {
                if(!obj || !obj->getNameInDocument() || !objSet.insert(obj).second)
                    return true;

                //we have already set the obj into the group, so in a case of multiple groups getGroupOfObject
                //would return anyone of it and hence it is possible that we miss an error. We need a custom check
                for(auto in : obj->getInList()) {
                    if(in!=owner && in->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false)) {
                        FC_WARN("Remove " << obj->getFullName() <<  " from " 
                                << owner->getFullName() << " because of multiple owner groups");
                        return true;
                    }
                }
                return false;
            });

            if(error) {
                // Since we are auto correcting, just issue a warning
                //
                // throw Base::RuntimeError("Object can only be in a single Group");
                FC_WARN("Auto correct group member for " << owner->getFullName());
            }
        }
    }

    if(p == &getExportGroupProperty() || p == &ExportMode) {
        _Conns.clear();
        for(auto obj : getExportGroupProperty().getValues()) {
            if(!obj || !obj->getNameInDocument())
                continue;
            queryChildExport(obj);
            _Conns.push_back(obj->Visibility.signalChanged.connect(boost::bind(
                            &GroupExtension::slotChildChanged,this,_1)));
        }
    } else if(p == &owner->Visibility) {
        if(!_togglingVisibility 
                && !owner->isRestoring() 
                && !owner->getDocument()->isPerformingTransaction())
        {
            bool touched = false;
            bool vis = owner->Visibility.getValue();

            // _checkParentGroup is used by GeoFeatureExtensionGroup (actually
            // its view provider) to inform us that we should not toggle
            // children visibility here.
            _checkParentGroup = _checkParentGroup 
                && GeoFeatureGroupExtension::getGroupOfObject(owner);

            auto hiddenChildren = Base::freecad_dynamic_cast<PropertyMap>(
                    owner->getPropertyByName("HiddenChildren"));
            if(hiddenChildren && hiddenChildren->getContainer()!=owner)
                hiddenChildren = 0;

            Base::FlagToggler<> guard(_togglingVisibility);

            std::map<std::string,std::string> hc;

            for(auto obj : Group.getValues()) {
                if(!obj || !obj->getNameInDocument())
                    continue;
                if(obj->Visibility.getValue()!=vis) {
                    if(vis && hiddenChildren && hiddenChildren->getValue(obj->getNameInDocument()))
                        continue;
                    if(!_checkParentGroup) {
                        touched = true;
                        obj->Visibility.setValue(vis);
                    }
                } else if (!vis && hiddenChildren)
                    hc.emplace(obj->getNameInDocument(),"");
            }

            if(!vis && hiddenChildren) {
                // If all children are invisible, do not keep the record, so
                // that next time this group is shown, all its children will be
                // shown.
                if(hc.size() == Group.getValues().size())
                    hc.clear();
                hiddenChildren->setValues(std::move(hc));
            }

            if(touched) {
                if(_GroupTouched.testStatus(Property::Output))
                    _GroupTouched.touch();
                else {
                    Base::ObjectStatusLocker<Property::Status, Property> guard(Property::Output, &_GroupTouched);
                    // Temporary set the Property::Output on _GroupTouched, so that
                    // it does not touch the owner object, but still signal
                    // Part::Feature shape cache update.
                    _GroupTouched.touch();
                }
            }
        }
    }

    App::Extension::extensionOnChanged(p);
}

void GroupExtension::slotChildChanged(const Property &prop) {
    auto obj = static_cast<DocumentObject*>(prop.getContainer());
    if(obj && !_togglingVisibility 
           && !obj->isRestoring() 
           && !obj->getDocument()->isPerformingTransaction())
    {
        if(ExportMode.getValue() == EXPORT_BY_VISIBILITY
                || _GroupTouched.testStatus(Property::Output))
        {
            _GroupTouched.touch();
        } else {
            Base::ObjectStatusLocker<Property::Status, Property> guard(Property::Output, &_GroupTouched);
            // Temporary set the Property::Output on _GroupTouched, so that it
            // does not touch the owner object, but can still inform of
            // children visibility changes
            _GroupTouched.touch();
        }

        auto owner = getExtendedObject();
        if(!owner->Visibility.getValue()) {
            auto hiddenChildren = Base::freecad_dynamic_cast<PropertyMap>(owner->getPropertyByName("HiddenChildren"));
            if(hiddenChildren && hiddenChildren->getContainer()==owner) {
                std::map<std::string,std::string> hc;
                for(auto obj : Group.getValues()) {
                    if(!obj || !obj->getNameInDocument())
                        continue;
                    if(!obj->Visibility.getValue()) 
                        hc.emplace(obj->getNameInDocument(),"");
                }
                if(hc.size() == Group.getValues().size())
                    hc.clear();
                hiddenChildren->setValues(std::move(hc));
            }
        }
    }
}

bool GroupExtension::extensionGetSubObject(DocumentObject *&ret, const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool /*transform*/, int depth) const 
{
    const char *dot;
    if(!subname || *subname==0) {
        auto obj = Base::freecad_dynamic_cast<const DocumentObject>(getExtendedContainer());
        ret = const_cast<DocumentObject*>(obj);
        return true;
    }
    dot=strchr(subname,'.');
    if(!dot)
        return false;
    if(subname[0]!='$')
        ret = Group.find(std::string(subname,dot));
    else{
        std::string name = std::string(subname+1,dot);
        for(auto child : Group.getValues()) {
            if(name == child->Label.getStrValue()){
                ret = child;
                break;
            }
        }
    }
    if(!ret) 
        return false;
    return ret->getSubObject(dot+1,pyObj,mat,true,depth+1);
}

bool GroupExtension::extensionGetSubObjects(std::vector<std::string> &ret, int reason) const {
    if(reason == DocumentObject::GS_DEFAULT && ExportMode.getValue() == EXPORT_DISABLED)
        return true;

    for(auto obj : getExportGroupProperty().getValues()) {
        if(obj && obj->getNameInDocument()) {
            if(reason!=DocumentObject::GS_DEFAULT || queryChildExport(obj))
                ret.push_back(std::string(obj->getNameInDocument())+'.');
        }
    }
    return true;
}

int GroupExtension::extensionIsElementVisible(const char *) const {
    if(ExportMode.getValue() == EXPORT_BY_CHILD_QUERY)
        return 1;
    return -1;
}

App::DocumentObjectExecReturn *GroupExtension::extensionExecute(void) {
    // This touch property is for propagating changes to upper group
    _GroupTouched.touch();
    return inherited::extensionExecute();
}

std::vector<App::DocumentObject*> GroupExtension::getAllChildren() const {
    std::vector<DocumentObject*> res;
    std::set<DocumentObject*> rset;
    getAllChildren(res,rset);
    return res;
}

void GroupExtension::getAllChildren(std::vector<App::DocumentObject*> &res,
        std::set<App::DocumentObject*> &rset) const
{
    for(auto obj : Group.getValues()) {
        if(!obj || !obj->getNameInDocument())
            continue;
        if(!rset.insert(obj).second)
            continue;
        res.push_back(obj);
        auto ext = obj->getExtensionByType<GroupExtension>(true,false);
        if(ext) 
            ext->getAllChildren(res,rset);
    }
}

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GroupExtensionPython, App::GroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GroupExtension>>;
}
