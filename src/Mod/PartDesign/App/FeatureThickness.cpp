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

/** Build a wall on the retained shell of a solid.
 *
 * The closing faces are removed by the ordinary skin-thickness operation.
 *
 * `centering` controls where the requested thickness lies relative to the
 * original shell:
 *
 *   -1 = completely inside
 *    0 = centered on the shell
 *   +1 = completely outside
 *
 * The total wall thickness is always `thickness`.
 */
Part::TopoShape makeRectoVersoThickness(
    const Part::TopoShape& solid,
    const std::vector<Part::TopoShape>& closingFaces,
    double thickness,
    double centering,
    double tolerance,
    bool intersection,
    Part::JoinType join,
    long tag
)
{
    if (std::abs(centering) > 1.0) {
        throw Base::CADKernelError("Recto-verso centering must be in the range [-1, 1]");
    }

    const double totalThickness = std::abs(thickness);
    if (totalThickness <= tolerance) {
        throw Base::CADKernelError("Recto-verso thickness must exceed the modeling tolerance");
    }

    // Map [-1, 1] to [0, 1].
    //
    // centering = -1 -> 0.0  => all thickness inside
    // centering =  0 -> 0.5  => half on each side
    // centering = +1 -> 1.0  => all thickness outside
    const double outsideFraction = (centering + 1.0) / 2.0;

    const double outsideDistance = totalThickness * outsideFraction;
    const double insideDistance = totalThickness - outsideDistance;

    // Signed offsets are only meaningful for consistently oriented solids.
    // Imported and programmatically constructed solids are not guaranteed to
    // have that orientation, so normalize it without resetting element names.
    Part::TopoShape orientedSolid = solid;
    orientedSolid.fixSolidOrientation();

    constexpr auto skinMode = static_cast<short>(BRepOffset_Skin);

    std::vector<Part::TopoShape> walls;

    if (outsideDistance > tolerance) {
        Part::TopoShape recto = orientedSolid.makeElementThickSolid(
            closingFaces,
            outsideDistance,
            tolerance,
            intersection,
            false,
            skinMode,
            join,
            "RectoVersoRecto"
        );

        ensureValidWall(recto, "Recto-verso positive-side wall is invalid");

        walls.push_back(std::move(recto));
    }

    if (insideDistance > tolerance) {
        Part::TopoShape verso = orientedSolid.makeElementThickSolid(
            closingFaces,
            -insideDistance,
            tolerance,
            intersection,
            false,
            skinMode,
            join,
            "RectoVersoVerso"
        );

        ensureValidWall(verso, "Recto-verso negative-side wall is invalid");

        walls.push_back(std::move(verso));
    }

    if (walls.empty()) {
        throw Base::CADKernelError("Recto-verso thickness produced no wall");
    }

    Part::TopoShape result(tag);

    if (walls.size() == 1) {
        result = std::move(walls.front());
    }
    else {
        result.makeElementFuse(walls, "RectoVerso", tolerance);
    }

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
    ADD_PROPERTY_TYPE(Mode, (0L), "Thickness", App::Prop_ReadOnly, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (0L), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(
        Reversed,
        (true),
        "Thickness",
        App::Prop_ReadOnly,
        "Apply the thickness towards the solids interior"
    );
    ADD_PROPERTY_TYPE(Intersection, (false), "Thickness", App::Prop_None, "Enable intersection-handling");
    ADD_PROPERTY_TYPE(Selection, (0L), "Thickness", App::Prop_None, "Selection Type");
    Selection.setEnums(SelectionEnums);
    ADD_PROPERTY_TYPE(
        Centering,
        (0.0),
        "Offset",
        App::Prop_None,
        "Offset factor to the existing faces [-1, 1]"
    );
    Centering.setConstraints(new App::PropertyFloatConstraint::Constraints(-1.0, 1.0, 0.01));
}

void Thickness::onDocumentRestored()
{
    Feature::onDocumentRestored();

    if (!Mode.isTouched()) {
        return;
    }

    const int mode = Mode.getValue();
    const double value = Value.getValue();

    if (mode == BRepOffset_RectoVerso) {
        Centering.setValue(0.0);
    }
    else if (Reversed.isTouched()) {
        Centering.setValue(Reversed.getValue() ? -1.0 : 1.0);

        Value.setValue(std::abs(value));
    }
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
        Centering.getValue(),
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

    for (TopExp_Explorer exp(result.getShape(), TopAbs_SOLID); exp.More(); exp.Next()) {
        Part::TopoShape solid;
        solid.setShape(exp.Current());
        solids.push_back(std::move(solid));
    }

    if (solids.empty()) {
        result = refineShapeIfActive(result);
        this->Shape.setValue(getSolid(result));
    }
    else {
        TopoShape final;
        final.makeElementFuse(solids);
        final = refineShapeIfActive(final);

        this->Shape.setValue(getSolid(final));
    }

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
            TopoShape result = makeRectoVersoThickness(
                solid,
                it->second,
                params.thickness,
                params.centering,
                params.tolerance,
                params.intersection,
                joinType,
                getID()
            );

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

    const double thickness = fabs(params.thickness);
    const double outsideDistance = thickness * (params.centering + 1.0) / 2.0;
    const double insideDistance = thickness - outsideDistance;

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
            TopoShape outer;
            TopoShape inner;

            if (outsideDistance > params.tolerance) {
                outer.setShape(solid.makeOffsetShape(
                    outsideDistance,
                    params.tolerance,
                    params.intersection,
                    false,
                    BRepOffset_Skin,
                    params.join
                ));
            }
            else {
                outer = solid;
            }

            if (insideDistance > params.tolerance) {
                inner.setShape(solid.makeOffsetShape(
                    -insideDistance,
                    params.tolerance,
                    params.intersection,
                    false,
                    BRepOffset_Skin,
                    params.join
                ));
            }
            else {
                inner = solid;
            }

            if (outer.isNull() || inner.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            TopoShape shell = outer.makeElementCut(inner);

            if (shell.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            shapes.push_back(std::move(shell));
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

    const double thickness = fabs(params.thickness);
    const double outsideDistance = thickness * (params.centering + 1.0) / 2.0;
    const double insideDistance = thickness - outsideDistance;

    std::vector<TopoShape> shapes;
    shapes.reserve(params.solidCount);

    for (int solidIndex = 1; solidIndex <= params.solidCount; ++solidIndex) {

        TopoShape solid = params.input.getSubTopoShape(TopAbs_SOLID, solidIndex);

        try {
            TopoShape outer;
            TopoShape inner;

            if (outsideDistance > params.tolerance) {
                outer.setShape(solid.makeOffsetShape(
                    outsideDistance,
                    params.tolerance,
                    params.intersection,
                    false,
                    BRepOffset_Skin,
                    params.join
                ));
            }
            else {
                outer = solid;
            }

            if (insideDistance > params.tolerance) {
                inner.setShape(solid.makeOffsetShape(
                    -insideDistance,
                    params.tolerance,
                    params.intersection,
                    false,
                    BRepOffset_Skin,
                    params.join
                ));
            }
            else {
                inner = solid;
            }

            if (outer.isNull() || inner.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            TopoShape shell = outer.makeElementCut(inner);

            if (shell.isNull()) {
                return new App::DocumentObjectExecReturn("Failed to make solid shell");
            }

            shapes.push_back(std::move(shell));
        }
        catch (Standard_Failure& e) {
            FC_ERR("Exception on making solid shell: " << e.GetMessageString());

            return new App::DocumentObjectExecReturn("Failed to make solid shell");
        }
    }

    params.result.makeCompound(shapes);

    return nullptr;
}

TopoShape Thickness::makePreviewDelta(
    const TopoShape& original,
    const TopoShape& result,
    const ThicknessParameters& params
)
{
    if (original.isNull() || result.isNull()) {
        return {};
    }

    const double centering = params.centering;

    // Completely inside:
    // show only the material removed from the original solid.
    if (centering <= -1.0) {
        return original.makeElementCut(result);
    }

    // Completely outside:
    // show only the material added to the original solid.
    if (centering >= 1.0) {
        return result.makeElementCut(original);
    }

    // Centered or partially shifted:
    // show both the added and removed material.
    TopoShape added = result.makeElementCut(original);
    TopoShape removed = original.makeElementCut(result);

    std::vector<TopoShape> changes;

    if (!added.isNull()) {
        changes.push_back(std::move(added));
    }

    if (!removed.isNull()) {
        changes.push_back(std::move(removed));
    }

    if (changes.empty()) {
        return {};
    }

    if (changes.size() == 1) {
        return std::move(changes.front());
    }

    TopoShape preview;
    preview.makeCompound(changes);

    return preview;
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
        std::abs(Value.getValue()),
        tolerance,
        Intersection.getValue(),
        Centering.getValue(),
        join,
        static_cast<int>(topShape.countSubShapes(TopAbs_SOLID))
    };

    if (identifySolids(params)) {
        PreviewShape.setValue(TopoShape());
        return;
    }

    if (params.thickness <= 2 * params.tolerance) {
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
    if (solid.isNull()) {
        return {};
    }

    const double thickness = params.thickness;

    const double outsideDistance = thickness * (params.centering + 1.0) / 2.0;

    const double insideDistance = thickness - outsideDistance;

    TopoShape outer;
    TopoShape inner;

    if (outsideDistance > params.tolerance) {
        outer.setShape(solid.makeOffsetShape(
            outsideDistance,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        ));
    }
    else {
        outer = solid;
    }

    if (insideDistance > params.tolerance) {
        inner.setShape(solid.makeOffsetShape(
            -insideDistance,
            params.tolerance,
            params.intersection,
            false,
            BRepOffset_Skin,
            params.join
        ));
    }
    else {
        inner = solid;
    }

    if (outer.isNull() || inner.isNull()) {
        return {};
    }

    TopoShape result = outer.makeElementCut(inner);

    if (result.isNull()) {
        return {};
    }

    return makePreviewDelta(solid, result, params);
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
            TopoShape result = makeRectoVersoThickness(
                solid,
                faces,
                params.thickness,
                params.centering,
                params.tolerance,
                params.intersection,
                joinType,
                getID()
            );

            if (result.isNull()) {
                continue;
            }

            TopoShape preview = makePreviewDelta(solid, result, params);

            if (!preview.isNull()) {
                previewShapes.push_back(std::move(preview));
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
                previewShapes.push_back(std::move(preview));
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
                previewShapes.push_back(std::move(preview));
            }
        }
        catch (Standard_Failure& e) {
            FC_WARN("Exception while creating Thickness preview: " << e.GetMessageString());
        }
    }
}
