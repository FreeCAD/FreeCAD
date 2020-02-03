"""Provides the object code for Draft Arc_3Points."""
## @package arc_3points
# \ingroup DRAFT
# \brief Provides the object code for Draft Arc_3Points.

# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
import math
import FreeCAD as App
import Part
import Draft
from draftutils.messages import _msg, _err, _log
from draftutils.translate import _tr
if App.GuiUp:
    import draftutils.gui_utils as gui_utils


def make_arc_3points(points, placement=None, face=False,
                     support=None, map_mode="Deactivated",
                     primitive=False):
    """Draw a circular arc defined by three points in the circumference.

    Parameters
    ----------
    points: list of Base::Vector3
        A list that must be three points.

    placement: Base::Placement, optional
        It defaults to `None`.
        It is a placement, comprised of a `Base` (`Base::Vector3`),
        and a `Rotation` (`Base::Rotation`).
        If it exists it moves the center of the new object to the point
        indicated by `placement.Base`, while `placement.Rotation`
        is ignored so that the arc keeps the same orientation
        with which it was created.

        If both `support` and `placement` are given,
        `placement.Base` is used for the `AttachmentOffset.Base`,
        and again `placement.Rotation` is ignored.

    face: bool, optional
        It defaults to `False`.
        If it is `True` it will create a face in the closed arc.
        Otherwise only the circumference edge will be shown.

    support: App::PropertyLinkSubList, optional
        It defaults to `None`.
        It is a list containing tuples to define the attachment
        of the new object.

        A tuple in the list needs two elements;
        the first is an external object, and the second is another tuple
        with the names of sub-elements on that external object
        likes vertices or faces.
        ::
            support = [(obj, ("Face1"))]
            support = [(obj, ("Vertex1", "Vertex5", "Vertex8"))]

        This parameter sets the `Support` property but it only really affects
        the position of the new object when the `map_mode`
        is set to other than `'Deactivated'`.

    map_mode: str, optional
        It defaults to `'Deactivated'`.
        It defines the type of `'MapMode'` of the new object.
        This parameter only works when a `support` is also provided.

        Example: place the new object on a face or another object.
        ::
            support = [(obj, ("Face1"))]
            map_mode = 'FlatFace'

        Example: place the new object on a plane created by three vertices
        of an object.
        ::
            support = [(obj, ("Vertex1", "Vertex5", "Vertex8"))]
            map_mode = 'ThreePointsPlane'

    primitive: bool, optional
        It defaults to `False`. If it is `True`, it will create a Part
        primitive instead of a Draft object.
        In this case, `placement`, `face`, `support`, and `map_mode`
        are ignored.

    Returns
    -------
    Part::Part2DObject or Part::Feature
        The new arc object.
        Normally it returns a parametric Draft object (`Part::Part2DObject`).
        If `primitive` is `True`, it returns a basic `Part::Feature`.
    """
    _log("make_arc_3points")
    _msg(16 * "-")
    _msg(_tr("Arc by 3 points"))

    if not isinstance(points, (list, tuple)):
        _err(_tr("Wrong input: must be list or tuple of three points."))
        return None

    if len(points) != 3:
        _err(_tr("Wrong input: must be three points."))
        return None

    if placement is not None:
        if not isinstance(placement, App.Placement):
            _err(_tr("Wrong input: incorrect placement"))
            return None

    p1, p2, p3 = points

    _edge = Part.Arc(p1, p2, p3)
    edge = _edge.toShape()
    radius = edge.Curve.Radius
    center = edge.Curve.Center

    _msg("p1: {}".format(p1))
    _msg("p2: {}".format(p2))
    _msg("p3: {}".format(p3))
    _msg(_tr("Radius: ") + "{}".format(radius))
    _msg(_tr("Center: ") + "{}".format(center))

    if primitive:
        obj = App.ActiveDocument.addObject("Part::Feature", "Arc")
        obj.Shape = edge
        _msg(_tr("Primitive object"))
        return obj

    rot = App.Rotation(edge.Curve.XAxis,
                       edge.Curve.YAxis,
                       edge.Curve.Axis, "ZXY")
    _placement = App.Placement(center, rot)
    start = edge.FirstParameter
    end = math.degrees(edge.LastParameter)
    obj = Draft.makeCircle(radius,
                           placement=_placement, face=face,
                           startangle=start, endangle=end,
                           support=support)

    if App.GuiUp:
        gui_utils.autogroup(obj)

    original_placement = obj.Placement

    if placement and not support:
        obj.Placement.Base = placement.Base
        _msg(_tr("Final placement: ") + "{}".format(obj.Placement))
    if face:
        _msg(_tr("Face: True"))
    if support:
        _msg(_tr("Support: ") + "{}".format(support))
        obj.MapMode = map_mode
        _msg(_tr("Map mode: " + "{}".format(map_mode)))
        if placement:
            obj.AttachmentOffset.Base = placement.Base
            obj.AttachmentOffset.Rotation = original_placement.Rotation
            _msg(_tr("Attachment offset: {}".format(obj.AttachmentOffset)))
        _msg(_tr("Final placement: ") + "{}".format(obj.Placement))

    return obj
