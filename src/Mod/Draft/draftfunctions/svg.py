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
"""Provides functions to return the SVG representation of various shapes.

Warning: this still uses the `Drawing.projectToSVG` method to provide
the SVG representation of certain objects.
Therefore, even if the Drawing Workbench is obsolete, the `Drawing` module
may not be removed completely yet. This must be checked.
"""
## @package svg
# \ingroup draftfuctions
# \brief Provides functions to return the SVG representation of shapes.

import math
import six
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils
import WorkingPlane
import draftutils.utils as utils

from draftutils.utils import param
from draftutils.messages import _msg, _wrn

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Drawing = lz.LazyLoader("Drawing", globals(), "Drawing")


## \addtogroup draftfuctions
# @{


def get_line_style(line_style, scale):
    """Return a linestyle scaled by a factor."""
    style = None

    if line_style == "Dashed":
        style = param.GetString("svgDashedLine", "0.09,0.05")
    elif line_style == "Dashdot":
        style = param.GetString("svgDashdotLine", "0.09,0.05,0.02,0.05")
    elif line_style == "Dotted":
        style = param.GetString("svgDottedLine", "0.02,0.02")
    elif line_style:
        if "," in line_style:
            style = line_style

    if style:
        style = style.split(",")
        try:
            # scale dashes
            style = ",".join([str(float(d)/scale) for d in style])
            # print("lstyle ", style)
        except:
            # TODO: trap only specific exception; what is the problem?
            # Bad string specification?
            return "none"
        else:
            return style

    return "none"


def getLineStyle(linestyle, scale):
    """Return a Line style. DEPRECATED. Use get_line_style."""
    utils.use_instead("get_line_style")
    return get_line_style(linestyle, scale)


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


def get_pattern(pat):
    """Get an SVG pattern."""
    patterns = utils.svg_patterns()

    if pat in patterns:
        return patterns[pat][0]
    return ''


def getPattern(pat):
    """Get an SVG pattern. DEPRECATED."""
    utils.use_instead("get_pattern")
    return get_pattern(pat)


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
    svg += 'fill:{}'.format(fill) + '"'
    svg += '/>\n'
    return svg


def getEllipse(plane,
               fill, stroke, linewidth, lstyle,
               edge):
    """Get the SVG representation from an elliptical edge. DEPRECATED."""
    utils.use_instead("get_ellipse")
    return get_ellipse(plane, fill, stroke, linewidth, lstyle, edge)


def get_arrow(obj,
              arrowtype, point, arrowsize, color, linewidth, angle=0):
    """Get the SVG representation from an arrow."""
    svg = ""

    if not App.GuiUp or not obj.ViewObject:
        return svg

    _cx_cy_r = 'cx="{}" cy="{}" r="{}"'.format(point.x, point.y, arrowsize)
    _rotate = 'rotate({},{},{})'.format(math.degrees(angle),
                                        point.x, point.y)
    _transl = 'translate({},{})'.format(point.x, point.y)
    _scale = 'scale({size},{size})'.format(size=arrowsize)
    _style = 'style="stroke-miterlimit:4;stroke-dasharray:none"'

    if obj.ViewObject.ArrowType == "Circle":
        svg += '<circle '
        svg += _cx_cy_r + ' '
        svg += 'fill="{}" stroke="{}" '.format("none", color)
        svg += 'style="stroke-width:{};'.format(linewidth)
        svg += 'stroke-miterlimit:4;stroke-dasharray:none" '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
    elif obj.ViewObject.ArrowType == "Dot":
        svg += '<circle '
        svg += _cx_cy_r + ' '
        svg += 'fill="{}" stroke="{}" '.format(color, "none")
        svg += _style + ' '
        svg += 'freecad:skip="1"'
        svg += '/>\n'
    elif obj.ViewObject.ArrowType == "Arrow":
        svg += '<path '
        svg += 'transform="'
        svg += _rotate + ' '
        svg += _transl + ' '
        svg += _scale + '" '
        svg += 'freecad:skip="1" '
        svg += 'fill="{}" stroke="{}" '.format(color, "none")
        svg += _style + ' '
        svg += 'd="M 0 0 L 4 1 L 4 -1 Z"'
        svg += '/>\n'
    elif obj.ViewObject.ArrowType == "Tick":
        svg += '<path '
        svg += 'transform="'
        svg += _rotate + ' '
        svg += _transl + ' '
        svg += _scale + '" '
        svg += 'freecad:skip="1" '
        svg += 'fill="{}" stroke="{}" '.format(color, "none")
        svg += _style + ' '
        svg += 'd="M -1 -2 L 0 2 L 1 2 L 0 -2 Z"'
        svg += '/>\n'
    elif obj.ViewObject.ArrowType == "Tick-2":
        svg += '<line '
        svg += 'transform="'
        svg += 'rotate({},{},{}) '.format(math.degrees(angle) + 45,
                                          point.x, point.y)
        svg += _transl + '" '
        svg += 'freecad:skip="1" '
        svg += 'fill="{}" stroke="{}" '.format("none", color)
        svg += 'style="stroke-dasharray:none;stroke-linecap:square;'
        svg += 'stroke-width:{}" '.format(linewidth)
        svg += 'x1="-{}" y1="0" '.format(2 * arrowsize)
        svg += 'x2="{}" y2="0"'.format(2 * arrowsize)
        svg += '/>\n'
    else:
        _wrn("getSVG: arrow type not implemented")

    return svg


