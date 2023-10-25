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
"""
## @package svg
# \ingroup draftfunctions
# \brief Provides functions to return the SVG representation of shapes.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils
import WorkingPlane
import draftutils.utils as utils
import draftfunctions.svgtext as svgtext

from draftfunctions.svgshapes import get_proj, get_circle, get_path
from draftutils.messages import _wrn, _err

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")


## \addtogroup draftfunctions
# @{


def get_line_style(line_style, scale):
    """Return a linestyle scaled by a factor."""
    param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    style = None

    if line_style == "Dashed":
        style = param.GetString("svgDashedLine", "2,2")
    elif line_style == "Dashdot":
        style = param.GetString("svgDashdotLine", "3,2,0.2,2")
    elif line_style == "Dotted":
        style = param.GetString("svgDottedLine", "0.2,2")
    elif line_style:
        if "," in line_style:
            style = line_style

    if style:
        style = style.split(",")
        try:
            # scale dashes
            style = ",".join([str(float(d)/scale) for d in style])
            # print("lstyle ", style)
        except Exception:
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
    _style = 'style="stroke-miterlimit:4;stroke-dasharray:none;stroke-linecap:square"'

    if obj.ViewObject.ArrowType == "Circle":
        svg += '<circle '
        svg += _cx_cy_r + ' '
        svg += 'fill="{}" stroke="{}" '.format("none", color)
        svg += 'style="stroke-width:{};'.format(linewidth)
        svg += 'stroke-miterlimit:4;stroke-dasharray:none;stroke-linecap:square" '
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


def format_point(coords, action='L'):
    """Return a string with a formatted point."""
    return "{action}{x},{y}".format(x=coords.x, y=coords.y, action=action)


def _svg_shape(svg, obj, plane,
               fillstyle, pathdata, stroke, linewidth, lstyle):
    """Return the SVG representation of a Part.Shape."""
    if "#" in fillstyle:
        fill = fillstyle
    elif fillstyle == "shape color":
        fill = "#888888"
    elif fillstyle in ("none",None):
        fill = "none"
    else:
        fill = 'url(#' + fillstyle + ')'

    svg += get_path(obj, plane,
                    fill, pathdata, stroke, linewidth, lstyle,
                    fill_opacity=None,
                    edges=obj.Edges, pathname="")
    return svg


def _svg_dimension(obj, plane, scale, linewidth, fontsize,
                   stroke, tstroke, pointratio, techdraw, rotation):
    """Return the SVG representation of a linear dimension."""
    if not App.GuiUp:
        _wrn("'{}': SVG can only be generated "
             "in GUI mode".format(obj.Label))
        return ""

    if not hasattr(obj.ViewObject, "Proxy") or not obj.ViewObject.Proxy:
        _err("'{}': doesn't have Proxy, "
             "SVG cannot be generated".format(obj.Label))
        return ""

    vobj = obj.ViewObject
    prx = vobj.Proxy

    if not hasattr(prx, "p1"):
        _err("'{}': doesn't have points, "
             "SVG cannot be generated".format(obj.Label))
        return ""

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

    if vobj.DisplayMode == "World":
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
        svg += ';stroke-miterlimit:4;stroke-dasharray:none;stroke-linecap:square" '
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

            if not hasattr(obj, "Diameter") \
                    or obj.Diameter \
                    or not prx.is_linked_to_circle():
                svg += get_arrow(obj,
                                 vobj.ArrowType,
                                 p2, arrowsize, stroke, linewidth,
                                 angle)

            svg += get_arrow(obj,
                             vobj.ArrowType,
                             p3, arrowsize, stroke, linewidth,
                             angle + math.pi)

    # drawing text
    svg += svgtext.get_text(plane, techdraw,
                            tstroke, fontsize, vobj.FontName,
                            tangle, tbase, prx.string)

    return svg


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
                or utils.get_type(obj) in ["Layer","BuildingPart"]):
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
            else:
                raise ValueError("'direction' cannot be: Vector(0, 0, 0)")
        elif isinstance(direction, WorkingPlane.plane):
            plane = direction

    stroke = "#000000"
    tstroke = stroke
    if color and override:
        if "#" in color:
            stroke = color
        else:
            stroke = utils.get_rgb(color)
        tstroke = stroke
    elif App.GuiUp:
        # find print color
        pc = get_print_color(obj)
        if pc:
            stroke = utils.get_rgb(pc)
        # get line color
        elif hasattr(obj, "ViewObject"):
            if hasattr(obj.ViewObject, "LineColor"):
                stroke = utils.get_rgb(obj.ViewObject.LineColor)
            elif hasattr(obj.ViewObject, "TextColor"):
                stroke = utils.get_rgb(obj.ViewObject.TextColor)
            if hasattr(obj.ViewObject, "TextColor"):
                tstroke = utils.get_rgb(obj.ViewObject.TextColor)

    lstyle = "none"
    if override:
        lstyle = get_line_style(linestyle, scale)
    else:
        if hasattr(obj, "ViewObject") and hasattr(obj.ViewObject, "DrawStyle"):
            lstyle = get_line_style(obj.ViewObject.DrawStyle, scale)

    if not obj:
        pass

    elif isinstance(obj, Part.Shape):
        svg = _svg_shape(svg, obj, plane,
                         fillstyle, pathdata, stroke, linewidth, lstyle)

    elif utils.get_type(obj) in ["Dimension", "LinearDimension"]:
        svg = _svg_dimension(obj, plane, scale, linewidth, fontsize,
                             stroke, tstroke, pointratio, techdraw, rotation)

    elif utils.get_type(obj) == "AngularDimension":
        if not App.GuiUp:
            _wrn("Export of dimensions to SVG is only available in GUI mode")

        if App.GuiUp:
            if obj.ViewObject.Proxy:
                if hasattr(obj.ViewObject.Proxy, "circle"):
                    prx = obj.ViewObject.Proxy

                    # drawing arc
                    fill = "none"
                    if obj.ViewObject.DisplayMode == "World":
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
                        halfarrowlength = 2 * arrowsize
                        arrowangle = 2 * math.asin(halfarrowlength / prx.circle.Curve.Radius)
                        if hasattr(obj.ViewObject, "FlipArrows") \
                                and obj.ViewObject.FlipArrows:
                            arrowangle = -arrowangle

                        _v1a = prx.circle.valueAt(prx.circle.FirstParameter
                                                  + arrowangle)
                        _v1b = prx.circle.valueAt(prx.circle.FirstParameter)

                        _v2a = prx.circle.valueAt(prx.circle.LastParameter
                                                  - arrowangle)
                        _v2b = prx.circle.valueAt(prx.circle.LastParameter)

                        u1 = get_proj(_v1a - _v1b, plane)
                        u2 = get_proj(_v2a - _v2b, plane)
                        angle1 = -DraftVecUtils.angle(u1)
                        angle2 = -DraftVecUtils.angle(u2)

                        svg += get_arrow(obj,
                                         obj.ViewObject.ArrowType,
                                         p2, arrowsize, stroke, linewidth,
                                         angle1)
                        svg += get_arrow(obj,
                                         obj.ViewObject.ArrowType,
                                         p3, arrowsize, stroke, linewidth,
                                         angle2)

                    # drawing text
                    if obj.ViewObject.DisplayMode == "World":
                        _diff = (prx.circle.LastParameter
                                 - prx.circle.FirstParameter)
                        t = prx.circle.tangentAt(prx.circle.FirstParameter
                                                 + _diff/2.0)
                        t = get_proj(t, plane)
                        tangle = -DraftVecUtils.angle(t)
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

                    svg += svgtext.get_text(plane, techdraw,
                                            tstroke, fontsize,
                                            obj.ViewObject.FontName,
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
            svg_path += 'stroke-linecap="square" '
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
            fontname = obj.ViewObject.FontName
            position = get_proj(obj.Placement.Base, plane)
            rotation = obj.Placement.Rotation
            justification = obj.ViewObject.Justification
            text = obj.Text
            svg += svgtext.get_text(plane, techdraw,
                                    tstroke, fontsize, fontname,
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
            svg += svgtext.get_text(plane, techdraw,
                                    tstroke, fontsize, n,
                                    r, p, t,
                                    linespacing, j)

    elif utils.get_type(obj) == "Axis":
        # returns the SVG representation of an Arch Axis system
        if not App.GuiUp:
            _wrn("Export of axes to SVG is only available in GUI mode")

        if App.GuiUp:
            vobj = obj.ViewObject
            fn = obj.ViewObject.FontName
            fill = 'none'
            rad = vobj.BubbleSize.Value/2
            n = 0
            for e in obj.Shape.Edges:
                svg += get_path(obj, plane,
                                fill, pathdata, stroke, linewidth, lstyle,
                                fill_opacity=None,
                                edges=[e])
            for t in obj.ViewObject.Proxy.getTextData():
                pos = t[1].add(App.Vector(0,-fontsize/2,0))
                svg += svgtext.get_text(plane, techdraw,
                                        tstroke, fontsize, fn,
                                        0.0, pos, t[0],
                                        1.0, "center")
            for b in obj.ViewObject.Proxy.getShapeData():
                if hasattr(b,"Curve") and isinstance(b.Curve,Part.Circle):
                    svg += get_circle(plane,
                                      fill, stroke, linewidth, "none",
                                      b)
                else:
                    sfill = stroke
                    svg += get_path(obj, plane,
                                    sfill, pathdata, stroke, linewidth, "none",
                                    fill_opacity=None,
                                    edges=b.Edges)

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
                    if App.GuiUp and hasattr(vobj,"ShapeColor"):
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

            if round(plane.axis.getAngle(App.Vector(0,0,1)),2) not in [0,3.14]:
                # if not in XY view, place the label at center
                p2 = obj.Shape.CenterOfMass
            else:
                _v = vobj.Proxy.coords.translation.getValue().getValue()
                p2 = obj.Placement.multVec(App.Vector(_v))

            _h = vobj.Proxy.header.translation.getValue().getValue()
            lspc = App.Vector(_h)
            p1 = p2 + lspc
            j = vobj.TextAlign
            t3 = svgtext.get_text(plane, techdraw,
                                  c, f1, n,
                                  a, get_proj(p1, plane), t1,
                                  linespacing, j, flip=True)
            svg += t3
            if t2:
                ofs = App.Vector(0, -lspc.Length, 0)
                if a:
                    Z = App.Vector(0, 0, 1)
                    ofs = App.Rotation(Z, -rotation).multVec(ofs)
                t4 = svgtext.get_text(plane, techdraw,
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
                    if (fillstyle == "shape color") and hasattr(vobj,"ShapeColor"):
                        fill = utils.get_rgb(vobj.ShapeColor,
                                             testbw=False)
                        fill_opacity = 1 - vobj.Transparency / 100.0
                    elif fillstyle in ("none",None):
                        fill = "none"
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
                fill = 'none' # Required if obj has a face. Edges processed here have no face.
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


def get_print_color(obj):
    """returns the print color of the parent layer, if available"""
    for parent in obj.InListRecursive:
        if (hasattr(parent,"ViewObject")
                and hasattr(parent.ViewObject,"UsePrintColor")
                and parent.ViewObject.UsePrintColor):
            if hasattr(parent.ViewObject,"LinePrintColor"):
                return parent.ViewObject.LinePrintColor
    return None


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
