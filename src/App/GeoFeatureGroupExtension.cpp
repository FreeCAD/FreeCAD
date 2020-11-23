/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <App/Document.h>

#include "Link.h"
#include "GeoFeatureGroupExtension.h"
#include "OriginFeature.h"
#include "Origin.h"
#include "OriginGroupExtension.h"
#include <Base/Console.h>
#include <Base/Tools.h>
//#include "GeoFeatureGroupPy.h"
//#include "FeaturePythonPyImp.h"

FC_LOG_LEVEL_INIT("App",true,true);

using namespace App;


EXTENSION_PROPERTY_SOURCE(App::GeoFeatureGroupExtension, App::GroupExtension)


//===========================================================================
// Feature
//===========================================================================

GeoFeatureGroupExtension::GeoFeatureGroupExtension(void)
{
    initExtensionType(GeoFeatureGroupExtension::getExtensionClassTypeId());
    Group.setScope(LinkScope::Child);

    _ExportChildren.setScope(LinkScope::Hidden);
    _ExportChildren.setStatus(Property::NoModify,true);
    EXTENSION_ADD_PROPERTY_TYPE(_ExportChildren,(0),"Base",
            (App::PropertyType)(Prop_Output|Prop_Hidden|Prop_ReadOnly),"");
}

GeoFeatureGroupExtension::~GeoFeatureGroupExtension(void)
{
}

void GeoFeatureGroupExtension::initExtension(ExtensionContainer* obj) {
    
    if(!obj->isDerivedFrom(App::GeoFeature::getClassTypeId()))
        throw Base::RuntimeError("GeoFeatureGroupExtension can only be applied to GeoFeatures");
        
    App::GroupExtension::initExtension(obj);
}

PropertyPlacement& GeoFeatureGroupExtension::placement() {

    if(!getExtendedContainer())
        throw Base::RuntimeError("GeoFeatureGroupExtension was not applied to GeoFeature");
    
    return static_cast<App::GeoFeature*>(getExtendedContainer())->Placement;
}


void GeoFeatureGroupExtension::transformPlacement(const Base::Placement &transform)
{
    // NOTE: Keep in sync with APP::GeoFeature
    Base::Placement plm = this->placement().getValue();
    plm = transform * plm;
    this->placement().setValue(plm);
}

DocumentObject* GeoFeatureGroupExtension::getGroupOfObject(const DocumentObject* obj)
{
    if(!obj)
        return nullptr;
    
    //we will find origins, but not origin features
    if(obj->isDerivedFrom(App::OriginFeature::getClassTypeId())) 
        return OriginGroupExtension::getGroupOfObject(obj);
    
    //compared to GroupExtension we do return here all GeoFeatureGroups including all extensions derived from it
    //like OriginGroup. That is needed as we use this function to get all local coordinate systems. Also there
    //is no reason to distinguish between GeoFeatuerGroups, there is only between group/geofeaturegroup
    auto list = obj->getInList();
    for (auto inObj : list) {
        
        //There is a chance that a derived geofeaturegroup links with a local link and hence is not 
        //the parent group even though it links to the object. We use hasObject as one and only truth 
        //if it has the object within the group
        auto group = inObj->getExtensionByType<GeoFeatureGroupExtension>(true);
        if(group && group->hasObject(obj))
            return inObj;
    }

    return nullptr;
}

Base::Placement GeoFeatureGroupExtension::globalGroupPlacement() {

    if(getExtendedObject()->isRecomputing())
        throw Base::RuntimeError("Global placement cannot be calculated on recompute");

    return recursiveGroupPlacement(this);
}


Base::Placement GeoFeatureGroupExtension::recursiveGroupPlacement(GeoFeatureGroupExtension* group) {

    
    auto inList = group->getExtendedObject()->getInList();
    for(auto* link : inList) {
        auto parent = link->getExtensionByType<GeoFeatureGroupExtension>(true);
        if(parent && parent->hasObject(group->getExtendedObject()))
            return recursiveGroupPlacement(parent) * group->placement().getValue();
    }
    
    return group->placement().getValue();
}

