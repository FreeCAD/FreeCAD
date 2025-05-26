// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Manuel Apeltauer, direkt cnc-systeme GmbH          *
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepProj_Projection.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeFix_Wireframe.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <sstream>
#endif

#include "FeatureProjectOnSurface.h"
#include <Base/Exception.h>


using namespace Part;

PROPERTY_SOURCE(Part::ProjectOnSurface, Part::Feature)
static std::array<const char*, 4> modes = {"All", "Faces", "Edges", nullptr};  // NOLINT

ProjectOnSurface::ProjectOnSurface()
{
    ADD_PROPERTY_TYPE(Mode,(0L), "Projection", App::Prop_None, "Projection mode");
    Mode.setEnums(modes.data());
    ADD_PROPERTY_TYPE(Height,(0.0), "Projection", App::Prop_None, "Extrusion height");
    ADD_PROPERTY_TYPE(Offset,(0.0), "Projection", App::Prop_None, "Offset of solid");
    ADD_PROPERTY_TYPE(Direction,(Base::Vector3d(0, 0, 1)), "Projection", App::Prop_None, "Direction of projection");
    ADD_PROPERTY_TYPE(SupportFace,(nullptr), "Projection", App::Prop_None, "Support faceo");
    ADD_PROPERTY_TYPE(Projection,(nullptr), "Projection", App::Prop_None, "Shapes to project onto support face");
}

App::DocumentObjectExecReturn* ProjectOnSurface::execute()
{
    try {
        tryExecute();
        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& error) {
        throw Base::ValueError(error.GetMessageString());
    }
}

void ProjectOnSurface::tryExecute()
{
    TopoDS_Face supportFace = getSupportFace();

    std::vector<TopoDS_Shape> shapes = getProjectionShapes();
    const auto& vec = Direction.getValue();
    gp_Dir dir(vec.x, vec.y, vec.z);

    std::vector<TopoDS_Shape> results;
    for (const auto& shape : shapes) {
        auto shapes = createProjectedWire(shape, supportFace, dir);
        results.insert(results.end(), shapes.begin(), shapes.end());
    }

    results = filterShapes(results);
    auto currentPlacement = Placement.getValue();
    Shape.setValue(createCompound(results));
    Placement.setValue(currentPlacement);
}

TopoDS_Face ProjectOnSurface::getSupportFace() const
{
    auto support = SupportFace.getValue<Part::Feature*>();
    if (!support) {
        throw Base::ValueError("No support face specified");
    }

    std::vector<std::string> subStrings = SupportFace.getSubValues();
    if (subStrings.size() != 1) {
        throw Base::ValueError("Expect exactly one support face");
    }

    auto topoSupport = Feature::getTopoShape(support,
                                                  ShapeOption::NeedSubElement
                                                | ShapeOption::ResolveLink
                                                | ShapeOption::Transform,
                                             subStrings[0].c_str());
    return TopoDS::Face(topoSupport.getShape());
}

std::vector<TopoDS_Shape> ProjectOnSurface::getProjectionShapes() const
{
    std::vector<TopoDS_Shape> shapes;
    auto objects = Projection.getValues();
    auto subvalues = Projection.getSubValues();
    if (objects.size() != subvalues.size()) {
        throw Base::ValueError("Number of objects and sub-names differ");
    }

    for (std::size_t index = 0; index < objects.size(); index++) {
        auto topoSupport = Feature::getTopoShape(objects[index],
                                                    ShapeOption::NeedSubElement
                                                  | ShapeOption::ResolveLink
                                                  | ShapeOption::Transform,
                                                 subvalues[index].c_str());
        shapes.push_back(topoSupport.getShape());
    }

    return shapes;
}

