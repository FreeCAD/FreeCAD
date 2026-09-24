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
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>

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
 * Two exact one-sided walls are built at half the requested thickness in
 * each direction and regular-fused across their shared source shell.
 */
Part::TopoShape makeRectoVersoThickness(
    const Part::TopoShape& solid,
    const std::vector<Part::TopoShape>& closingFaces,
    double thickness,
    double tolerance,
    bool intersection,
    Part::JoinType join,
    long tag
)
{
    const double distance = std::abs(thickness) / 2.0;
    if (distance <= tolerance) {
        throw Base::CADKernelError("Recto-verso half-thickness must exceed the modeling tolerance");
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

const char* Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char* Thickness::JoinEnums[] = {"Arc", "Intersection", nullptr};
const char* Thickness::SelectionEnums[] = {"Selected Faces", "Selected Solids", "All Solids", nullptr};

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
    ADD_PROPERTY_TYPE(Selection, (0L), "Thickness", App::Prop_None, "Selection Type");
    Selection.setEnums(SelectionEnums);
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

    TopoShape topShape;
    try {
        topShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // Set transform to identity so OCC will perform this operation
    // in local coordinates.
    topShape.setTransform(Base::Matrix4D());

    if (auto* base = getBaseObject(/* silent = */ true)) {
        Placement.setValue(base->Placement.getValue());
    }

    const std::vector<std::string>& subStrings = Base.getSubValues(true);

    const double tolerance = Precision::Confusion();
    const bool reversed = Reversed.getValue();

    auto join = static_cast<int>(Join.getValue());

    // We do not offer tangent join type.
    if (join == 1) {
        join = 2;
    }

    TopoShape result;

    ThicknessParameters params {
        topShape,
        result,
        subStrings,
        {},
        (reversed ? -1. : 1.) * Value.getValue(),
        tolerance,
        Intersection.getValue(),
        static_cast<int16_t>(Mode.getValue()),
        join,
        static_cast<int>(topShape.countSubShapes(TopAbs_SOLID))
    };

    if (auto* error = identifySolids(params)) {
        return error;
    }

    const auto selectionMode = static_cast<SelectionMode>(Selection.getValue());

    App::DocumentObjectExecReturn* error = nullptr;

    switch (selectionMode) {
        case SelectionMode::SelectedFaces:
            error = executeSelectedFaces(params);
            break;

        case SelectionMode::SelectedSolids:
            error = executeSelectedSolids(params);
            break;

        case SelectionMode::AllSolids:
            error = executeAllSolids(params);
            break;
    }

    if (error) {
        return error;
    }

    this->rawShape = result;

    std::vector<Part::TopoShape> solids;

    for (TopExp_Explorer exp(result.getShape(), TopAbs_SOLID);
         exp.More();
         exp.Next()) {

        Part::TopoShape solid;
        solid.setShape(exp.Current());
        solids.push_back(std::move(solid));
    }

    TopoShape final;
    final.makeElementFuse(solids);
    final = refineShapeIfActive(final);

    this->Shape.setValue(getSolid(final));

    return App::DocumentObject::StdReturn;
}

App::DocumentObjectExecReturn* Thickness::identifySolids(ThicknessParameters& params)
{
    if (!params.solidCount) {
        return new App::DocumentObjectExecReturn("No solid");
    }

    for (const auto& subString : params.subStrings) {
        TopoDS_Shape face;

        try {
            face = params.input.getSubShape(subString.c_str());
        }
        catch (...) {
        }

        if (face.IsNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Invalid face reference")
            );
        }

        if (face.ShapeType() != TopAbs_FACE) {
            FC_WARN(getFullName() << ": Ignore non-face selection " << subString);
            continue;
        }

        const int solidIndex = params.input.findAncestor(face, TopAbs_SOLID);

        if (!solidIndex) {
            FC_WARN(getFullName() << ": Ignore face not belonging to a solid " << subString);
            continue;
        }

        params.closeFaces[solidIndex].emplace_back(face);
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Thickness::executeSelectedFaces(ThicknessParameters& params)
{
    if (fabs(params.thickness) <= 2 * params.tolerance) {
        params.result = params.input;
        return nullptr;
    }

    std::vector<TopoShape> shapes;
    shapes.reserve(params.solidCount);

    const auto joinType = static_cast<Part::JoinType>(params.join);

    for (int solidIndex = 1; solidIndex <= params.solidCount; ++solidIndex) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        const auto it = params.closeFaces.find(solidIndex);

        // Solid is unaffected: keep it unchanged.
        if (it == params.closeFaces.end()) {
            shapes.push_back(solid);
            continue;
        }

        try {
            TopoShape result;

            if (params.mode == BRepOffset_RectoVerso) {
                result = makeRectoVersoThickness(
                    solid,
                    it->second,
                    params.thickness,
                    params.tolerance,
                    params.intersection,
                    joinType,
                    getID()
                );
            }
            else {
                result = solid.makeElementThickSolid(
                    it->second,
                    params.thickness,
                    params.tolerance,
                    params.intersection,
                    false,
                    params.mode,
                    joinType
                );
            }

            if (!result.isNull()) {
                shapes.push_back(result);
            }
        }
        catch (Standard_Failure& e) {
            FC_ERR("Exception on making thick solid: " << e.GetMessageString());

            return new App::DocumentObjectExecReturn("Failed to make thick solid");
        }
    }

    params.result.makeCompound(shapes);

    return nullptr;
}

App::DocumentObjectExecReturn* Thickness::executeSelectedSolids(ThicknessParameters& params)
{
    if (fabs(params.thickness) <= 2 * params.tolerance) {
        params.result = params.input;
        return nullptr;
    }

    std::vector<TopoShape> shapes;
    shapes.reserve(params.solidCount);

    for (int solidIndex = 1; solidIndex <= params.solidCount; ++solidIndex) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        // Solid is not affected.
        if (!params.closeFaces.contains(solidIndex)) {
            shapes.push_back(solid);
            continue;
        }

        try {
            TopoShape shell = makeSolidShell(solid, params);

            if (shell.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            shapes.push_back(shell);
        }
        catch (Standard_Failure& e) {
            FC_ERR("Exception on making solid shell: " << e.GetMessageString());

            return new App::DocumentObjectExecReturn("Failed to make solid shell");
        }
    }

    params.result.makeCompound(shapes);

    return nullptr;
}

App::DocumentObjectExecReturn* Thickness::executeAllSolids(ThicknessParameters& params)
{
    if (fabs(params.thickness) <= 2 * params.tolerance) {
        params.result = params.input;
        return nullptr;
    }

    std::vector<TopoShape> shapes;
    shapes.reserve(params.solidCount);

    for (int solidIndex = 1; solidIndex <= params.solidCount; ++solidIndex) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        try {
            TopoShape shell = makeSolidShell(solid, params);

            if (shell.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            shapes.push_back(shell);
        }
        catch (Standard_Failure& e) {
            FC_ERR("Exception on making solid shell: " << e.GetMessageString());

            return new App::DocumentObjectExecReturn("Failed to make solid shell");
        }
    }

    params.result.makeCompound(shapes);

    return nullptr;
}

TopoShape Thickness::makeSolidShell(const TopoShape& solid, const ThicknessParameters& params)
{
    const double thickness = params.thickness;

    if (params.mode == BRepOffset_RectoVerso) {
        const double halfThickness = 0.5 * fabs(thickness);

        const auto outerOffset = solid.makeOffsetShape(
            halfThickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        const auto innerOffset = solid.makeOffsetShape(
            -halfThickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        TopoShape outer(outerOffset);
        TopoShape inner(innerOffset);

        if (outer.isNull() || inner.isNull()) {
            return {};
        }

        return outer.makeElementCut(inner);
    }

    // Skin
    if (thickness > 0.0) {
        // Normal: thickness goes outward.
        const auto outerOffset = solid.makeOffsetShape(
            thickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        TopoShape outer(outerOffset);

        if (outer.isNull()) {
            return {};
        }

        // Outer offset minus original solid = outward shell.
        return outer.makeElementCut(solid);
    }

    // Reversed: thickness goes inward.
    const auto innerOffset = solid.makeOffsetShape(
        thickness,
        params.tolerance,
        params.intersection,
        false,
        BRepOffset_Skin,
        params.join
    );

    TopoShape inner(innerOffset);

    if (inner.isNull()) {
        return {};
    }

    // Original solid minus inner offset = inward shell.
    return solid.makeElementCut(inner);
}

void Thickness::updatePreviewShape()
{
    TopoShape topShape;

    try {
        topShape = getBaseTopoShape();
    }
    catch (Base::Exception&) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    if (topShape.isNull()) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    topShape.setTransform(Base::Matrix4D());

    const std::vector<std::string>& subStrings = Base.getSubValues(true);

    const double tolerance = Precision::Confusion();
    const bool reversed = Reversed.getValue();

    auto join = static_cast<int>(Join.getValue());

    // We do not offer tangent join type.
    if (join == 1) {
        join = 2;
    }

    TopoShape result;

    ThicknessParameters params {
        topShape,
        result,
        subStrings,
        {},
        (reversed ? -1. : 1.) * Value.getValue(),
        tolerance,
        Intersection.getValue(),
        static_cast<int16_t>(Mode.getValue()),
        join,
        static_cast<int>(topShape.countSubShapes(TopAbs_SOLID))
    };

    if (identifySolids(params)) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    if (fabs(params.thickness) <= 2 * params.tolerance) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    std::vector<TopoShape> previewShapes;
    previewShapes.reserve(params.solidCount);

    switch (static_cast<SelectionMode>(Selection.getValue())) {
        case SelectionMode::SelectedFaces:
            updatePreviewSelectedFaces(params, previewShapes);
            break;

        case SelectionMode::SelectedSolids:
            updatePreviewSelectedSolids(params, previewShapes);
            break;

        case SelectionMode::AllSolids:
            updatePreviewAllSolids(params, previewShapes);
            break;
    }

    if (previewShapes.empty()) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    TopoShape preview;
    preview.makeCompound(previewShapes);

    PreviewShape.setValue(preview);
}

TopoShape Thickness::makeSolidPreview(const TopoShape& solid, const ThicknessParameters& params)
{
    const double thickness = params.thickness;

    if (params.mode == BRepOffset_RectoVerso) {
        const double halfThickness = 0.5 * fabs(thickness);

        const auto outerOffset = solid.makeOffsetShape(
            halfThickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        const auto innerOffset = solid.makeOffsetShape(
            -halfThickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        TopoShape outer(outerOffset);
        TopoShape inner(innerOffset);

        if (outer.isNull() || inner.isNull()) {
            return {};
        }

        // Only the geometry added outside the original solid.
        TopoShape outerDelta = outer.makeElementCut(solid);

        // Only the geometry removed inside the original solid.
        TopoShape innerDelta = solid.makeElementCut(inner);

        if (outerDelta.isNull() || innerDelta.isNull()) {
            return {};
        }

        TopoShape result;
        result.makeCompound({outerDelta, innerDelta});

        return result;
    }

    if (thickness > 0.0) {
        // Outward thickness:
        // show only the material added outside the original solid.
        const auto offset = solid.makeOffsetShape(
            thickness,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        );

        TopoShape outer(offset);

        if (outer.isNull()) {
            return {};
        }

        return outer.makeElementCut(solid);
    }

    // Inward thickness:
    // show the volume that becomes the cavity.
    const auto offset = solid.makeOffsetShape(
        thickness,
        params.tolerance,
        params.intersection,
        false,
        BRepOffset_Skin,
        params.join
    );

    TopoShape inner(offset);

    if (inner.isNull()) {
        return {};
    }

    return inner;
}

void Thickness::updatePreviewSelectedFaces(
    ThicknessParameters& params,
    std::vector<TopoShape>& previewShapes
)
{
    const auto joinType = static_cast<Part::JoinType>(params.join);

    for (const auto& [solidIndex, faces] : params.closeFaces) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        try {
            TopoShape result;

            if (params.mode == BRepOffset_RectoVerso) {
                result = makeRectoVersoThickness(
                    solid,
                    faces,
                    params.thickness,
                    params.tolerance,
                    params.intersection,
                    joinType,
                    getID()
                );
            }
            else {
                result = solid.makeElementThickSolid(
                    faces,
                    params.thickness,
                    params.tolerance,
                    params.intersection,
                    false,
                    params.mode,
                    joinType
                );
            }

            if (result.isNull()) {
                continue;
            }

            TopoShape preview;

            if (params.thickness > 0.0) {
                // Only added material.
                preview = result.makeElementCut(solid);
            }
            else {
                // Only removed material / cavity.
                preview = solid.makeElementCut(result);
            }

            if (!preview.isNull()) {
                previewShapes.push_back(preview);
            }
        }
        catch (Standard_Failure& e) {
            FC_WARN("Exception while creating Thickness preview: " << e.GetMessageString());
        }
    }
}

void Thickness::updatePreviewSelectedSolids(
    ThicknessParameters& params,
    std::vector<TopoShape>& previewShapes
)
{
    for (const auto& [solidIndex, faces] : params.closeFaces) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        try {
            TopoShape preview = makeSolidPreview(solid, params);

            if (!preview.isNull()) {
                previewShapes.push_back(preview);
            }
        }
        catch (Standard_Failure& e) {
            FC_WARN("Exception while creating Thickness preview: " << e.GetMessageString());
        }
    }
}

void Thickness::updatePreviewAllSolids(ThicknessParameters& params, std::vector<TopoShape>& previewShapes)
{
    for (int solidIndex = 1; solidIndex <= params.solidCount; ++solidIndex) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        try {
            TopoShape preview = makeSolidPreview(solid, params);

            if (!preview.isNull()) {
                previewShapes.push_back(preview);
            }
        }
        catch (Standard_Failure& e) {
            FC_WARN("Exception while creating Thickness preview: " << e.GetMessageString());
        }
    }
}
