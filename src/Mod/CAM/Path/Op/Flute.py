# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

__title__ = "CAM Flute Operation"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of the Flute operation."

import collections
import math

import FreeCAD
from PySide import QtCore
import Path
import Path.Geom as PathGeom
import Path.Op.Base as PathOp
import Path.Base.Generator.follow_wire as WireFollowGenerator
import Path.Base.Generator.linking as linking
import PathScripts.PathUtils as PathUtils

from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

_PREC = FreeCAD.Base.Precision.confusion()  # standard FreeCAD coincidence epsilon (1e-7)

_SECTION_Z_TOL_FRAC = 0.05  # z_tol = max(1mm, depth * this fraction)
_CHAIN_MAX_EDGES = 200  # guard against infinite loop (not a tolerance)
_WALL_SLOPE_MAX = 5.0  # dz/ds above this → discard as a side wall
_WALL_DS_MIN = _PREC * 10  # floating-point "is this exactly zero" epsilon
# Middle-only sample points — endpoints excluded because a legitimately tapered
# channel's edges converge there too and would give a false degenerate signal.
_PCA_DEGENERATE_T_VALUES = (0.25, 0.5, 0.75)
_VBIT_ANGLE_FRAC = 0.025  # 2.5% of groove half-angle — fuzzy margin for V-bit fit check
# Max deviation of the oriented tangent dot product from 1.0 for two connected
# edges to count as one continuous flute (same idiom as the old collinearity
# test).  1e-3 ≈ 2.6° kink; below it edges merge, above it a corner splits.
_TANGENT_DOT_TOL = 1e-3

_Tol = collections.namedtuple(
    "_Tol",
    [
        "edge",
        "chain2",
        "floor_snap",
        "axial_floor",
        "bb_overlap",
        "section_overshoot",
        "pca_degenerate",
        "arc_chord",
    ],
)


def _make_tolerances(geom_tol):
    """Derive all absolute-distance tolerances from the job's GeometryTolerance.
    Fallback of 0.01 mm matches the default used across other CAM operations.
    """
    t = geom_tol if geom_tol and geom_tol > 0 else 0.01
    return _Tol(
        edge=t * 0.01,  # exact topological edge-coincidence
        chain2=(t * 200.0) ** 2,  # vertex snap distance while chaining (stored squared)
        floor_snap=t * 50.0,  # floor-seam Z snapping
        axial_floor=t * 50.0,  # axial-leave floor-point detection
        bb_overlap=t * 100.0,  # face grouping by bounding-box touch
        section_overshoot=t * 1000.0,  # section-plane / candidate-filter margin
        pca_degenerate=t * 5.0,  # centerline-coincides-with-edge test
        arc_chord=t * 10.0,  # arc discretization chord (matches LeadInOut dressup)
    )


# ---------------------------------------------------------------------------
# Face analysis helpers
# ---------------------------------------------------------------------------


def _analyze_face_cylinder(face):
    """Centerline from a Part.Cylinder face using the face BoundBox directly.

    The BoundBox gives us everything we need:
      - Groove center X (or Y): midpoint of the perpendicular extents
      - Shallow end: the BoundBox extreme at ZMax (stock surface)
      - Deep end:    the BoundBox extreme at ZMin (floor)
      - Width:       2 * surface.Radius

    The cylinder axis direction tells us which BoundBox dimension the groove
    runs along, and the sign of that axis component tells us which extreme
    is the shallow (entry) end.

    Returns the same dict as _analyze_face_pca, or None if not a cylinder.
    """
    try:
        surface = face.Surface
        if not isinstance(surface, Part.Cylinder):
            return None

        axis = surface.Axis
        radius = surface.Radius
        bb = face.BoundBox

        ax, ay, az = abs(axis.x), abs(axis.y), abs(axis.z)

        if ay >= ax and ay >= az:
            # Groove runs primarily in Y
            cx = (bb.XMin + bb.XMax) / 2.0
            if axis.y < 0:
                # Moving along +axis → Y decreases → deep end = YMin
                p_start = FreeCAD.Vector(cx, bb.YMax, bb.ZMax)
                p_end = FreeCAD.Vector(cx, bb.YMin, bb.ZMin)
            else:
                # Moving along +axis → Y increases → deep end = YMax
                p_start = FreeCAD.Vector(cx, bb.YMin, bb.ZMax)
                p_end = FreeCAD.Vector(cx, bb.YMax, bb.ZMin)

        elif ax >= ay and ax >= az:
            # Groove runs primarily in X
            cy = (bb.YMin + bb.YMax) / 2.0
            if axis.x < 0:
                p_start = FreeCAD.Vector(bb.XMax, cy, bb.ZMax)
                p_end = FreeCAD.Vector(bb.XMin, cy, bb.ZMin)
            else:
                p_start = FreeCAD.Vector(bb.XMin, cy, bb.ZMax)
                p_end = FreeCAD.Vector(bb.XMax, cy, bb.ZMin)

        else:
            return None  # Z-dominant - not a typical flute orientation

        Path.Log.debug(
            "CAM_Flute BoundBox:\n"
            "  X: [{:.4f}, {:.4f}]  Y: [{:.4f}, {:.4f}]  Z: [{:.4f}, {:.4f}]\n"
            "  axis=({:.4f},{:.4f},{:.4f})  radius={:.4f}\n"
            "CAM_Flute centerline:\n"
            "  start=({:.4f},{:.4f},{:.4f})  end=({:.4f},{:.4f},{:.4f})\n".format(
                bb.XMin,
                bb.XMax,
                bb.YMin,
                bb.YMax,
                bb.ZMin,
                bb.ZMax,
                axis.x,
                axis.y,
                axis.z,
                radius,
                p_start.x,
                p_start.y,
                p_start.z,
                p_end.x,
                p_end.y,
                p_end.z,
            )
        )

        def _clean(v, tol=_PREC):
            return 0.0 if abs(v) < tol else v

        return {
            "start": p_start,
            "end": p_end,
            "top_z": _clean(bb.ZMax),
            "width": _clean(2.0 * radius),
        }
    except Exception:
        return None


def _line_coincides_with_an_edge(face, p_a, p_b, tol):
    """True if the MIDDLE portion of the line p_a->p_b lies ON one of the
    face's own boundary edges (within tol.pca_degenerate).

    A real floor's centerline runs through the face's interior. A face that
    legitimately tapers to a point at one end (e.g. a pointed channel) will
    have its boundary edges genuinely converge near that end -- that's not
    a sign of a missing partner face, so the endpoints are deliberately
    excluded. Only the middle of the line is tested: a single wall of an
    unselected V-groove pair has zero interior width along its ENTIRE
    length, so it still coincides with the shared valley edge there too.
    """
    try:
        edges = face.Edges
    except Exception:
        return False

    for edge in edges:
        coincides = True
        for t in _PCA_DEGENERATE_T_VALUES:
            p = FreeCAD.Vector(
                p_a.x + t * (p_b.x - p_a.x),
                p_a.y + t * (p_b.y - p_a.y),
                p_a.z + t * (p_b.z - p_a.z),
            )
            try:
                dist = edge.distToShape(Part.Vertex(p))[0]
            except Exception:
                coincides = False
                break
            if dist > tol.pca_degenerate:
                coincides = False
                break
        if coincides:
            return True
    return False


def _analyze_face_pca(face, tol):
    """Centerline fallback via edge sampling + PCA for non-cylindrical faces.

    Samples all face edges, uses PCA to split into two end-zones, then
    finds the lowest-Z cluster in each zone.  Key insight: for any groove
    profile the deepest cross-section point IS on the centreline.

    Returns dict with keys start, end, top_z, width, or None on failure.
    """
    pts = []
    try:
        for edge in face.Edges:
            pts.extend(edge.discretize(Number=100))
    except Exception:
        pass

    if len(pts) < 4:
        return None

    mx = sum(p.x for p in pts) / len(pts)
    my = sum(p.y for p in pts) / len(pts)

    cxx = sum((p.x - mx) ** 2 for p in pts)
    cyy = sum((p.y - my) ** 2 for p in pts)
    cxy = sum((p.x - mx) * (p.y - my) for p in pts)

    angle = 0.5 * math.atan2(2.0 * cxy, cxx - cyy)
    ux, uy = math.cos(angle), math.sin(angle)
    vx, vy = -uy, ux

    s_vals = [(p.x - mx) * ux + (p.y - my) * uy for p in pts]
    t_vals = [(p.x - mx) * vx + (p.y - my) * vy for p in pts]

    s_min, s_max = min(s_vals), max(s_vals)
    span = s_max - s_min
    if span < _PREC:
        return None

    zone = span * 0.20
    a_pts = [p for p, s in zip(pts, s_vals) if s <= s_min + zone]
    b_pts = [p for p, s in zip(pts, s_vals) if s >= s_max - zone]

    if not a_pts or not b_pts:
        return None

    def _lowest_z_center(zone_pts):
        z_floor = min(p.z for p in zone_pts)
        z_range = max(p.z for p in zone_pts) - z_floor
        tol = max(0.01, z_range * 0.02)
        near = [p for p in zone_pts if p.z <= z_floor + tol]
        return FreeCAD.Vector(
            sum(p.x for p in near) / len(near),
            sum(p.y for p in near) / len(near),
            sum(p.z for p in near) / len(near),
        )

    p_a = _lowest_z_center(a_pts)
    p_b = _lowest_z_center(b_pts)
    if p_a.z < p_b.z:
        p_a, p_b = p_b, p_a

    # Reject a degenerate result: if the computed centerline coincides with
    # one of the face's own boundary edges, this face has no floor of its
    # own -- it's most likely a single wall of an unselected V-groove pair.
    if _line_coincides_with_an_edge(face, p_a, p_b, tol):
        FreeCAD.Console.PrintWarning(
            translate(
                "CAM_Flute",
                "Face appears to be a single wall of a V-groove (its "
                "centerline coincides with a face edge). Select both walls "
                "of the groove, or select the valley edge directly.\n",
            )
        )
        return None

    top_z = max(p.z for p in pts)

    def _clean(v, tol=_PREC):
        return 0.0 if abs(v) < tol else v

    return {
        "start": p_a,
        "end": p_b,
        "top_z": _clean(top_z),
        "width": _clean(max(t_vals) - min(t_vals)),
    }