std::vector<DocumentObject*>
GeoFeatureGroupExtension::addObjects(std::vector<App::DocumentObject*> objects)
{
    const auto & grp = Group.getValues();
    std::vector<DocumentObject*> ret;
    ret.reserve(objects.size()*2);

    auto owner = getExtendedObject();

    auto objSet = owner->getInListEx(true);
    objSet.insert(owner);
    objSet.insert(grp.begin(), grp.end());

    for (unsigned i=0; i<objects.size(); ++i) {
        auto object = objects[i];
        if (!objSet.insert(object).second || !allowObject(object))
            continue;
        
        //only one geofeaturegroup per object. 
        auto *group = App::GeoFeatureGroupExtension::getGroupOfObject(object);
        if(group) {
            if (group == owner)
                continue;
            group->getExtensionByType<App::GroupExtension>()->removeObject(object);
        }
        ret.push_back(object);

        //cross CoordinateSystem links are not allowed, so we need to move the whole link group 
        recursiveCSRelevantLinks(object, objects);
    }
    
    if(!ret.empty()) {
        Base::ObjectStatusLocker<Property::Status, Property> guard(Property::User3, &Group);
        std::vector<App::DocumentObject *> objs;
        objs.reserve(grp.size() + ret.size());
        objs.insert(objs.end(), grp.begin(), grp.end());
        objs.insert(objs.end(), ret.begin(), ret.end());
        buildExport(objs);
        Group.setValues(objs);
    }
    return ret;
}

void GeoFeatureGroupExtension::buildExport(
        const std::vector<App::DocumentObject *> &children) 
{
    auto owner = getExtendedObject();

    // NOTE: if there is view provider attached, we rely on
    // ViewProviderGeoFeatureGroupExtension to populate _ExportChildren based
    // on how each child claim their own children. If no view provider is
    // attached (e.g. Gui module is not loaded), we rely on the following logic
    // to include only children that do not appear in the inList of any other
    // child. This logic is similar to ViewProviderGeoFeatureGroupExtension,
    // but not exactly the same.  So there may have some discrepancy depending
    // on whether the Gui module is loaded or not.
    if(owner->testStatus(ObjectStatus::ViewProviderAttached))
        return;

    std::set<App::DocumentObject *> childSet(children.begin(),children.end());
    std::vector<App::DocumentObject *> exportChildren;
    for(auto child : children) {
        bool exclude = false;
        for(auto obj : child->getInList()) {
            if(childSet.count(obj)) {
                exclude = true;
                break;
            }
        }
        if(!exclude)
            exportChildren.push_back(child);
    }
    _ExportChildren.setValue(std::move(exportChildren));
}

std::vector<DocumentObject*> GeoFeatureGroupExtension::removeObjects(std::vector<App::DocumentObject*> objects)  {
    
    std::vector<DocumentObject*> removed;
    std::vector<DocumentObject*> grp = Group.getValues();
    
    for(auto object : objects) {

        //cross CoordinateSystem links are not allowed, so we need to remove the whole link group 
        std::vector< DocumentObject* > links = getCSRelevantLinks(object);
        links.push_back(object);
        
        //remove all links out of group       
        for(auto link : links) {
            auto end = std::remove(grp.begin(), grp.end(), link);
            if(end != grp.end()) {
                grp.erase(end, grp.end());
                removed.push_back(link);
            }
        }
    }
    
    if(!removed.empty()) {
        Base::ObjectStatusLocker<Property::Status, Property> guard(Property::User3, &Group);
        buildExport(grp);
        Group.setValues(grp);
    }
    
    return removed;
}