std::vector<TopoDS_Shape>
ProjectOnSurface::filterShapes(const std::vector<TopoDS_Shape>& shapes) const
{
    std::vector<TopoDS_Shape> filtered;
    const char* mode = Mode.getValueAsString();
    if (strcmp(mode, "All") == 0) {
        for (const auto& it : shapes) {
            if (!it.IsNull()) {
                filtered.push_back(it);
            }
        }
    }
    else if (strcmp(mode, "Faces") == 0) {
        for (const auto& it : shapes) {
            if (!it.IsNull() && it.ShapeType() == TopAbs_FACE) {
                filtered.push_back(it);
            }
        }
    }
    else if (strcmp(mode, "Edges") == 0) {
        for (const auto& it : shapes) {
            if (it.IsNull()) {
                continue;
            }
            if (it.ShapeType() == TopAbs_EDGE || it.ShapeType() == TopAbs_WIRE) {
                filtered.push_back(it);
            }
            else if (it.ShapeType() == TopAbs_FACE) {
                auto wires = getWires(TopoDS::Face(it));
                for (const auto& jt : wires) {
                    filtered.push_back(jt);
                }
            }
        }
    }

    return filtered;
}

TopoDS_Shape ProjectOnSurface::createCompound(const std::vector<TopoDS_Shape>& shapes)
{
    TopLoc_Location loc = getOffsetPlacement();
    bool isIdentity = loc.IsIdentity();
    TopoDS_Compound aCompound;
    if (!shapes.empty()) {
        TopoDS_Builder aBuilder;
        aBuilder.MakeCompound(aCompound);
        for (const auto& it : shapes) {
            if (isIdentity) {
                aBuilder.Add(aCompound, it);
            }
            else {
                aBuilder.Add(aCompound, it.Moved(loc));
            }
        }
    }
    return {std::move(aCompound)};
}

std::vector<TopoDS_Shape> ProjectOnSurface::createProjectedWire(const TopoDS_Shape& shape,
                                                                const TopoDS_Face& supportFace,
                                                                const gp_Dir& dir)
{
    if (shape.IsNull()) {
        return {};
    }
    if (shape.ShapeType() == TopAbs_FACE) {
        auto wires = projectFace(TopoDS::Face(shape), supportFace, dir);
        auto face = createFaceFromWire(wires, supportFace);
        auto face_or_solid = createSolidIfHeight(face);
        if (!face_or_solid.IsNull()) {
            return {face_or_solid};
        }
        if (!face.IsNull()) {
            return {face};
        }

        return wires;
    }
    if (shape.ShapeType() == TopAbs_WIRE || shape.ShapeType() == TopAbs_EDGE) {
        return projectWire(shape, supportFace, dir);
    }

    return {};
}

TopoDS_Face ProjectOnSurface::createFaceFromWire(const std::vector<TopoDS_Shape>& wires,
                                                 const TopoDS_Face& supportFace) const
{
    if (wires.empty()) {
        return {};
    }

    std::vector<TopoDS_Wire> wiresInParametricSpace = createWiresFromWires(wires, supportFace);
    return createFaceFromParametricWire(wiresInParametricSpace, supportFace);
}

TopoDS_Face
ProjectOnSurface::createFaceFromParametricWire(const std::vector<TopoDS_Wire>& wires,
                                               const TopoDS_Face& supportFace) const
{
    auto surface = BRep_Tool::Surface(supportFace);

    // try to create a face from the wires
    // the first wire is the otherwise
    // the following wires are the inside wires
    BRepBuilderAPI_MakeFace faceMaker;
    bool first = true;
    for (const auto& wire : wires) {
        if (first) {
            first = false;
            // change the wire direction, otherwise no face is created
            auto currentWire = TopoDS::Wire(wire.Reversed());
            if (supportFace.Orientation() == TopAbs_REVERSED) {
                currentWire = wire;
            }
            faceMaker = BRepBuilderAPI_MakeFace(surface, currentWire);
            ShapeFix_Face fix(faceMaker.Face());
            fix.Perform();
            auto aFace = fix.Face();
            BRepCheck_Analyzer aChecker(aFace);
            if (!aChecker.IsValid()) {
                faceMaker = BRepBuilderAPI_MakeFace(surface, TopoDS::Wire(currentWire.Reversed()));
            }
        }
        else {
            // make a copy of the current face maker
            // if the face fails just try again with the copy
            TopoDS_Face tempCopy = BRepBuilderAPI_MakeFace(faceMaker.Face()).Face();
            faceMaker.Add(TopoDS::Wire(wire.Reversed()));
            ShapeFix_Face fix(faceMaker.Face());
            fix.Perform();
            auto aFace = fix.Face();
            BRepCheck_Analyzer aChecker(aFace);
            if (!aChecker.IsValid()) {
                faceMaker = BRepBuilderAPI_MakeFace(tempCopy);
                faceMaker.Add(TopoDS::Wire(wire));
            }
        }
    }
    // auto doneFlag = faceMaker.IsDone();
    // auto error = faceMaker.Error();
    return faceMaker.Face();
}