def _analyze_face(face, tol):
    """Determine the centerline of a single groove face.

    Tries exact cylinder-surface detection first; falls back to edge-
    sampling PCA for BSpline or other surface types.
    """
    result = _analyze_face_cylinder(face)
    if result is not None:
        FreeCAD.Console.PrintLog("CAM_Flute: centerline via cylinder surface (exact)\n")
        return result
    FreeCAD.Console.PrintLog("CAM_Flute: centerline via edge-sampling PCA (fallback)\n")
    return _analyze_face_pca(face, tol)


# ---------------------------------------------------------------------------
# V-bottom (multi-face) grouping helpers
# ---------------------------------------------------------------------------


def _find_shared_edges(face_a, face_b, tol):
    """Return edges that are geometrically coincident between two faces."""
    shared = []
    for ea in face_a.Edges:
        com_a = ea.CenterOfMass
        len_a = ea.Length
        for eb in face_b.Edges:
            if abs(len_a - eb.Length) > tol.edge:
                continue
            if com_a.distanceToPoint(eb.CenterOfMass) > tol.edge:
                continue
            # Verify endpoints match (in either order)
            pa0 = ea.Vertexes[0].Point
            pa1 = ea.Vertexes[-1].Point
            pb0 = eb.Vertexes[0].Point
            pb1 = eb.Vertexes[-1].Point
            if (pa0.distanceToPoint(pb0) < tol.edge and pa1.distanceToPoint(pb1) < tol.edge) or (
                pa0.distanceToPoint(pb1) < tol.edge and pa1.distanceToPoint(pb0) < tol.edge
            ):
                shared.append(ea)
                break
    return shared


def _is_valley_edge(edge, face_a, face_b):
    """Return True if the shared edge sits below both adjacent face centroids.

    For any groove in a flute operation the valley edge is always the lowest
    part of the two wall faces - its Z centroid is below the average face Z.
    This is simpler and more reliable than a cross-product sign test, which
    depends on the BRep edge orientation and can flip sign unexpectedly.
    """
    try:
        edge_z = edge.CenterOfMass.z
        face_z = (face_a.CenterOfMass.z + face_b.CenterOfMass.z) / 2.0
        return edge_z < face_z - _PREC * 10
    except Exception:
        return False


def _group_flutes(face_tuples, tol):
    """Group (face, sub_name, base_obj) tuples into flute groups.

    Primary grouping: faces whose XY bounding boxes overlap or touch (within
    tol.bb_overlap) are treated as parts of the same groove.  This correctly
    merges ramp + flat faces (which share an edge but are not a concave
    V-bottom) as well as V-groove face pairs.

    Within each resulting group, valley-edge detection is still run so that
    two-face V-bottom groups can expose their shared concave edge for
    centerline extraction.

    Returns list of dicts:
        {
          'faces':       [(face, sub, base), ...],
          'valley_edge': list[Part.Edge] or None,
        }
    """
    n = len(face_tuples)
    if n == 0:
        return []

    def _bb_overlaps_xy(bb_a, bb_b):
        return (
            bb_a.XMin <= bb_b.XMax + tol.bb_overlap
            and bb_a.XMax >= bb_b.XMin - tol.bb_overlap
            and bb_a.YMin <= bb_b.YMax + tol.bb_overlap
            and bb_a.YMax >= bb_b.YMin - tol.bb_overlap
        )

    parent = list(range(n))

    def _find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def _union(x, y):
        rx, ry = _find(x), _find(y)
        if rx != ry:
            parent[rx] = ry

    for i in range(n):
        for j in range(i + 1, n):
            if _bb_overlaps_xy(face_tuples[i][0].BoundBox, face_tuples[j][0].BoundBox):
                _union(i, j)

    groups_map = {}
    for i in range(n):
        groups_map.setdefault(_find(i), []).append(i)

    result = []
    for indices in groups_map.values():
        flute_faces = [face_tuples[k] for k in indices]

        # Check for concave (valley) edges within the group - used only for
        # V-bottom two-face centerline detection. The valley boundary between
        # the two faces can be split into several B-Rep edge segments (e.g. a
        # tapered V-groove with a seam partway along); collect ALL of them so
        # the centerline isn't truncated at the first segment found.
        valley_edges = []
        if len(indices) == 2:
            fa, fb = flute_faces[0][0], flute_faces[1][0]
            shared = _find_shared_edges(fa, fb, tol)
            for edge in shared:
                if _is_valley_edge(edge, fa, fb):
                    valley_edges.append(edge)

        result.append({"faces": flute_faces, "valley_edge": valley_edges or None})

    return result


def _centerline_from_valley_edge(edges, face_tuples, tol):
    """Get start/end/top_z from one or more V-bottom valley edges.

    The valley boundary between two faces may consist of several colinear/
    curved edge segments rather than a single edge. Chain them end-to-end
    by matching coincident endpoints and use the two outer extremities of
    the resulting chain as the cutting centerline. Start = shallower end.
    top_z = highest Z across all face bounding boxes.
    """
    if not isinstance(edges, (list, tuple)):
        edges = [edges]

    if len(edges) == 1:
        v0 = edges[0].Vertexes[0].Point
        v1 = edges[0].Vertexes[-1].Point
    else:
        remaining = list(edges[1:])
        chain = [edges[0].Vertexes[0].Point, edges[0].Vertexes[-1].Point]
        changed = True
        while remaining and changed:
            changed = False
            for e in list(remaining):
                p0, p1 = e.Vertexes[0].Point, e.Vertexes[-1].Point
                if chain[-1].distanceToPoint(p0) < tol.edge:
                    chain.append(p1)
                elif chain[-1].distanceToPoint(p1) < tol.edge:
                    chain.append(p0)
                elif chain[0].distanceToPoint(p0) < tol.edge:
                    chain.insert(0, p1)
                elif chain[0].distanceToPoint(p1) < tol.edge:
                    chain.insert(0, p0)
                else:
                    continue
                remaining.remove(e)
                changed = True
        v0, v1 = chain[0], chain[-1]

    if v0.z < v1.z:
        v0, v1 = v1, v0  # v0 = shallow, v1 = deep

    top_z = max(ft[0].BoundBox.ZMax for ft in face_tuples)

    # Groove half-angle from the first planar wall face normal.
    # For a V-wall: normal points mostly horizontal (outward) with a Z
    # component encoding the slope.  half_angle = arcsin(|n.z|).
    groove_half_angle = 0.0
    for ft in face_tuples:
        try:
            n = ft[0].Surface.Axis
            nlen = math.sqrt(n.x**2 + n.y**2 + n.z**2)
            if nlen > _PREC * 0.01:
                groove_half_angle = math.degrees(math.asin(min(1.0, abs(n.z) / nlen)))
                break
        except Exception:
            pass

    def _clean(val, tol=_PREC):
        return 0.0 if abs(val) < tol else val

    return {
        "start": FreeCAD.Vector(v0),
        "end": FreeCAD.Vector(v1),
        "top_z": _clean(top_z),
        "width": 0.0,
        "groove_half_angle": groove_half_angle,
    }


def _centerline_from_faces(face_tuples):
    """Run PCA across all faces in a group (fallback for >2-face groups)."""
    pts = []
    for face, _sub, _base in face_tuples:
        try:
            for edge in face.OuterWire.Edges:
                pts.extend(edge.discretize(Number=20))
        except Exception:
            pass

    if len(pts) < 2:
        return None

    # Re-use the single-face PCA logic on the combined point cloud
    mx = sum(p.x for p in pts) / len(pts)
    my = sum(p.y for p in pts) / len(pts)
    cxx = sum((p.x - mx) ** 2 for p in pts)
    cyy = sum((p.y - my) ** 2 for p in pts)
    cxy = sum((p.x - mx) * (p.y - my) for p in pts)

    angle = 0.5 * math.atan2(2.0 * cxy, cxx - cyy)
    ux, uy = math.cos(angle), math.sin(angle)
    vx, vy = -uy, ux

    proj = []
    for p in pts:
        dx, dy = p.x - mx, p.y - my
        proj.append((p, dx * ux + dy * uy, dx * vx + dy * vy))

    s_min = min(r[1] for r in proj)
    s_max = max(r[1] for r in proj)
    t_min = min(r[2] for r in proj)
    t_max = max(r[2] for r in proj)
    zone = (s_max - s_min) * 0.05

    def _avg(points):
        return FreeCAD.Vector(
            sum(p.x for p in points) / len(points),
            sum(p.y for p in points) / len(points),
            sum(p.z for p in points) / len(points),
        )

    a_pts = [p for p, s, _t in proj if s <= s_min + zone]
    b_pts = [p for p, s, _t in proj if s >= s_max - zone]
    p_a, p_b = _avg(a_pts), _avg(b_pts)
    if p_a.z < p_b.z:
        p_a, p_b = p_b, p_a

    top_z = max(p.z for p in pts)

    def _clean(v, tol=_PREC):
        return 0.0 if abs(v) < tol else v

    return {
        "start": p_a,
        "end": p_b,
        "top_z": _clean(top_z),
        "width": _clean(t_max - t_min),
    }


