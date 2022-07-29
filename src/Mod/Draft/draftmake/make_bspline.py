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
"""Provides functions to create BSpline objects."""
## @package make_bspline
# \ingroup draftmake
# \brief Provides functions to create BSpline objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.translate import translate
from draftobjects.bspline import BSpline

if App.GuiUp:
    from draftviewproviders.view_bspline import ViewProviderBSpline


def make_bspline(pointslist, closed=False, placement=None, face=None, support=None):
    """make_bspline(pointslist, [closed], [placement])

    Creates a B-Spline object from the given list of vectors.

    Parameters
    ----------
    pointlist : [Base.Vector]
        List of points to create the polyline.
        Instead of a pointslist, you can also pass a Part Wire.
        TODO: Change the name so!

    closed : bool
        If closed is True or first and last points are identical,
        the created BSpline will be closed.

    placement : Base.Placement
        If a placement is given, it is used.

    face : Bool
        If face is False, the rectangle is shown as a wireframe,
        otherwise as a face.

    support :
        TODO: Describe
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if len(pointslist) < 2:
        _err = "Draft.make_bspline: not enough points"
        App.Console.PrintError(translate("draft", _err)+"\n")
        return
    if (pointslist[0] == pointslist[-1]):
        if len(pointslist) > 2:
            closed = True
            pointslist.pop()
            _err = "Draft.make_bspline: Equal endpoints forced Closed"
            App.Console.PrintWarning(translate("Draft", _err) + _err + "\n")
        else:
            # len == 2 and first == last   GIGO
            _err = "Draft.make_bspline: Invalid pointslist"
            App.Console.PrintError(translate("Draft", _err)+"\n")
            return
    # should have sensible parms from here on
    if placement:
        utils.type_check([(placement,App.Placement)], "make_bspline")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BSpline"
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    BSpline(obj)
    obj.Closed = closed
    obj.Points = pointslist
    obj.Support = support
    if face is not None:
        obj.MakeFace = face
    if placement: obj.Placement = placement
    if App.GuiUp:
        ViewProviderBSpline(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeBSpline = make_bspline

## @}