void GeoFeatureGroupExtension::extensionOnChanged(const Property* p) {
    auto owner = getExtendedObject();

    //objects are only allowed in a single GeoFeatureGroup
    if(p == &Group && !Group.testStatus(Property::User3)) {
    
        if((!owner->isRestoring()
                || owner->getDocument()->testStatus(Document::Importing))
            && !owner->getDocument()->isPerformingTransaction())
        {
            bool error = false;
            auto children = Group.getValues();
            std::unordered_map<App::DocumentObject*, bool> objMap;
            for(auto it=children.begin(),itNext=it;it!=children.end();it=itNext) {
                ++itNext;
                auto obj = *it;
                if(!obj || !obj->getNameInDocument() || !allowObject(obj)) {
                    error = true;
                    itNext = children.erase(it);
                    if(obj)
                        FC_WARN("Remove invalid member " << obj->getFullName() 
                                <<  " from " << owner->getFullName());
                    continue;
                }
                auto res = objMap.insert(std::make_pair(obj,true));
                if(!res.second || !res.first->second) {
                    error = true;
                    itNext = children.erase(it);
                    FC_WARN("Remove duplicated member " << obj->getFullName() 
                            <<  " from " << owner->getFullName());
                    continue;
                }

                bool &valid = res.first->second;

                //we have already set the obj into the group, so in a case of multiple groups getGroupOfObject
                //would return anyone of it and hence it is possible that we miss an error. We need a custom check
                for (auto in : obj->getInList()) {
                    if(in == owner)
                        continue;
                    auto parent = in->getExtensionByType<GeoFeatureGroupExtension>(true);
                    if(parent && parent->hasObject(obj)) {
                        error = true;
                        valid = false;
                        itNext = children.erase(it);
                        FC_WARN("Remove " << obj->getFullName() <<  " from " 
                                << owner->getFullName() << " because of multiple owner groups");
                        break;
                    }
                }
            }

            bool retry;
            do {
                retry = false;
                for(auto it=children.begin();it!=children.end();++it) {
                    auto obj = *it;
                    auto &valid = objMap[obj];
                    for(auto link : getCSRelevantLinks(obj)) {
                        auto iter = objMap.find(link);
                        if(iter == objMap.end() || !iter->second) {
                            FC_WARN("Remove " << obj->getFullName() <<  " from " 
                                    << owner->getFullName() << " because of cross coordinate link " 
                                    << link->getFullName());
                            children.erase(it);
                            valid = false;
                            retry = true;
                            error = true;
                            break;
                        }
                    }
                    if(retry)
                        break;
                }
            }while(retry);

            buildExport(children);

            //if an error was found we need to correct the values and inform the user
            if(error) {
                Base::ObjectStatusLocker<Property::Status, Property> guard(Property::User3, &Group);
                Group.setValues(children);

#if 1
                FC_THROWM(Base::RuntimeError,"Auto correct group member for " << owner->getFullName());
#else
                // Since we are auto correcting, just issue a warning
                FC_WARN("Auto correct group member for " << owner->getFullName());
#endif
            }
        }

        // Skip handling in parent class GroupExtension
        return;
    }

    if(p == &owner->Visibility) {
        // Skip visibility handling in parent class GroupExtension
        return;
    }

    App::GroupExtension::extensionOnChanged(p);
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getScopedObjectsFromLinks(const DocumentObject* obj, LinkScope scope) {

    if(!obj)
        return std::vector< DocumentObject* >();

    //we get all linked objects. We can't use outList() as this includes the links from expressions
    std::vector< App::DocumentObject* > result;
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        auto vec = getScopedObjectsFromLink(prop, scope);
        result.insert(result.end(), vec.begin(), vec.end());
    }

    //clear all null objects and duplicates
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getScopedObjectsFromLink(App::Property* prop, LinkScope scope) {

    if(!prop)
        return std::vector< DocumentObject* >();

    std::vector< App::DocumentObject* > result;
    auto link = Base::freecad_dynamic_cast<PropertyLinkBase>(prop);
    if(link && link->getScope()==scope)
        link->getLinks(result);

    //getLinks() guarantees no nullptrs
    //
    //it is important to remove all nullptrs
    // result.erase(std::remove(result.begin(), result.end(), nullptr), result.end());
    return result;
}

void GeoFeatureGroupExtension::getCSOutList(const App::DocumentObject* obj,
                                            std::vector< DocumentObject* >& vec) {

    if(!obj)
        return;

    //we get all relevant linked objects. We can't use outList() as this includes the links from expressions,
    //also we only want links with scope Local
    auto result = getScopedObjectsFromLinks(obj, LinkScope::Local);
    
    //we remove all links to origin features and origins, they belong to a CS too and can't be moved
    result.erase(std::remove_if(result.begin(), result.end(), [](App::DocumentObject* obj)->bool {
        return (obj->isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
                obj->isDerivedFrom(App::Origin::getClassTypeId()));
    }), result.end());

    vec.insert(vec.end(), result.begin(), result.end());

    //post process the vector
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

void GeoFeatureGroupExtension::getCSInList(const DocumentObject* obj,
                                           std::vector< DocumentObject* >& vec) {

    if(!obj)
        return;

    //search the inlist for objects that have non-expression links to us
    for(App::DocumentObject* parent : obj->getInList()) {

        //not interested in other groups (and here we mean all groups, normal ones and geofeaturegroup)
        if(parent->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
            continue;

        //check if the link is real Local scope one or if it is a expression one (could also be both, so it is not 
        //enough to check the expressions)
        auto res = getScopedObjectsFromLinks(parent, LinkScope::Local);
        if(std::find(res.begin(), res.end(), obj) != res.end())
            vec.push_back(parent);
    }

    //clear all duplicates
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getCSRelevantLinks(const DocumentObject* obj) {

    if(!obj || !obj->getNameInDocument() || obj->testStatus(App::ObjectStatus::Remove))
        return std::vector< DocumentObject* >();

    //get all out links 
    std::vector<DocumentObject*> vec;

    recursiveCSRelevantLinks(obj, vec);

    //post process the list after we added many things
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());

    return vec;
}

void GeoFeatureGroupExtension::recursiveCSRelevantLinks(const DocumentObject* obj, 
                                                        std::vector< DocumentObject* >& vec) {

    if(!obj)
        return;

    std::vector< DocumentObject* > links;
    getCSOutList(obj, links);
    getCSInList(obj, links);

    //go on traversing the graph in all directions!
    for(auto o : links) {   
        if(!o || !o->getNameInDocument()
              || o->testStatus(App::ObjectStatus::Remove)
              || o == obj 
              || std::find(vec.begin(), vec.end(), o) != vec.end())
            continue;

        vec.push_back(o);
        recursiveCSRelevantLinks(o, vec);
    }
}

bool GeoFeatureGroupExtension::extensionGetSubObject(DocumentObject *&ret, const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const 
{
    ret = 0;
    const char *dot;
    if(!subname || *subname==0) {
        auto obj = dynamic_cast<const DocumentObject*>(getExtendedContainer());
        ret = const_cast<DocumentObject*>(obj);
        if(mat && transform) 
            *mat *= const_cast<GeoFeatureGroupExtension*>(this)->placement().getValue().toMatrix();
    }else if((dot=strchr(subname,'.'))) {
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
        if(ret) {
            if(dot) ++dot;
            if(dot && *dot 
                    && !ret->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId())
                    && !ret->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) 
            {
                // Consider this
                // Body
                //  | -- Pad
                //        | -- Sketch
                //
                // Suppose we want to getSubObject(Body,"Pad.Sketch.")
                // Because of the special property of geo feature group, both
                // Pad and Sketch are children of Body, so we can't call
                // getSubObject(Pad,"Sketch.") as this will transform Sketch
                // using Pad's placement.
                //
                const char *next = strchr(dot,'.');
                if(next) {
                    App::DocumentObject *nret=0;
                    extensionGetSubObject(nret,dot,pyObj,mat,transform,depth+1);
                    if(nret) {
                        ret = nret;
                        return true;
                    }
                }
            }
            if(mat && transform) 
                *mat *= const_cast<GeoFeatureGroupExtension*>(this)->placement().getValue().toMatrix();
            ret = ret->getSubObject(dot?dot:"",pyObj,mat,true,depth+1);
        }
    }
    return true;
}

bool GeoFeatureGroupExtension::areLinksValid(const DocumentObject* obj, bool silent) {

    if(!obj)
        return true;

    //no cross CS link for local links.
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        if(!isLinkValid(prop, silent)) {
            return false;
        }
    }

    return true;
}

bool GeoFeatureGroupExtension::isLinkValid(App::Property* prop, bool silent) {

    if(!prop)
        return true;

    //get the object that holds the property
    if(!prop->getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId()))
        return true; //this link comes not from a document object, scopes are meaningless
    auto obj = static_cast<App::DocumentObject*>(prop->getContainer());

    //no cross CS link for local links.
    auto result = getScopedObjectsFromLink(prop, LinkScope::Local);
    if(result.size()) {
        auto group = getGroupOfObject(obj);
        for(auto link : result) {
            auto grp = getGroupOfObject(link);
            if(grp != group) {
                if(!silent) {
                    FC_WARN(prop->getFullName() << "(links to "
                            << link->getFullName() << ") is out of scope:"
                            << (grp?grp->getFullName().c_str():"<global>")
                            << " vs. "
                            << (group?group->getFullName().c_str():"<global>"));
                }
                return false;
            }
        }
    }

    //for links with scope SubGroup we need to check if all features are part of subgroups
    if(obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
        result = getScopedObjectsFromLink(prop, LinkScope::Child);
        auto groupExt = obj->getExtensionByType<App::GeoFeatureGroupExtension>();
        for(auto link : result) {
            if(!groupExt->hasObject(link, true)) {
                if(!silent) {
                    FC_WARN(link->getFullName() << " is out of scope of group "
                            << obj->getFullName());
                }
                return false;
            }
        }
    }

    return true;
}

void GeoFeatureGroupExtension::getInvalidLinkObjects(const DocumentObject* obj, std::vector< DocumentObject* >& vec) {

    if(!obj)
        return;

    //no cross CS link for local links.
    auto result = getScopedObjectsFromLinks(obj, LinkScope::Local);
    auto group = obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()) ? obj : getGroupOfObject(obj);
    for(auto link : result) {
        if(getGroupOfObject(link) != group) 
            vec.push_back(link);
    }

    //for links with scope SubGroup we need to check if all features are part of subgroups
    if(group) {
        result = getScopedObjectsFromLinks(obj, LinkScope::Child);
        auto groupExt = group->getExtensionByType<App::GeoFeatureGroupExtension>();
        for(auto link : result) {
            if(!groupExt->hasObject(link, true)) 
                vec.push_back(link);
        }
    }
}

int GeoFeatureGroupExtension::extensionIsElementVisible(const char *) const {
    return -1;
}

// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GeoFeatureGroupExtensionPython, App::GeoFeatureGroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>>;
}
