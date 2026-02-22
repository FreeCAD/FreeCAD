# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides functions to create Arc objects by using 3 points."""
## @package make_arc_3points
# \ingroup draftmake
# \brief Provides functions to create Arc objects by using 3 points.

## \addtogroup draftmake
# @{
import math

import FreeCAD as App
import Part
from draftmake import make_circle
from draftutils import utils
from draftutils.messages import _err
from draftutils.translate import translate


def make_arc_3points(points, placement=None, face=False, support=None, primitive=False):
    """Draw a circular arc defined by three points on the circumference.

    Parameters
    ----------
    points: list of Base::Vector3
        A list that must be three points.

    placement: Base::Placement, optional
        It is adjusted to match the geometry of the created edge.
        The Z axis of the adjusted placement will be parallel to the
        (negative) edge axis. Its Base will match the edge center.

    face: bool, optional
        It defaults to `False`.
        If it is `True` it will create a face in the closed arc.
        Otherwise only the circumference edge will be shown.

    support: App::PropertyLinkSubList, optional
        It defaults to `None`.
        It is a list containing tuples to define the attachment
        of the new object.

        This parameter sets the `Support` property but it only really
        affects the position of the new object if its `MapMode` is
        set to other than `'Deactivated'`.

    primitive: bool, optional
        It defaults to `False`. If it is `True`, it will create a Part
        primitive instead of a Draft object.
        In this case, `placement`, `face` and `support` are ignored.

    Returns
    -------
    Part::Part2DObject or Part::Feature
        The new arc object.
        Normally it returns a parametric Draft object (`Part::Part2DObject`).
        If `primitive` is `True`, it returns a basic `Part::Feature`.

    None
        Returns `None` if there is a problem and the object cannot be created.
    """
    _name = "make_arc_3points"

    try:
        utils.type_check([(points, (list, tuple))], name=_name)
    except TypeError:
        _err(translate("draft", "Points:") + " {}".format(points))
        _err(translate("draft", "Wrong input: must be a list or tuple of 3 points exactly."))
        return None

    if len(points) != 3:
        _err(translate("draft", "Points:") + " {}".format(points))
        _err(translate("draft", "Wrong input: must be list or tuple of 3 points exactly."))
        return None

    p1, p2, p3 = points

    try:
        utils.type_check([(p1, App.Vector), (p2, App.Vector), (p3, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft", "Wrong input: incorrect type of points."))
        return None

    if placement is not None:
        try:
            utils.type_check([(placement, App.Placement)], name=_name)
        except TypeError:
            _err(translate("draft", "Placement:") + " {}".format(placement))
            _err(translate("draft", "Wrong input: incorrect type of placement."))
            return None

    try:
        edge = Part.Arc(p1, p2, p3).toShape()
    except Part.OCCError as error:
        _err(translate("draft", "Cannot generate shape:") + " " + "{}".format(error))
        return None

    if primitive:
        obj = App.ActiveDocument.addObject("Part::Feature", "Arc")
        obj.Shape = edge
        return obj

    return make_circle.make_circle(edge, placement=placement, face=face, support=support)


## @}
