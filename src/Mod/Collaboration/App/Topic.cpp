// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <App/GeoFeature.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/Attacher.h>

#include "Topic.h"

using namespace Collaboration;

PROPERTY_SOURCE_WITH_EXTENSIONS(Collaboration::Topic, App::AnnotationLabel);

Topic::Topic()
{

    ADD_PROPERTY_TYPE(PlacementType, (""), "Label", App::Prop_None, "Type of placement for the topic");
    PlacementType.setEnums({"Absolute", "Geometry"});

    ADD_PROPERTY_TYPE(Geometry, (nullptr), "Label", App::Prop_None, "Geometry to associate the topic with");

    GroupExtension::initExtension(this);
}

Topic::~Topic() = default;

void Topic::onChanged(const App::Property* prop)
{
    if (prop == &PlacementType) {
        changePlacementType(PlacementType.getValueAsString());
    }
    else if (prop == &Geometry) {
        mustExecute();
    }

    AnnotationLabel::onChanged(prop);
}

std::optional<Base::Vector3d> Topic::getGeometryPosition() const
{
    App::DocumentObject* obj = Geometry.getValue();
    if (!obj) {
        return std::nullopt;
    }

    App::PropertyPlacement* placementProp = obj->getPlacementProperty();
    if (!placementProp) {
        // Of objects without a placement property we can't determine the position.
        return std::nullopt;
    }

    Attacher::AttachEngine3D engine;
    App::PropertyLinkSubList references;
    references.setValue(obj, Geometry.getSubValues());

    Base::Placement globalPlacement = placementProp->getValue();
    // Use the global placement as a fallback.
    Base::Placement placement = globalPlacement;
    try {
        // We first try with Inertial CS.
        engine.setUp(references, Attacher::eMapMode::mmInertialCS, false, false, 0.0, 0.0);
        placement = engine.calculateAttachedPlacement(globalPlacement);
    }
    catch (Base::ValueError& e) {
        // Inertial CS failed, try Object XY.
        engine.setUp(references, Attacher::eMapMode::mmObjectXY, false, false, 0.0, 0.0);
        placement = engine.calculateAttachedPlacement(globalPlacement);
    }

    return placement.getPosition();
}

App::DocumentObjectExecReturn* Topic::execute()
{
    // Set the value of the base position to trigger updates.
    BasePosition.setValue(BasePosition.getValue());
    return App::DocumentObject::StdReturn;
}

void Topic::changePlacementType(const char* type)
{
    std::string_view svType = type ? std::string_view(type) : std::string_view();
    if (svType == "Absolute") {
        if (auto maybeGeomPos = getGeometryPosition()) {
            // There was geometry before, include that position into the base.
            Base::Vector3d geomPos = *maybeGeomPos;
            Base::Vector3d position = BasePosition.getValue();
            BasePosition.setValue(position + geomPos);
        }
    }
    else if (svType == "Geometry") {
        if (auto maybeGeomPos = getGeometryPosition()) {
            Base::Vector3d geomPos = *maybeGeomPos;
            Base::Vector3d position = BasePosition.getValue();
            BasePosition.setValue(position - geomPos);
        }
    }
}
