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
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeaturePartFuse.h>

#include "ViewProviderBoolean.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderBoolean,PartGui::ViewProviderPart)

ViewProviderBoolean::ViewProviderBoolean() = default;

ViewProviderBoolean::~ViewProviderBoolean() = default;

std::vector<App::DocumentObject*> ViewProviderBoolean::claimChildren()const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(getObject<Part::Boolean>()->Base.getValue());
    temp.push_back(getObject<Part::Boolean>()->Tool.getValue());

    return temp;
}

QIcon ViewProviderBoolean::getIcon() const
{
    App::DocumentObject* obj = getObject();
    if (obj) {
        Base::Type type = obj->getTypeId();
        if (type == Base::Type::fromName("Part::Common"))
            return Gui::BitmapFactory().iconFromTheme("Part_Common");
        else if (type == Base::Type::fromName("Part::Fuse"))
            return Gui::BitmapFactory().iconFromTheme("Part_Fuse");
        else if (type == Base::Type::fromName("Part::Cut"))
            return Gui::BitmapFactory().iconFromTheme("Part_Cut");
        else if (type == Base::Type::fromName("Part::Section"))
            return Gui::BitmapFactory().iconFromTheme("Part_Section");
    }

    return ViewProviderPart::getIcon();
}

void ViewProviderBoolean::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->is<Part::PropertyShapeHistory>()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        if (hist.size() != 2)
            return;
        Part::Boolean* objBool = getObject<Part::Boolean>();
        if (!objBool)
            return;
        Part::Feature* objBase = dynamic_cast<Part::Feature*>(
                Part::Feature::getShapeOwner(objBool->Base.getValue()));
        Part::Feature* objTool = dynamic_cast<Part::Feature*>(
                Part::Feature::getShapeOwner(objBool->Tool.getValue()));
        if (objBase && objTool) {
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();
            const TopoDS_Shape& toolShape = objTool->Shape.getValue();
            const TopoDS_Shape& boolShape = objBool->Shape.getValue();

            TopTools_IndexedMapOfShape baseMap, toolMap, boolMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);
            TopExp::MapShapes(toolShape, TopAbs_FACE, toolMap);
            TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

            auto vpBase = dynamic_cast<PartGui::ViewProviderPart*>(
                    Gui::Application::Instance->getViewProvider(objBase));
            auto vpTool = dynamic_cast<PartGui::ViewProviderPart*>(
                    Gui::Application::Instance->getViewProvider(objTool));
            if (vpBase && vpTool) {
                std::vector<App::Material> colBase = vpBase->ShapeAppearance.getValues();
                std::vector<App::Material> colTool = vpTool->ShapeAppearance.getValues();
                std::vector<App::Material> colBool;
                colBool.resize(boolMap.Extent(), this->ShapeAppearance[0]);
                applyTransparency(vpBase->Transparency.getValue(),colBase);
                applyTransparency(vpTool->Transparency.getValue(),colTool);

                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyMaterial(hist[0], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeAppearance[0]) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyMaterial(hist[0], colBase, colBool);
                }

                if (static_cast<int>(colTool.size()) == toolMap.Extent()) {
                    applyMaterial(hist[1], colTool, colBool);
                }
                else if (!colTool.empty() && colTool[0] != this->ShapeAppearance[0]) {
                    colTool.resize(toolMap.Extent(), colTool[0]);
                    applyMaterial(hist[1], colTool, colBool);
                }

                // If the view provider has set a transparency then override the values
                // of the input shapes
                if (Transparency.getValue() > 0) {
                    applyTransparency(Transparency.getValue(), colBool);
                }

                this->ShapeAppearance.setValues(colBool);
            }
        }
    }
    else if (prop->isDerivedFrom<App::PropertyLink>()) {
        App::DocumentObject *pBase = static_cast<const App::PropertyLink*>(prop)->getValue();
        if (pBase)
            Gui::Application::Instance->hideViewProvider(pBase);
    }
}

bool ViewProviderBoolean::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::Boolean* pBool = getObject<Part::Boolean>();
    App::DocumentObject *pBase = pBool->Base.getValue();
    App::DocumentObject *pTool = pBool->Tool.getValue();

    if (pBase)
        Gui::Application::Instance->showViewProvider(pBase);
    if (pTool)
        Gui::Application::Instance->showViewProvider(pTool);

    return true;
}

PROPERTY_SOURCE(PartGui::ViewProviderMultiFuse,PartGui::ViewProviderPart)

ViewProviderMultiFuse::ViewProviderMultiFuse() = default;

ViewProviderMultiFuse::~ViewProviderMultiFuse() = default;

std::vector<App::DocumentObject*> ViewProviderMultiFuse::claimChildren()const
{
    return getObject<Part::MultiFuse>()->Shapes.getValues();
}

QIcon ViewProviderMultiFuse::getIcon() const
{
    return Gui::BitmapFactory().iconFromTheme("Part_Fuse");
}