std::vector<TopoDS_Wire>
ProjectOnSurface::createWiresFromWires(const std::vector<TopoDS_Shape>& wires,
                                       const TopoDS_Face& supportFace) const
{
    auto surface = BRep_Tool::Surface(supportFace);

    // create a wire of all edges in parametric space on the surface of the face to
    // projected
    //  --> otherwise BRepBuilderAPI_MakeFace can not make a face from the wire!
    std::vector<TopoDS_Wire> wiresInParametricSpace;
    for (const auto& wire : wires) {
        std::vector<TopoDS_Shape> edges;
        for (TopExp_Explorer xp(wire, TopAbs_EDGE); xp.More(); xp.Next()) {
            edges.push_back(TopoDS::Edge(xp.Current()));
        }
        if (edges.empty()) {
            continue;
        }

        std::vector<TopoDS_Edge> edgesInParametricSpace;
        for (const auto& edge : edges) {
            Standard_Real first {};
            Standard_Real last {};
            auto currentCurve = BRep_Tool::CurveOnSurface(TopoDS::Edge(edge),
                                                          supportFace,
                                                          first,
                                                          last);
            if (!currentCurve) {
                continue;
            }

            BRepBuilderAPI_MakeEdge mkEdge(currentCurve, surface, first, last);
            auto edgeInParametricSpace = mkEdge.Edge();
            edgesInParametricSpace.push_back(edgeInParametricSpace);
        }

        auto aWire = fixWire(edgesInParametricSpace, supportFace);
        wiresInParametricSpace.push_back(aWire);
    }

    return wiresInParametricSpace;
}

TopoDS_Shape ProjectOnSurface::createSolidIfHeight(const TopoDS_Face& face) const
{
    if (face.IsNull()) {
        return face;
    }
    double height = Height.getValue();
    if (height < Precision::Confusion() || Mode.getValue() != 0L) {
        return face;
    }

    const auto& vec = Direction.getValue();
    gp_Vec directionToExtrude(vec.x, vec.y, vec.z);
    directionToExtrude.Reverse();
    directionToExtrude.Multiply(height);

    BRepPrimAPI_MakePrism extrude(face, directionToExtrude);
    return extrude.Shape();
}

std::vector<TopoDS_Wire> ProjectOnSurface::getWires(const TopoDS_Face& face) const
{
    std::vector<TopoDS_Wire> wires;
    auto outerWire = ShapeAnalysis::OuterWire(face);
    wires.push_back(outerWire);
    for (TopExp_Explorer xp(face, TopAbs_WIRE); xp.More(); xp.Next()) {
        auto currentWire = TopoDS::Wire(xp.Current());
        if (!currentWire.IsSame(outerWire)) {
            wires.push_back(currentWire);
        }
    }

    return wires;
}

TopoDS_Wire ProjectOnSurface::fixWire(const TopoDS_Shape& shape,
                                      const TopoDS_Face& supportFace) const
{
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer xp(shape, TopAbs_EDGE); xp.More(); xp.Next()) {
        edges.push_back(TopoDS::Edge(xp.Current()));
    }
    return fixWire(edges, supportFace);
}

