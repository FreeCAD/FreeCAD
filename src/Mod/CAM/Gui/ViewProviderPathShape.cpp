/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/CAM/App/FeaturePathShape.h>

#include "ViewProviderPathShape.h"


using namespace Gui;
using namespace PathGui;

PROPERTY_SOURCE(PathGui::ViewProviderPathShape, PathGui::ViewProviderPath)

QIcon ViewProviderPathShape::getIcon() const
{
    return Gui::BitmapFactory().pixmap("CAM_Shape");
}

std::vector<App::DocumentObject*> ViewProviderPathShape::claimChildren() const
{
    return std::vector<App::DocumentObject*>(
            static_cast<Path::FeatureShape*>(getObject())->Sources.getValues());
}

bool ViewProviderPathShape::canDragObjects() const
{
    return true;
}

bool ViewProviderPathShape::canDragObject(App::DocumentObject* obj) const
{
    return obj && obj->isDerivedFrom<Part::Feature>();
}

void ViewProviderPathShape::dragObject(App::DocumentObject* obj)
{
    Path::FeatureShape *feature = static_cast<Path::FeatureShape*>(getObject());
    std::vector<App::DocumentObject*> sources = feature->Sources.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it) {
        if (*it == obj) {
            sources.erase(it);
            feature->Sources.setValues(sources);
            break;
        }
    }
}

bool ViewProviderPathShape::canDropObjects() const
{
    return true;
}

bool ViewProviderPathShape::canDropObject(App::DocumentObject* obj) const
{
    return canDragObject(obj);
}

void ViewProviderPathShape::dropObject(App::DocumentObject* obj)
{
    Path::FeatureShape *feature = static_cast<Path::FeatureShape*>(getObject());
    std::vector<App::DocumentObject*> sources = feature->Sources.getValues();
    sources.push_back(obj);
    feature->Sources.setValues(sources);
}

void ViewProviderPathShape::updateData(const App::Property* prop)
{
    PathGui::ViewProviderPath::updateData(prop);
    if (prop->isDerivedFrom<App::PropertyLinkList>()) {
        std::vector<App::DocumentObject*> pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
            if (*it)
                Gui::Application::Instance->hideViewProvider(*it);
        }
    }
}

bool ViewProviderPathShape::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Path::FeatureShape *feature = static_cast<Path::FeatureShape*>(getObject());
    std::vector<App::DocumentObject*> pShapes =feature->Sources.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it)
            Gui::Application::Instance->showViewProvider(*it);
    }
    return true;
}
