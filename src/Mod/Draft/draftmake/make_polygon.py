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
"""This module provides the object code for Draft make_rectangle function.
"""
## @package make_rectangle
# \ingroup DRAFT
# \brief This module provides the code for Draft make_rectangle function.

import FreeCAD as App

from draftutils.gui_utils import format_object
from draftutils.gui_utils import select

from draftutils.utils import type_check

from draftobjects.polygon import Polygon
from draftviewproviders.view_base import ViewProviderDraft



def make_polygon(nfaces, radius=1, inscribed=True, placement=None, face=None, support=None):
    """makePolgon(nfaces,[radius],[inscribed],[placement],[face]): Creates a
    polygon object with the given number of faces and the radius.
    if inscribed is False, the polygon is circumscribed around a circle
    with the given radius, otherwise it is inscribed. If face is True,
    the resulting shape is displayed as a face, otherwise as a wireframe.
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if nfaces < 3: return None
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Polygon")
    Polygon(obj)
    obj.FacesNumber = nfaces
    obj.Radius = radius
    if face != None:
        obj.MakeFace = face
    if inscribed:
        obj.DrawMode = "inscribed"
    else:
        obj.DrawMode = "circumscribed"
    obj.Support = support
    if placement: obj.Placement = placement
    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        format_object(obj)
        select(obj)

    return obj