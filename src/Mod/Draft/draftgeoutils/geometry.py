# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions for general geometrical calculations."""
## @package geometry
# \ingroup draftgeoutils
# \brief Provides various functions for general geometrical calculations.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

import draftutils.gui_utils as gui_utils

from draftgeoutils.general import geomType, vec

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def findPerpendicular(point, edgeslist, force=None):
    """Find the perpendicular distance between a point and a list of edges.

    If force is specified, only the edge[force] will be considered,
    and it will be considered infinite.

    Returns
    -------
    [vector_from_point_to_closest_edge, edge_index]
        The vector and the index in the list.

    None
        If no perpendicular vector could be found.
    """
    if not isinstance(edgeslist, list):
        try:
            edgeslist = edgeslist.Edges
        except AttributeError:
            print("Doesn't have 'Edges'")
            return None

    if force is None:
        valid = None
        for edge in edgeslist:
            dist = findDistance(point, edge, strict=True)
            if dist:
                if not valid:
                    valid = [dist, edgeslist.index(edge)]
                else:
                    if dist.Length < valid[0].Length:
                        valid = [dist, edgeslist.index(edge)]
        return valid
    else:
        edge = edgeslist[force]
        dist = findDistance(point, edge)
        if dist:
            return [dist, force]
        else:
            return None
        return None


def findDistance(point, edge, strict=False):
    """Return a vector from the point to its closest point on the edge.

    If `strict` is `True`, the vector will be returned
    only if its endpoint lies on the `edge`.
    Edge can also be a list of 2 points.
    """
    if isinstance(point, App.Vector):
        if isinstance(edge, list):
            segment = edge[1].sub(edge[0])
            chord = edge[0].sub(point)
            norm = segment.cross(chord)
            perp = segment.cross(norm)
            dist = DraftVecUtils.project(chord, perp)

            if not dist:
                return None

            newpoint = point.add(dist)

            if dist.Length == 0:
                return None

            if strict:
                s1 = newpoint.sub(edge[0])
                s2 = newpoint.sub(edge[1])
                if (s1.Length <= segment.Length
                        and s2.Length <= segment.Length):
                    return dist
                else:
                    return None
            else:
                return dist

        elif geomType(edge) == "Line":
            segment = vec(edge)
            chord = edge.Vertexes[0].Point.sub(point)
            norm = segment.cross(chord)
            perp = segment.cross(norm)
            dist = DraftVecUtils.project(chord, perp)

            if not dist:
                return None

            newpoint = point.add(dist)

            if (dist.Length == 0):
                return None

            if strict:
                s1 = newpoint.sub(edge.Vertexes[0].Point)
                s2 = newpoint.sub(edge.Vertexes[-1].Point)
                if (s1.Length <= segment.Length
                        and s2.Length <= segment.Length):
                    return dist
                else:
                    return None
            else:
                return dist

        elif geomType(edge) == "Circle":
            ve1 = edge.Vertexes[0].Point
            if len(edge.Vertexes) > 1:
                ve2 = edge.Vertexes[-1].Point
            else:
                ve2 = None
            center = edge.Curve.Center
            segment = center.sub(point)

            if segment.Length == 0:
                return None

            ratio = (segment.Length - edge.Curve.Radius) / segment.Length
            dist = segment.multiply(ratio)
            newpoint = App.Vector.add(point, dist)

            if dist.Length == 0:
                return None

            if strict and ve2:
                ang1 = DraftVecUtils.angle(ve1.sub(center))
                ang2 = DraftVecUtils.angle(ve2.sub(center))
                angpt = DraftVecUtils.angle(newpoint.sub(center))
                if ((angpt <= ang2 and angpt >= ang1)
                        or (angpt <= ang1 and angpt >= ang2)):
                    return dist
                else:
                    return None
            else:
                return dist

        elif (geomType(edge) == "BSplineCurve"
              or geomType(edge) == "BezierCurve"):
            try:
                pr = edge.Curve.parameter(point)
                np = edge.Curve.value(pr)
                dist = np.sub(point)
            except Part.OCCError:
                print("DraftGeomUtils: Unable to get curve parameter "
                      "for point ", point)
                return None
            else:
                return dist
        else:
            print("DraftGeomUtils: Couldn't project point")
            return None
    else:
        print("DraftGeomUtils: Couldn't project point")
        return None


