// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Krrish777 <777krrish[at]gmail.com>                 *
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


#pragma once

#include <Mod/Measure/MeasureGlobal.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <Bnd_Box.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <Base/Vector3D.h>

namespace App
{
class SubObjectT;
}

namespace Measure
{

// Value order is written into saved documents by PropertyEnumeration (index-only
// storage), so it is frozen once released and reordering breaks old files.
enum class MeasureSnapMode : int
{
    Auto = 0,
    None = 1,
    Vertex = 2,
    Center = 3,
    Midpoint = 4,
    Axis = 5
};

enum class MeasureSnapFlag : int
{
    FlagVertex = 1 << 0,
    FlagCenter = 1 << 1,
    FlagMidpoint = 1 << 2,
    FlagAxis = 1 << 3
};

class MeasureExport MeasureSnap
{
public:
    struct SnapPoint
    {
        gp_Pnt point;
        std::optional<gp_Dir> axisDir;
    };

    static std::optional<SnapPoint> computeSnapPoint(
        const TopoDS_Shape& shape,
        MeasureSnapMode mode,
        const Base::Vector3d* cursor
    );
    static int getAvailableSnapTypes(const TopoDS_Shape& shape);

    // A degenerate or curveless edge (sphere pole, cone apex) carries no 3D curve,
    // so adaptors raise on it and callers must reject it first.
    static bool edgeHasCurve(const TopoDS_Edge& edge);

    // Origin is the surface frame's own, not a measurement anchor
    static std::optional<gp_Ax1> axisOfFace(const TopoDS_Face& face);

    static gp_Pnt projectOntoAxis(const gp_Ax1& axis, const gp_Pnt& p);

    // Returns a deterministic pair for parallel axes
    static std::optional<std::pair<gp_Pnt, gp_Pnt>> closestPointsOnAxes(
        const gp_Ax1& a,
        const gp_Ax1& b
    );

    // Finite edge standing in for the infinite axis in a shape-to-shape extrema query,
    // spanning twice the pairBounds diagonal so any foot inside pairBounds stays interior.
    static TopoDS_Edge boundedAxisEdge(const gp_Ax1& axis, const Bnd_Box& pairBounds);

    // Sentinel-terminated for setEnums, the vector overload would mark the property
    // custom and write the label list into every saved document.
    static const char** snapModeEnums();

    static const char* snapModeLabel(MeasureSnapMode mode);

    // Out-of-range values fall back to Auto
    static MeasureSnapMode snapModeFromIndex(long index);

    static bool typeUsesSnapping(const std::string& measureTypeIdentifier);

    // Auto returns the best available snap, in order Center, Midpoint, Vertex, Axis
    static MeasureSnapMode pickPreviewType(int availableFlags, MeasureSnapMode activeMode);

    static std::optional<std::pair<gp_Pnt, gp_Pnt>> axisPreviewSegment(
        const gp_Ax1& axis,
        const Bnd_Box& bounds
    );

    static TopoDS_Shape resolveShape(const App::SubObjectT& subject);

    static std::vector<gp_Pnt> previewPoints(const TopoDS_Shape& shape, MeasureSnapMode mode);
};

}  // namespace Measure
