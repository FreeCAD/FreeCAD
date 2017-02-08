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

void GeoFeatureGroupExtension::addObject(App::DocumentObject* obj)  {
    
    if(!allowObject(obj))
        return;
    
    //only one geofeaturegroup per object. This is the reason why we need to override addObject, 
    //we need to check here for GeoFeatureGroups only. It is allowed to be at the same time in a 
    //GeoFeatureGroup and a Group
    auto *group = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
    if(group && group != getExtendedObject())
        group->getExtensionByType<App::GroupExtension>()->removeObject(obj);
    
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Group.getValues();
        grp.push_back(obj);
        Group.setValues(grp);
    }
}


// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::GeoFeatureGroupExtensionPython, App::GeoFeatureGroupExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>>;
}
