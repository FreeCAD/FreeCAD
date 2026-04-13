// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 FreeCAD contributors                               *
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

#include "FaceMakerBuildFace.h"

#include <Bnd_Box.hxx>
#include <BOPAlgo_BuilderFace.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRepLib_FindSurface.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <Geom_Conic.hxx>
#include <Geom_Line.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAPI.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <algorithm>
#include <cmath>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("FaceMakerBuildFace", true, true)

TYPESYSTEM_SOURCE(Part::FaceMakerBuildFace, Part::FaceMakerPublic)

std::string Part::FaceMakerBuildFace::getUserFriendlyName() const
{
    return tr("BuildFace facemaker").toStdString();
}

std::string Part::FaceMakerBuildFace::getBriefExplanation() const
{
    return tr("Splits edges at intersections and finds all bounded face regions. "
              "Handles arbitrary overlapping geometry.")
        .toStdString();
}

void Part::FaceMakerBuildFace::setPlane(const gp_Pln& plane)
{
    myPlane = plane;
    planeSupplied = true;
}

// Split self-intersecting edges (e.g., figure-8 BSplines) at their crossing
// points.  BuilderAlgo only finds inter-edge intersections, so a single edge
// that crosses itself must be handled here.
// Records the mapping original → fragments in myPreSplitHistory so that
// postBuild() can chain it with mySplitter for proper element naming.
TopTools_ListOfShape Part::FaceMakerBuildFace::splitSelfIntersecting(
    const TopTools_ListOfShape& edges,
    const gp_Pln& plane
)
{
    const Standard_Real tol = Precision::Confusion();
    TopTools_ListOfShape result;

    for (TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(it.Value());
        try {
            Standard_Real first {}, last {};
            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
            if (curve.IsNull() || curve->IsKind(STANDARD_TYPE(Geom_Line))
                || curve->IsKind(STANDARD_TYPE(Geom_Conic))) {
                result.Append(edge);
                continue;
            }
            Handle(Geom2d_Curve) curve2d = GeomAPI::To2d(curve, plane);
            if (curve2d.IsNull()) {
                result.Append(edge);
                continue;
            }
            Geom2dAPI_InterCurveCurve selfInt(curve2d, tol);
            if (selfInt.NbPoints() == 0) {
                result.Append(edge);
                continue;
            }
            std::vector<Standard_Real> params;
            for (int i = 1; i <= selfInt.NbPoints(); i++) {
                Geom2dAPI_ProjectPointOnCurve proj(selfInt.Point(i), curve2d, first, last);
                for (int j = 1; j <= proj.NbPoints(); j++) {
                    Standard_Real p = proj.Parameter(j);
                    if (p - first > tol && last - p > tol) {
                        params.push_back(p);
                    }
                }
            }
            if (params.empty()) {
                result.Append(edge);
                continue;
            }
            std::sort(params.begin(), params.end());
            params.erase(
                std::unique(
                    params.begin(),
                    params.end(),
                    [tol](double a, double b) { return b - a < tol; }
                ),
                params.end()
            );
            Standard_Real prev = first;
            TopTools_ListOfShape fragments;
            for (Standard_Real p : params) {
                if (p - prev > tol) {
                    BRepBuilderAPI_MakeEdge me(curve, prev, p);
                    if (me.IsDone()) {
                        fragments.Append(me.Edge());
                    }
                    prev = p;
                }
            }
            if (last - prev > tol) {
                BRepBuilderAPI_MakeEdge me(curve, prev, last);
                if (me.IsDone()) {
                    fragments.Append(me.Edge());
                }
            }
            if (fragments.Size() > 0) {
                if (myPreSplitHistory.IsNull()) {
                    myPreSplitHistory = new BRepTools_History();
                }
                for (TopTools_ListIteratorOfListOfShape fi(fragments); fi.More(); fi.Next()) {
                    myPreSplitHistory->AddModified(edge, fi.Value());
                    result.Append(fi.Value());
                }
            }
            else {
                result.Append(edge);
            }
        }
        catch (const Standard_Failure& e) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("splitSelfIntersecting: " << e.GetMessageString());
            }
            result.Append(edge);
        }
        catch (...) {
            FC_WARN("splitSelfIntersecting: unknown exception");
            result.Append(edge);
        }
    }
    if (!myPreSplitHistory.IsNull()) {
        BRep_Builder builder;
        builder.MakeCompound(myPreSplitCompound);
        for (TopTools_ListIteratorOfListOfShape ri(result); ri.More(); ri.Next()) {
            builder.Add(myPreSplitCompound, ri.Value());
        }
    }
    return result;
}