def get_spline_normal(edge, tol=-1):
    """Find the normal of a BSpline edge."""

    if is_straight_line(shape, tol):
        return None

    plane = edge.findPlane(tol)
    if plane:
        normal = plane.Axis
        return normal
    else:
        return None


def get_normal(shape, tol=-1):
    """Find the normal of a shape or list of points, if possible."""

    # for points
    if isinstance(shape, (list, tuple)):
        if len(shape) <= 2:
            return None
        else:
            poly = Part.makePolygon(shape)
            if is_straight_line(poly, tol):
                return None

            plane = poly.findPlane(tol)
            if plane:
                normal = plane.Axis
                return normal
            else:
                return None

    # for shapes
    if is_straight_line(shape, tol):
        return None

    else:
        plane = find_plane(shape, tol)
        if plane:
            normal = plane.Axis
        else:
            return None

    # Check the 3D view to flip the normal if the GUI is available
    if App.GuiUp:
        v_dir = gui_utils.get_3d_view().getViewDirection()
        if normal.getAngle(v_dir) < 0.78:
            normal = normal.negative()

    return normal


def getRotation(v1, v2=App.Vector(0, 0, 1)):
    """Get the rotation Quaternion between 2 vectors."""
    if (v1.dot(v2) > 0.999999) or (v1.dot(v2) < -0.999999):
        # vectors are opposite
        return None

    axis = v1.cross(v2)
    axis.normalize()
    # angle = math.degrees(math.sqrt(v1.Length^2 * v2.Length^2) + v1.dot(v2))
    angle = math.degrees(DraftVecUtils.angle(v1, v2, axis))
    return App.Rotation(axis, angle)


def is_planar(shape, tol=-1):
    """Return True if the given shape or list of points is planar."""

    # for points
    if isinstance(shape, list):
        if len(shape) <= 3:
            return True
        else:
            poly = Part.makePolygon(shape)
            if is_straight_line(poly, tol):
                return True
            plane = poly.findPlane(tol)
            if plane:
                return True
            else:
                return False

    # for shapes
    # because Part.Shape.findPlane return None for Vertex and straight edges
    if shape.ShapeType == "Vertex":
        return True

    if is_straight_line(shape, tol):
        return True

    plane = find_plane(shape, tol)
    if plane:
        return True
    else:
        return False


def is_straight_line(shape, tol=-1):
    """Return True if shape is a straight line.
    function used in other methods because Part.Shape.findPlane assign a
    plane and normal to straight wires creating privileged directions
    and to deal with straight wires with overlapped edges."""

    if len(shape.Faces) != 0:
        return False

    if len(shape.Edges) == 0:
        return False

    if len(shape.Edges) >= 1:
        start_edge = shape.Edges[0]
        dir_start_edge = start_edge.tangentAt(start_edge.FirstParameter)
        #set tolerance
        if tol <=0:
            err = shape.globalTolerance(tol)
        else:
            err = tol

        for edge in shape.Edges:
            first_point = edge.firstVertex().Point
            last_point = edge.lastVertex().Point
            dir_edge = edge.tangentAt(edge.FirstParameter)
            # check if edge is curve or no parallel to start_edge
            # because sin(x) = x + O(x**3), for small angular deflection it's
            # enough use the cross product of directions (or dot with a normal)
            if (abs(edge.Length - first_point.distanceToPoint(last_point)) > err
                or dir_start_edge.cross(dir_edge).Length > err):
                return False

    return True


