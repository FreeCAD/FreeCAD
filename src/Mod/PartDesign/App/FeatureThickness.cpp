// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include <BRepOffset_Mode.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>


#include <Base/Exception.h>
#include "FeatureThickness.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

namespace
{
void ensureValidWall(const Part::TopoShape& wall, const char* message)
{
    if (wall.isNull() || !wall.isValid() || wall.countSubShapes(TopAbs_SOLID) != 1) {
        throw Base::CADKernelError(message);
    }
}

/** Build a wall centered on the retained shell of a solid.
 *
 * The closing faces are removed by the ordinary skin-thickness operation.
 * Two exact one-sided walls are built at +offset and -offset and regular-fused
 * across their shared source shell. Consequently, offset is the distance on
 * each side and the total wall thickness is twice its absolute value.
 */
Part::TopoShape makeRectoVersoThickness(
    const Part::TopoShape& solid,
    const std::vector<Part::TopoShape>& closingFaces,
    double offset,
    double tolerance,
    bool intersection,
    Part::JoinType join,
    long tag
)
{
    const double distance = std::abs(offset);
    if (distance <= tolerance) {
        throw Base::CADKernelError("Recto-verso thickness must exceed the modeling tolerance");
    }

    // Signed offsets are only meaningful for consistently oriented solids.
    // Imported and programmatically constructed solids are not guaranteed to
    // have that orientation, so normalize it without resetting element names.
    Part::TopoShape orientedSolid = solid;
    orientedSolid.fixSolidOrientation();

    constexpr auto skinMode = static_cast<short>(BRepOffset_Skin);
    Part::TopoShape recto = orientedSolid.makeElementThickSolid(
        closingFaces,
        distance,
        tolerance,
        intersection,
        false,
        skinMode,
        join,
        "RectoVersoRecto"
    );
    Part::TopoShape verso = orientedSolid.makeElementThickSolid(
        closingFaces,
        -distance,
        tolerance,
        intersection,
        false,
        skinMode,
        join,
        "RectoVersoVerso"
    );
    ensureValidWall(recto, "Recto-verso positive-side wall is invalid");
    ensureValidWall(verso, "Recto-verso negative-side wall is invalid");

    Part::TopoShape result(tag);
    result.makeElementFuse({recto, verso}, "RectoVerso", tolerance);
    if (result.isNull() || !result.isValid() || result.countSubShapes(TopAbs_SOLID) != 1) {
        throw Base::CADKernelError("Recto-verso thickness produced an invalid solid");
    }
    return result;
}
}  // namespace

const char* PartDesign::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char* PartDesign::Thickness::JoinEnums[] = {"Arc", "Intersection", nullptr};

PROPERTY_SOURCE(PartDesign::Thickness, PartDesign::DressUp)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Value, (1.0), "Thickness", App::Prop_None, "Thickness value");
    ADD_PROPERTY_TYPE(Mode, (0L), "Thickness", App::Prop_None, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (0L), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(
        Reversed,
        (true),
        "Thickness",
        App::Prop_None,
        "Apply the thickness towards the solids interior"
    );
    ADD_PROPERTY_TYPE(Intersection, (false), "Thickness", App::Prop_None, "Enable intersection-handling");
}

int16_t Thickness::mustExecute() const
{
    if (Placement.isTouched() || Value.isTouched() || Mode.isTouched() || Join.isTouched()) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Thickness::execute()
{
    if (onlyHaveRefined()) {
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

    // Set transform to identity so occ will perform this operation
    // in local coordinates
    TopShape.setTransform(Base::Matrix4D());
    if (auto* base = getBaseObject(/* silent = */ true)) {
        Placement.setValue(base->Placement.getValue());
    }

    const std::vector<std::string>& subStrings = Base.getSubValues(true);

    // If the base has no sub elements listed just return a copy of the base.
    if (subStrings.empty()) {
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    std::map<int, std::vector<TopoShape>> closeFaces;
    for (const auto& it : subStrings) {
        TopoDS_Shape face;
        try {
            face = TopShape.getSubShape(it.c_str());
        }
        catch (...) {
        }
        if (face.IsNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Invalid face reference")
            );
        }
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
    if (!count) {
        return new App::DocumentObjectExecReturn("No solid");
    }
    // we do not offer tangent join type
    if (join == 1) {
        join = 2;
    }

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
                const auto joinType = static_cast<Part::JoinType>(join);
                if (mode == BRepOffset_RectoVerso) {
                    res = makeRectoVersoThickness(
                        solid,
                        *faces,
                        thickness,
                        tol,
                        intersection,
                        joinType,
                        getID()
                    );
                }
                else {
                    res = solid.makeElementThickSolid(
                        *faces,
                        thickness,
                        tol,
                        intersection,
                        false,
                        mode,
                        joinType
                    );
                }
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
    }
    else if (shapes.empty()) {
        result = TopShape;
    }
    else {
        result = shapes.front();
    }
    // store shape before refinement
    this->rawShape = result;
    result = refineShapeIfActive(result);
    this->Shape.setValue(getSolid(result));
    return App::DocumentObject::StdReturn;
}
