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

PROPERTY_SOURCE(PartGui::ViewProviderCompound,PartGui::ViewProviderPart)

ViewProviderCompound::ViewProviderCompound()
{
    sPixmap = "Part_Compound.svg";
}

ViewProviderCompound::~ViewProviderCompound() = default;

std::vector<App::DocumentObject*> ViewProviderCompound::claimChildren() const
{
    return getObject<Part::Compound>()->Links.getValues();
}

bool ViewProviderCompound::onDelete(const std::vector<std::string> &)
{
    // get the input shapes
    Part::Compound* pComp = getObject<Part::Compound>();
    std::vector<App::DocumentObject*> pLinks = pComp->Links.getValues();
    for (auto pLink : pLinks) {
        if (pLink)
            Gui::Application::Instance->showViewProvider(pLink);
    }

    return true;
}

void ViewProviderCompound::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->is<Part::PropertyShapeHistory>()) {
        const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            (prop)->getValues();
        Part::Compound* objComp = getObject<Part::Compound>();
        std::vector<App::DocumentObject*> sources = objComp->Links.getValues();

        if (hist.size() != sources.size()) {
            // avoid duplicates without changing the order
            // See also Compound::execute
            std::set<App::DocumentObject*> tempSources;
            std::vector<App::DocumentObject*> filter;
            for (auto source : sources) {
                Part::Feature* objBase = dynamic_cast<Part::Feature*>(source);
                if (objBase) {
                    auto pos = tempSources.insert(objBase);
                    if (pos.second) {
                        filter.push_back(objBase);
                    }
                }
            }

            sources = filter;
        }
        if (hist.size() != sources.size())
            return;

        const TopoDS_Shape& compShape = objComp->Shape.getValue();
        TopTools_IndexedMapOfShape compMap;
        TopExp::MapShapes(compShape, TopAbs_FACE, compMap);

        std::vector<App::Material> compCol;
        compCol.resize(compMap.Extent(), this->ShapeAppearance[0]);

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
                std::vector<App::Material> baseCol = vpBase->ShapeAppearance.getValues();
                applyTransparency(vpBase->Transparency.getValue(), baseCol);
                if (static_cast<int>(baseCol.size()) == baseMap.Extent()) {
                    applyMaterial(hist[index], baseCol, compCol);
                }
                else if (!baseCol.empty() && baseCol[0] != this->ShapeAppearance[0]) {
                    baseCol.resize(baseMap.Extent(), baseCol[0]);
                    applyMaterial(hist[index], baseCol, compCol);
                }
            }
        }

        // If the view provider has set a transparency then override the values
        // of the input shapes
        if (Transparency.getValue() > 0) {
            applyTransparency(Transparency.getValue(), compCol);
        }

        this->ShapeAppearance.setValues(compCol);
    }
    else if (prop->isDerivedFrom<App::PropertyLinkList>()) {
        const std::vector<App::DocumentObject *>& pBases = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (auto pBase : pBases) {
            if (pBase) Gui::Application::Instance->hideViewProvider(pBase);
        }
    }
}

bool ViewProviderCompound::canDragObjects() const
{
    return true;
}

bool ViewProviderCompound::canDragObject(App::DocumentObject* obj) const
{
    return obj->isDerivedFrom<Part::Feature>();
}

void ViewProviderCompound::dragObject(App::DocumentObject* obj)
{
    Part::Compound* pComp = getObject<Part::Compound>();
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
    return obj->isDerivedFrom<Part::Feature>();
}

void ViewProviderCompound::dropObject(App::DocumentObject* obj)
{
    Part::Compound* pComp = getObject<Part::Compound>();
    std::vector<App::DocumentObject*> pShapes = pComp->Links.getValues();
    pShapes.push_back(obj);
    pComp->Links.setValues(pShapes);
}