void ViewProviderMultiFuse::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->is<Part::PropertyShapeHistory>()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        Part::MultiFuse* objBool = getObject<Part::MultiFuse>();
        std::vector<App::DocumentObject*> sources = objBool->Shapes.getValues();
        if (hist.size() != sources.size())
            return;

        const TopoDS_Shape& boolShape = objBool->Shape.getValue();
        TopTools_IndexedMapOfShape boolMap;
        TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

        std::vector<App::Material> colBool;
        colBool.resize(boolMap.Extent(), this->ShapeAppearance[0]);

        int index=0;
        for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
            Part::Feature* objBase = dynamic_cast<Part::Feature*>(Part::Feature::getShapeOwner(*it));
            if (!objBase)
                continue;
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();

            TopTools_IndexedMapOfShape baseMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

            auto vpBase = dynamic_cast<PartGui::ViewProviderPart*>(Gui::Application::Instance->getViewProvider(objBase));
            if (vpBase) {
                std::vector<App::Material> colBase = vpBase->ShapeAppearance.getValues();
                applyTransparency(vpBase->Transparency.getValue(),colBase);
                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyMaterial(hist[index], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeAppearance[0]) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyMaterial(hist[index], colBase, colBool);
                }
            }
        }

        // If the view provider has set a transparency then override the values
        // of the input shapes
        if (Transparency.getValue() > 0) {
            applyTransparency(Transparency.getValue(), colBool);
        }

        this->ShapeAppearance.setValues(colBool);
    }
    else if (prop->isDerivedFrom<App::PropertyLinkList>()) {
        std::vector<App::DocumentObject*> pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (auto it : pShapes) {
            if (it) {
                Gui::Application::Instance->hideViewProvider(it);
            }
        }
    }
}

bool ViewProviderMultiFuse::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::MultiFuse* pBool = getObject<Part::MultiFuse>();
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (auto it : pShapes) {
        if (it) {
            Gui::Application::Instance->showViewProvider(it);
        }
    }

    return true;
}

bool ViewProviderMultiFuse::canDragObjects() const
{
    return true;
}

bool ViewProviderMultiFuse::canDragObject(App::DocumentObject* obj) const
{
    (void)obj;
    // return Part::Feature::hasShapeOwner(obj);
    return true;
}

void ViewProviderMultiFuse::dragObject(App::DocumentObject* obj)
{
    Part::MultiFuse* pBool = getObject<Part::MultiFuse>();
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
    (void)obj;
    // return Part::Feature::hasShapeOwner(obj);
    return true;
}

void ViewProviderMultiFuse::dropObject(App::DocumentObject* obj)
{
    Part::MultiFuse* pBool = getObject<Part::MultiFuse>();
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    pShapes.push_back(obj);
    pBool->Shapes.setValues(pShapes);
}

PROPERTY_SOURCE(PartGui::ViewProviderMultiCommon,PartGui::ViewProviderPart)

ViewProviderMultiCommon::ViewProviderMultiCommon() = default;

ViewProviderMultiCommon::~ViewProviderMultiCommon() = default;

std::vector<App::DocumentObject*> ViewProviderMultiCommon::claimChildren()const
{
    return getObject<Part::MultiCommon>()->Shapes.getValues();
}

QIcon ViewProviderMultiCommon::getIcon() const
{
    return Gui::BitmapFactory().iconFromTheme("Part_Common");
}

void ViewProviderMultiCommon::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->is<Part::PropertyShapeHistory>()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        Part::MultiCommon* objBool = getObject<Part::MultiCommon>();
        std::vector<App::DocumentObject*> sources = objBool->Shapes.getValues();
        if (hist.size() != sources.size())
            return;

        const TopoDS_Shape& boolShape = objBool->Shape.getValue();
        TopTools_IndexedMapOfShape boolMap;
        TopExp::MapShapes(boolShape, TopAbs_FACE, boolMap);

        std::vector<App::Material> colBool;
        colBool.resize(boolMap.Extent(), this->ShapeAppearance[0]);

        int index=0;
        for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
            Part::Feature* objBase = dynamic_cast<Part::Feature*>(Part::Feature::getShapeOwner(*it));
            if (!objBase)
                continue;
            const TopoDS_Shape& baseShape = objBase->Shape.getValue();

            TopTools_IndexedMapOfShape baseMap;
            TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

            auto vpBase = dynamic_cast<PartGui::ViewProviderPart*>(Gui::Application::Instance->getViewProvider(objBase));
            if (vpBase) {
                std::vector<App::Material> colBase = vpBase->ShapeAppearance.getValues();
                applyTransparency(vpBase->Transparency.getValue(),colBase);
                if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
                    applyMaterial(hist[index], colBase, colBool);
                }
                else if (!colBase.empty() && colBase[0] != this->ShapeAppearance[0]) {
                    colBase.resize(baseMap.Extent(), colBase[0]);
                    applyMaterial(hist[index], colBase, colBool);
                }
            }
        }

        // If the view provider has set a transparency then override the values
        // of the input shapes
        if (Transparency.getValue() > 0) {
            applyTransparency(Transparency.getValue(), colBool);
        }

        this->ShapeAppearance.setValues(colBool);
    }
    else if (prop->isDerivedFrom<App::PropertyLinkList>()) {
        std::vector<App::DocumentObject*> pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (auto it : pShapes) {
            if (it) {
                Gui::Application::Instance->hideViewProvider(it);
            }
        }
    }
}

bool ViewProviderMultiCommon::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::MultiCommon* pBool = getObject<Part::MultiCommon>();
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    for (auto it : pShapes) {
        if (it) {
            Gui::Application::Instance->showViewProvider(it);
        }
    }

    return true;
}

bool ViewProviderMultiCommon::canDragObjects() const
{
    return true;
}

bool ViewProviderMultiCommon::canDragObject(App::DocumentObject* obj) const
{
    (void)obj;
    // return Part::Feature::hasShapeOwner(obj);
    return true;
}

void ViewProviderMultiCommon::dragObject(App::DocumentObject* obj)
{
    Part::MultiCommon* pBool = getObject<Part::MultiCommon>();
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
    (void)obj;
    // return Part::Feature::hasShapeOwner(obj);
    return true;
}

void ViewProviderMultiCommon::dropObject(App::DocumentObject* obj)
{
    Part::MultiCommon* pBool = getObject<Part::MultiCommon>();
    std::vector<App::DocumentObject*> pShapes = pBool->Shapes.getValues();
    pShapes.push_back(obj);
    pBool->Shapes.setValues(pShapes);
}

