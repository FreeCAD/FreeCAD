# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to create Circle objects."""
## @package make_circle
# \ingroup draftmake
# \brief Provides functions to create Circle objects.

## \addtogroup draftmake
# @{
import math

import FreeCAD as App
import Part
import DraftGeomUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.circle import Circle

if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraft


def make_circle(radius, placement=None, face=None, startangle=None, endangle=None, support=None):
    """make_circle(radius, [placement, face, startangle, endangle])
    or make_circle(edge,[face]):

    Creates a circle object with given parameters.

    Parameters
    ----------
    radius : the radius of the circle.

    placement :
        If placement is given, it is used.

    face : Bool
        If face is False, the circle is shown as a wireframe,
        otherwise as a face.

    startangle : start angle of the arc (in degrees)

    endangle : end angle of the arc (in degrees)
        if startangle and endangle are equal, a circle is created,
        if they are different an arc is created

    edge : edge.Curve must be a 'Part.Circle'
        the circle is created from the given edge

    support :
        TODO: Describe
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if placement:
        utils.type_check([(placement,App.Placement)], "make_circle")

    if startangle != endangle:
        _name = "Arc"
    else:
        _name = "Circle"

    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython", _name)

    Circle(obj)

    if face is not None:
        obj.MakeFace = face

    if isinstance(radius,Part.Edge):
        edge = radius
        if DraftGeomUtils.geomType(edge) == "Circle":
            obj.Radius = edge.Curve.Radius
            placement = App.Placement(edge.Placement)
            delta = edge.Curve.Center.sub(placement.Base)
            placement.move(delta)
            # Rotation of the edge
            rotOk = App.Rotation(edge.Curve.XAxis, edge.Curve.YAxis, edge.Curve.Axis, "ZXY")
            placement.Rotation = rotOk
            if len(edge.Vertexes) > 1:
                v0 = edge.Curve.XAxis
                v1 = (edge.Vertexes[0].Point).sub(edge.Curve.Center)
                v2 = (edge.Vertexes[-1].Point).sub(edge.Curve.Center)
                # Angle between edge.Curve.XAxis and the vector from center to start of arc
                a0 = math.degrees(App.Vector.getAngle(v0, v1))
                # Angle between edge.Curve.XAxis and the vector from center to end of arc
                a1 = math.degrees(App.Vector.getAngle(v0, v2))
                obj.FirstAngle = a0
                obj.LastAngle = a1
    else:
        obj.Radius = radius
        if (startangle is not None) and (endangle is not None):
            if startangle == -0: startangle = 0
            obj.FirstAngle = startangle
            obj.LastAngle = endangle

    obj.Support = support

    if placement:
        obj.Placement = placement

    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeCircle = make_circle

## @}
