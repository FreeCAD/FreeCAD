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

PROPERTY_SOURCE(PartGui::ViewProviderBoolean,PartGui::ViewProviderPart)

ViewProviderBoolean::ViewProviderBoolean()
{
}

ViewProviderBoolean::~ViewProviderBoolean()
{
}

std::vector<App::DocumentObject*> ViewProviderBoolean::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Base.getValue());
    temp.push_back(static_cast<Part::Boolean*>(getObject())->Tool.getValue());

    return temp;
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

void ViewProviderBoolean::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyShapeHistory::getClassTypeId()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        if (hist.size() != 2)
            return;
        Part::Boolean* objBool = dynamic_cast<Part::Boolean*>(getObject());
        if (!objBool)
            return;
        Part::Feature* objBase = dynamic_cast<Part::Feature*>(objBool->Base.getValue());
        Part::Feature* objTool = dynamic_cast<Part::Feature*>(objBool->Tool.getValue());
        if (objBase && objTool) {
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();
            const TopoDS_Shape& toolShape = objTool->Shape.getValue();
            const TopoDS_Shape& boolShape = objBool->Shape.getValue();

            TopTools_IndexedMapOfShape baseMap, toolMap, boolMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);
            TopExp::MapShapes(toolShape, TopAbs_FACE, toolMap);
            TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

            Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
            Gui::ViewProvider* vpTool = Gui::Application::Instance->getViewProvider(objTool);
            if (vpBase && vpTool) {
                std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
                std::vector<App::Color> colTool = static_cast<PartGui::ViewProviderPart*>(vpTool)->DiffuseColor.getValues();
                std::vector<App::Color> colBool;
                colBool.resize(boolMap.Extent(), this->ShapeColor.getValue());
                applyTransparency(static_cast<PartGui::ViewProviderPart*>(vpBase)->Transparency.getValue(),colBase);
                applyTransparency(static_cast<PartGui::ViewProviderPart*>(vpTool)->Transparency.getValue(),colTool);

                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyColor(hist[0], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeColor.getValue()) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyColor(hist[0], colBase, colBool);
                }

                if (static_cast<int>(colTool.size()) == toolMap.Extent()) {
                    applyColor(hist[1], colTool, colBool);
                }
                else if (!colTool.empty() && colTool[0] != this->ShapeColor.getValue()) {
                    colTool.resize(toolMap.Extent(), colTool[0]);
                    applyColor(hist[1], colTool, colBool);
                }

                this->DiffuseColor.setValues(colBool);
            }
        }
    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        App::DocumentObject *pBase = static_cast<const App::PropertyLink*>(prop)->getValue();
        if (pBase)
            Gui::Application::Instance->hideViewProvider(pBase);
    }
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

void ViewProviderBoolean::replaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    Part::Boolean* pBool = static_cast<Part::Boolean*>(getObject());
    if (oldValue == pBool->Base.getValue()) {
        pBool->Base.setValue(newValue);
    }
    else if (oldValue == pBool->Tool.getValue()) {
        pBool->Tool.setValue(newValue);
    }
}


PROPERTY_SOURCE(PartGui::ViewProviderMultiFuse,PartGui::ViewProviderPart)

ViewProviderMultiFuse::ViewProviderMultiFuse()
{
}

ViewProviderMultiFuse::~ViewProviderMultiFuse()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiFuse::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiFuse*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiFuse::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Fuse");
}

void ViewProviderMultiFuse::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyShapeHistory::getClassTypeId()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        Part::MultiFuse* objBool = static_cast<Part::MultiFuse*>(getObject());
        std::vector<App::DocumentObject*> sources = objBool->Shapes.getValues();
        if (hist.size() != sources.size())
            return;

        const TopoDS_Shape& boolShape = objBool->Shape.getValue();
        TopTools_IndexedMapOfShape boolMap;
        TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

        std::vector<App::Color> colBool;
        colBool.resize(boolMap.Extent(), this->ShapeColor.getValue());

        int index=0;
        for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
            Part::Feature* objBase = dynamic_cast<Part::Feature*>(*it);
            if (!objBase)
                continue;
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();
 
            TopTools_IndexedMapOfShape baseMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

            Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
            if (vpBase) {
                std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
                applyTransparency(static_cast<PartGui::ViewProviderPart*>(vpBase)->Transparency.getValue(),colBase);
                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyColor(hist[index], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeColor.getValue()) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyColor(hist[index], colBase, colBool);
                }
            }
        }

        this->DiffuseColor.setValues(colBool);
    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
        std::vector<App::DocumentObject*> pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
            if (*it)
                Gui::Application::Instance->hideViewProvider(*it);
        }
    }
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
    return obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId());
}

void ViewProviderMultiFuse::dropObject(App::DocumentObject* obj)
{
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    pShapes.push_back(obj);
    pBool->Shapes.setValues(pShapes);
}

void ViewProviderMultiFuse::replaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    std::replace(pShapes.begin(), pShapes.end(), oldValue, newValue);
    pBool->Shapes.setValues(pShapes);
}

PROPERTY_SOURCE(PartGui::ViewProviderMultiCommon,PartGui::ViewProviderPart)

ViewProviderMultiCommon::ViewProviderMultiCommon()
{
}

ViewProviderMultiCommon::~ViewProviderMultiCommon()
{
}

std::vector<App::DocumentObject*> ViewProviderMultiCommon::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>(static_cast<Part::MultiCommon*>(getObject())->Shapes.getValues());
}

QIcon ViewProviderMultiCommon::getIcon(void) const
{
    return Gui::BitmapFactory().pixmap("Part_Common");
}

void ViewProviderMultiCommon::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyShapeHistory::getClassTypeId()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        Part::MultiCommon* objBool = static_cast<Part::MultiCommon*>(getObject());
        std::vector<App::DocumentObject*> sources = objBool->Shapes.getValues();
        if (hist.size() != sources.size())
            return;

        const TopoDS_Shape& boolShape = objBool->Shape.getValue();
        TopTools_IndexedMapOfShape boolMap;
        TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

        std::vector<App::Color> colBool;
        colBool.resize(boolMap.Extent(), this->ShapeColor.getValue());

        int index=0;
        for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
            Part::Feature* objBase = dynamic_cast<Part::Feature*>(*it);
            if (!objBase)
                continue;
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();
 
            TopTools_IndexedMapOfShape baseMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

            Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
            if (vpBase) {
                std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
                applyTransparency(static_cast<PartGui::ViewProviderPart*>(vpBase)->Transparency.getValue(),colBase);
                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyColor(hist[index], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeColor.getValue()) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyColor(hist[index], colBase, colBool);
                }
            }
        }

        this->DiffuseColor.setValues(colBool);
    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
        std::vector<App::DocumentObject*> pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (std::vector<App::DocumentObject*>::iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
            if (*it)
                Gui::Application::Instance->hideViewProvider(*it);
        }
    }
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

void ViewProviderMultiCommon::replaceObject(App::DocumentObject* oldValue, App::DocumentObject* newValue)
{
    Part::MultiFuse* pBool = static_cast<Part::MultiFuse*>(getObject());
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    std::replace(pShapes.begin(), pShapes.end(), oldValue, newValue);
    pBool->Shapes.setValues(pShapes);
}