TopoDS_Wire ProjectOnSurface::fixWire(const std::vector<TopoDS_Edge>& edges,
                                      const TopoDS_Face& supportFace) const
{
    // try to sort and heal all wires
    // if the wires are not clean making a face will fail!
    ShapeAnalysis_FreeBounds shapeAnalyzer;
    Handle(TopTools_HSequenceOfShape) shapeList = new TopTools_HSequenceOfShape;
    Handle(TopTools_HSequenceOfShape) aWireHandle;
    Handle(TopTools_HSequenceOfShape) aWireWireHandle;

    for (const auto& it : edges) {
        shapeList->Append(it);
    }

    const double tolerance = 0.0001;
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(shapeList, tolerance, false, aWireHandle);
    ShapeAnalysis_FreeBounds::ConnectWiresToWires(aWireHandle, tolerance, false, aWireWireHandle);
    if (!aWireWireHandle) {
        return {};
    }
    for (auto it = 1; it <= aWireWireHandle->Length(); ++it) {
        auto aShape = TopoDS::Wire(aWireWireHandle->Value(it));
        ShapeFix_Wire aWireRepair(aShape, supportFace, tolerance);
        aWireRepair.FixAddCurve3dMode() = 1;
        aWireRepair.FixAddPCurveMode() = 1;
        aWireRepair.Perform();

        ShapeFix_Wireframe aWireFramFix(aWireRepair.Wire());
        aWireFramFix.FixWireGaps();
        aWireFramFix.FixSmallEdges();
        return TopoDS::Wire(aWireFramFix.Shape());
    }
    return {};
}

namespace {
TopoDS_Wire getProjectedWire(BRepProj_Projection& projection, const TopoDS_Shape& reference)
{
    double minDistance = std::numeric_limits<double>::max();
    TopoDS_Wire wireToTake;
    for (; projection.More(); projection.Next()) {
        auto it = projection.Current();
        BRepExtrema_DistShapeShape distanceMeasure(it, reference);
        distanceMeasure.Perform();
        auto currentDistance = distanceMeasure.Value();
        if (currentDistance > minDistance) {
            continue;
        }
        wireToTake = it;
        minDistance = currentDistance;
    }

    return wireToTake;
}
}

std::vector<TopoDS_Shape> ProjectOnSurface::projectFace(const TopoDS_Face& face,
                                                        const TopoDS_Face& supportFace,
                                                        const gp_Dir& dir)
{
    std::vector<TopoDS_Shape> shapes;
    std::vector<TopoDS_Wire> wires = getWires(face);
    for (const auto& wire : wires) {
        BRepProj_Projection aProjection(wire, supportFace, dir);
        TopoDS_Wire wireToTake = getProjectedWire(aProjection, face);
        auto aWire = fixWire(wireToTake, supportFace);
        shapes.push_back(aWire);
    }

    return shapes;
}

std::vector<TopoDS_Shape> ProjectOnSurface::projectWire(const TopoDS_Shape& wire,
                                                        const TopoDS_Face& supportFace,
                                                        const gp_Dir& dir)
{
    std::vector<TopoDS_Shape> shapes;
    BRepProj_Projection aProjection(wire, supportFace, dir);
    TopoDS_Wire wireToTake = getProjectedWire(aProjection, wire);
    for (TopExp_Explorer xp(wireToTake, TopAbs_EDGE); xp.More(); xp.Next()) {
        shapes.push_back(TopoDS::Edge(xp.Current()));
    }

    return shapes;
}

TopLoc_Location ProjectOnSurface::getOffsetPlacement() const
{
    double offset = Offset.getValue();
    if (offset == 0) {
        return {};
    }

    auto vec = Direction.getValue();
    vec.Normalize();
    vec.Scale(offset, offset, offset);
    Base::Matrix4D mat;
    mat.move(vec);
    gp_Trsf move = TopoShape::convert(mat);
    return TopLoc_Location(move);
}

const char* ProjectOnSurface::getViewProviderName() const
{
    return "PartGui::ViewProviderProjectOnSurface";
}
