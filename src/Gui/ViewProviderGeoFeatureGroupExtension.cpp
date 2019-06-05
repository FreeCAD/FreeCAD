/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include "ViewProviderGeoFeatureGroupExtension.h"
#include "Command.h"
#include "Application.h"
#include "Document.h"
#include <App/GeoFeatureGroupExtension.h>
#include "SoFCUnifiedSelection.h"

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGeoFeatureGroupExtension, Gui::ViewProviderGroupExtension)

ViewProviderGeoFeatureGroupExtension::ViewProviderGeoFeatureGroupExtension()
{
    initExtensionType(ViewProviderGeoFeatureGroupExtension::getExtensionClassTypeId());

    pcGroupChildren = new SoFCSelectionRoot;
    pcGroupChildren->ref();
}

ViewProviderGeoFeatureGroupExtension::~ViewProviderGeoFeatureGroupExtension()
{
    pcGroupChildren->unref();
    pcGroupChildren = 0;
}


std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroupExtension::extensionClaimChildren3D(void) const {

    //all object in the group must be claimed in 3D, as we are a coordinate system for all of them
    auto* ext = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    if (ext) {
        auto objs = ext->Group.getValues();
        return objs;
    }
    return std::vector<App::DocumentObject*>();
}

std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroupExtension::extensionClaimChildren(void) const {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    const std::vector<App::DocumentObject*> &model = group->Group.getValues ();
    std::set<App::DocumentObject*> outSet; //< set of objects not to claim (childrens of childrens)

    // search for objects handled (claimed) by the features
    for (auto obj: model) {
        //stuff in another geofeaturegroup is not in the model anyway
        if (!obj || obj->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) { continue; }
        
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider ( obj );
        if (!vp || vp == getExtendedViewProvider()) { continue; }

        auto children = vp->claimChildren();
        std::remove_copy ( children.begin (), children.end (), std::inserter (outSet, outSet.begin () ), nullptr);
    }

    // remove the otherwise handled objects, preserving their order so the order in the TreeWidget is correct
    std::vector<App::DocumentObject*> Result;
    for(auto obj : model) {
        if(!obj || !obj->getNameInDocument())
            continue;
        if(outSet.count(obj)) 
            obj->setStatus(App::ObjectStatus::GeoExcluded,true);
        else {
            obj->setStatus(App::ObjectStatus::GeoExcluded,false);
            Result.push_back(obj);
        }
    }
    return Result;
}

void ViewProviderGeoFeatureGroupExtension::extensionAttach(App::DocumentObject* pcObject)
{
    ViewProviderGroupExtension::extensionAttach(pcObject);
    getExtendedViewProvider()->addDisplayMaskMode(pcGroupChildren, "Group");
}

void ViewProviderGeoFeatureGroupExtension::extensionSetDisplayMode(const char* ModeName)
{
    if ( strcmp("Group",ModeName)==0 )
        getExtendedViewProvider()->setDisplayMaskMode("Group");

    ViewProviderGroupExtension::extensionSetDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderGeoFeatureGroupExtension::extensionGetDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGroupExtension::extensionGetDisplayModes();

    // add your own modes
    StrList.push_back("Group");

    return StrList;
}

void ViewProviderGeoFeatureGroupExtension::extensionUpdateData(const App::Property* prop)
{
    auto obj = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    if (obj && prop == &obj->placement()) {
        getExtendedViewProvider()->setTransformation ( obj->placement().getValue().toMatrix() );
    }
    else {
        ViewProviderGroupExtension::extensionUpdateData ( prop );
    }
}

namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGeoFeatureGroupExtensionPython, Gui::ViewProviderGeoFeatureGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGeoFeatureGroupExtension>;
}