def _check_tool_fit(info, tool):
    """Warn if the tool appears too large for the detected groove geometry.

    Checks two independent conditions from the centerline info dict:
      - width > 0: rounded/flat channel - tool diameter vs groove width.
      - groove_half_angle > 0: V-groove - V-bit half-angle vs groove half-angle.
    Always returns None; caller continues regardless.
    """
    tool_dia = float(tool.Diameter)
    is_vbit = hasattr(tool, "CuttingEdgeAngle")

    # Width check only applies to flat/bull-nose tools - a V-bit's effective
    # cutting width depends on depth, not diameter.
    def _qty(mm):
        return FreeCAD.Units.Quantity(mm, FreeCAD.Units.Length).UserString

    if not is_vbit:
        groove_width = info.get("width", 0.0)
        if groove_width > 0.0 and tool_dia > groove_width:
            FreeCAD.Console.PrintWarning(
                translate(
                    "CAM_Flute",
                    "CAM_Flute: tool diameter ({}) exceeds groove width ({})"
                    " - path may overcut.\n",
                ).format(_qty(tool_dia), _qty(groove_width))
            )

    if is_vbit:
        try:
            tool_ha = tool.CuttingEdgeAngle.Value / 2.0
        except Exception:
            tool_ha = 0.0

        groove_ha = info.get("groove_half_angle", 0.0)
        if groove_ha == 0.0:
            # Valley edge not detected - estimate from PCA width and depth.
            width = info.get("width", 0.0)
            depth = info["top_z"] - info["end"].z
            if width > 0.0 and depth > 0.0:
                groove_ha = math.degrees(math.atan2(width / 2.0, depth))

        Path.Log.debug(
            "CAM_Flute: tool half-angle={:.2f}° groove half-angle={:.2f}°\n".format(
                tool_ha, groove_ha
            )
        )
        if tool_ha > 0.0 and groove_ha > 0.0 and tool_ha > groove_ha * (1.0 + _VBIT_ANGLE_FRAC):
            FreeCAD.Console.PrintWarning(
                translate(
                    "CAM_Flute",
                    "CAM_Flute: V-bit half-angle ({:.1f}°) exceeds groove"
                    " half-angle ({:.1f}°) - flanks may contact walls before"
                    " reaching depth.\n",
                ).format(tool_ha, groove_ha)
            )


def _get_centerline(group, tol):
    """Return the centerline dict for a flute group (any size)."""
    valley_edge = group.get("valley_edge")
    face_tuples = group["faces"]

    if valley_edge is not None:
        # 2-face V-bottom: the valley edge IS the centerline
        return _centerline_from_valley_edge(valley_edge, face_tuples, tol)
    elif len(face_tuples) == 1:
        # Single flat/bull-nose face: PCA
        return _analyze_face(face_tuples[0][0], tol)
    else:
        # >2 faces with no detected valley edge: run PCA across all faces
        Path.Log.debug(
            "Flute group has {} faces with no detected valley edge; "
            "falling back to combined PCA.\n".format(len(face_tuples))
        )
        return _centerline_from_faces(face_tuples)


