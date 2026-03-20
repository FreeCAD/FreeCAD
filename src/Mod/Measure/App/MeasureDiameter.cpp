/***************************************************************************
 *   Copyright (c) 2025 Kavin Teenakul <andythe_great@protonmail.com>                *
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
 ***************************************************************************/


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

#include "MeasureDiameter.h"

using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureDiameter, Measure::MeasureBase)

MeasureDiameter::MeasureDiameter()
{
    ADD_PROPERTY_TYPE(Element, (nullptr), "Measurement", App::Prop_None, "Element to get the diameter from");
    Element.setScope(App::LinkScope::Global);
    Element.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Diameter,
        (0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Diameter of selection"
    );
}

MeasureDiameter::~MeasureDiameter() = default;

bool MeasureDiameter::isValidSelection(const App::MeasureSelection& selection)
{
    if (selection.empty() || selection.size() > 1) {
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

bool MeasureDiameter::isPrioritizedSelection(const App::MeasureSelection& selection)
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

void MeasureDiameter::parseSelection(const App::MeasureSelection& selection)
{
    auto element = selection.front();
    auto objT = element.object;

    std::vector<std::string> subElementList {objT.getSubName()};
    Element.setValue(objT.getObject(), subElementList);
}

App::DocumentObjectExecReturn* MeasureDiameter::execute()
{
    auto info = getMeasureInfoFirst();
    if (!info || !info->valid) {
        return new App::DocumentObjectExecReturn("Cannot calculate diameter");
    }

    Diameter.setValue(info->radius * 2.0);
    return DocumentObject::StdReturn;
}

void MeasureDiameter::onChanged(const App::Property* prop)
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

Base::Placement MeasureDiameter::getPlacement() const
{
    auto loc = getMeasureInfoFirst()->pointOnCurve;
    auto p = Base::Placement();
    p.setPosition(loc);
    return p;
}

Base::Vector3d MeasureDiameter::getPointOnCurve() const
{
    return getMeasureInfoFirst()->pointOnCurve;
}

Part::MeasureRadiusInfoPtr MeasureDiameter::getMeasureInfoFirst() const
{
    const App::DocumentObject* object = Element.getValue();
    const std::vector<std::string>& subElements = Element.getSubValues();

    if (!object || subElements.empty()) {
        return std::make_shared<Part::MeasureRadiusInfo>();
    }

    App::SubObjectT subject {object, subElements.front().c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        return std::make_shared<Part::MeasureRadiusInfo>();
    }

    return std::dynamic_pointer_cast<Part::MeasureRadiusInfo>(info);
}

std::vector<App::DocumentObject*> MeasureDiameter::getSubject() const
{
    return {Element.getValue()};
}
