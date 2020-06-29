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


def getSplineNormal(edge):
    """Find the normal of a BSpline edge."""
    startPoint = edge.valueAt(edge.FirstParameter)
    endPoint = edge.valueAt(edge.LastParameter)
    midParameter = (edge.FirstParameter
                    + (edge.LastParameter - edge.FirstParameter) / 2)
    midPoint = edge.valueAt(midParameter)
    v1 = midPoint - startPoint
    v2 = midPoint - endPoint
    n = v1.cross(v2)
    n.normalize()
    return n


def getNormal(shape):
    """Find the normal of a shape or list of points, if possible."""
    if isinstance(shape, (list, tuple)):
        if len(shape) >= 3:
            v1 = shape[1].sub(shape[0])
            v2 = shape[2].sub(shape[0])
            n = v2.cross(v1)
            if n.Length:
                return n
        return None

    n = App.Vector(0, 0, 1)
    if shape.isNull():
        return n

    if (shape.ShapeType == "Face") and hasattr(shape, "normalAt"):
        n = shape.copy().normalAt(0.5, 0.5)
    elif shape.ShapeType == "Edge":
        if geomType(shape.Edges[0]) in ["Circle", "Ellipse"]:
            n = shape.Edges[0].Curve.Axis
        elif (geomType(shape.Edges[0]) == "BSplineCurve"
              or geomType(shape.Edges[0]) == "BezierCurve"):
            n = getSplineNormal(shape.Edges[0])
    else:
        for e in shape.Edges:
            if geomType(e) in ["Circle", "Ellipse"]:
                n = e.Curve.Axis
                break
            elif (geomType(e) == "BSplineCurve"
                  or geomType(e) == "BezierCurve"):
                n = getSplineNormal(e)
                break

            e1 = vec(shape.Edges[0])
            for i in range(1, len(shape.Edges)):
                e2 = vec(shape.Edges[i])
                if 0.1 < abs(e1.getAngle(e2)) < 3.14:
                    n = e1.cross(e2).normalize()
                    break

    # Check the 3D view to flip the normal if the GUI is available
    if App.GuiUp:
        vdir = gui_utils.get_3d_view().getViewDirection()
        if n.getAngle(vdir) < 0.78:
            n = n.negative()

    if not n.Length:
        return None

    return n


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


def isPlanar(shape):
    """Return True if the given shape or list of points is planar."""
    n = getNormal(shape)
    if not n:
        return False

    if isinstance(shape, list):
        if len(shape) <= 3:
            return True
        else:
            for v in shape[3:]:
                pv = v.sub(shape[0])
                rv = DraftVecUtils.project(pv, n)
                if not DraftVecUtils.isNull(rv):
                    return False
    else:
        if len(shape.Vertexes) <= 3:
            return True

        for p in shape.Vertexes[1:]:
            pv = p.Point.sub(shape.Vertexes[0].Point)
            rv = DraftVecUtils.project(pv, n)
            if not DraftVecUtils.isNull(rv):
                return False

    return True


def calculatePlacement(shape):
    """Return a placement located in the center of gravity of the shape.

    If the given shape is planar, return a placement located at the center
    of gravity of the shape, and oriented towards the shape's normal.
    Otherwise, it returns a null placement.
    """
    if not isPlanar(shape):
        return App.Placement()

    pos = shape.BoundBox.Center
    norm = getNormal(shape)
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

## @}