def _dist2(a, b):
    return (a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2


def _clip_and_scale(pts, f, stock_top_z):
    """Scale a full-depth point list to depth fraction f.

    pts: ordered list of FreeCAD.Vector, shallow end first, deep end last.

    At fraction f (0 < f <= 1):
      - Include only the f * total_XY_length of path closest to the deep end.
      - The new chain-start is forced to stock_top_z.
      - pts[-1] is forced to pass_floor_z = stock_top_z - f*(stock_top_z - floor_z).
      - Intermediate Z values scale: Z_i = stock_top_z - f*(stock_top_z - Z_i_full).
    """
    if not pts or f < _PREC:
        return []

    n = len(pts)
    floor_z = min(p.z for p in pts)  # use actual deepest point, not pts[-1]
    pass_floor_z = stock_top_z - f * (stock_top_z - floor_z)

    if n == 1:
        return [FreeCAD.Vector(pts[0].x, pts[0].y, pass_floor_z)]

    # Cumulative XY arc lengths from the deep end toward pts[0].
    rev = [0.0] * n
    for i in range(n - 2, -1, -1):
        dx = pts[i].x - pts[i + 1].x
        dy = pts[i].y - pts[i + 1].y
        rev[i] = rev[i + 1] + math.sqrt(dx * dx + dy * dy)

    L_total = rev[0]
    if L_total < _PREC:
        return [FreeCAD.Vector(pts[-1].x, pts[-1].y, pass_floor_z)]

    target = f * L_total

    result_rev = []
    for i in range(n - 1, -1, -1):
        if rev[i] <= target + _PREC:
            z_f = stock_top_z - f * (stock_top_z - pts[i].z)
            result_rev.append(FreeCAD.Vector(pts[i].x, pts[i].y, z_f))
        else:
            if result_rev:
                j = i + 1
                seg_len = rev[i] - rev[j]
                if seg_len > _PREC:
                    t = (target - rev[j]) / seg_len
                    px = pts[j].x + t * (pts[i].x - pts[j].x)
                    py = pts[j].y + t * (pts[i].y - pts[j].y)
                else:
                    px, py = pts[j].x, pts[j].y
            else:
                px, py = pts[-1].x, pts[-1].y
            result_rev.append(FreeCAD.Vector(px, py, stock_top_z))
            break

    if not result_rev:
        return [FreeCAD.Vector(pts[-1].x, pts[-1].y, pass_floor_z)]

    result_rev.reverse()
    result_rev[0] = FreeCAD.Vector(result_rev[0].x, result_rev[0].y, stock_top_z)
    # Snap the exit to pass_floor_z only when pts[-1] is at (or near) the floor.
    # For Ramp Start End the path returns to stock_top_z at the exit — don't
    # force that back down to pass_floor_z or the exit ramp is destroyed.
    depth_range = stock_top_z - floor_z
    if depth_range > _PREC and (pts[-1].z - floor_z) < 0.25 * depth_range:
        result_rev[-1] = FreeCAD.Vector(result_rev[-1].x, result_rev[-1].y, pass_floor_z)
    return result_rev


def _pts_to_wire(pts):
    """Build a polyline Part.Wire from an ordered list of FreeCAD.Vector.
    Returns None if fewer than 2 distinct points.
    """
    edges = [
        Part.makeLine(pts[i], pts[i + 1])
        for i in range(len(pts) - 1)
        if _dist2(pts[i], pts[i + 1]) > _PREC**2
    ]
    if not edges:
        return None
    try:
        return Part.Wire(edges)
    except Exception:
        return None


# ---------------------------------------------------------------------------
# Centerline directly from a selected wire (edges) - bypasses face/solid
# analysis entirely. The wire IS the centerline.
# ---------------------------------------------------------------------------


def _tangent_at_endpoint(edge, endpoint):
    """Unit tangent of edge at whichever of its ends is nearest `endpoint`,
    pointing in the direction of increasing parameter.  Returns None on failure.
    """
    try:
        p_first = edge.valueAt(edge.FirstParameter)
        p_last = edge.valueAt(edge.LastParameter)
        if endpoint.distanceToPoint(p_first) <= endpoint.distanceToPoint(p_last):
            param = edge.FirstParameter
        else:
            param = edge.LastParameter
        t = edge.tangentAt(param)
        length = t.Length
        if length < _PREC:
            return None
        return FreeCAD.Vector(t.x / length, t.y / length, t.z / length)
    except Exception:
        return None


def _tangent_continuous(prev_edge, cur_edge, junction, prev_start, cur_end):
    """True if two connected edges meet smoothly (G1) at `junction`.

    Compares the travel direction arriving along prev_edge (prev_start → junction)
    with the travel direction departing along cur_edge (junction → cur_end).  A
    collinear pair of straight lines and a pair of smoothly-joined arcs both pass;
    a sharp corner (L-shape) fails.  tangentAt is orientation-agnostic, so each
    tangent is flipped to match the actual direction of travel before comparison.
    """
    t1 = _tangent_at_endpoint(prev_edge, junction)
    t2 = _tangent_at_endpoint(cur_edge, junction)
    if t1 is None or t2 is None:
        return False
    # Sign of each tangent along its travel direction.
    arrive = junction - prev_start
    depart = cur_end - junction
    d = t1.dot(t2)
    if t1.dot(arrive) < 0:
        d = -d
    if t2.dot(depart) < 0:
        d = -d
    # d ≈ 1 when the travel directions align (smooth); deviation grows with kink.
    return (1.0 - d) <= _TANGENT_DOT_TOL


def _split_at_corners(edges, tol):
    """Order a connected edge set and split it into tangent-continuous sub-chains.

    Consecutive edges that meet smoothly (collinear straight lines or
    smoothly-joined curves) stay in one sub-chain; a sharp corner starts a new
    one, so each sub-chain becomes an independent flute path.  If the edges do
    not form a single simple open chain (branching / not connected), each edge
    is returned as its own sub-chain.

    Returns a list of ordered edge lists.
    """
    if len(edges) == 1:
        return [list(edges)]

    chain = _chain_wire_edges(edges, tol)
    if not chain:
        return [[edge] for edge in edges]

    subchains = []
    current = [chain[0][2]]
    for i in range(1, len(chain)):
        prev_start, junction, prev_edge = chain[i - 1]
        _cur_start, cur_end, cur_edge = chain[i]
        if _tangent_continuous(prev_edge, cur_edge, junction, prev_start, cur_end):
            current.append(cur_edge)
        else:
            subchains.append(current)
            current = [cur_edge]
    subchains.append(current)
    return subchains


def _split_into_wires(edges, tol):
    """Split a flat list of edges into connected components (separate wires).

    Each component becomes one independent flute (mirrors how separate face
    groups become separate flutes).
    """
    remaining = list(edges)
    wires = []
    while remaining:
        comp = [remaining.pop(0)]
        changed = True
        while changed:
            changed = False
            for e in list(remaining):
                v0, v1 = e.Vertexes[0].Point, e.Vertexes[-1].Point
                for c in comp:
                    cv0, cv1 = c.Vertexes[0].Point, c.Vertexes[-1].Point
                    if (
                        _dist2(v0, cv0) < tol.chain2
                        or _dist2(v0, cv1) < tol.chain2
                        or _dist2(v1, cv0) < tol.chain2
                        or _dist2(v1, cv1) < tol.chain2
                    ):
                        comp.append(e)
                        remaining.remove(e)
                        changed = True
                        break
        wires.append(comp)
    return wires


def _chain_wire_edges(edges, tol):
    """Chain a connected set of edges into ordered (seg_start, seg_end, edge)
    triples by matching coincident endpoints.

    Assumes edges form a single simple open path (no branching).  Returns
    None if they don't all connect into one chain.
    """
    if not edges:
        return None

    remaining = list(edges[1:])
    e0 = edges[0]
    chain = [(e0.Vertexes[0].Point, e0.Vertexes[-1].Point, e0)]

    changed = True
    while remaining and changed:
        changed = False
        cur_s, cur_e = chain[0][0], chain[-1][1]
        for e in list(remaining):
            v0, v1 = e.Vertexes[0].Point, e.Vertexes[-1].Point
            if _dist2(cur_e, v0) < tol.chain2:
                chain.append((v0, v1, e))
            elif _dist2(cur_e, v1) < tol.chain2:
                chain.append((v1, v0, e))
            elif _dist2(cur_s, v0) < tol.chain2:
                chain.insert(0, (v1, v0, e))
            elif _dist2(cur_s, v1) < tol.chain2:
                chain.insert(0, (v0, v1, e))
            else:
                continue
            remaining.remove(e)
            changed = True
            break

    if remaining:
        return None  # could not connect all edges into a single chain

    return chain


def _wire_from_edges(edges, tol):
    """Convert a connected set of selected edges directly into a Part.Wire.
    The edges ARE the centerline — no face/solid analysis.

    Orients so the shallower (higher Z) end is first.

    Returns (Part.Wire, top_z, floor_z), or (None, None, None) on failure.
    """
    chain = _chain_wire_edges(edges, tol)
    if not chain:
        FreeCAD.Console.PrintWarning(
            translate("CAM_Flute", "Selected edges do not form a single connected wire.\n")
        )
        return None, None, None

    # Orient so the shallower (higher Z) free end is the start.
    if chain[0][0].z < chain[-1][1].z:
        chain = [(e, s, edge) for s, e, edge in reversed(chain)]

    pts = PathGeom.edgesToPoints(
        [edge for _, _, edge in chain],
        tol.arc_chord,
        startPoint=chain[0][0],
        error=tol.arc_chord * 0.01,
    )
    if not pts:
        return None, None, None

    wire = _pts_to_wire(pts)
    if wire is None:
        return None, None, None

    all_z = [p.z for p in pts]
    return wire, max(all_z), min(all_z)


# ---------------------------------------------------------------------------
# Profile detection via solid section
# ---------------------------------------------------------------------------


def _detect_floor_wire(face, base_obj, info, tol, group_extent=None):
    """Section base_obj.Shape with a vertical plane through the groove
    centreline and return a Part.Wire representing the floor profile.

    Returns (Part.Wire, floor_z, top_z), or (None, None, None) on any failure.
    The caller falls back to a plain ramp line using the centerline endpoints.
    """
    try:
        start = info["start"]
        end_info = info["end"]
        top_z = info["top_z"]
        floor_z = end_info.z

        dx = end_info.x - start.x
        dy = end_info.y - start.y
        L = math.sqrt(dx * dx + dy * dy)
        if L < _PREC:
            return None, None, None

        path_dir = FreeCAD.Vector(dx / L, dy / L, 0.0)

        # Plane normal is perpendicular to path_dir in XY (i.e. across the groove).
        cx = (start.x + end_info.x) / 2.0
        cy = (start.y + end_info.y) / 2.0
        cz = (top_z + floor_z) / 2.0

        # Use the full group XY span (all selected faces) when available so the
        # plane covers ramp + flat + any exit ramp even though section_info only
        # describes the ramp cylinder face.  Without this, the flat section that
        # extends past the ramp end would fall outside the plane.
        extent = group_extent if (group_extent is not None and group_extent > L) else L
        depth = abs(top_z - floor_z)
        half = max(extent, depth) + tol.section_overshoot
        z_dir = FreeCAD.Vector(0.0, 0.0, 1.0)
        ctr = FreeCAD.Vector(cx, cy, cz)

        p1 = ctr - half * path_dir - half * z_dir
        p2 = ctr + half * path_dir - half * z_dir
        p3 = ctr + half * path_dir + half * z_dir
        p4 = ctr - half * path_dir + half * z_dir
        plane_face = Part.Face(Part.makePolygon([p1, p2, p3, p4, p1]))

        section = base_obj.Shape.section(plane_face)
        all_edges = section.Edges
        if not all_edges:
            Path.Log.debug("CAM_Flute: section returned no edges\n")
            return None, None, None

        Path.Log.debug("CAM_Flute: section returned {} edge(s)\n".format(len(all_edges)))

        # --- filter candidates --------------------------------------------------
        def path_proj(v):
            return (v.x - start.x) * path_dir.x + (v.y - start.y) * path_dir.y

        z_tol = max(1.0, abs(top_z - floor_z) * _SECTION_Z_TOL_FRAC)
        p_min = path_proj(start) - tol.section_overshoot
        p_max = path_proj(start) + extent + tol.section_overshoot

        candidates = []
        for edge in all_edges:
            bb = edge.BoundBox
            if bb.ZMax < floor_z - z_tol or bb.ZMin > top_z + z_tol:
                continue
            mid = edge.CenterOfMass
            if path_proj(mid) < p_min or path_proj(mid) > p_max:
                continue
            # Reject near-vertical edges (side walls).
            v0 = edge.Vertexes[0].Point
            v1 = edge.Vertexes[-1].Point
            ds = abs(path_proj(v1) - path_proj(v0))
            dz = abs(v1.z - v0.z)
            if ds < _WALL_DS_MIN and dz > 0.5:
                continue
            if ds > _WALL_DS_MIN and dz / ds > _WALL_SLOPE_MAX:
                continue
            candidates.append(edge)

        if not candidates:
            Path.Log.debug("CAM_Flute: no floor edge candidates\n")
            return None, None, None

        Path.Log.debug("CAM_Flute: {} candidate floor edge(s)\n".format(len(candidates)))

        # --- chain from start ---------------------------------------------------
        def _orient(edge, toward_pt):
            v0 = edge.Vertexes[0].Point
            v1 = edge.Vertexes[-1].Point
            if _dist2(v0, toward_pt) <= _dist2(v1, toward_pt):
                return v0, v1
            return v1, v0

        remaining = list(candidates)
        best_d, best_e = 1e18, None
        for e in remaining:
            d = min(_dist2(e.Vertexes[0].Point, start), _dist2(e.Vertexes[-1].Point, start))
            if d < best_d:
                best_d, best_e = d, e

        if best_e is None:
            return None, None, None

        remaining.remove(best_e)
        cur_s, cur_e = _orient(best_e, start)
        chain = [(cur_s, cur_e, best_e)]

        for _ in range(_CHAIN_MAX_EDGES):
            nxt_e = nxt_s = nxt_end = None
            bd = tol.chain2
            for e in remaining:
                v0 = e.Vertexes[0].Point
                v1 = e.Vertexes[-1].Point
                d0 = _dist2(v0, cur_e)
                d1 = _dist2(v1, cur_e)
                if d0 < bd:
                    bd, nxt_e, nxt_s, nxt_end = d0, e, v0, v1
                elif d1 < bd:
                    bd, nxt_e, nxt_s, nxt_end = d1, e, v1, v0
            if nxt_e is None:
                break
            if nxt_end.z > top_z + z_tol:
                break
            if nxt_end.z < floor_z - z_tol:
                # Deeper than the known groove floor: this is geometry outside
                # the selected group (e.g. an adjacent groove or a step ledge
                # past the tip), not a continuation of this flute's profile.
                break
            remaining.remove(nxt_e)
            chain.append((nxt_s, nxt_end, nxt_e))
            cur_e = nxt_end

        Path.Log.debug("CAM_Flute: chained {} edge(s)\n".format(len(chain)))

        # --- convert chain to wire ----------------------------------------------
        pts = PathGeom.edgesToPoints(
            [edge for _, _, edge in chain],
            tol.arc_chord,
            startPoint=chain[0][0],
            error=tol.arc_chord * 0.01,
        )
        if not pts:
            return None, None, None

        # Snap floor-level points to the exact minimum Z found.  Two adjacent
        # faces can produce slightly different Z values at their shared edge when
        # sectioned (modeling seam); snapping removes any tiny Z step.
        floor_z = min(p.z for p in pts)
        pts = [
            (
                FreeCAD.Vector(p.x, p.y, floor_z)
                if abs(p.z - floor_z) < tol.floor_snap
                else FreeCAD.Vector(p)
            )
            for p in pts
        ]
        top_z = max(p.z for p in pts)

        wire = _pts_to_wire(pts)
        if wire is None:
            return None, None, None

        Path.Log.debug(
            "CAM_Flute: floor wire {} pts, floor_z={:.3f}, top_z={:.3f}\n".format(
                len(pts), floor_z, top_z
            )
        )
        return wire, floor_z, top_z

    except Exception as exc:
        import traceback

        Path.Log.debug(
            "CAM_Flute: _detect_floor_waypoints error: {}\n{}\n".format(exc, traceback.format_exc())
        )
        return None, None, None


def _rescale_pts_z(pts, geo_top_z, geo_floor_z, op_start_z, op_final_z):
    """Linearly remap waypoint Z values from [geo_floor_z, geo_top_z] to
    [op_final_z, op_start_z].  Preserves profile shape while honouring the
    Depths tab settings."""
    if abs(geo_top_z - geo_floor_z) < _PREC:
        return pts
    z_range_src = geo_top_z - geo_floor_z
    z_range_dst = op_start_z - op_final_z
    result = []
    for p in pts:
        t = (p.z - geo_floor_z) / z_range_src  # 0 = floor, 1 = top
        result.append(FreeCAD.Vector(p.x, p.y, op_final_z + t * z_range_dst))
    return result


def _apply_axial_leave_pts(pts, axial_leave, stock_top_z, tol):
    """Raise the floor Z of all waypoints by axial_leave and trim the entry/exit
    ends inward proportionally (same path angle, shallower depth)."""
    if not pts or axial_leave <= _PREC:
        return pts

    floor_z = min(p.z for p in pts)
    total_d = stock_top_z - floor_z
    if total_d <= axial_leave + _PREC:
        return pts

    new_floor_z = floor_z + axial_leave
    frac = axial_leave / total_d

    # Raise all floor-level points
    result = [
        (
            FreeCAD.Vector(p.x, p.y, new_floor_z)
            if abs(p.z - floor_z) < tol.axial_floor
            else FreeCAD.Vector(p)
        )
        for p in pts
    ]

    # Trim entry: move first point inward if it starts at stock_top_z
    if len(result) >= 2 and abs(result[0].z - stock_top_z) < tol.axial_floor:
        sx = result[0].x + frac * (result[1].x - result[0].x)
        sy = result[0].y + frac * (result[1].y - result[0].y)
        result[0] = FreeCAD.Vector(sx, sy, stock_top_z)

    # Trim exit: move last point inward if it ends at stock_top_z
    if len(result) >= 2 and abs(result[-1].z - stock_top_z) < tol.axial_floor:
        ex = result[-1].x + frac * (result[-2].x - result[-1].x)
        ey = result[-1].y + frac * (result[-2].y - result[-1].y)
        result[-1] = FreeCAD.Vector(ex, ey, stock_top_z)

    return result


# ---------------------------------------------------------------------------
# 2D profile synthesis
# ---------------------------------------------------------------------------


def _simplify_collinear(pts, chord):
    """Remove points that are collinear with their neighbours in 3D space.

    Keeps only points whose perpendicular distance from the segment formed by
    their predecessor and successor exceeds chord * 0.01.  Exactly collinear
    points (e.g. intermediate samples on a straight edge with linear Z) are
    removed; curved or smooth-Z segments are preserved.
    """
    if len(pts) < 3:
        return list(pts)
    tol_sq = (chord * 0.01) ** 2
    result = [pts[0]]
    for i in range(1, len(pts) - 1):
        a, c = result[-1], pts[i + 1]
        b = pts[i]
        acx, acy, acz = c.x - a.x, c.y - a.y, c.z - a.z
        ac2 = acx * acx + acy * acy + acz * acz
        if ac2 < _PREC:
            result.append(b)
            continue
        abx, aby, abz = b.x - a.x, b.y - a.y, b.z - a.z
        t = (abx * acx + aby * acy + abz * acz) / ac2
        px = a.x + t * acx
        py = a.y + t * acy
        pz = a.z + t * acz
        dx, dy, dz = b.x - px, b.y - py, b.z - pz
        if dx * dx + dy * dy + dz * dz < tol_sq:
            continue
        result.append(b)
    result.append(pts[-1])
    return result


def _wire_is_flat(edges, tol):
    """Return True if all edges lie at essentially the same Z (2D wire)."""
    zs = [v.Point.z for e in edges for v in e.Vertexes]
    return (max(zs) - min(zs)) < tol.edge if zs else False


def _apply_blind_end_compensation(pts, tool_radius, tol):
    """Pull a path's floor-depth endpoint back by tool_radius along the approach
    direction so the cutting edge — not the tool centre — reaches the groove end.

    Only applies when the path terminates at depth (last point at the deepest Z);
    a path that ramps back up to the surface is left unchanged.  Points within
    tool_radius (XY) of the blind end are trimmed first, otherwise the
    compensated endpoint would be appended after points already past it, making
    the path overshoot then reverse.

    Mutates and returns pts.
    """
    if len(pts) < 2 or tool_radius <= _PREC * 10:
        return pts
    floor_z = min(p.z for p in pts)
    if abs(pts[-1].z - floor_z) >= tol.floor_snap:
        return pts

    blind_end = pts[-1]
    tr2 = tool_radius * tool_radius
    while len(pts) > 1:
        dx = pts[-1].x - blind_end.x
        dy = pts[-1].y - blind_end.y
        if dx * dx + dy * dy >= tr2:
            break
        pts.pop()

    dx = blind_end.x - pts[-1].x
    dy = blind_end.y - pts[-1].y
    seg_len = math.sqrt(dx * dx + dy * dy)
    if seg_len > _PREC:
        pts.append(
            FreeCAD.Vector(
                blind_end.x - (dx / seg_len) * tool_radius,
                blind_end.y - (dy / seg_len) * tool_radius,
                blind_end.z,
            )
        )
    return pts


def _apply_2d_profile(
    edges,
    start_z,
    floor_z,
    flute_type,
    ramp_type,
    ramp_frac,
    tol,
    flip=False,
    path_frac=1.0,
    path_offset=0.0,
    ramp_length_mm=0.0,
):
    """Synthesize a 3D wire from a flat (constant-Z) edge set by applying a Z ramp profile.

    flute_type:     "Ramp Full" | "Ramp Start" | "Ramp Start End"
    ramp_type:      "Linear" | "S-Curve" | "Smooth" | "Fillet"
    ramp_frac:      fraction of total path length occupied by ramp(s) (0–1).
                    Ignored when ramp_length_mm > 0.
    ramp_length_mm: if > 0, overrides ramp_frac with length/total_wire_length.
    flip:           if True, reverse the wire direction before applying the profile.
    path_frac:      fraction of the (post-flip) wire to use; 1.0 = full length.
    path_offset:    fractional start position (0–1) within the full wire.
                    path_offset=1-path_frac → clip from exit end (Ramp Full fan).
                    path_offset=(1-path_frac)/2 → symmetric centre clip (Ramp Start End fan).
                    path_offset=0 → clip from entry end (Ramp Start fan).

    Returns a Part.Wire with the profile Z applied, or None on failure.
    """
    chain = _chain_wire_edges(edges, tol)
    if not chain:
        return None

    # Always discretize all edge types so intermediate arc-length positions
    # exist for Z-profile sampling.  _simplify_collinear removes redundant
    # collinear points afterward (e.g. straight edge + linear Z → 2 pts).
    raw = []
    for seg_s, seg_e, edge in chain:
        disc = edge.discretize(Distance=tol.arc_chord)
        if disc and _dist2(disc[0], seg_s) > _dist2(disc[-1], seg_s):
            disc = list(reversed(disc))
        for p in disc:
            pv = FreeCAD.Vector(p.x, p.y, start_z)
            if not raw or _dist2(raw[-1], pv) > (tol.arc_chord * 0.01) ** 2:
                raw.append(pv)

    if not raw:
        return None

    if flip:
        raw = list(reversed(raw))

    # Cumulative XY arc lengths
    n = len(raw)
    arc_len = [0.0] * n
    for i in range(1, n):
        dx = raw[i].x - raw[i - 1].x
        dy = raw[i].y - raw[i - 1].y
        arc_len[i] = arc_len[i - 1] + math.sqrt(dx * dx + dy * dy)
    total_full = arc_len[-1]
    if total_full < _PREC:
        return None

    # RampLength in mm overrides ramp_frac when set.
    if ramp_length_mm > _PREC:
        ramp_frac = min(1.0, ramp_length_mm / total_full)

    # Sub-path clipping: keeps path_frac of the wire starting at path_offset.
    # Used for progressive pass shortening — clips the XY path BEFORE applying
    # the Z profile so ramp angles stay constant across all step-down passes.
    if path_frac < 1.0 - _PREC or path_offset > _PREC:
        t0 = path_offset * total_full
        t1 = min((path_offset + path_frac) * total_full, total_full)
        kept_raw = []
        kept_arc = []
        for i in range(n):
            if t0 - _PREC <= arc_len[i] <= t1 + _PREC:
                kept_raw.append(raw[i])
                kept_arc.append(max(0.0, arc_len[i] - t0))
        if not kept_raw:
            return None
        raw = kept_raw
        arc_len = kept_arc
        total = arc_len[-1]
        if total < _PREC:
            return None
    else:
        total = total_full

    depth = start_z - floor_z

    def _z_at(t):
        """Return Z for parametric position t ∈ [0, 1] along the wire."""
        if flute_type == "Ramp Full":
            f = t
        elif flute_type == "Ramp Start":
            f = min(t / ramp_frac, 1.0) if ramp_frac > _PREC else 1.0
        elif flute_type == "Ramp Start End":
            half = ramp_frac / 2.0
            if half < _PREC:
                f = 1.0
            elif t <= half:
                f = t / half
            elif t >= 1.0 - half:
                f = (1.0 - t) / half
            else:
                f = 1.0
        else:
            f = t

        # f is the normalized ramp position: 0 at the surface entry, 1 at the
        # floor.  Each curve reshapes it into a depth fraction.
        if ramp_type == "S-Curve":
            # Smoothstep: zero slope at BOTH ends (eased entry and floor).
            f = f * f * (3.0 - 2.0 * f)
        elif ramp_type == "Smooth":
            # Quarter-sine: tangent (zero slope) at the floor, angled entry.
            # Length-driven; the tangent flattening compresses as the ramp
            # length shrinks relative to depth.
            f = math.sin(f * math.pi / 2)
        elif ramp_type == "Fillet":
            # Quarter-ellipse: the roundest tangent-to-floor blend that fits the
            # ramp box, so the tool rolls tangentially into the floor for any
            # ramp length/depth.  Steeper (near-vertical) at the entry.
            f = math.sqrt(max(0.0, 1.0 - (1.0 - f) ** 2))

        return start_z - f * depth

    pts = [FreeCAD.Vector(p.x, p.y, _z_at(arc_len[i] / total)) for i, p in enumerate(raw)]
    pts = _simplify_collinear(pts, tol.arc_chord)
    return _pts_to_wire(pts)


# ---------------------------------------------------------------------------
# Operation class
# ---------------------------------------------------------------------------


class ObjectFlute(PathOp.ObjectOp):
    """Proxy object for the Flute operation.

    A flute is a ramping slot cut – shallow at one end, deeper at the other.
    The cutting path is derived from selected bottom faces:

    - A single face (flat/bull-nose):  centerline found by PCA.
    - Two faces sharing a concave edge (V-bottom):  the shared edge is the
      centerline.
    - Multiple groups (e.g. two V-flutes side-by-side with a ridge between
      them):  the face adjacency graph is analysed so that faces connected
      only by a convex (peak) edge are placed in separate flute groups.

    Multiple passes step down incrementally.  Each intermediate pass starts
    where the ramp first contacts uncut stock so that air-cut time is
    minimised.
    """

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureFinishDepth
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureLinking
        )

    def initOperation(self, obj):
        obj.addProperty(
            "App::PropertyBool",
            "ReverseDirection",
            "Flute",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Reverse the cut direction (enters at the deep end).",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "AxialStockToLeave",
            "Flute",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Set the stock to leave in the axial (depth) direction.",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "BlindEndCompensation",
            "Flute",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Pull the path end back by the tool radius when the flute "
                "terminates at depth (blind end). Has no effect when the "
                "path ramps back up to stock surface.",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "FlutingType",
            "Flute2D",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Z profile applied when a flat (2D) wire is selected: "
                "RampFull ramps the full length; RampStart ramps only the "
                "entry; RampStartEnd ramps both entry and exit.",
            ),
        )
        obj.FlutingType = ["Ramp Full", "Ramp Start", "Ramp Start End"]
        obj.addProperty(
            "App::PropertyEnumeration",
            "RampType",
            "Flute2D",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Shape of the Z ramp on 2D wires: Linear is a straight plunge; "
                "S-Curve eases at both ends; Smooth is tangent to the floor "
                "with an angled entry; Fillet rounds tangentially into the floor.",
            ),
        )
        obj.RampType = ["Linear", "S-Curve", "Smooth", "Fillet"]
        obj.addProperty(
            "App::PropertyBool",
            "FlipStart2D",
            "Flute2D",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Reverse which end of the flat wire is treated as the entry "
                "point for the 2D ramp profile.",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "RampLength",
            "Flute2D",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "Length of each ramp segment in mm (2D wires only). "
                "When greater than zero, overrides Ramp %. "
                "The fraction is derived at compute time from this length "
                "divided by the actual wire length.",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "MultiPassStrategy",
            "Depth",
            QtCore.QT_TRANSLATE_NOOP(
                "App::Property",
                "How roughing passes are distributed across step-down depths. "
                "Constant Angle: same ramp slope every pass — entry point walks, "
                "path shortens (lower peak chip load). "
                "Variable Angle: full path length every pass — angle steepens "
                "each depth (uniform XY engagement, longer cycle time).",
            ),
        )
        obj.MultiPassStrategy = ["Constant Angle", "Variable Angle"]

    def opSetDefaultValues(self, obj, job):
        obj.ReverseDirection = False
        obj.AxialStockToLeave = 0.0
        obj.BlindEndCompensation = False
        obj.FlutingType = "Ramp Full"
        obj.RampType = "Linear"
        obj.FlipStart2D = False
        obj.RampLength = 0.0
        obj.MultiPassStrategy = "Constant Angle"

        d = None
        if job and job.Stock:
            d = PathUtils.guessDepths(job.Stock.Shape, None)

        if d is not None:
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
        else:
            obj.OpFinalDepth.Value = -10.0
            obj.OpStartDepth.Value = 0.0

    def opApplyPropertyLimits(self, obj):
        # No property range clamping needed for this operation.
        pass

    def opUpdateDepths(self, obj):
        """Auto-populate start/final depths from the face bounding boxes."""
        if not hasattr(obj, "Base") or not obj.Base:
            return

        z_max = None
        z_min = None

        for base, sub_list in obj.Base:
            for sub in sub_list:
                try:
                    bb = base.Shape.getElement(sub).BoundBox
                    z_max = bb.ZMax if z_max is None else max(z_max, bb.ZMax)
                    z_min = bb.ZMin if z_min is None else min(z_min, bb.ZMin)
                except Part.OCCError as err:
                    Path.Log.error(err)

        if z_max is not None:
            obj.OpStartDepth = z_max
        if z_min is not None:
            obj.OpFinalDepth = z_min

    def onChanged(self, obj, prop):
        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def _emitFlutePath(
        self,
        obj,
        wire,
        geo_top_z,
        geo_floor_z,
        sub_names,
        op_start_z,
        op_final_z,
        axial_leave,
        finish_d,
        step_down,
        retract_z,
        linking_kwargs,
        tool_pos,
        tol,
    ):
        """Take a Part.Wire (from face analysis, edge selection, or 2D profile
        synthesis) through Z remap, axial-leave, blind-end compensation, pass
        generation, linking, and G-code emission.

        Returns (new_tool_pos, emitted) where emitted is True if any G-code
        was appended to self.commandlist.
        """
        # --- remap Z from geometry to operation Depths tab -------------------
        pts = PathGeom.edgesToPoints(wire.Edges, tol.arc_chord)
        if not pts:
            return tool_pos, False

        if abs(geo_top_z - geo_floor_z) > _PREC:
            pts = _rescale_pts_z(pts, geo_top_z, geo_floor_z, op_start_z, op_final_z)

        # --- axial stock to leave -------------------------------------------
        if axial_leave > _PREC:
            pts = _apply_axial_leave_pts(pts, axial_leave, op_start_z, tol)

        # --- blind end compensation -----------------------------------------
        if getattr(obj, "BlindEndCompensation", False):
            try:
                tool_radius = obj.ToolController.Tool.Diameter.Value / 2.0
            except Exception:
                tool_radius = 0.0
            _apply_blind_end_compensation(pts, tool_radius, tol)

        # --- step-down pass loop --------------------------------------------
        floor_z = min(p.z for p in pts)
        total_depth = op_start_z - floor_z
        if total_depth <= _PREC:
            FreeCAD.Console.PrintWarning(
                translate("CAM_Flute", "No depth to cut for: {}\n").format(sub_names)
            )
            return tool_pos, False

        multi_pass_strategy = getattr(obj, "MultiPassStrategy", "Constant Angle")
        rough_stop = max(0.0, total_depth - max(0.0, finish_d))
        pass_pts_list = []
        depth = 0.0
        while depth < rough_stop - _PREC:
            depth = min(depth + step_down, rough_stop)
            f = depth / total_depth
            if multi_pass_strategy == "Variable Angle":
                wps = [FreeCAD.Vector(p.x, p.y, op_start_z + (p.z - op_start_z) * f) for p in pts]
            else:
                wps = _clip_and_scale(pts, f, op_start_z)
            if obj.ReverseDirection:
                wps = list(reversed(wps))
            pass_pts_list.append(wps)

        if finish_d > _PREC and total_depth > rough_stop + _PREC:
            if multi_pass_strategy == "Variable Angle":
                wps = list(pts)  # f=1.0: original Z values unchanged
            else:
                wps = _clip_and_scale(pts, 1.0, op_start_z)
            if obj.ReverseDirection:
                wps = list(reversed(wps))
            pass_pts_list.append(wps)

        if not pass_pts_list:
            FreeCAD.Console.PrintWarning(
                translate("CAM_Flute", "No passes computed for: {}\n").format(sub_names)
            )
            return tool_pos, False

        first_cut = pass_pts_list[0][0]
        entry_pos = FreeCAD.Vector(first_cut.x, first_cut.y, obj.SafeHeight.Value)

        if tool_pos is not None:
            linking_kwargs["start_position"] = tool_pos
            linking_kwargs["target_position"] = entry_pos
            self.commandlist.extend(linking.get_linking_moves(**linking_kwargs))

        for wps in pass_pts_list:
            pass_wire = _pts_to_wire(wps)
            if pass_wire is None:
                continue
            cmds = WireFollowGenerator.generate(
                pass_wire,
                retract_z,
                self.horizFeed,
                self.vertFeed,
                arc_chord=tol.arc_chord,
            )
            self.commandlist.extend(cmds)

        last_pt = pass_pts_list[-1][-1]
        return FreeCAD.Vector(last_pt.x, last_pt.y, retract_z), True

    def _emit2dPasses(
        self,
        obj,
        wire_edges,
        flute_type,
        ramp_type,
        ramp_frac,
        flip,
        op_start_z,
        op_final_z,
        axial_leave,
        finish_d,
        step_down,
        retract_z,
        linking_kwargs,
        tool_pos,
        tol,
        sub_names,
    ):
        """Emit multi-pass G-code for Ramp Start / Ramp Start End 2D profiles.

        Re-synthesises the Z profile at each step-down depth so every pass has
        the correct geometric shape.  _clip_and_scale clips from pts[-1], which
        gives wrong results for profiles where the floor is not at the exit:
          - Ramp Start End: floor is in the middle; clipping from exit gives
            only the exit-ramp half in early passes.
          - Ramp Start:     floor IS at pts[-1] but clipping from there gives
            flat-only passes until the very last pass adds the ramp.

        Returns (new_tool_pos, emitted).
        """
        effective_final_z = op_final_z + max(0.0, axial_leave)
        total_depth = op_start_z - effective_final_z
        if total_depth <= _PREC:
            FreeCAD.Console.PrintWarning(
                translate("CAM_Flute", "No depth to cut for: {}\n").format(sub_names)
            )
            return tool_pos, False

        rough_stop = max(0.0, total_depth - max(0.0, finish_d))
        # Each pass stores (pass_floor_z, depth_fraction f).
        # f drives path_frac / path_offset so all passes keep the same ramp angle:
        #   Ramp Full:       path_offset = 1-f  (fan grows from exit toward entry)
        #   Ramp Start:      path_offset = 0    (fan grows from entry toward exit)
        #   Ramp Start End:  path_offset = (1-f)/2  (symmetric fan from centre)
        passes = []
        d = 0.0
        while d < rough_stop - _PREC:
            d = min(d + step_down, rough_stop)
            passes.append((op_start_z - d, d / total_depth))
        if finish_d > _PREC and total_depth > rough_stop + _PREC:
            passes.append((effective_final_z, 1.0))

        if not passes:
            return tool_pos, False

        multi_pass_strategy = getattr(obj, "MultiPassStrategy", "Constant Angle")
        ramp_len_prop = getattr(obj, "RampLength", None)
        ramp_length_mm = ramp_len_prop.Value if ramp_len_prop is not None else 0.0

        pass_wires = []
        for pf, f in passes:
            if multi_pass_strategy == "Variable Angle":
                # Full XY path every pass — angle steepens with each depth step.
                w = _apply_2d_profile(
                    wire_edges,
                    op_start_z,
                    pf,
                    flute_type,
                    ramp_type,
                    ramp_frac,
                    tol,
                    flip,
                    ramp_length_mm=ramp_length_mm,
                )
            else:
                # Constant Angle: fan from start — same slope every pass, path shortens.
                if flute_type == "Ramp Start End":
                    p_offset = (1.0 - f) / 2.0  # symmetric fan from centre
                else:
                    p_offset = 1.0 - f  # fan from start (entry walks)
                w = _apply_2d_profile(
                    wire_edges,
                    op_start_z,
                    pf,
                    flute_type,
                    ramp_type,
                    ramp_frac,
                    tol,
                    flip,
                    path_frac=f,
                    path_offset=p_offset,
                    ramp_length_mm=ramp_length_mm,
                )
            if w is not None:
                pass_wires.append(w)

        if not pass_wires:
            return tool_pos, False

        fp = PathGeom.edgesToPoints(pass_wires[0].Edges, tol.arc_chord)
        if not fp:
            return tool_pos, False

        entry_pos = FreeCAD.Vector(fp[0].x, fp[0].y, obj.SafeHeight.Value)
        if tool_pos is not None:
            linking_kwargs["start_position"] = tool_pos
            linking_kwargs["target_position"] = entry_pos
            self.commandlist.extend(linking.get_linking_moves(**linking_kwargs))

        try:
            tool_radius = obj.ToolController.Tool.Diameter.Value / 2.0
        except Exception:
            tool_radius = 0.0

        # Blind-end compensation only applies when the path ends at floor depth.
        # Ramp Start End always exits back to surface — no blind end.
        do_blind = (
            getattr(obj, "BlindEndCompensation", False)
            and tool_radius > _PREC * 10
            and flute_type != "Ramp Start End"
        )

        emitted = False
        last_pts = fp
        for w in pass_wires:
            pts = PathGeom.edgesToPoints(w.Edges, tol.arc_chord)
            if not pts:
                continue

            if do_blind:
                _apply_blind_end_compensation(pts, tool_radius, tol)

            if obj.ReverseDirection:
                pts = list(reversed(pts))

            pass_wire = _pts_to_wire(pts)
            if pass_wire is None:
                continue

            cmds = WireFollowGenerator.generate(
                pass_wire,
                retract_z,
                self.horizFeed,
                self.vertFeed,
                arc_chord=tol.arc_chord,
            )
            if cmds:
                self.commandlist.extend(cmds)
                emitted = True
                last_pts = pts

        if emitted and last_pts:
            return FreeCAD.Vector(last_pts[-1].x, last_pts[-1].y, retract_z), True
        return tool_pos, False

    def opExecute(self, obj):
        """Build the path for all selected flute faces and/or edges."""
        Path.Log.track()

        self.commandlist = []

        if not hasattr(obj, "Base") or not obj.Base:
            FreeCAD.Console.PrintError(
                translate("CAM_Flute", "No base geometry selected for Flute operation.\n")
            )
            return False

        step_down = obj.StepDown.Value
        if step_down <= 0:
            FreeCAD.Console.PrintError(
                translate("CAM_Flute", "StepDown must be greater than zero.\n")
            )
            return False

        # All absolute-distance tolerances used below scale with the Job's
        # configured GeometryTolerance (same property Profile/Deburr/Engrave/
        # Waterline/etc. already use), rather than being fixed mm values.
        geom_tol = self.job.GeometryTolerance.Value if getattr(self, "job", None) else None
        tol = _make_tolerances(geom_tol)

        # Selected subelements can be faces (analysed via solid section) or
        # edges (used directly as the centerline wire, no analysis needed).
        face_tuples = []
        edge_tuples = []
        for base, sub_list in obj.Base:
            for sub in sub_list:
                try:
                    elem = base.Shape.getElement(sub)
                except Part.OCCError as err:
                    Path.Log.error(err)
                    continue
                if elem.ShapeType == "Face":
                    face_tuples.append((elem, sub, base))
                elif elem.ShapeType == "Edge":
                    edge_tuples.append((elem, sub, base))

        if not face_tuples and not edge_tuples:
            FreeCAD.Console.PrintError(
                translate("CAM_Flute", "No valid faces or edges found in base geometry.\n")
            )
            return False

        groups = _group_flutes(face_tuples, tol) if face_tuples else []

        Path.Log.debug(
            "CAM_Flute: {} face(s) -> {} group(s), {} edge(s)\n".format(
                len(face_tuples), len(groups), len(edge_tuples)
            )
        )

        if obj.Comment:
            self.commandlist.append(Path.Command("({})".format(obj.Comment)))
        self.commandlist.append(Path.Command("({})".format(obj.Label)))
        self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        solids = []
        if getattr(self, "job", None) and hasattr(self.job, "Model"):
            solids = [b.Shape for b in self.job.Model.Group if hasattr(b, "Shape")]

        linking_kwargs = {
            "start_position": None,
            "target_position": None,
            "heights_clearance": (obj.SafeHeight.Value, obj.ClearanceHeight.Value),
            "solids": None,
            "tool_shape": None,
            "tool_diameter": None,
            "collision_clearance": obj.CollisionClearance.Value,
        }
        strategy = obj.CollisionAvoidanceStrategy
        if strategy == "Clearance Height":
            linking_kwargs["heights_clearance"] = obj.ClearanceHeight.Value
        elif strategy == "Line of Sight":
            linking_kwargs["solids"] = solids
        elif strategy == "Tool Diameter":
            linking_kwargs["solids"] = solids
            linking_kwargs["tool_diameter"] = obj.ToolController.Tool.Diameter.Value
        elif strategy == "Tool Shape":
            linking_kwargs["solids"] = solids
            linking_kwargs["tool_shape"] = obj.ToolController.Tool.BitBody.Shape

        retract_z = (
            obj.ClearanceHeight.Value if strategy == "Clearance Height" else obj.SafeHeight.Value
        )

        op_start_z = obj.StartDepth.Value
        op_final_z = obj.FinalDepth.Value
        axial_leave = getattr(obj, "AxialStockToLeave", None)
        axial_leave = axial_leave.Value if axial_leave is not None else 0.0
        finish_d = getattr(obj, "FinishDepth", None)
        finish_d = finish_d.Value if finish_d is not None else 0.0

        any_path = False
        tool_pos = None

        for group in groups:
            sub_names = [ft[1] for ft in group["faces"]]

            # --- centerline selection ---
            # V-bottom groups (valley edge present) MUST use the valley-edge
            # centerline - it bisects the two faces, unlike either face's own
            # cylinder axis. Only fall back to per-face cylinder analysis for
            # groups with no valley edge (e.g. ramp + flat merged by XY-overlap),
            # where _get_centerline's PCA-on-combined-faces is less accurate
            # than just analysing the ramp cylinder face directly.
            section_info = None
            section_face = None
            section_base = None

            if group.get("valley_edge") is not None:
                section_info = _get_centerline(group, tol)
                section_face = group["faces"][0][0]
                section_base = group["faces"][0][2]
            else:
                for ft in group["faces"]:
                    cyl_info = _analyze_face_cylinder(ft[0])
                    if cyl_info is not None:
                        section_info = cyl_info
                        section_face = ft[0]
                        section_base = ft[2]
                        break

                if section_info is None:
                    section_info = _get_centerline(group, tol)
                    section_face = group["faces"][0][0]
                    section_base = group["faces"][0][2]

            if section_info is None:
                FreeCAD.Console.PrintWarning(
                    translate("CAM_Flute", "Could not determine centerline for: {}\n").format(
                        sub_names
                    )
                )
                continue

            _check_tool_fit(section_info, self.tool)

            geo_top_z = section_info["top_z"]
            geo_floor_z = section_info["end"].z

            Path.Log.debug(
                "CAM_Flute: centerline start=({:.3f},{:.3f},{:.3f}) "
                "end=({:.3f},{:.3f},{:.3f})\n".format(
                    section_info["start"].x,
                    section_info["start"].y,
                    section_info["start"].z,
                    section_info["end"].x,
                    section_info["end"].y,
                    section_info["end"].z,
                )
            )

            # --- profile detection via solid section ---
            # Compute the XY span of ALL faces in the group so the section
            # plane covers ramp + flat + any exit ramp.
            bb_list = [ft[0].BoundBox for ft in group["faces"]]
            group_xy_span = math.sqrt(
                (max(b.XMax for b in bb_list) - min(b.XMin for b in bb_list)) ** 2
                + (max(b.YMax for b in bb_list) - min(b.YMin for b in bb_list)) ** 2
            )
            wire, detected_floor_z, detected_top_z = _detect_floor_wire(
                section_face, section_base, section_info, tol, group_extent=group_xy_span
            )

            if wire is None:
                # Fall back to a plain ramp wire using the centerline endpoints.
                Path.Log.debug("CAM_Flute: falling back to plain ramp wire\n")
                wire = _pts_to_wire(
                    [
                        FreeCAD.Vector(section_info["start"]),
                        FreeCAD.Vector(section_info["end"]),
                    ]
                )
                if wire is None:
                    continue
            else:
                # Re-derive Z range from the detected wire, not the centerline
                # estimate.  PCA / cylinder analysis can disagree with the real
                # sectioned floor depth.
                geo_top_z = detected_top_z
                geo_floor_z = detected_floor_z

            tool_pos, emitted = self._emitFlutePath(
                obj,
                wire,
                geo_top_z,
                geo_floor_z,
                sub_names,
                op_start_z,
                op_final_z,
                axial_leave,
                finish_d,
                step_down,
                retract_z,
                linking_kwargs,
                tool_pos,
                tol,
            )
            any_path = any_path or emitted

        # --- edges selected directly as the flute path ----------------------
        # Each connected component of selected edges is one flute.
        # Mixed selections (some flat, some carrying Z) are rejected — the
        # user must create separate operations for 2D and 3D wires.
        if edge_tuples:
            edge_by_id = {id(e): sub for e, sub, _b in edge_tuples}
            wire_groups = _split_into_wires([e for e, _sub, _b in edge_tuples], tol)
            Path.Log.debug(
                "CAM_Flute: {} edge(s) -> {} wire(s)\n".format(len(edge_tuples), len(wire_groups))
            )

            flat_groups = [wg for wg in wire_groups if _wire_is_flat(wg, tol)]
            solid_groups = [wg for wg in wire_groups if not _wire_is_flat(wg, tol)]

            if flat_groups and solid_groups:
                Path.Log.warning(
                    "Flute: mixed 2D (flat) and 3D wires in the same selection "
                    "are not supported. Use separate operations for each type.\n"
                )
            elif flat_groups:
                # 2D path: split connected groups into collinear sub-groups.
                # Edges that are connected but NOT collinear (L-shapes, curves)
                # each become their own independent flute path.
                flute_type = getattr(obj, "FlutingType", "Ramp Full")
                ramp_type = getattr(obj, "RampType", "Linear")
                ramp_frac = 1.0  # full wire; overridden by RampLength if set
                flip = getattr(obj, "FlipStart2D", False)
                processed_groups = []
                for group in flat_groups:
                    processed_groups.extend(_split_at_corners(group, tol))
                Path.Log.debug(
                    "CAM_Flute: {} flat group(s) -> {} 2D flute path(s) after corner split\n".format(
                        len(flat_groups), len(processed_groups)
                    )
                )
                for wire_edges in processed_groups:
                    sub_names = [edge_by_id.get(id(e), "?") for e in wire_edges]
                    tool_pos, emitted = self._emit2dPasses(
                        obj,
                        wire_edges,
                        flute_type,
                        ramp_type,
                        ramp_frac,
                        flip,
                        op_start_z,
                        op_final_z,
                        axial_leave,
                        finish_d,
                        step_down,
                        retract_z,
                        linking_kwargs,
                        tool_pos,
                        tol,
                        sub_names,
                    )
                    any_path = any_path or emitted
            else:
                # 3D wires: edges carry Z information directly.  Split each
                # connected component at sharp corners so a smooth run (straight
                # or curved) is one flute and an L-corner becomes two.
                solid_paths = []
                for group in solid_groups:
                    solid_paths.extend(_split_at_corners(group, tol))
                for wire_edges in solid_paths:
                    sub_names = [edge_by_id.get(id(e), "?") for e in wire_edges]
                    wire, geo_top_z, geo_floor_z = _wire_from_edges(wire_edges, tol)
                    if wire is None:
                        continue
                    tool_pos, emitted = self._emitFlutePath(
                        obj,
                        wire,
                        geo_top_z,
                        geo_floor_z,
                        sub_names,
                        op_start_z,
                        op_final_z,
                        axial_leave,
                        finish_d,
                        step_down,
                        retract_z,
                        linking_kwargs,
                        tool_pos,
                        tol,
                    )
                    any_path = any_path or emitted

        if not any_path:
            self.commandlist.clear()
            return False

        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )
        return True


# ---------------------------------------------------------------------------
# Module-level API expected by PathOpGui.SetupOperation
# ---------------------------------------------------------------------------


def SetupProperties():
    return [
        "ReverseDirection",
        "AxialStockToLeave",
        "BlindEndCompensation",
        "MultiPassStrategy",
        "FlutingType",
        "RampType",
        "RampLength",
        "FlipStart2D",
    ]


def Create(name, obj=None, parentJob=None):
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectFlute(obj, name, parentJob)
    return obj