def getArrow(obj,
             arrowtype, point, arrowsize, color, linewidth, angle=0):
    """Get the SVG representation from an arrow. DEPRECATED."""
    utils.use_instead("get_arrow")
    return get_arrow(obj,
                     arrowtype, point, arrowsize, color, linewidth, angle)


def get_overshoot(point, shootsize, color, linewidth, angle=0):
    """Get the SVG representation of a dimension line overshoot."""
    svg = '<line '
    svg += 'transform="'
    svg += 'rotate({},{},{}) '.format(math.degrees(angle),
                                      point.x, point.y)
    svg += 'translate({},{})" '.format(point.x, point.y)
    svg += 'freecad:skip="1" '
    svg += 'fill="{}" stroke="{}" '.format("none", color)
    svg += 'style="stroke-dasharray:none;stroke-linecap:square;'
    svg += 'stroke-width:{}" '.format(linewidth)
    svg += 'x1="0" y1="0" '
    svg += 'x2="{}" y2="0"'.format(-1 * shootsize)
    svg += '/>\n'
    return svg


def getOvershoot(point, shootsize, color, linewidth, angle=0):
    """Get the SVG representation of a dimension line overshoot. DEPRECATED."""
    utils.use_instead("get_overshoot")
    return get_overshoot(point, shootsize, color, linewidth, angle)


def _get_text_techdraw(text, tcolor, fontsize, anchor,
                       align, fontname, angle, base,
                       linespacing):
    """Return the SVG representation of text for TechDraw display.

    `text` is a list of textual elements; they are iterated, styled,
    and added around a `<text>` tag.
    ::
        <text ...> text[0] </text>
        <text ...> text[1] </text>
    """
    svg = ""
    for i in range(len(text)):
        _t = text[i].replace("&", "&amp;")
        _t = _t.replace("<", "&lt;")
        t = _t.replace(">", "&gt;")

        if six.PY2 and not isinstance(t, six.text_type):
            t = t.decode("utf8")

        # possible workaround if UTF8 is unsupported
        #   import unicodedata as U
        #   v = list()
        #   for c in U.normalize("NFKD", t):
        #       if not U.combining(c):
        #           v.append(c)
        #
        #   t = u"".join(v)
        #   t = t.encode("utf8")

        svg += '<text '
        svg += 'stroke-width="0" stroke="{}" '.format(tcolor)
        svg += 'fill="{}" font-size="{}" '.format(tcolor, fontsize)
        svg += 'style="text-anchor:{};text-align:{};'.format(anchor,
                                                             align.lower())
        svg += 'font-family:{}" '.format(fontname)
        svg += 'transform="'
        svg += 'rotate({},{},{}) '.format(math.degrees(angle),
                                          base.x,
                                          base.y - i * linespacing)
        svg += 'translate({},{}) '.format(base.x,
                                          base.y - i * linespacing)
        svg += 'scale(1,-1)"'
        # svg += 'freecad:skip="1"'
        svg += '>\n'
        svg += t
        svg += '</text>\n'
    return svg


def _get_text_header(tcolor, fontsize, anchor, align,
                     fontname, angle, base, flip):
    """Return the initial <text> tag with style options.

    The text must be added after this tag, and then must be closed.
    ::
        <text ...>
        ...
        </text>
    """
    svg = '<text '
    svg += 'stroke-width="0" stroke="{}" '.format(tcolor)
    svg += 'fill="{}" font-size="{}" '.format(tcolor, fontsize)
    svg += 'style="text-anchor:{};text-align:{};'.format(anchor,
                                                         align.lower())
    svg += 'font-family:{}" '.format(fontname)
    svg += 'transform="'
    svg += 'rotate({},{},{}) '.format(math.degrees(angle),
                                      base.x,
                                      base.y)
    if flip:
        svg += 'translate({},{}) '.format(base.x, base.y)
    else:
        svg += 'translate({},{}) '.format(base.x, -base.y)
    # svg += 'scale({},-{}) '.format(tmod/2000, tmod/2000)

    if flip:
        svg += 'scale(1,-1) '
    else:
        svg += 'scale(1,1) '

    svg += '" '
    svg += 'freecad:skip="1"'
    svg += '>\n'
    return svg