def are_coplanar(shape_a, shape_b, tol=-1):
    """Return True if exist a plane containing both shapes."""

    if not is_planar(shape_a, tol) or not is_planar(shape_b, tol):
        return False

    if shape_a.isEqual(shape_b):
        return True

    plane_a = find_plane(shape_a, tol)
    plane_b = find_plane(shape_b, tol)

    #set tolerance
    if tol <=0:
        err = 1e-7
    else:
        err = tol

    if plane_a and plane_b:
        normal_a = plane_a.Axis
        normal_b = plane_b.Axis
        proj = plane_a.projectPoint(plane_b.Position)
        if (normal_a.cross(normal_b).Length > err
            or plane_b.Position.sub(proj).Length > err):
            return False
        else:
            return True

    elif plane_a and not plane_b:
        normal_a = plane_a.Axis
        for vertex in shape_b.Vertexes:
            dir_ver_b = vertex.Point.sub(plane_a.Position).normalize()
            if abs(normal_a.dot(dir_ver_b)) > err:
                proj = plane_a.projectPoint(vertex.Point)
                if vertex.Point.sub(proj).Length > err:
                    return False
        return True

    elif plane_b and not plane_a:
        normal_b = plane_b.Axis
        for vertex in shape_a.Vertexes:
            dir_ver_a = vertex.Point.sub(plane_b.Position).normalize()
            if abs(normal_b.dot(dir_ver_a)) > err:
                proj = plane_b.projectPoint(vertex.Point)
                if vertex.Point.sub(proj).Length > err:
                    return False
        return True
    # not normal_a and not normal_b:
    else:
        points_a = [vertex.Point for vertex in shape_a.Vertexes]
        points_b = [vertex.Point for vertex in shape_b.Vertexes]
        poly = Part.makePolygon(points_a + points_b)
        if is_planar(poly, tol):
            return True
        else:
            return False


def get_spline_surface_normal(shape, tol=-1):
    """Check if shape formed by BSpline surfaces is planar and get normal.
    If shape is not planar return None."""

    if len(shape.Faces) == 0:
        return None

    #set tolerance
    if tol <=0:
        err = shape.globalTolerance(tol)
    else:
        err = tol

    first_surf = shape.Faces[0].Surface

    if not first_surf.isPlanar(tol):
        return None

    # find bounds of first_surf
    u0, u1, v0, v1 = first_surf.bounds()
    u = (u0 + u1)/2
    v = (v0 + v1)/2
    first_normal = first_surf.normal(u, v)
    # check if all faces are planar and parallel
    for face in shape.Faces:
        surf = face.Surface
        if not surf.isPlanar(tol):
            return None
        u0, u1, v0, v1 = surf.bounds()
        u = (u0 + u1)/2
        v = (v0 + v1)/2
        surf_normal = surf.normal(u, v)
        if first_normal.cross(surf_normal).Length > err:
            return None

    normal = first_normal

    return normal

def find_plane(shape, tol=-1):
    """Find the plane containing the shape if possible.
    Use this function as a workaround due Part.Shape.findPlane
    fail to find plane on BSpline surfaces."""

    if shape.ShapeType == "Vertex":
        return None

    if is_straight_line(shape, tol):
        return None

    plane = shape.findPlane(tol)
    if plane:
        return plane
    elif len(shape.Faces) >= 1:
        # in case shape have BSpline surfaces
        normal = get_spline_surface_normal(shape, tol)
        if normal:
            position = shape.CenterOfMass
            return Part.Plane(position, normal)
        else:
            return None
    else:
        return None


def calculatePlacement(shape):
    """Return a placement located in the center of gravity of the shape.

    If the given shape is planar, return a placement located at the center
    of gravity of the shape, and oriented towards the shape's normal.
    Otherwise, it returns a null placement.
    """
    if not is_planar(shape):
        return App.Placement()

    pos = shape.BoundBox.Center
    norm = get_normal(shape)
    # for backward compatibility with previous getNormal implementation
    if norm == None:
        norm = App.Vector(0, 0, 1)
    pla = App.Placement()
    pla.Base = pos
    r = getRotation(norm)

    if r:
        pla.Rotation = r

    return pla


def mirror(point, edge):
    """Find mirror point relative to an edge."""
    normPoint = point.add(findDistance(point, edge, False))

    if normPoint:
        normPoint_point = App.Vector.sub(point, normPoint)
        normPoint_refl = normPoint_point.negative()
        refl = App.Vector.add(normPoint, normPoint_refl)
        return refl
    else:
        return None


#compatibility layer

getSplineNormal = get_spline_normal

getNormal = get_normal

isPlanar =  is_planar


## @}
