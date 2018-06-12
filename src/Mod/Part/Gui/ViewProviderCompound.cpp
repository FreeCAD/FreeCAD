/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <Gui/Application.h>
#include <Mod/Part/App/FeatureCompound.h>
#include "ViewProviderCompound.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderCompound,PartGui::ViewProviderDerivedPart)

ViewProviderCompound::ViewProviderCompound()
{
}

ViewProviderCompound::~ViewProviderCompound()
{
}

bool ViewProviderCompound::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::Compound* pComp = static_cast<Part::Compound*>(getObject());
    std::vector<App::DocumentObject*> pLinks = pComp->Links.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pLinks.begin(); it != pLinks.end(); ++it) {
        if (*it)
            Gui::Application::Instance->showViewProvider(*it);
    }

    return true;
}

bool ViewProviderCompound::canDragObjects() const
{
    return true;
}

bool ViewProviderCompound::canDragObject(App::DocumentObject* obj) const
{
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderCompound::dragObject(App::DocumentObject* obj)
{
    Part::Compound* pComp = static_cast<Part::Compound*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pComp->Links.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it == obj) {
            pShapes.erase(it);
            pComp->Links.setValues(pShapes);
            break;
        }
    }
}

bool ViewProviderCompound::canDropObjects() const
{
    return true;
}

bool ViewProviderCompound::canDropObject(App::DocumentObject* obj) const
{
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderCompound::dropObject(App::DocumentObject* obj)
{
    Part::Compound* pComp = static_cast<Part::Compound*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pComp->Links.getValues();
    pShapes.push_back(obj);
    pComp->Links.setValues(pShapes);
}
