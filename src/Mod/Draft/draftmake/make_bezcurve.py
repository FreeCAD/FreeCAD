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
"""Provides functions to create BezCurve objects."""
## @package make_bezcurve
# \ingroup draftmake
# \brief Provides functions to create BezCurve objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.translate import translate
from draftobjects.bezcurve import BezCurve

if App.GuiUp:
    from draftviewproviders.view_bezcurve import ViewProviderBezCurve


def make_bezcurve(pointslist,
                  closed=False, placement=None, face=None, support=None,
                  degree=None):
    """make_bezcurve(pointslist, [closed], [placement])

    Creates a Bezier Curve object from the given list of vectors.

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

    degree : int
        Degree of the BezCurve
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if not isinstance(pointslist,list):
        nlist = []
        for v in pointslist.Vertexes:
            nlist.append(v.Point)
        pointslist = nlist
    if placement:
        utils.type_check([(placement,App.Placement)], "make_bezcurve")
    if len(pointslist) == 2: fname = "Line"
    else: fname = "BezCurve"
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython",fname)
    BezCurve(obj)
    obj.Points = pointslist
    if degree:
        obj.Degree = degree
    else:
        import Part
        obj.Degree = min((len(pointslist)-(1 * (not closed))),
                         Part.BezierCurve().MaxDegree)
    obj.Closed = closed
    obj.Support = support
    if face is not None:
        obj.MakeFace = face
    obj.Proxy.resetcontinuity(obj)
    if placement: obj.Placement = placement
    if App.GuiUp:
        ViewProviderBezCurve(obj.ViewObject)
#        if not face: obj.ViewObject.DisplayMode = "Wireframe"
#        obj.ViewObject.DisplayMode = "Wireframe"
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeBezCurve = make_bezcurve

## @}
