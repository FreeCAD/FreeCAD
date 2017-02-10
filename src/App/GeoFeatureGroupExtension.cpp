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
    //compared to GroupExtension we do return here all geofeaturegroups including all extensions erived from it
    //like origingroup. That is needed as we use this function to get all local coordinate systems. Also there 
    //is no reason to distuinguish between geofeatuergroups, there is only between group/geofeaturegroup
    auto list = obj->getInList();
    for (auto obj : list) {
        if(obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
            return obj;
    }

    return nullptr;
}

Base::Placement GeoFeatureGroupExtension::globalGroupPlacement() {
    
    return recursiveGroupPlacement(this);
}


Base::Placement GeoFeatureGroupExtension::recursiveGroupPlacement(GeoFeatureGroupExtension* group) {

    
    auto inList = group->getExtendedObject()->getInList();
    for(auto* link : inList) {
        if(link->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
            return recursiveGroupPlacement(link->getExtensionByType<GeoFeatureGroupExtension>()) * group->placement().getValue();
    }
    
    return group->placement().getValue();
}

std::vector<DocumentObject*> GeoFeatureGroupExtension::addObject(App::DocumentObject* object)  {
    
    if(!allowObject(object))
        return std::vector<DocumentObject*>();
    
    //cross CoordinateSystem links are not allowed, so we need to move the whole link group 
    auto links = getCSRelevantLinks(object);
    links.push_back(object);
    
    auto ret = links;
    std::vector<DocumentObject*> grp = Group.getValues();
    for( auto obj : links) {
        //only one geofeaturegroup per object. 
        auto *group = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
        if(group && group != getExtendedObject())
            group->getExtensionByType<App::GroupExtension>()->removeObject(obj);
        
        if (!hasObject(obj))
            grp.push_back(obj);
        else 
            ret.erase(std::remove(ret.begin(), ret.end(), obj), ret.end());
    }
    
    Group.setValues(grp);
    return ret;
}


std::vector<DocumentObject*> GeoFeatureGroupExtension::removeObject(App::DocumentObject* object)  {
       
    //cross CoordinateSystem links are not allowed, so we need to remove the whole link group 
    auto links = getCSRelevantLinks(object);
    links.push_back(object);
    
    //remove all links out of group
    std::vector<DocumentObject*> grp = Group.getValues();
    for(auto link : links)
        grp.erase(std::remove(grp.begin(), grp.end(), link), grp.end());
    
    Group.setValues(grp);
    return links;
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getObjectsFromLinks(DocumentObject* obj) {

    //we get all linked objects. We can't use outList() as this includes the links from expressions
    std::vector< App::DocumentObject* > result;
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        if(prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId()))
            result.push_back(static_cast<App::PropertyLink*>(prop)->getValue());
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
            auto vec = static_cast<App::PropertyLinkList*>(prop)->getValues();
            result.insert(result.end(), vec.begin(), vec.end());
        }
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSub::getClassTypeId()))
            result.push_back(static_cast<App::PropertyLinkSub*>(prop)->getValue());
        else if(prop->getTypeId().isDerivedFrom(App::PropertyLinkSubList::getClassTypeId())) {
            auto vec = static_cast<App::PropertyLinkList*>(prop)->getValues();
            result.insert(result.end(), vec.begin(), vec.end());
        }
    }

    //clear all null objects and douplicates
    result.erase(std::remove(result.begin(), result.end(), nullptr), result.end());
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    
    return result;
}


std::vector< DocumentObject* > GeoFeatureGroupExtension::getCSOutList(App::DocumentObject* obj) {

    if(!obj)
        return std::vector< App::DocumentObject* >();

    //if the object is a geofeaturegroup than all dependencies belong to that CS,  we don't want them
    if(obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
        return std::vector< App::DocumentObject* >();
    
    //we get all linked objects. We can't use outList() as this includes the links from expressions
    auto result = getObjectsFromLinks(obj);
    
    //we remove all links to origin features and origins, they belong to a CS too and can't be moved
    result.erase(std::remove_if(result.begin(), result.end(), [](App::DocumentObject* obj)->bool {
        return (obj->isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
                obj->isDerivedFrom(App::Origin::getClassTypeId()));
    }), result.end());

    //collect all dependencies of those objects
    std::vector< App::DocumentObject* > links;
    for(App::DocumentObject *obj : result) { 
        auto vec = getCSOutList(obj);
        links.insert(links.end(), vec.begin(), vec.end());
    }

    if (!links.empty()) {
        result.insert(result.end(), links.begin(), links.end());
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }

    return result;
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getCSInList(DocumentObject* obj) {

    if(!obj)
        return std::vector< App::DocumentObject* >();
    
    //we get all objects that link to it
    std::vector< App::DocumentObject* > result;
    
    //search the inlist for objects that have non-expression links to us
    for(App::DocumentObject* parent : obj->getInList()) {
                
        //not interested in other groups (and here we mean all groups, normal ones and geofeaturegroup)
        if(parent->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
            continue;
        
        //check if the link is real or if it is a expression one (could also be both, so it is not 
        //enough to check the expressions)
        auto res = getObjectsFromLinks(parent);
        if(std::find(res.begin(), res.end(), obj) != res.end())
            result.push_back(parent);
    }
    
    //clear all douplicates
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    //collect all links to those objects
    std::vector< App::DocumentObject* > links;
    for(App::DocumentObject *obj : result) { 
        auto vec = getCSInList(obj);
        links.insert(links.end(), vec.begin(), vec.end());
    }

    if (!links.empty()) {
        result.insert(result.end(), links.begin(), links.end());
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }

    return result;
}

std::vector< DocumentObject* > GeoFeatureGroupExtension::getCSRelevantLinks(DocumentObject* obj) {

    //we need to get the outlist of all inlist objects and ourself. This is needed to handle things
    //like Booleans: the boolean is our parent, than there is a second object under it which relates 
    //to obj and needs to be handled.
    auto in = getCSInList(obj);
    in.push_back(obj); //there may be nothing in inlist
    std::vector<App::DocumentObject*> result;
    for(auto o : in) {
        
        auto out = getCSOutList(o);
        result.insert(result.end(), out.begin(), out.end());
    }
    
    //there will be many douplicates and also the passed obj in
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    result.erase(std::remove(result.begin(), result.end(), obj), result.end());
    
    return result;
}

// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GeoFeatureGroupExtensionPython, App::GeoFeatureGroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>>;
}
