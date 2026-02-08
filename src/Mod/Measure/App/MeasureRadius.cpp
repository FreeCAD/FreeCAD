// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp_CLProps.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>

#include <Mod/Part/App/PartFeature.h>

#include "MeasureRadius.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureRadius, Measure::MeasureBase)


MeasureRadius::MeasureRadius()
{
    ADD_PROPERTY_TYPE(Element, (nullptr), "Measurement", App::Prop_None, "Element to get the radius from");
    Element.setScope(App::LinkScope::Global);
    Element.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Radius,
        (0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Radius of selection"
    );
}

MeasureRadius::~MeasureRadius() = default;

//! validate all the object+subelement pairs in the selection. Must be circle or arc
//! and have a geometry handler available.  We only calculate radius if there is a
//! single valid item in the selection
bool MeasureRadius::isValidSelection(const App::MeasureSelection& selection)
{

    if (selection.empty() || selection.size() > 1) {
        // too few or too many selections
        return false;
    }

    auto element = selection.front();
    auto type = App::MeasureManager::getMeasureElementType(element);

    if (type == App::MeasureElementType::INVALID) {
        return false;
    }

    if (type != App::MeasureElementType::CIRCLE && type != App::MeasureElementType::ARC
        && type != App::MeasureElementType::CYLINDER && type != App::MeasureElementType::DISC) {
        return false;
    }

    return true;
}

//! return true if the selection is particularly interesting to MeasureRadius.
//! In this case we claim circles and arcs.
bool MeasureRadius::isPrioritizedSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 1) {
        return false;
    }

    auto element = selection.front();
    auto type = App::MeasureManager::getMeasureElementType(element);

    if (type == App::MeasureElementType::CIRCLE || type == App::MeasureElementType::ARC
        || type == App::MeasureElementType::CYLINDER || type == App::MeasureElementType::DISC) {
        return true;
    }

    return false;
}


//! Set properties from first item in selection. assumes a valid selection.
void MeasureRadius::parseSelection(const App::MeasureSelection& selection)
{
    auto element = selection.front();
    auto objT = element.object;

    std::vector<std::string> subElementList {objT.getSubName()};
    Element.setValue(objT.getObject(), subElementList);
}


App::DocumentObjectExecReturn* MeasureRadius::execute()
{
    auto info = getMeasureInfoFirst();
    if (!info || !info->valid) {
        return new App::DocumentObjectExecReturn("Cannot calculate radius");
    }

    Radius.setValue(info->radius);
    return DocumentObject::StdReturn;
}


void MeasureRadius::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Element) {
        auto ret = recompute();
        delete ret;
    }

    MeasureBase::onChanged(prop);
}


//! return a placement (location + orientation) for the first element
Base::Placement MeasureRadius::getPlacement() const
{
    auto loc = getMeasureInfoFirst()->pointOnCurve;
    auto p = Base::Placement();
    p.setPosition(loc);
    return p;
}


//! return the pointOnCurve element in MeasureRadiusInfo for the first element
Base::Vector3d MeasureRadius::getPointOnCurve() const
{
    return getMeasureInfoFirst()->pointOnCurve;
}

//! get the handler's result for the first element
Part::MeasureRadiusInfoPtr MeasureRadius::getMeasureInfoFirst() const
{
    const App::DocumentObject* object = Element.getValue();
    const std::vector<std::string>& subElements = Element.getSubValues();

    if (!object || subElements.empty()) {
        // NOLINTNEXTLINE(modernize-return-braced-init-list)
        return std::make_shared<Part::MeasureRadiusInfo>();
    }

    App::SubObjectT subject {object, subElements.front().c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        // NOLINTNEXTLINE(modernize-return-braced-init-list)
        return std::make_shared<Part::MeasureRadiusInfo>();
    }

    auto radiusInfo = std::dynamic_pointer_cast<Part::MeasureRadiusInfo>(info);
    return radiusInfo;
}

//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureRadius::getSubject() const
{
    return {Element.getValue()};
}
