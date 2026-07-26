// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "FaceMakerUnified.h"

#include <BOPTools_AlgoTools3D.hxx>
#include <BRep_Tool.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepFill_Filling.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GeomAbs_Shape.hxx>
#include <GProp_GProps.hxx>
#include <IntTools_Context.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <algorithm>
#include <numeric>
#include <set>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("FaceMakerUnified", true, true)

using namespace Part;

TYPESYSTEM_SOURCE(Part::FaceMakerUnified, Part::FaceMakerBuildFace)

std::string FaceMakerUnified::getUserFriendlyName() const
{
    return {tr("Unified facemaker").toStdString()};
}

std::string FaceMakerUnified::getBriefExplanation() const
{
    return {tr("Unified: handles nested holes, overlapping wires, and curved surfaces").toStdString()};
}

namespace
{

TopoDS_Face makeFaceFromWire(const TopoDS_Wire& w, const gp_Pln& plane)
{
    if (!BRep_Tool::IsClosed(w)) {
        return {};
    }
    BRepBuilderAPI_MakeFace mf(plane, w);
    return mf.IsDone() ? mf.Face() : TopoDS_Face();
}

double shapeArea(const TopoDS_Shape& s)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(s, props);
    return props.Mass();
}

int findRoot(std::vector<int>& parent, int i)
{
    while (parent[i] != i) {
        parent[i] = parent[parent[i]];
        i = parent[i];
    }
    return i;
}

// Detect partially overlapping wire groups using union-find.
// Returns a group ID per wire AND the per-wire faces built along the way
// (null for non-closed wires), so callers can reuse them.
// Wires that partially overlap share a group; full containment (hole-in-outer)
// is NOT grouped — even-odd handles that.
struct OverlapResult
{
    std::vector<int> groups;
    std::vector<TopoDS_Face> wireFaces;
};

OverlapResult findOverlapGroups(const std::vector<TopoDS_Wire>& wires, const gp_Pln& plane)
{
    int n = static_cast<int>(wires.size());
    OverlapResult out;
    out.groups.resize(n);
    out.wireFaces.resize(n);
    std::iota(out.groups.begin(), out.groups.end(), 0);

    std::vector<Bnd_Box> boxes(n);
    std::vector<double> areas(n, 0.0);

    int closedCount = 0;
    for (int i = 0; i < n; ++i) {
        if (!BRep_Tool::IsClosed(wires[i])) {
            continue;
        }
        out.wireFaces[i] = makeFaceFromWire(wires[i], plane);
        if (!out.wireFaces[i].IsNull()) {
            BRepBndLib::AddOptimal(wires[i], boxes[i], Standard_False);
            areas[i] = shapeArea(out.wireFaces[i]);
            ++closedCount;
        }
    }
    if (closedCount < 2) {
        return out;
    }

    const double tol = Precision::Confusion();
    for (int i = 0; i < n; ++i) {
        if (out.wireFaces[i].IsNull()) {
            continue;
        }
        for (int j = i + 1; j < n; ++j) {
            if (out.wireFaces[j].IsNull() || boxes[i].IsOut(boxes[j])) {
                continue;
            }
            BRepAlgoAPI_Common common(out.wireFaces[i], out.wireFaces[j]);
            if (!common.IsDone() || common.Shape().IsNull()) {
                continue;
            }
            double ca = shapeArea(common.Shape());
            // Partial overlap only — exclude full containment
            if (ca > tol && ca < areas[i] - tol && ca < areas[j] - tol) {
                out.groups[findRoot(out.groups, i)] = findRoot(out.groups, j);
            }
        }
    }
    // Flatten
    for (int i = 0; i < n; ++i) {
        out.groups[i] = findRoot(out.groups, i);
    }
    return out;
}

TopoDS_Face fillNonPlanar(const TopoDS_Wire& wire)
{
    try {
        BRepFill_Filling filler;
        for (TopExp_Explorer ex(wire, TopAbs_EDGE); ex.More(); ex.Next()) {
            filler.Add(TopoDS::Edge(ex.Current()), GeomAbs_C0);
        }
        filler.Build();
        if (filler.IsDone()) {
            return filler.Face();
        }
    }
    catch (const Standard_Failure& e) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_LOG("fillNonPlanar: " << e.GetMessageString());
        }
    }
    return {};
}

}  // namespace

void FaceMakerUnified::Build_Essence()
{
    if (myWires.empty()) {
        return;
    }

    TopTools_ListOfShape edges = collectEdgesFromWires();

    gp_Pln plane;
    if (findPlane(edges, plane)) {
        edges = splitSelfIntersecting(edges, plane);
        edges = splitAtIntersections(edges);

        std::vector<TopoDS_Face> allFaces = collectBoundedFaces(edges, plane);
        if (allFaces.empty()) {
            return;
        }

        // Overlap-group-aware even-odd classification.
        // Count containment per overlap group (not per wire) so that
        // partially overlapping wires act as a union outline while
        // fully nested wires still create holes via even-odd.
        OverlapResult overlap = findOverlapGroups(myWires, plane);
        const auto& groups = overlap.groups;
        const auto& wireFaces = overlap.wireFaces;

        Handle(IntTools_Context) ctx = new IntTools_Context();
        for (const auto& face : allFaces) {
            gp_Pnt pt;
            gp_Pnt2d pt2d;
            if (BOPTools_AlgoTools3D::PointInFace(face, pt, pt2d, ctx) != 0) {
                myShapesToReturn.push_back(face);
                continue;
            }

            // Count how many overlap groups contain this point.
            // A group "contains" the point if ANY wire in the group does.
            std::set<int> containingGroups;
            for (size_t i = 0; i < wireFaces.size(); ++i) {
                if (!wireFaces[i].IsNull()
                    && ctx->IsPointInFace(pt, wireFaces[i], Precision::Confusion())) {
                    containingGroups.insert(groups[i]);
                }
            }

            int groupCount = static_cast<int>(containingGroups.size());
            if (groupCount == 0 || groupCount % 2 == 1) {
                myShapesToReturn.push_back(face);
            }
        }
        return;
    }

    // Non-planar fallback: try BRepBuilderAPI_MakeFace per wire,
    // then BRepFill_Filling for closed wires.
    for (const auto& w : myWires) {
        BRepBuilderAPI_MakeFace mf(w);
        TopoDS_Face face = mf.IsDone() ? mf.Face() : TopoDS_Face();
        if (face.IsNull() && BRep_Tool::IsClosed(w)) {
            face = fillNonPlanar(w);
        }
        if (!face.IsNull() && shapeArea(face) > Precision::Confusion()) {
            myShapesToReturn.push_back(face);
        }
    }
}
