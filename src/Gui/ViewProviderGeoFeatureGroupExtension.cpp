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
#include <Inventor/nodes/SoGroup.h>

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGeoFeatureGroupExtension, Gui::ViewProviderGroupExtension)

ViewProviderGeoFeatureGroupExtension::ViewProviderGeoFeatureGroupExtension()
{
    initExtensionType(ViewProviderGeoFeatureGroupExtension::getExtensionClassTypeId());
    
    pcGroupChildren = new SoGroup();
    pcGroupChildren->ref();
}

ViewProviderGeoFeatureGroupExtension::~ViewProviderGeoFeatureGroupExtension()
{
    pcGroupChildren->unref();
    pcGroupChildren = 0;
}


std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroupExtension::extensionClaimChildren3D(void) const {
    auto* ext = getExtendedViewProvider()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();
    return ext ? ext->getGeoSubObjects() : std::vector<App::DocumentObject*>();
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
    } else {
        ViewProviderGroupExtension::extensionUpdateData ( prop );
    }
}

std::vector< App::DocumentObject* > ViewProviderGeoFeatureGroupExtension::getLinkedObjects(App::DocumentObject* obj) {

    if(!obj)
        return std::vector< App::DocumentObject* >();

    //we get all linked objects, and that recursively
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

    //clear all null objects
    result.erase(std::remove(result.begin(), result.end(), nullptr), result.end());

    //collect all dependencies of those objects
    std::vector< App::DocumentObject* > links;
    for(App::DocumentObject *obj : result) {
        auto vec = getLinkedObjects(obj);
        links.insert(links.end(), vec.begin(), vec.end());
    }

    if (!links.empty()) {
        result.insert(result.end(), links.begin(), links.end());
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }

    return result;
}

void ViewProviderGeoFeatureGroupExtension::extensionDropObject(App::DocumentObject* obj) {
    
    // Open command
    App::DocumentObject* grp = static_cast<App::DocumentObject*>(getExtendedViewProvider()->getObject());
    App::Document* doc = grp->getDocument();
    Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
    gui->openCommand("Move object");
    
    //links between different CS are not allowed, hence we need to ensure if all dependencies are in 
    //the same geofeaturegroup
    auto vec = getLinkedObjects(obj);
    
    //remove all objects already in the correct group
    vec.erase(std::remove_if(vec.begin(), vec.end(), [this](App::DocumentObject* o){    
        return App::GroupExtension::getGroupOfObject(o) == this->getExtendedViewProvider()->getObject();
    }), vec.end());

    vec.push_back(obj);
    
    for(App::DocumentObject* o : vec) {       
        // build Python command for execution
        QString cmd;
        cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").addObject("
                            "App.getDocument(\"%1\").getObject(\"%3\"))")
                            .arg(QString::fromLatin1(doc->getName()))
                            .arg(QString::fromLatin1(grp->getNameInDocument()))
                            .arg(QString::fromLatin1(o->getNameInDocument()));

        Gui::Command::doCommand(Gui::Command::App, cmd.toUtf8());
    }
    gui->commitCommand();
}


void ViewProviderGeoFeatureGroupExtension::extensionDragObject(App::DocumentObject* obj) {
    //links between different coordinate systems are not allowed, hence draging one object also needs
    //to drag out all dependend objects
    auto vec = getLinkedObjects(obj);
       
    //add this object
    vec.push_back(obj);
    
    for(App::DocumentObject* obj : vec)
        ViewProviderGroupExtension::extensionDragObject(obj);
}



namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGeoFeatureGroupExtensionPython, Gui::ViewProviderGeoFeatureGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGeoFeatureGroupExtension>;
}