def get_text(plane, techdraw,
             tcolor, fontsize, fontname,
             angle, base, text,
             linespacing=0.5, align="center", flip=True):
    """Get the SVG representation of a textual element."""
    if isinstance(angle, App.Rotation):
        if not plane:
            angle = angle.Angle
        else:
            if plane.axis.getAngle(angle.Axis) < 0.001:
                angle = angle.Angle
            elif abs(plane.axis.getAngle(angle.Axis) - math.pi) < 0.001:
                if abs(angle.Angle) > 0.1:
                    angle = -angle.Angle
                else:
                    angle = angle.Angle
            elif abs(plane.axis.getAngle(angle.Axis) - math.pi/2) < 0.001:
                # text is perpendicular to view, so it shouldn't appear
                return ""
            else:
                # TODO maybe there is something better to do here?
                angle = 0

    # text should be a list of strings separated by a newline
    if not isinstance(text, list):
        text = text.split("\n")

    if align.lower() == "center":
        anchor = "middle"
    elif align.lower() == "left":
        anchor = "start"
    else:
        anchor = "end"

    if techdraw:
        # For TechDraw display each item in the text list is placed
        # in an individual tag.
        # <text ...> text[0] </text>
        # <text ...> text[1] </text>
        svg = _get_text_techdraw(text, tcolor, fontsize, anchor,
                                 align, fontname, angle, base,
                                 linespacing)
    else:
        # If the SVG is not for TechDraw, and there is a single item
        # in the text list, place it in a single tag.
        # <text ...> text </text>
        #
        # For multiple elements, place each element inside a <tspan> tag.
        # <text ...>
        #   <tspan>text[0]</tspan>
        #   <tspan>text[1]</tspan>
        # </text>
        svg = _get_text_header(tcolor, fontsize, anchor, align,
                               fontname, angle, base, flip)

        if len(text) == 1:
            try:
                _t = text[0].replace("&", "&amp;").replace("<", "&lt;")
                svg += _t.replace(">", "&gt;")
            except:
                # TODO: trap only specific exception; what is the problem?
                # Bad UTF8 string specification? This can be removed
                # once the code is only used with Python 3.
                _t = text[0].decode("utf8")
                _t = _t.replace("&", "&amp;").replace("<", "&lt;")
                svg += _t.replace(">", "&gt;")
        else:
            for i in range(len(text)):
                if i == 0:
                    svg += '<tspan>'
                else:
                    svg += '<tspan x="0" dy="{}">'.format(linespacing)

                try:
                    _t = text[i].replace("&", "&amp;").replace("<", "&lt;")
                    svg += _t.replace(">", "&gt;")
                except:
                    # TODO: trap only specific exception; what is the problem?
                    # Bad UTF8 string specification? This can be removed
                    # once the code is only used with Python 3.
                    _t = text[i].decode("utf8")
                    _t = _t.replace("&", "&amp;").replace("<", "&lt;")
                    svg += _t.replace(">", "&gt;")

                svg += '</tspan>\n'
        svg += '</text>\n'
    return svg


def getText(plane, techdraw,
            tcolor, fontsize, fontname,
            angle, base, text,
            linespacing=0.5, align="center", flip=True):
    """Get the SVG representation of a textual element. DEPRECATED."""
    utils.use_instead("get_text")
    return get_text(plane, techdraw,
                    tcolor, fontsize, fontname,
                    angle, base, text,
                    linespacing, align, flip)


def format_point(coords, action='L'):
    """Return a string with a formatted point."""
    return "{action}{x},{y}".format(x=coords.x, y=coords.y, action=action)


