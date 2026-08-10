// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
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

#pragma once

#include <utility>
#include <vector>

#include <Base/BoundBox.h>
#include <Base/Vector3D.h>

#include <Mod/Part/PartGlobal.h>

class TopoDS_Shape;


namespace Part
{

/// Building a section cap from tessellation rather than from the exact solid.
///
/// Cutting a real assembly with OCCT costs minutes: one 92 face solid in a
/// customer model took 103 s inside BRepAlgoAPI_Section alone. The viewer,
/// however, already holds a triangulation of everything on screen, and slicing
/// triangles with a plane is linear and trivial. The cap that comes out is
/// exact with respect to what is drawn, since it is derived from the very
/// triangles being drawn, and only approximate with respect to the underlying
/// B-rep - which matters solely when the user asks for real geometry, and can
/// afford to wait for it.
///
/// The functions here are deliberately free of Coin and OCCT so they can be
/// tested directly.
namespace SectionCap
{

/// A single crossing of one triangle by the cutting plane.
/// One edge of the mesh, named by its two vertex indices, lower first.
///
/// Two triangles sharing an edge name it identically, which is what lets the
/// chaining below match endpoints by integer equality instead of by distance.
using MeshEdge = std::pair<int, int>;

struct Segment
{
    Base::Vector3d start;
    Base::Vector3d end;
    /// The edges the endpoints sit on. Absent for segments not cut from a mesh.
    MeshEdge startEdge {-1, -1};
    MeshEdge endEdge {-1, -1};
};

/// Triangles are supplied flattened: `indices` holds 3 entries per triangle,
/// each an index into `points`.
struct TriangleSoup
{
    std::vector<Base::Vector3d> points;
    std::vector<int> indices;
};

/// FreeCAD's own meshing default. In Radians
constexpr double meshAngularDeflection = 0.5;

/// Mesh a solid into a watertight triangle soup.
///
/// The shape's own triangulation is per face and unwelded, so a contour cut
/// through it breaks at every face boundary. This rebuilds and merges, giving a
/// closed manifold: every edge is shared by two triangles, so every cut closes.
/// Pass deflection <= 0 to size it from the shape.
PartExport TriangleSoup meshSolid(const TopoDS_Shape& shape, double deflection = 0.0);

/// Where the plane crosses one triangle, if it crosses at all.
///
/// Uses a half open sign test, so a triangle yields exactly zero or two
/// crossings and a vertex lying on the plane cannot produce a duplicate or a
/// dangling segment. Returns false when the triangle does not cross, including
/// the case of one vertex merely resting on the plane.
///
/// Exposed per triangle so the viewer can slice while it walks the scene graph,
/// without first copying every triangle into a soup - on a large assembly that
/// copy is hundreds of megabytes and dominates the cost.
PartExport std::optional<Segment> planeTriangleIntersection(
    const Base::Vector3d& a,
    const Base::Vector3d& b,
    const Base::Vector3d& c,
    const Base::Vector3d& normal,
    double offset
);

/// Every place the plane crosses a triangle, as an unordered segment list.
/// The plane's crossing of one triangle, carrying the edges it crossed.
PartExport std::optional<Segment> triangleCrossing(
    const Base::Vector3d& a,
    const Base::Vector3d& b,
    const Base::Vector3d& c,
    int ia,
    int ib,
    int ic,
    const Base::Vector3d& normal,
    double offset
);

PartExport std::vector<Segment> sliceTriangles(
    const TriangleSoup& soup,
    const Base::Vector3d& normal,
    double offset
);

/// Join segments end to end into loops, matching endpoints by mesh edge.
PartExport std::vector<std::vector<Base::Vector3d>> chainLoops(const std::vector<Segment>& segments);

/// A solid cap covering the region enclosed by `loops`, as triangles.
///
/// Uses a scanline parity sweep, so holes are excluded without ever being
/// identified as holes - which is what makes this possible without a polygon
/// triangulator. The sweep stops at the loops' own vertex levels rather than at
/// evenly spaced ones: no vertex then falls inside a band, so the same edges
/// are crossed in the same order top and bottom and each pair of crossings is
/// one exact trapezoid. The cap boundary is therefore the loops themselves, and
/// there is no strip height to pick.
///
/// Without this the section is see-through and you look into the inside of the
/// body you just cut.
PartExport TriangleSoup fillLoops(
    const std::vector<std::vector<Base::Vector3d>>& loops,
    const Base::Vector3d& u,
    const Base::Vector3d& v
);

/// The box's extent projected onto `normal`, so a plane that misses a body can
/// be rejected without visiting a triangle. False when the box is void.
///
/// Takes the bounding box rather than the soup so this stays O(1). The caller
/// measures the box once, when the triangles are harvested; walking every point
/// again on each plane move is the very cost the rejection exists to avoid.
/// The box is a conservative hull, so the rejection is conservative too: a
/// plane it fails to reject simply slices to nothing.
PartExport bool extentAlong(
    const Base::BoundBox3d& bounds,
    const Base::Vector3d& normal,
    double& lo,
    double& hi
);

/// True if the loop's first and last point meet within `tolerance`.
PartExport bool isClosed(const std::vector<Base::Vector3d>& loop, double tolerance);

/// Whether chaining ran the contour all the way back to where it started.
PartExport bool isClosedExactly(const std::vector<Base::Vector3d>& loop);


/// Hatch lines across a cap that is already triangulated.
///
/// Every triangle is material, so any crossing is inside - no parity, no closed
/// boundary needed. `levelDir` is the direction the lines march along: a line
/// is the points with `p * levelDir == k * spacing`, on an absolute grid so
/// neighbouring bodies stay in step.
PartExport std::vector<Segment> hatchTriangles(
    const TriangleSoup& cap,
    const Base::Vector3d& levelDir,
    double spacing,
    std::size_t maxSegments = 500000
);

}  // namespace SectionCap
}  // namespace Part
