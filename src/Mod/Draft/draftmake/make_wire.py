# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provides functions to create multipoint Wire objects."""
## @package make_wire
# \ingroup draftmake
# \brief Provides functions to create multipoint Wire objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import DraftGeomUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.wire import Wire

if App.GuiUp:
    from draftviewproviders.view_wire import ViewProviderWire


def make_wire(pointslist, closed=False, placement=None, face=None, support=None, bs2wire=False):
    """make_wire(pointslist, [closed], [placement])

    Creates a Wire object from the given list of vectors.  If face is
    true (and wire is closed), the wire will appear filled. Instead of
    a pointslist, you can also pass a Part Wire.

    Parameters
    ----------
    pointslist : [Base.Vector]
        List of points to create the polyline

    closed : bool
        If closed is True or first and last points are identical,
        the created polyline will be closed.

    placement : Base.Placement
        If a placement is given, it is used.

    face : Bool
        If face is False, the rectangle is shown as a wireframe,
        otherwise as a face.

    support :
        TODO: Describe

    bs2wire : bool
        TODO: Describe
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return None

    import Part

    if isinstance(pointslist, (list,tuple)):
        for pnt in pointslist:
            if not isinstance(pnt, App.Vector):
                App.Console.PrintError(
                    "Items must be Base.Vector objects, not {}\n".format(
                    type(pnt)))
                return None

    elif isinstance(pointslist, Part.Wire):
        for edge in pointslist.Edges:
            if not DraftGeomUtils.is_straight_line(edge):
                App.Console.PrintError("All edges must be straight lines\n")
                return None
        closed = pointslist.isClosed()
        pointslist = [v.Point for v in pointslist.OrderedVertexes]

    else:
        App.Console.PrintError("Can't make Draft Wire from {}\n".format(
            type(pointslist)))
        return None


    if len(pointslist) == 0:
        App.Console.PrintWarning("Draft Wire created with empty point list\n")

    if placement:
        utils.type_check([(placement, App.Placement)], "make_wire")
        ipl = placement.inverse()
        if not bs2wire:
            pointslist = [ipl.multVec(p) for p in pointslist]

    if len(pointslist) == 2:
        fname = "Line"
    else:
        fname = "Wire"

    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython", fname)
    Wire(obj)
    obj.Points = pointslist
    obj.Closed = closed
    obj.Support = support

    if face is not None:
        obj.MakeFace = face

    if placement:
        obj.Placement = placement

    if App.GuiUp:
        ViewProviderWire(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeWire = make_wire

## @}
