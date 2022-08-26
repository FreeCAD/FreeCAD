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
"""Provides functions to create Polygon objects."""
## @package make_polygon
# \ingroup draftmake
# \brief Provides functions to create Polygon objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftobjects.polygon import Polygon
from draftviewproviders.view_base import ViewProviderDraft


def make_polygon(nfaces, radius=1, inscribed=True, placement=None, face=None, support=None):
    """makePolgon(edges,[radius],[inscribed],[placement],[face])

    Creates a polygon object with the given number of edges and radius.

    Parameters
    ----------
    edges : int
        Number of edges of the polygon.

    radius :
        Radius of the control circle.

    inscribed : bool
        Defines is the polygon is inscribed or not into the control circle.

    placement : Base.Placement
        If placement is given, it is used.

    face : bool
        If face is True, the resulting shape is displayed as a face,
        otherwise as a wireframe.

    support :
        TODO: Describe
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if nfaces < 3: return None
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Polygon")
    Polygon(obj)
    obj.FacesNumber = nfaces
    obj.Radius = radius
    if face is not None:
        obj.MakeFace = face
    if inscribed:
        obj.DrawMode = "inscribed"
    else:
        obj.DrawMode = "circumscribed"
    obj.Support = support
    if placement: obj.Placement = placement
    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makePolygon = make_polygon

## @}
