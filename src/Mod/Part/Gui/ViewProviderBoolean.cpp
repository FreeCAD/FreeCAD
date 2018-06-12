/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "ViewProviderBoolean.h"
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/FeaturePartBoolean.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/FeaturePartCommon.h>

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderBoolean,PartGui::ViewProviderDerivedPart)

ViewProviderBoolean::ViewProviderBoolean()
{
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}

QIcon ViewProviderBoolean::getIcon(void) const
{
    App::DocumentObject* obj = getObject();
    if (obj) {
        Base::Type type = obj->getTypeId();
        if (type == Base::Type::fromName("Part::Common"))
            return Gui::BitmapFactory().pixmap("Part_Common");
        else if (type == Base::Type::fromName("Part::Fuse"))
            return Gui::BitmapFactory().pixmap("Part_Fuse");
        else if (type == Base::Type::fromName("Part::Cut"))
            return Gui::BitmapFactory().pixmap("Part_Cut");
        else if (type == Base::Type::fromName("Part::Section"))
            return Gui::BitmapFactory().pixmap("Part_Section");
    }

    return ViewProviderPart::getIcon();
}



bool ViewProviderBoolean::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::Boolean* pBool = static_cast<Part::Boolean*>(getObject()); 
    App::DocumentObject *pBase = pBool->Base.getValue();
    App::DocumentObject *pTool = pBool->Tool.getValue();

    if (pBase)
        Gui::Application::Instance->showViewProvider(pBase);
    if (pTool)
        Gui::Application::Instance->showViewProvider(pTool);

    return true;
}

PROPERTY_SOURCE(PartGui::ViewProviderMultiFuse,PartGui::ViewProviderDerivedPart)

ViewProviderMultiFuse::ViewProviderMultiFuse()
{
}

ViewProviderMultiFuse::~ViewProviderMultiFuse()
{
}


QIcon ViewProviderMultiFuse::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Fuse");
}


bool ViewProviderMultiFuse::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it)
            Gui::Application::Instance->showViewProvider(*it);
    }

    return true;
}

bool ViewProviderMultiFuse::canDragObjects() const
{
    return true;
}

bool ViewProviderMultiFuse::canDragObject(App::DocumentObject* obj) const
{
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderMultiFuse::dragObject(App::DocumentObject* obj)
{
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
	// do not drag if less than 3 objects
	if (pShapes.size() < 3) return;
	
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it == obj) {
            pShapes.erase(it);
            pBool->Shapes.setValues(pShapes);
            break;
        }
    }
}

bool ViewProviderMultiFuse::canDropObjects() const
{
    return true;
}

bool ViewProviderMultiFuse::canDropObject(App::DocumentObject* obj) const
{
	if (! obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) return false;
	// checks if the part is not already in
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it == obj) {
            return false;
        }
    }
	
    return true;
}

void ViewProviderMultiFuse::dropObject(App::DocumentObject* obj)
{
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
	// checks if the part is not already in
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it == obj) {
            return ;
        }
    }
    pShapes.push_back(obj);
    pBool->Shapes.setValues(pShapes);
}


PROPERTY_SOURCE(PartGui::ViewProviderMultiCommon,PartGui::ViewProviderDerivedPart)

ViewProviderMultiCommon::ViewProviderMultiCommon()
{
}

ViewProviderMultiCommon::~ViewProviderMultiCommon()
{
}

QIcon ViewProviderMultiCommon::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Common");
}


bool ViewProviderMultiCommon::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::MultiCommon* pBool = static_cast<Part::MultiCommon*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it)
            Gui::Application::Instance->showViewProvider(*it);
    }

    return true;
}

bool ViewProviderMultiCommon::canDragObjects() const
{
    return true;
}

bool ViewProviderMultiCommon::canDragObject(App::DocumentObject* obj) const
{
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderMultiCommon::dragObject(App::DocumentObject* obj)
{
    Part::MultiCommon* pBool = static_cast<Part::MultiCommon*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
        if (*it == obj) {
            pShapes.erase(it);
            pBool->Shapes.setValues(pShapes);
            break;
        }
    }
}

bool ViewProviderMultiCommon::canDropObjects() const
{
    return true;
}

bool ViewProviderMultiCommon::canDropObject(App::DocumentObject* obj) const
{
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderMultiCommon::dropObject(App::DocumentObject* obj)
{
    Part::MultiCommon* pBool = static_cast<Part::MultiCommon*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
	
	
    pShapes.push_back(obj);
    pBool->Shapes.setValues(pShapes);
}