def _get_path_circ_ellipse(plane, edge, vertex, edata,
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
        snip = Drawing.projectToSVG(edge, drawing_plane_normal)

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
                         get_proj(vertex[-1].Point, plane)]
        else:
            endpoints = [get_proj(vertex[-1].Point, plane)]

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

    for _, _edges in enumerate(egroups):
        edata = ""
        vertex = ()  # skipped for the first edge

        for edgeindex, edge in enumerate(_edges):
            previousvs = vertex
            # vertexes of an edge (reversed if needed)
            vertex = edge.Vertexes
            if previousvs:
                if (vertex[0].Point - previousvs[-1].Point).Length > 1e-6:
                    vertex.reverse()

            if edgeindex == 0:
                v = get_proj(vertex[0].Point, plane)
                edata += 'M {} {} '.format(v.x, v.y)
            else:
                if (vertex[0].Point - previousvs[-1].Point).Length > 1e-6:
                    raise ValueError('edges not ordered')

            iscircle = DraftGeomUtils.geomType(edge) == "Circle"
            isellipse = DraftGeomUtils.geomType(edge) == "Ellipse"

            if iscircle or isellipse:
                _type, data = _get_path_circ_ellipse(plane, edge, vertex,
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
                v = get_proj(vertex[-1].Point, plane)
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


def get_svg(obj,
            scale=1, linewidth=0.35, fontsize=12,
            fillstyle="shape color", direction=None, linestyle=None,
            color=None, linespacing=None, techdraw=False, rotation=0,
            fillspaces=False, override=True):
    """Return a string containing an SVG representation of the object.

    Paramaeters
    -----------
    scale: float, optional
        It defaults to 1.
        It allows scaling line widths down, so they are resolution-independent.

    linewidth: float, optional
        It defaults to 0.35.

    fontsize: float, optional
        It defaults to 12, which is interpreted as `pt` unit (points).
        It is used if the given object contains any text.

    fillstyle: str, optional
        It defaults to 'shape color'.

    direction: Base::Vector3, optional
        It defaults to `None`.
        It is an arbitrary projection vector or a `WorkingPlane.Plane`
        instance.

    linestyle: optional
        It defaults to `None`.

    color: optional
        It defaults to `None`.

    linespacing: float, optional
        It defaults to `None`.

    techdraw: bool, optional
        It defaults to `False`.
        If it is `True`, it sets some options for generating SVG strings
        for displaying inside TechDraw.

    rotation: float, optional
        It defaults to 0.

    fillspaces: bool, optional
        It defaults to `False`.

    override: bool, optional
        It defaults to `True`.
    """
    # If this is a group, recursively call this function to gather
    # all the SVG strings from the contents of the group
    if hasattr(obj, "isDerivedFrom"):
        if (obj.isDerivedFrom("App::DocumentObjectGroup")
                or utils.get_type(obj) == "Layer"):
            svg = ""
            for child in obj.Group:
                svg += get_svg(child,
                               scale, linewidth, fontsize,
                               fillstyle, direction, linestyle,
                               color, linespacing, techdraw,
                               rotation, fillspaces, override)
            return svg

    pathdata = []
    svg = ""
    linewidth = float(linewidth)/scale
    if not override:
        if hasattr(obj, "ViewObject") and hasattr(obj.ViewObject, "LineWidth"):
            if hasattr(obj.ViewObject.LineWidth, "Value"):
                lw = obj.ViewObject.LineWidth.Value
            else:
                lw = obj.ViewObject.LineWidth
            linewidth = lw * linewidth

    fontsize = (float(fontsize)/scale)/2
    if linespacing:
        linespacing = float(linespacing)/scale
    else:
        linespacing = 0.5

    # print(obj.Label, "line spacing", linespacing, "scale", scale)

    # The number of times the dots are smaller than the arrow size
    pointratio = 0.75
    plane = None

    if direction:
        if isinstance(direction, App.Vector):
            if direction != App.Vector(0, 0, 0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis_SVG(App.Vector(0, 0, 0),
                                              direction.negative().negative(),
                                              0)
        elif isinstance(direction, WorkingPlane.plane):
            plane = direction

    stroke = "#000000"
    if color and override:
        if "#" in color:
            stroke = color
        else:
            stroke = utils.get_rgb(color)
    elif App.GuiUp:
        if hasattr(obj, "ViewObject"):
            if hasattr(obj.ViewObject, "LineColor"):
                stroke = utils.get_rgb(obj.ViewObject.LineColor)
            elif hasattr(obj.ViewObject, "TextColor"):
                stroke = utils.get_rgb(obj.ViewObject.TextColor)

    lstyle = "none"
    if override:
        lstyle = get_line_style(linestyle, scale)
    else:
        if hasattr(obj, "ViewObject") and hasattr(obj.ViewObject, "DrawStyle"):
            lstyle = get_line_style(obj.ViewObject.DrawStyle, scale)

    if not obj:
        pass

    elif isinstance(obj, Part.Shape):
        if "#" in fillstyle:
            fill = fillstyle
        elif fillstyle == "shape color":
            fill = "#888888"
        else:
            fill = 'url(#'+fillstyle+')'
        svg += get_path(obj, plane,
                        fill, pathdata, stroke, linewidth, lstyle,
                        fill_opacity=None,
                        edges=obj.Edges, pathname="")

    elif utils.get_type(obj) in ["Dimension", "LinearDimension"]:
        if not App.GuiUp:
            _wrn("Export of dimensions to SVG is only available in GUI mode")

        if App.GuiUp and obj.ViewObject.Proxy:
            vobj = obj.ViewObject

            if hasattr(vobj.Proxy, "p1"):
                prx = vobj.Proxy
                ts = len(prx.string) * vobj.FontSize.Value / 4.0
                rm = (prx.p3 - prx.p2).Length/2.0 - ts

                _diff32 = prx.p3 - prx.p2
                _diff23 = prx.p2 - prx.p3

                _v32 = DraftVecUtils.scaleTo(_diff32, rm)
                _v23 = DraftVecUtils.scaleTo(_diff23, rm)

                p2a = get_proj(prx.p2 + _v32, plane)
                p2b = get_proj(prx.p3 + _v23, plane)
                p1 = get_proj(prx.p1, plane)
                p2 = get_proj(prx.p2, plane)
                p3 = get_proj(prx.p3, plane)
                p4 = get_proj(prx.p4, plane)

                tbase = get_proj(prx.tbase, plane)
                r = prx.textpos.rotation.getValue().getValue()
                _rv = App.Rotation(r[0], r[1], r[2], r[3])
                rv = _rv.multVec(App.Vector(1, 0, 0))
                angle = -DraftVecUtils.angle(get_proj(rv, plane))
                # angle = -DraftVecUtils.angle(p3.sub(p2))

                svg = ''
                nolines = False
                if hasattr(vobj, "ShowLine"):
                    if not vobj.ShowLine:
                        nolines = True

                # drawing lines
                if not nolines:
                    svg += '<path '

                if vobj.DisplayMode == "2D":
                    tangle = angle
                    if tangle > math.pi/2:
                        tangle = tangle-math.pi
                    # elif (tangle <= -math.pi/2) or (tangle > math.pi/2):
                    #    tangle = tangle + math.pi

                    if rotation != 0:
                        # print("dim: tangle:", tangle,
                        #       " rot: ", rotation,
                        #       " text: ", prx.string)
                        if abs(tangle + math.radians(rotation)) < 0.0001:
                            tangle += math.pi
                            _v = App.Vector(0, 2.0/scale, 0)
                            _rot = DraftVecUtils.rotate(_v, tangle)
                            tbase = tbase + _rot

                    if not nolines:
                        svg += 'd="M ' + str(p1.x) + ' ' + str(p1.y) + ' '
                        svg += 'L ' + str(p2.x) + ' ' + str(p2.y) + ' '
                        svg += 'L ' + str(p3.x) + ' ' + str(p3.y) + ' '
                        svg += 'L ' + str(p4.x) + ' ' + str(p4.y) + '" '
                else:
                    tangle = 0
                    if rotation != 0:
                        tangle = -math.radians(rotation)

                    tbase = tbase + App.Vector(0, -2.0/scale, 0)
                    if not nolines:
                        svg += 'd="M ' + str(p1.x) + ' ' + str(p1.y) + ' '
                        svg += 'L ' + str(p2.x) + ' ' + str(p2.y) + ' '
                        svg += 'L ' + str(p2a.x) + ' ' + str(p2a.y) + ' '
                        svg += 'M ' + str(p2b.x) + ' ' + str(p2b.y) + ' '
                        svg += 'L ' + str(p3.x) + ' ' + str(p3.y) + ' '
                        svg += 'L ' + str(p4.x) + ' ' + str(p4.y) + '" '

                if not nolines:
                    svg += 'fill="none" stroke="'
                    svg += stroke + '" '
                    svg += 'stroke-width="' + str(linewidth) + ' px" '
                    svg += 'style="stroke-width:' + str(linewidth)
                    svg += ';stroke-miterlimit:4;stroke-dasharray:none" '
                    svg += 'freecad:basepoint1="'+str(p1.x)+' '+str(p1.y)+'" '
                    svg += 'freecad:basepoint2="'+str(p4.x)+' '+str(p4.y)+'" '
                    svg += 'freecad:dimpoint="'+str(p2.x)+' '+str(p2.y)+'"'
                    svg += '/>\n'

                    # drawing dimension and extension lines overshoots
                    if hasattr(vobj, "DimOvershoot") and vobj.DimOvershoot.Value:
                        shootsize = vobj.DimOvershoot.Value/pointratio
                        svg += get_overshoot(p2, shootsize, stroke,
                                             linewidth, angle)
                        svg += get_overshoot(p3, shootsize, stroke,
                                             linewidth, angle + math.pi)

                    if hasattr(vobj, "ExtOvershoot") and vobj.ExtOvershoot.Value:
                        shootsize = vobj.ExtOvershoot.Value/pointratio
                        shootangle = -DraftVecUtils.angle(p1 - p2)
                        svg += get_overshoot(p2, shootsize, stroke,
                                             linewidth, shootangle)
                        svg += get_overshoot(p3, shootsize, stroke,
                                             linewidth, shootangle)

                    # drawing arrows
                    if hasattr(vobj, "ArrowType"):
                        arrowsize = vobj.ArrowSize.Value/pointratio
                        if hasattr(vobj, "FlipArrows"):
                            if vobj.FlipArrows:
                                angle = angle + math.pi

                        svg += get_arrow(obj,
                                         vobj.ArrowType,
                                         p2, arrowsize, stroke, linewidth,
                                         angle)
                        svg += get_arrow(obj,
                                         vobj.ArrowType,
                                         p3, arrowsize, stroke, linewidth,
                                         angle + math.pi)

                # drawing text
                svg += get_text(plane, techdraw,
                                stroke, fontsize, vobj.FontName,
                                tangle, tbase, prx.string)

    elif utils.get_type(obj) == "AngularDimension":
        if not App.GuiUp:
            _wrn("Export of dimensions to SVG is only available in GUI mode")

        if App.GuiUp:
            if obj.ViewObject.Proxy:
                if hasattr(obj.ViewObject.Proxy, "circle"):
                    prx = obj.ViewObject.Proxy

                    # drawing arc
                    fill = "none"
                    if obj.ViewObject.DisplayMode == "2D":
                        svg += get_path(obj, plane,
                                        fill, pathdata, stroke, linewidth,
                                        lstyle, fill_opacity=None,
                                        edges=[prx.circle])
                    else:
                        if hasattr(prx, "circle1"):
                            svg += get_path(obj, plane,
                                            fill, pathdata, stroke, linewidth,
                                            lstyle, fill_opacity=None,
                                            edges=[prx.circle1])
                            svg += get_path(obj, plane,
                                            fill, pathdata, stroke, linewidth,
                                            lstyle, fill_opacity=None,
                                            edges=[prx.circle2])
                        else:
                            svg += get_path(obj, plane,
                                            fill, pathdata, stroke, linewidth,
                                            lstyle, fill_opacity=None,
                                            edges=[prx.circle])

                    # drawing arrows
                    if hasattr(obj.ViewObject, "ArrowType"):
                        p2 = get_proj(prx.p2, plane)
                        p3 = get_proj(prx.p3, plane)
                        arrowsize = obj.ViewObject.ArrowSize.Value/pointratio
                        arrowlength = 4*obj.ViewObject.ArrowSize.Value

                        _v1a = prx.circle.valueAt(prx.circle.FirstParameter
                                                  + arrowlength)
                        _v1b = prx.circle.valueAt(prx.circle.FirstParameter)

                        _v2a = prx.circle.valueAt(prx.circle.LastParameter
                                                  - arrowlength)
                        _v2b = prx.circle.valueAt(prx.circle.LastParameter)

                        u1 = get_proj(_v1a - _v1b, plane)
                        u2 = get_proj(_v2a - _v2b, plane)
                        angle1 = -DraftVecUtils.angle(u1)
                        angle2 = -DraftVecUtils.angle(u2)

                        if hasattr(obj.ViewObject, "FlipArrows"):
                            if obj.ViewObject.FlipArrows:
                                angle1 = angle1 + math.pi
                                angle2 = angle2 + math.pi

                        svg += get_arrow(obj,
                                         obj.ViewObject.ArrowType,
                                         p2, arrowsize, stroke, linewidth,
                                         angle1)
                        svg += get_arrow(obj,
                                         obj.ViewObject.ArrowType,
                                         p3, arrowsize, stroke, linewidth,
                                         angle2)

                    # drawing text
                    if obj.ViewObject.DisplayMode == "2D":
                        _diff = (prx.circle.LastParameter
                                 - prx.circle.FirstParameter)
                        t = prx.circle.tangentAt(prx.circle.FirstParameter
                                                 + _diff/2.0)
                        t = get_proj(t, plane)
                        tangle = DraftVecUtils.angle(t)
                        if (tangle <= -math.pi/2) or (tangle > math.pi/2):
                            tangle = tangle + math.pi

                        _diff = (prx.circle.LastParameter
                                 - prx.circle.FirstParameter)
                        _va = prx.circle.valueAt(prx.circle.FirstParameter
                                                 + _diff/2.0)
                        tbase = get_proj(_va, plane)

                        _v = App.Vector(0, 2.0/scale, 0)
                        tbase = tbase + DraftVecUtils.rotate(_v, tangle)
                        # print(tbase)
                    else:
                        tangle = 0
                        tbase = get_proj(prx.tbase, plane)

                    svg += get_text(plane, techdraw,
                                    stroke, fontsize, obj.ViewObject.FontName,
                                    tangle, tbase, prx.string)

    elif utils.get_type(obj) == "Label":
        if getattr(obj.ViewObject, "Line", True):
            # Some Labels may have no Line property
            # Draw multisegment line
            proj_points = list(map(lambda x: get_proj(x, plane), obj.Points))
            path_dir_list = [format_point(proj_points[0], action='M')]
            path_dir_list += map(format_point, proj_points[1:])
            path_dir_str = " ".join(path_dir_list)

            svg_path = '<path '
            svg_path += 'fill="none" '
            svg_path += 'stroke="{}" '.format(stroke)
            svg_path += 'stroke-width="{}" '.format(linewidth)
            svg_path += 'd="{}"'.format(path_dir_str)
            svg_path += '/>'
            svg += svg_path

            # Draw arrow.
            # We are different here from 3D view
            # if Line is set to 'off', no arrow is drawn
            if hasattr(obj.ViewObject, "ArrowType") and len(obj.Points) >= 2:
                last_segment = App.Vector(obj.Points[-1] - obj.Points[-2])
                _v = get_proj(last_segment, plane)
                angle = -DraftVecUtils.angle(_v) + math.pi
                svg += get_arrow(obj,
                                 obj.ViewObject.ArrowType,
                                 proj_points[-1],
                                 obj.ViewObject.ArrowSize.Value/pointratio,
                                 stroke, linewidth, angle)

        if not App.GuiUp:
            _wrn("Export of texts to SVG is only available in GUI mode")

        # print text
        if App.GuiUp:
            fontname = obj.ViewObject.TextFont
            position = get_proj(obj.Placement.Base, plane)
            rotation = obj.Placement.Rotation
            justification = obj.ViewObject.TextAlignment
            text = obj.Text
            svg += get_text(plane, techdraw,
                            stroke, fontsize, fontname,
                            rotation, position, text,
                            linespacing, justification)

    elif utils.get_type(obj) in ["Annotation", "DraftText", "Text"]:
        # returns an svg representation of a document annotation
        if not App.GuiUp:
            _wrn("Export of texts to SVG is only available in GUI mode")

        if App.GuiUp:
            n = obj.ViewObject.FontName
            if utils.get_type(obj) == "Annotation":
                p = get_proj(obj.Position, plane)
                r = obj.ViewObject.Rotation.getValueAs("rad")
                t = obj.LabelText
            else:  # DraftText (old) or Text (new, 0.19)
                p = get_proj(obj.Placement.Base, plane)
                r = obj.Placement.Rotation
                t = obj.Text

            j = obj.ViewObject.Justification
            svg += get_text(plane, techdraw,
                            stroke, fontsize, n,
                            r, p, t,
                            linespacing, j)

    elif utils.get_type(obj) == "Axis":
        # returns the SVG representation of an Arch Axis system
        if not App.GuiUp:
            _wrn("Export of axes to SVG is only available in GUI mode")

        if App.GuiUp:
            vobj = obj.ViewObject
            lorig = lstyle
            fill = 'none'
            rad = vobj.BubbleSize.Value/2
            n = 0
            for e in obj.Shape.Edges:
                lstyle = lorig
                svg += get_path(obj, plane,
                                fill, pathdata, stroke, linewidth, lstyle,
                                fill_opacity=None,
                                edges=[e])
                lstyle = "none"
                pos = ["Start"]
                if hasattr(vobj, "BubblePosition"):
                    if vobj.BubblePosition == "Both":
                        pos = ["Start", "End"]
                    else:
                        pos = [vobj.BubblePosition]
                for p in pos:
                    if p == "Start":
                        p1 = e.Vertexes[0].Point
                        p2 = e.Vertexes[1].Point
                    else:
                        p1 = e.Vertexes[1].Point
                        p2 = e.Vertexes[0].Point
                    dv = p2.sub(p1)
                    dv.normalize()
                    center = p2.add(dv.scale(rad, rad, rad))
                    svg += get_circle(plane,
                                      fill, stroke, linewidth, lstyle,
                                      Part.makeCircle(rad, center))
                    if (hasattr(vobj.Proxy, "bubbletexts")
                            and len(vobj.Proxy.bubbletexts) >= n):
                        bubb = vobj.Proxy.bubbletexts
                        svg += '<text '
                        svg += 'fill="{}" '.format(stroke)
                        svg += 'font-size="{}" '.format(rad)
                        svg += 'style="text-anchor:middle;'
                        svg += 'text-align:center;'
                        svg += 'font-family: sans;" '
                        svg += 'transform="'
                        svg += 'translate({},{}) '.format(center.x + rad/4.0,
                                                          center.y - rad/3.0)
                        svg += 'scale(1,-1)"> '
                        svg += '<tspan>'
                        svg += bubb[n].string.getValues()[0]
                        svg += '</tspan>\n'
                        svg += '</text>\n'
                        n += 1
            lstyle = lorig

    elif utils.get_type(obj) == "Pipe":
        fill = stroke
        if obj.Base and obj.Diameter:
            svg += get_path(obj, plane,
                            fill, pathdata, stroke, linewidth, lstyle,
                            fill_opacity=None,
                            edges=obj.Base.Shape.Edges)
        for f in obj.Shape.Faces:
            if len(f.Edges) == 1:
                if isinstance(f.Edges[0].Curve, Part.Circle):
                    svg += get_circle(plane,
                                      fill, stroke, linewidth, lstyle,
                                      f.Edges[0])

    elif utils.get_type(obj) == "Rebar":
        fill = "none"
        basewire = obj.Base.Shape.Wires[0].copy()
        # Not applying rounding because the results are not correct
        # if hasattr(obj, "Rounding") and obj.Rounding:
        #     basewire = DraftGeomUtils.filletWire(
        #         basewire, obj.Rounding * obj.Diameter.Value
        #     )
        wires = []
        for placement in obj.PlacementList:
            wire = basewire.copy()
            wire.Placement = placement.multiply(basewire.Placement)
            wires.append(wire)
        svg += get_path(obj, plane,
                        fill, pathdata, stroke, linewidth, lstyle,
                        fill_opacity=None,
                        wires=wires)

    elif utils.get_type(obj) == "PipeConnector":
        pass

    elif utils.get_type(obj) == "Space":
        fill_opacity = 1

        # returns an SVG fragment for the text of a space
        if not App.GuiUp:
            _wrn("Export of spaces to SVG is only available in GUI mode")

        if App.GuiUp:
            vobj = obj.ViewObject
            if fillspaces and hasattr(obj, "Proxy"):
                if not hasattr(obj.Proxy, "face"):
                    obj.Proxy.getArea(obj, notouch=True)
                if hasattr(obj.Proxy, "face"):
                    # setting fill
                    if App.GuiUp:
                        fill = utils.get_rgb(vobj.ShapeColor,
                                             testbw=False)
                        fill_opacity = 1 - vobj.Transparency / 100.0
                    else:
                        fill = "#888888"
                    svg += get_path(obj, plane,
                                    fill, pathdata, stroke, linewidth,
                                    lstyle, fill_opacity=fill_opacity,
                                    wires=[obj.Proxy.face.OuterWire])
            c = utils.get_rgb(vobj.TextColor)
            n = vobj.FontName
            a = 0
            if rotation != 0:
                a = math.radians(rotation)

            t1 = vobj.Proxy.text1.string.getValues()
            t2 = vobj.Proxy.text2.string.getValues()
            scale = vobj.FirstLine.Value/vobj.FontSize.Value
            f1 = fontsize * scale

            _v = vobj.Proxy.coords.translation.getValue().getValue()
            p2 = obj.Placement.multVec(App.Vector(_v))

            _h = vobj.Proxy.header.translation.getValue().getValue()
            lspc = App.Vector(_h)
            p1 = p2 + lspc
            j = vobj.TextAlign
            t3 = get_text(plane, techdraw,
                          c, f1, n,
                          a, get_proj(p1, plane), t1,
                          linespacing, j, flip=True)
            svg += t3
            if t2:
                ofs = App.Vector(0, -lspc.Length, 0)
                if a:
                    Z = App.Vector(0, 0, 1)
                    ofs = App.Rotation(Z, -rotation).multVec(ofs)
                t4 = get_text(plane, techdraw,
                              c, fontsize, n,
                              a, get_proj(p1, plane).add(ofs), t2,
                              linespacing, j, flip=True)
                svg += t4

    elif hasattr(obj, 'Shape'):
        # In the past we tested for a Part Feature
        # elif obj.isDerivedFrom('Part::Feature'):
        #
        # however, this doesn't work for App::Links; instead we
        # test for a 'Shape'. All Part::Features should have a Shape,
        # and App::Links can have one as well.
        if obj.Shape.isNull():
            return ''

        fill_opacity = 1
        # setting fill
        if obj.Shape.Faces:
            if App.GuiUp:
                try:
                    m = obj.ViewObject.DisplayMode
                except AttributeError:
                    m = None

                vobj = obj.ViewObject
                if m != "Wireframe":
                    if fillstyle == "shape color":
                        fill = utils.get_rgb(vobj.ShapeColor,
                                             testbw=False)
                        fill_opacity = 1 - vobj.Transparency / 100.0
                    else:
                        fill = 'url(#'+fillstyle+')'
                        svg += get_pattern(fillstyle)
                else:
                    fill = "none"
            else:
                fill = "#888888"
        else:
            fill = 'none'

        if len(obj.Shape.Vertexes) > 1:
            wiredEdges = []
            if obj.Shape.Faces:
                for i, f in enumerate(obj.Shape.Faces):
                    # place outer wire first
                    wires = [f.OuterWire]
                    wires.extend([w for w in f.Wires if w.hashCode() != f.OuterWire.hashCode()])
                    svg += get_path(obj, plane,
                                    fill, pathdata, stroke, linewidth, lstyle,
                                    fill_opacity=fill_opacity,
                                    wires=f.Wires,
                                    pathname='%s_f%04d' % (obj.Name, i))
                    wiredEdges.extend(f.Edges)
            else:
                for i, w in enumerate(obj.Shape.Wires):
                    svg += get_path(obj, plane,
                                    fill, pathdata, stroke, linewidth, lstyle,
                                    fill_opacity=fill_opacity,
                                    edges=w.Edges,
                                    pathname='%s_w%04d' % (obj.Name, i))
                    wiredEdges.extend(w.Edges)
            if len(wiredEdges) != len(obj.Shape.Edges):
                for i, e in enumerate(obj.Shape.Edges):
                    if DraftGeomUtils.findEdge(e, wiredEdges) is None:
                        svg += get_path(obj, plane,
                                        fill, pathdata, stroke, linewidth,
                                        lstyle, fill_opacity=fill_opacity,
                                        edges=[e],
                                        pathname='%s_nwe%04d' % (obj.Name, i))
        else:
            # closed circle or spline
            if obj.Shape.Edges:
                if isinstance(obj.Shape.Edges[0].Curve, Part.Circle):
                    svg = get_circle(plane,
                                     fill, stroke, linewidth, lstyle,
                                     obj.Shape.Edges[0])
                else:
                    svg = get_path(obj, plane,
                                   fill, pathdata, stroke, linewidth, lstyle,
                                   fill_opacity=fill_opacity,
                                   edges=obj.Shape.Edges)

        if (App.GuiUp
                and hasattr(obj.ViewObject, "EndArrow")
                and obj.ViewObject.EndArrow
                and hasattr(obj.ViewObject, "ArrowType")
                and len(obj.Shape.Vertexes) > 1):
            p1 = get_proj(obj.Shape.Vertexes[-1].Point, plane)
            p2 = get_proj(obj.Shape.Vertexes[-2].Point, plane)
            angle = -DraftVecUtils.angle(p2 - p1)

            arrowsize = obj.ViewObject.ArrowSize.Value/pointratio
            svg += get_arrow(obj,
                             obj.ViewObject.ArrowType,
                             p1, arrowsize, stroke, linewidth, angle)

    # techdraw expects bottom-to-top coordinates
    if techdraw:
        svg = '<g transform ="scale(1,-1)">\n    ' + svg + '</g>\n'

    return svg


def getSVG(obj,
           scale=1, linewidth=0.35, fontsize=12,
           fillstyle="shape color", direction=None,
           linestyle=None,
           color=None, linespacing=None,
           techdraw=False, rotation=0,
           fillSpaces=False, override=True):
    """Return SVG string of the object. DEPRECATED. Use 'get_svg'."""
    utils.use_instead("get_svg")
    return get_svg(obj,
                   scale=scale, linewidth=linewidth, fontsize=fontsize,
                   fillstyle=fillstyle, direction=direction,
                   linestyle=linestyle,
                   color=color, linespacing=linespacing,
                   techdraw=techdraw, rotation=rotation,
                   fillspaces=fillSpaces, override=override)

## @}
