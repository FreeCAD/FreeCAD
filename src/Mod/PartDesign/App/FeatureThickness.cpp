/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <map>
#include <string>
#include <vector>

#include "PreCompiled.h"    // NOLINT
#ifndef _PreComp_
# include <Precision.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/Exception.h>
#include "FeatureThickness.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char *PartDesign::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char *PartDesign::Thickness::JoinEnums[] = {"Arc", "Intersection", nullptr};

PROPERTY_SOURCE(PartDesign::Thickness, PartDesign::DressUp)

Thickness::Thickness() {
    ADD_PROPERTY_TYPE(Value, (1.0), "Thickness", App::Prop_None, "Thickness value");
    ADD_PROPERTY_TYPE(Mode, (0L), "Thickness", App::Prop_None, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (0L), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(Reversed, (true), "Thickness", App::Prop_None,
                      "Apply the thickness towards the solids interior");
    ADD_PROPERTY_TYPE(Intersection, (false), "Thickness", App::Prop_None,
                      "Enable intersection-handling");
}

int16_t Thickness::mustExecute() const {
    if (Placement.isTouched() ||
        Value.isTouched() ||
        Mode.isTouched() ||
        Join.isTouched()) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Thickness::execute() {
    if (onlyHasToRefine()){
        TopoShape result = refineShapeIfActive(rawShape);
        Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }

    // Base shape
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    const std::vector<std::string>& subStrings = Base.getSubValues(true);

    // If the base has no sub elements listed just return a copy of the base.
    if (subStrings.empty()) {
        // We must set the placement of the feature in case it's empty.
        this->positionByBaseFeature();
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    /* If the feature was ever empty, then Placement was set by positionByBaseFeature.  However,
     * makeThickSolid apparently requires the placement to be empty, so we have to clear it */
    this->Placement.setValue(Base::Placement());

    std::map<int, std::vector<TopoShape>> closeFaces;
    for ( const auto& it : subStrings ) {
        TopoDS_Shape face;
        try {
            face = TopShape.getSubShape(it.c_str());
        }
        catch (...) {
        }
        if (face.IsNull())
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Invalid face reference"));
        // We found the sub element (face) so let's get its history index in our shape
        int index = TopShape.findAncestor(face, TopAbs_SOLID);
        if (!index) {
            FC_WARN(getFullName() << ": Ignore non-solid face  " << it);
            continue;
        }
        closeFaces[index].emplace_back(face);
    }

    bool reversed = Reversed.getValue();
    bool intersection = Intersection.getValue();
    double thickness = (reversed ? -1. : 1.) * Value.getValue();
    double tol = Precision::Confusion();
    auto mode = static_cast<int16_t>(Mode.getValue());
    auto join = Join.getValue();

    std::vector<TopoShape> shapes;
    auto count = static_cast<int>(TopShape.countSubShapes(TopAbs_SOLID));
    if (!count)
        return new App::DocumentObjectExecReturn("No solid");
    // we do not offer tangent join type
    if (join == 1)
        join = 2;

    if (fabs(thickness) > 2 * tol) {
        auto mapIterator = closeFaces.begin();
        for (auto loopIndex = 1; loopIndex <= count; ++loopIndex) {
            std::vector<TopoShape> dummy;
            const auto* faces = &dummy;
            TopoShape solid = TopShape;
            // expect the sub element indexes in the map to be in order and matching our loop index,
            // and effectively ignore them if they are not.
            if (mapIterator != closeFaces.end() && loopIndex >= mapIterator->first) {
                faces = &mapIterator->second;
                solid = TopShape.getSubTopoShape(TopAbs_SOLID, mapIterator->first);
            }
            TopoShape res(0);
            try {
                res = solid.makeElementThickSolid(*faces,
                                                  thickness,
                                                  tol,
                                                  intersection,
                                                  false,
                                                  mode,
                                                  static_cast<Part::JoinType>(join));
                shapes.push_back(res);
            }
            catch (Standard_Failure& e) {
                FC_ERR("Exception on making thick solid: " << e.GetMessageString());
                return new App::DocumentObjectExecReturn("Failed to make thick solid");
            }
            if (mapIterator != closeFaces.end()) {
                ++mapIterator;
            }
        }
    }

    TopoShape result(0);
    if (shapes.size() > 1) {
        result.makeElementFuse(shapes);
    } else if (shapes.empty()) {
        result = TopShape;
    } else {
        result = shapes.front();
    }
    // store shape before refinement
    this->rawShape = result;
    result = refineShapeIfActive(result);
    this->Shape.setValue(getSolid(result));
    return App::DocumentObject::StdReturn;
}
