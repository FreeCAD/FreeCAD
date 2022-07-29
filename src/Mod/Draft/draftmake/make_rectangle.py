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
"""Provides functions to create Rectangle objects."""
## @package make_rectangle
# \ingroup draftmake
# \brief This module provides the code for Draft make_rectangle function.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.rectangle import Rectangle

if App.GuiUp:
    from draftviewproviders.view_rectangle import ViewProviderRectangle


def make_rectangle(length, height=0, placement=None, face=None, support=None):
    """make_rectangle(length, width, [placement], [face])

    Creates a Rectangle object with length in X direction and height in Y
    direction.

    Parameters
    ----------
    length, height : dimensions of the rectangle

    placement : Base.Placement
        If a placement is given, it is used.

    face : Bool
        If face is False, the rectangle is shown as a wireframe,
        otherwise as a face.

    Rectangles can also be constructed by giving them a list of four vertices
    as first argument: make_rectangle(list_of_vertices, face=...)
    but you are responsible to check yourself that these 4 vertices are ordered
    and actually form a rectangle, otherwise the result might be wrong. Placement
    is ignored when constructing a rectangle this way (face argument is kept).
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if isinstance(length,(list,tuple)) and (len(length) == 4):
        verts = length
        xv = verts[1].sub(verts[0])
        yv = verts[3].sub(verts[0])
        zv = xv.cross(yv)
        rr = App.Rotation(xv, yv, zv, "XYZ")
        rp = App.Placement(verts[0], rr)
        return make_rectangle(xv.Length, yv.Length, rp, face, support)

    if placement:
        utils.type_check([(placement,App.Placement)], "make_rectangle")

    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Rectangle")
    Rectangle(obj)

    obj.Length = length
    obj.Height = height
    obj.Support = support

    if face is not None:
        obj.MakeFace = face

    if placement: obj.Placement = placement

    if App.GuiUp:
        ViewProviderRectangle(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeRectangle = make_rectangle

## @}
