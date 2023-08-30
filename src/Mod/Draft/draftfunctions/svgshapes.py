# -*- coding: utf8 -*-
# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2018 George Shuklin (amarao)                            *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides functions to return the SVG representation of some shapes.
"""
## @package svgshapes
# \ingroup draftfunctions
# \brief Provides functions to return the SVG representation of some shapes.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils
import draftutils.utils as utils

from draftutils.messages import _msg, _wrn

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
TechDraw = lz.LazyLoader("TechDraw", globals(), "TechDraw")

## \addtogroup draftfunctions
# @{


def get_proj(vec, plane=None):
    """Get a projection of the vector in the plane's u and v directions.

    TODO: check if the same function for SVG and DXF projection can be used
    so that this function is not just duplicated code.
    This function may also be present elsewhere, like `WorkingPlane`
    or `DraftGeomUtils`, so we should avoid code duplication.

    Parameters
    ----------
    vec: Base::Vector3
        An arbitrary vector that will be projected on the U and V directions.

    plane: WorkingPlane.Plane
        An object of type `WorkingPlane`.
    """
    if not plane:
        return vec

    nx = DraftVecUtils.project(vec, plane.u)
    lx = nx.Length

    if abs(nx.getAngle(plane.u)) > 0.1:
        lx = -lx

    ny = DraftVecUtils.project(vec, plane.v)
    ly = ny.Length

    if abs(ny.getAngle(plane.v)) > 0.1:
        ly = -ly

    # if techdraw: buggy - we now simply do it at the end
    #    ly = -ly
    return App.Vector(lx, ly, 0)


def getProj(vec, plane=None):
    """Get a projection of a vector. DEPRECATED."""
    utils.use_instead("get_proj")
    return get_proj(vec, plane)


def get_discretized(edge, plane):
    """Get a discretized edge on a plane."""
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    pieces = param.GetFloat("svgDiscretization", 10.0)

    if pieces == 0:
        pieces = 10

    d = int(edge.Length/pieces)
    if d == 0:
        d = 1

    edata = ""
    for i in range(d + 1):
        _length = edge.LastParameter - edge.FirstParameter
        _point = edge.FirstParameter + float(i)/d * _length
        _vec = edge.valueAt(_point)
        v = get_proj(_vec, plane)

        if not edata:
            edata += 'M ' + str(v.x) + ' ' + str(v.y) + ' '
        else:
            edata += 'L ' + str(v.x) + ' ' + str(v.y) + ' '

    return edata


def getDiscretized(edge, plane):
    """Get a discretized edge on a plane. DEPRECATED."""
    utils.use_instead("get_discretized")
    return get_discretized(edge, plane)


def _get_path_circ_ellipse(plane, edge, verts, edata,
                           iscircle, isellipse,
                           fill, stroke, linewidth, lstyle):
    """Get the edge data from a path that is a circle or ellipse."""
    if hasattr(App, "DraftWorkingPlane"):
        drawing_plane_normal = App.DraftWorkingPlane.axis
    else:
        drawing_plane_normal = App.Vector(0, 0, 1)

    if plane:
        drawing_plane_normal = plane.axis

    center = edge.Curve
    ax = center.Axis

    # The angle between the curve axis and the plane is not 0 nor 180 degrees
    _angle = math.degrees(ax.getAngle(drawing_plane_normal))
    if round(_angle, 2) not in (0, 180):
        edata += get_discretized(edge, plane)
        return "edata", edata

    # The angle is 0 or 180, coplanar
    occversion = Part.OCC_VERSION.split(".")
    done = False
    if int(occversion[0]) >= 7 and int(occversion[1]) >= 1:
        # if using occ >= 7.1, use HLR algorithm
        snip = TechDraw.projectToSVG(edge, drawing_plane_normal)

        if snip:
            try:
                _a = snip.split('path d="')[1]
                _a = _a.split('"')[0]
                _a = _a.split("A")[1]
                A = "A " + _a
            except IndexError:
                # TODO: trap only specific exception.
                # Check the problem. Split didn't produce a two element list?
                _wrn("Circle or ellipse: "
                     "cannot split the projection snip "
                     "obtained by 'projectToSVG', "
                     "continue manually.")
            else:
                edata += A
                done = True

    if not done:
        if len(edge.Vertexes) == 1 and iscircle:
            # Complete circle not only arc
            svg = get_circle(plane,
                             fill, stroke, linewidth, lstyle,
                             edge)
            # If it's a circle we will return the final SVG string,
            # otherwise it will process the `edata` further
            return "svg", svg
        elif len(edge.Vertexes) == 1 and isellipse:
            # Complete ellipse not only arc
            # svg = get_ellipse(plane,
            #                   fill, stroke, linewidth,
            #                   lstyle, edge)
            # return svg

            # Difference in angles
            _diff = (center.LastParameter - center.FirstParameter)/2.0
            endpoints = [get_proj(center.value(_diff), plane),
                         get_proj(verts[-1].Point, plane)]
        else:
            endpoints = [get_proj(verts[-1].Point, plane)]

        # Arc with more than one vertex
        if iscircle:
            rx = ry = center.Radius
            rot = 0
        else:  # ellipse
            rx = center.MajorRadius
            ry = center.MinorRadius
            _rot = center.AngleXU * center.Axis * App.Vector(0, 0, 1)
            rot = math.degrees(_rot)
            if rot > 90:
                rot -= 180
            if rot < -90:
                rot += 180

        # Be careful with the sweep flag
        _diff = edge.ParameterRange[1] - edge.ParameterRange[0]
        _diff = _diff / math.pi
        flag_large_arc = (_diff % 2) > 1

        # flag_sweep = (center.Axis * drawing_plane_normal >= 0) \
        #     == (edge.LastParameter > edge.FirstParameter)
        #     == (edge.Orientation == "Forward")

        # Another method: check the direction of the angle
        # between tangents
        _diff = edge.LastParameter - edge.FirstParameter
        t1 = edge.tangentAt(edge.FirstParameter)
        t2 = edge.tangentAt(edge.FirstParameter + _diff/10)
        flag_sweep = DraftVecUtils.angle(t1, t2, drawing_plane_normal) < 0

        for v in endpoints:
            edata += ('A {} {} {} '
                      '{} {} '
                      '{} {} '.format(rx, ry, rot,
                                      int(flag_large_arc),
                                      int(flag_sweep),
                                      v.x, v.y))

    return "edata", edata


def _get_path_bspline(plane, edge, edata):
    """Convert the edge to a BSpline and discretize it."""
    bspline = edge.Curve.toBSpline(edge.FirstParameter, edge.LastParameter)
    if bspline.Degree > 3 or bspline.isRational():
        try:
            bspline = bspline.approximateBSpline(0.05, 50, 3, 'C0')
        except RuntimeError:
            _wrn("Debug: unable to approximate bspline from edge")

    if bspline.Degree <= 3 and not bspline.isRational():
        for bezierseg in bspline.toBezier():
            if bezierseg.Degree > 3:  # should not happen
                _wrn("Bezier segment of degree > 3")
                raise AssertionError
            elif bezierseg.Degree == 1:
                edata += 'L '
            elif bezierseg.Degree == 2:
                edata += 'Q '
            elif bezierseg.Degree == 3:
                edata += 'C '

            for pole in bezierseg.getPoles()[1:]:
                v = get_proj(pole, plane)
                edata += '{} {} '.format(v.x, v.y)
    else:
        _msg("Debug: one edge (hash {}) "
             "has been discretized "
             "with parameter 0.1".format(edge.hashCode()))

        for linepoint in bspline.discretize(0.1)[1:]:
            v = get_proj(linepoint, plane)
            edata += 'L {} {} '.format(v.x, v.y)

    return edata


def get_circle(plane,
               fill, stroke, linewidth, lstyle,
               edge):
    """Get the SVG representation from a circular edge."""
    cen = get_proj(edge.Curve.Center, plane)
    rad = edge.Curve.Radius

    if hasattr(App, "DraftWorkingPlane"):
        drawing_plane_normal = App.DraftWorkingPlane.axis
    else:
        drawing_plane_normal = App.Vector(0, 0, 1)

    if plane:
        drawing_plane_normal = plane.axis

    if round(edge.Curve.Axis.getAngle(drawing_plane_normal), 2) in [0, 3.14]:
        # Perpendicular projection: circle
        svg = '<circle '
        svg += 'cx="{}" cy="{}" r="{}" '.format(cen.x, cen.y, rad)
    else:
        # Any other projection: ellipse
        svg = '<path d="{}" '.format(get_discretized(edge, plane))

    svg += 'stroke="{}" '.format(stroke)
    # Editor: why is stroke-width repeated? Is this really necessary
    # for the generated SVG?
    svg += 'stroke-width="{} px" '.format(linewidth)
    svg += 'style="'
    svg += 'stroke-width:{};'.format(linewidth)
    svg += 'stroke-miterlimit:4;'
    svg += 'stroke-dasharray:{};'.format(lstyle)
    svg += 'stroke-linecap:square;'
    svg += 'fill:{}'.format(fill) + '"'
    svg += '/>\n'
    return svg


def getCircle(plane,
              fill, stroke, linewidth, lstyle,
              edge):
    """Get the SVG representation from a circular edge."""
    utils.use_instead("get_circle")
    return get_circle(plane, fill, stroke, linewidth, lstyle, edge)


def get_ellipse(plane,
                fill, stroke, linewidth, lstyle,
                edge):
    """Get the SVG representation from an elliptical edge."""
    cen = get_proj(edge.Curve.Center, plane)
    mir = edge.Curve.MinorRadius
    mar = edge.Curve.MajorRadius
    svg = '<ellipse '
    svg += 'cx="{}" cy="{}" '.format(cen.x, cen.y)
    svg += 'rx="{}" ry="{}" '.format(mar, mir)
    svg += 'stroke="{}" '.format(stroke)
    svg += 'stroke-width="{} px" '.format(linewidth)
    svg += 'style="'
    svg += 'stroke-width:{};'.format(linewidth)
    svg += 'stroke-miterlimit:4;'
    svg += 'stroke-dasharray:{};'.format(lstyle)
    svg += 'stroke-linecap:square;'
    svg += 'fill:{}'.format(fill) + '"'
    svg += '/>\n'
    return svg


def getEllipse(plane,
               fill, stroke, linewidth, lstyle,
               edge):
    """Get the SVG representation from an elliptical edge. DEPRECATED."""
    utils.use_instead("get_ellipse")
    return get_ellipse(plane, fill, stroke, linewidth, lstyle, edge)


def get_path(obj, plane,
             fill, pathdata, stroke, linewidth, lstyle,
             fill_opacity=None,
             edges=[], wires=[], pathname=None):
    """Get the SVG representation from an object's edges or wires.

    TODO: the `edges` and `wires` must not default to empty list `[]`
    but to `None`. Verify that the code doesn't break with this change.

    `edges` and `wires` are mutually exclusive. If no `wires` are provided,
    sort the `edges`, and use them. If `wires` are provided, sort the edges
    in these `wires`, and use them.
    """
    svg = "<path "

    if pathname is None:
        svg += 'id="{}" '.format(obj.Name)
    elif pathname != "":
        svg += 'id="{}" '.format(pathname)

    svg += ' d="'

    if not wires:
        egroups = Part.sortEdges(edges)
    else:
        egroups = []
        first = True
        for w in wires:
            wire = w.copy()
            if first:
                first = False
            else:
                # invert further wires to create holes
                wire = DraftGeomUtils.invert(wire)

            wire.fixWire()
            egroups.append(Part.__sortEdges__(wire.Edges))

    for _edges in egroups:
        edata = ""

        for edgeindex, edge in enumerate(_edges):
            if edgeindex == 0:
                verts = edge.Vertexes
                if len(_edges) > 1:
                    last_pt = verts[-1].Point
                    nextverts = _edges[1].Vertexes
                    if (last_pt - nextverts[0].Point).Length > 1e-6 \
                            and (last_pt - nextverts[-1].Point).Length > 1e-6:
                        verts.reverse()
                v = get_proj(verts[0].Point, plane)
                edata += 'M {} {} '.format(v.x, v.y)
            else:
                previousverts = verts
                verts = edge.Vertexes
                if (verts[0].Point - previousverts[-1].Point).Length > 1e-6:
                    verts.reverse()
                    if (verts[0].Point - previousverts[-1].Point).Length > 1e-6:
                        raise ValueError('edges not ordered')

            iscircle = DraftGeomUtils.geomType(edge) == "Circle"
            isellipse = DraftGeomUtils.geomType(edge) == "Ellipse"

            if iscircle or isellipse:
                _type, data = _get_path_circ_ellipse(plane, edge, verts,
                                                     edata,
                                                     iscircle, isellipse,
                                                     fill, stroke,
                                                     linewidth, lstyle)
                if _type == "svg":
                    # final svg string already calculated, so just return it
                    return data

                # else the `edata` was properly augmented, so re-assing it
                edata = data
            elif DraftGeomUtils.geomType(edge) == "Line":
                v = get_proj(verts[-1].Point, plane)
                edata += 'L {} {} '.format(v.x, v.y)
            else:
                # If it's not a circle nor ellipse nor straight line
                # convert the curve to BSpline
                edata = _get_path_bspline(plane, edge, edata)

        if fill != 'none':
            edata += 'Z '

        if edata in pathdata:
            # do not draw a path on another identical path
            return ""
        else:
            svg += edata
            pathdata.append(edata)

    svg += '" '
    svg += 'stroke="{}" '.format(stroke)
    svg += 'stroke-width="{} px" '.format(linewidth)
    svg += 'style="'
    svg += 'stroke-width:{};'.format(linewidth)
    svg += 'stroke-miterlimit:4;'
    svg += 'stroke-dasharray:{};'.format(lstyle)
    svg += 'stroke-linecap:square;'
    svg += 'fill:{};'.format(fill)
    # fill_opacity must be a number, but if it's `None` it is omitted
    if fill_opacity is not None:
        svg += 'fill-opacity:{};'.format(fill_opacity)

    svg += 'fill-rule: evenodd"'
    svg += '/>\n'
    return svg


def getPath(obj, plane,
            fill, pathdata, stroke, linewidth, lstyle,
            fill_opacity,
            edges=[], wires=[], pathname=None):
    """Get the SVG representation from a path. DEPRECATED."""
    utils.use_instead("get_path")
    return get_path(obj, plane,
                    fill, pathdata, stroke, linewidth, lstyle,
                    fill_opacity,
                    edges=edges, wires=wires, pathname=pathname)

## @}
