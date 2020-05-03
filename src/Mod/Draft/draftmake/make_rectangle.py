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
"""This module provides the code for Draft make_rectangle function.
"""
## @package make_rectangle
# \ingroup DRAFT
# \brief This module provides the code for Draft make_rectangle function.

import FreeCAD as App

from draftutils.gui_utils import format_object
from draftutils.gui_utils import select

from draftutils.utils import type_check

from draftobjects.rectangle import Rectangle
if App.GuiUp:
    from draftviewproviders.view_rectangle import ViewProviderRectangle


def make_rectangle(length, height, placement=None, face=None, support=None):
    """makeRectangle(length, width, [placement], [face])

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
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if placement: type_check([(placement,App.Placement)], "make_rectangle")

    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Rectangle")
    Rectangle(obj)

    obj.Length = length
    obj.Height = height
    obj.Support = support

    if face != None:
        obj.MakeFace = face

    if placement: obj.Placement = placement

    if App.GuiUp:
        ViewProviderRectangle(obj.ViewObject)
        format_object(obj)
        select(obj)

    return obj


makeRectangle = make_rectangle