bool Part::FaceMakerBuildFace::findPlane(const TopTools_ListOfShape& edges, gp_Pln& plane) const
{
    if (planeSupplied) {
        plane = myPlane;
        return true;
    }
    // Copy edges to strip cached surface info that can mislead FindSurface.
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
        builder.Add(comp, BRepBuilderAPI_Copy(it.Value()).Shape());
    }
    BRepLib_FindSurface planeFinder(comp, -1, Standard_True);
    if (!planeFinder.Found()) {
        return false;
    }
    plane = GeomAdaptor_Surface(planeFinder.Surface()).Plane();
    return true;
}

TopTools_ListOfShape Part::FaceMakerBuildFace::splitAtIntersections(const TopTools_ListOfShape& edges)
{
    if (edges.Size() <= 1) {
        return edges;
    }
    mySplitter.SetArguments(edges);
    mySplitter.SetRunParallel(true);
    mySplitter.SetNonDestructive(Standard_True);
    mySplitter.Build();
    if (!mySplitter.IsDone()) {
        FC_WARN("FaceMakerBuildFace: failed to split edges at intersections");
        return edges;
    }
    TopTools_ListOfShape result;
    for (TopExp_Explorer exp(mySplitter.Shape(), TopAbs_EDGE); exp.More(); exp.Next()) {
        result.Append(exp.Current());
    }
    return result;
}

void Part::FaceMakerBuildFace::Build_Essence()
{
    TopTools_ListOfShape edges;
    for (const TopoDS_Wire& w : myWires) {
        for (TopExp_Explorer exp(w, TopAbs_EDGE); exp.More(); exp.Next()) {
            edges.Append(exp.Current());
        }
    }
    if (edges.IsEmpty()) {
        return;
    }

    gp_Pln plane;
    if (!findPlane(edges, plane)) {
        FC_LOG("FaceMakerBuildFace: cannot determine plane from edges");
        return;
    }

    edges = splitSelfIntersecting(edges, plane);
    edges = splitAtIntersections(edges);

    // Build base face larger than all geometry so BuilderFace can
    // distinguish bounded regions from the unbounded exterior.
    Bnd_Box geomBox;
    for (TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
        BRepBndLib::Add(it.Value(), geomBox);
    }
    const Standard_Real aMax = std::max(1.0e8, 10.0 * std::sqrt(geomBox.SquareExtent()));
    TopoDS_Face baseFace = BRepBuilderAPI_MakeFace(plane, -aMax, aMax, -aMax, aMax).Face();
    // BuilderFace requires FORWARD orientation on the base face
    baseFace.Orientation(TopAbs_FORWARD);

    TopTools_ListOfShape faceEdges;
    for (TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next()) {
        const TopoDS_Edge& e = TopoDS::Edge(it.Value());
        faceEdges.Append(e.Oriented(TopAbs_FORWARD));
        faceEdges.Append(e.Oriented(TopAbs_REVERSED));
    }
    BRepLib::BuildPCurveForEdgesOnPlane(faceEdges, baseFace);

    // SetAvoidInternalShapes prevents dangling edges from becoming
    // internal wires that create degenerate geometry when extruded.
    BOPAlgo_BuilderFace faceBuilder;
    faceBuilder.SetFace(baseFace);
    faceBuilder.SetShapes(faceEdges);
    faceBuilder.SetAvoidInternalShapes(Standard_True);
    faceBuilder.Perform();
    if (faceBuilder.HasErrors()) {
        FC_WARN("FaceMakerBuildFace: BOPAlgo_BuilderFace failed");
        return;
    }

    const double outerThreshold = aMax * aMax;
    for (TopTools_ListIteratorOfListOfShape it(faceBuilder.Areas()); it.More(); it.Next()) {
        Bnd_Box box;
        BRepBndLib::Add(it.Value(), box);
        if (box.SquareExtent() > outerThreshold) {
            continue;
        }
        GProp_GProps props;
        BRepGProp::SurfaceProperties(it.Value(), props);
        if (props.Mass() < Precision::Confusion()) {
            continue;
        }
        myShapesToReturn.push_back(it.Value());
    }
}
