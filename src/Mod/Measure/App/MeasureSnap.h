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
// storage), so it is frozen once released; reordering breaks old files.
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
    // Snapped preview point; axisDir is set for Axis snaps only.
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

    // Axis line of a cylindrical, conical, or revolution face.
    // Origin is the surface frame's own, not a measurement anchor.
    static std::optional<gp_Ax1> axisOfFace(const TopoDS_Face& face);

    // Foot of the perpendicular from p onto the infinite line; may lie past the origin.
    static gp_Pnt projectOntoAxis(const gp_Ax1& axis, const gp_Pnt& p);

    // Closest points between two infinite axis lines; a deterministic pair for
    // parallel axes, nothing if the extrema solve fails.
    static std::optional<std::pair<gp_Pnt, gp_Pnt>> closestPointsOnAxes(
        const gp_Ax1& a,
        const gp_Ax1& b
    );

    // Finite edge standing in for the infinite axis in a shape-to-shape extrema query,
    // spanning twice the pairBounds diagonal so any foot inside pairBounds stays interior.
    static TopoDS_Edge boundedAxisEdge(const gp_Ax1& axis, const Bnd_Box& pairBounds);

    // Sentinel-terminated for setEnums(); the vector overload would mark the property
    // custom and write the label list into every saved document.
    static const char** snapModeEnums();

    static const char* snapModeLabel(MeasureSnapMode mode);

    // Convert a stored index to a mode; out-of-range values fall back to Auto.
    static MeasureSnapMode snapModeFromIndex(long index);

    // Whether a measurement type consults a snap point; Length/Area/Radius/Diameter do not.
    static bool typeUsesSnapping(const std::string& measureTypeIdentifier);

    // Preview type for a hovered element: the active mode if its flag is set, else None;
    // Auto returns the best available snap (Center, then Midpoint, Vertex, then Axis).
    static MeasureSnapMode pickPreviewType(int availableFlags, MeasureSnapMode activeMode);

    // Endpoints of the axis preview line: the bbox centre projected onto the axis,
    // extended each way by a bbox-proportional length. Nothing for a void box.
    static std::optional<std::pair<gp_Pnt, gp_Pnt>> axisPreviewSegment(
        const gp_Ax1& axis,
        const Bnd_Box& bounds
    );

    // Placed sub-shape a preselection refers to, matching what execute() measures on;
    // a null shape if it cannot be resolved.
    static TopoDS_Shape resolveShape(const App::SubObjectT& subject);

    // Points to preview for a resolved snap type: both edge endpoints for Vertex, a single
    // point for Center/Midpoint, the two axis-line endpoints for Axis, empty otherwise.
    static std::vector<gp_Pnt> previewPoints(const TopoDS_Shape& shape, MeasureSnapMode type);
};

}  // namespace Measure
