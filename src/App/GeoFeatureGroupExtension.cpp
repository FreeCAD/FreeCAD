/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2014    *
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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

#include "GeoFeatureGroupExtension.h"
#include "OriginFeature.h"
#include "Origin.h"
#include "OriginGroupExtension.h"
#include <Base/Console.h>
//#include "GeoFeatureGroupPy.h"
//#include "FeaturePythonPyImp.h"

using namespace App;


EXTENSION_PROPERTY_SOURCE(App::GeoFeatureGroupExtension, App::GroupExtension)


//===========================================================================
// Feature
//===========================================================================

GeoFeatureGroupExtension::GeoFeatureGroupExtension(void)
{
    initExtensionType(GeoFeatureGroupExtension::getExtensionClassTypeId());
    Group.setScope(LinkScope::Child);
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
        if(inObj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()) &&
           inObj->getExtensionByType<GeoFeatureGroupExtension>()->hasObject(obj)) {            

            return inObj;
        }
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
        if(link->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())){
            if (link->getExtensionByType<GeoFeatureGroupExtension>()->hasObject(group->getExtendedObject()))
                return recursiveGroupPlacement(link->getExtensionByType<GeoFeatureGroupExtension>()) * group->placement().getValue();
        }
    }
    
    return group->placement().getValue();
}

std::vector<DocumentObject*> GeoFeatureGroupExtension::addObjects(std::vector<App::DocumentObject*> objects)  {
    
    std::vector<DocumentObject*> grp = Group.getValues();
    std::vector<DocumentObject*> ret;
    
    for(auto object : objects) {
        
        if(!allowObject(object))
            continue;
        
        //cross CoordinateSystem links are not allowed, so we need to move the whole link group 
        std::vector<App::DocumentObject*> links = getCSRelevantLinks(object);
        links.push_back(object);
        
        for( auto obj : links) {
            //only one geofeaturegroup per object. 
            auto *group = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
            if(group && group != getExtendedObject())
                group->getExtensionByType<App::GroupExtension>()->removeObject(obj);
            
            if (!hasObject(obj)) {
                grp.push_back(obj);
                ret.push_back(obj);
            }
        }
    }
    
    Group.setValues(grp);
    return ret;
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
    
    if(!removed.empty())
        Group.setValues(grp);
    
    return removed;
}

void GeoFeatureGroupExtension::extensionOnChanged(const Property* p) {

    //objects are only allowed in a single GeoFeatureGroup
    if((strcmp(p->getName(), "Group")==0)) {
    
        if(!getExtendedObject()->getDocument()->isPerformingTransaction()) {
                
            bool error = false;
            auto corrected = Group.getValues();
            for(auto obj : Group.getValues()) {

                //we have already set the obj into the group, so in a case of multiple groups getGroupOfObject
                //would return anyone of it and hence it is possible that we miss an error. We need a custom check
                auto list = obj->getInList();
                for (auto in : list) {
                    if(in->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()) && //is GeoFeatureGroup?
                    in != getExtendedObject() &&                                                  //is a different one?
                    in->getExtensionByType<App::GeoFeatureGroupExtension>()->hasObject(obj)) {    //is not a non-grouping link?
                        error = true;
                        corrected.erase(std::remove(corrected.begin(), corrected.end(), obj), corrected.end());
                    }
                }
            }

            //if an error was found we need to correct the values and inform the user
            if(error) {
                Group.setValues(corrected);
                throw Base::RuntimeError("Object can only be in a single GeoFeatureGroup");
            }
        }
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

    if(prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId()) && 
        static_cast<App::PropertyLink*>(prop)->getScope() == scope) {

        result.push_back(static_cast<App::PropertyLink*>(prop)->getValue());
    }

    if(prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId()) &&
            static_cast<App::PropertyLinkList*>(prop)->getScope() == scope) {

        auto vec = static_cast<App::PropertyLinkList*>(prop)->getValues();
        result.insert(result.end(), vec.begin(), vec.end());
    }

    if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSub::getClassTypeId()) &&
            static_cast<App::PropertyLinkSub*>(prop)->getScope() == scope) {

        result.push_back(static_cast<App::PropertyLinkSub*>(prop)->getValue());
    }

    if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSubList::getClassTypeId()) &&
            static_cast<App::PropertyLinkSubList*>(prop)->getScope() == scope) {

        auto vec = static_cast<App::PropertyLinkSubList*>(prop)->getValues();
        result.insert(result.end(), vec.begin(), vec.end());
    }

    //it is important to remove all nullptrs
    result.erase(std::remove(result.begin(), result.end(), nullptr), result.end());
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

    if(!obj)
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
        if(!o || o == obj ||  std::find(vec.begin(), vec.end(), o) != vec.end())
            continue;

        vec.push_back(o);
        recursiveCSRelevantLinks(o, vec);
    }
}


bool GeoFeatureGroupExtension::areLinksValid(const DocumentObject* obj) {

    if(!obj)
        return true;

    //no cross CS link for local links.
    //Base::Console().Message("Check object links: %s\n", obj->getNameInDocument());
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        if(!isLinkValid(prop)) {
            //Base::Console().Message("Invalid link: %s\n", prop->getName());
            return false;
        }
    }

    return true;
}

bool GeoFeatureGroupExtension::isLinkValid(App::Property* prop) {

    if(!prop)
        return true;

    //get the object that holds the property
    if(!prop->getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId()))
        return true; //this link comes not from a document object, scopes are meaningless
    auto obj = static_cast<App::DocumentObject*>(prop->getContainer());

    //no cross CS link for local links.
    auto result = getScopedObjectsFromLink(prop, LinkScope::Local);
    auto group = getGroupOfObject(obj);
    for(auto link : result) {
        if(getGroupOfObject(link) != group) 
            return false;
    }

    //for links with scope SubGroup we need to check if all features are part of subgroups
    if(obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
        result = getScopedObjectsFromLink(prop, LinkScope::Child);
        auto groupExt = obj->getExtensionByType<App::GeoFeatureGroupExtension>();
        for(auto link : result) {
            if(!groupExt->hasObject(link, true)) 
                return false;
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


// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GeoFeatureGroupExtensionPython, App::GeoFeatureGroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>>;
}
