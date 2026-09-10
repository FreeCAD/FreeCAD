# SPDX-License-Identifier: LGPL-2.1-or-later

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

from __future__ import annotations

## @package make_circle
# \ingroup draftmake
# \brief Provides functions to create Circle objects.

## \addtogroup draftmake
# @{
import math
from typing import Protocol, TypeGuard, cast

import FreeCAD as App
import Part
import DraftVecUtils
from draftgeoutils import general as geo_general
from draftobjects.circle import Circle, CircleObject
from draftobjects.type_hints import QuantityValueInput
from draftutils import utils
from draftutils import gui_utils

if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraft


class _CircleVertex(Protocol):
    Point: App.Vector


class _CircularEdge(Protocol):
    Curve: Part.Circle
    Vertexes: list[_CircleVertex]


def _new_circle_object(doc: App.Document, name: str) -> CircleObject:
    obj = doc.addObject("Part::Part2DObjectPython", name)
    Circle(obj)
    return cast(CircleObject, obj)


def _is_circular_edge(edge: Part.Edge) -> TypeGuard[_CircularEdge]:
    return geo_general.geomType(edge) == "Circle"


def _get_normal(axis, ref_rot):
    local_axis = ref_rot.inverted().multVec(axis)
    x, y, z = [abs(coord) for coord in list(local_axis)]
    # Use local X, Y or Z axis for comparison:
    if z >= x and z >= y:
        local_comp_vec = App.Vector(0, 0, 1)
    elif y >= x and y >= z:
        local_comp_vec = App.Vector(0, -1, 0)  # -Y to match the Front view
    else:
        local_comp_vec = App.Vector(1, 0, 0)
    comp_vec = ref_rot.multVec(local_comp_vec)
    axis = App.Vector(axis)  # create independent copy
    if axis.getAngle(comp_vec) > math.pi / 2:
        axis = axis.negative()
    return axis


def make_circle(radius, placement=None, face=None, startangle=None, endangle=None, support=None):
    """make_circle(radius, [placement], [face], [startangle], [endangle])
    or make_circle(edge, [placement], [face]):

    Creates a circle object with given parameters.

    If startangle and endangle are provided and not equal, the object will be
    an arc instead of a full circle.

    Parameters
    ----------
    radius: the radius of the circle or the shape of a circular edge
        If it is an edge, startangle and endangle are ignored.
        edge.Curve must be a Part.Circle.

    placement: optional
        If radius is an edge, placement is adjusted to match the geometry
        of the edge. The Z axis of the adjusted placement will be parallel
        to the (negative) edge axis. Its Base will match the edge center.

    face: Bool
        If face is False, the circle is shown as a wireframe,
        otherwise as a face.

    startangle: start angle of the circle (in degrees)
        Recalculated if not in the -360 to 360 range.

    endangle: end angle of the circle (in degrees)
        Recalculated if not in the -360 to 360 range.

    support: App::PropertyLinkSubList, optional
        It defaults to `None`.
        It is a list containing tuples to define the attachment
        of the new object.

        This parameter sets the `Support` property but it only really
        affects the position of the new object if its `MapMode` is
        set to other than `'Deactivated'`.
    """

    doc = App.ActiveDocument
    if not doc:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if placement:
        utils.type_check([(placement, App.Placement)], "make_circle")

    if (isinstance(radius, Part.Edge) and len(radius.Vertexes) > 1) or startangle != endangle:
        name = "Arc"
    else:
        name = "Circle"

    obj = _new_circle_object(doc, name)

    if face is not None:
        obj.MakeFace = face

    if isinstance(radius, Part.Edge) and _is_circular_edge(radius):
        edge = radius
        curve = edge.Curve
        obj.Radius = curve.Radius
        axis = curve.Axis
        center = curve.Center
        ref_rot = App.Rotation() if placement is None else placement.Rotation
        normal = _get_normal(axis, ref_rot)
        ref_x_axis = ref_rot.multVec(App.Vector(1, 0, 0))
        ref_y_axis = ref_rot.multVec(App.Vector(0, 1, 0))
        rot = App.Rotation(ref_x_axis, ref_y_axis, normal, "ZXY")
        placement = App.Placement(center, rot)
        if len(edge.Vertexes) > 1:
            v1 = (edge.Vertexes[0].Point).sub(center)
            v2 = (edge.Vertexes[-1].Point).sub(center)
            if not axis.isEqual(normal, 1e-4):
                v1, v2 = v2, v1
            x_axis = rot.multVec(App.Vector(1, 0, 0))
            obj.FirstAngle = math.degrees(DraftVecUtils.angle(x_axis, v1, normal))
            obj.LastAngle = math.degrees(DraftVecUtils.angle(x_axis, v2, normal))
    else:
        obj.Radius = cast(QuantityValueInput, radius)
        if (startangle is not None) and (endangle is not None):
            obj.FirstAngle = math.copysign(abs(startangle) % 360, startangle)
            obj.LastAngle = math.copysign(abs(endangle) % 360, endangle)

    obj.AttachmentSupport = support

    if placement:
        obj.Placement = placement

    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeCircle = make_circle

## @}
