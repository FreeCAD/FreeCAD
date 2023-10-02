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
"""Provides functions to create a fusion of two shapes."""
## @package fuse
# \ingroup draftfunctions
# \brief Provides functions to create a fusion of two shapes.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftmake.make_wire import Wire

if App.GuiUp:
    from draftviewproviders.view_wire import ViewProviderWire


def fuse(object1, object2):
    """fuse(oject1, object2)

    Returns an object made from the union of the 2 given objects.
    If the objects are coplanar, a special Draft Wire is used, otherwise we use
    a standard Part fuse.

    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    import Part
    import DraftGeomUtils
    # testing if we have holes:
    holes = False
    fshape = object1.Shape.fuse(object2.Shape)
    fshape = fshape.removeSplitter()
    for f in fshape.Faces:
        if len(f.Wires) > 1:
            holes = True
    if DraftGeomUtils.isCoplanar(object1.Shape.fuse(object2.Shape).Faces) and not holes:
        obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Fusion")
        Wire(obj)
        if App.GuiUp:
            ViewProviderWire(obj.ViewObject)
        obj.Base = object1
        obj.Tool = object2
    elif holes:
        # temporary hack, since Part::Fuse objects don't remove splitters
        obj = App.ActiveDocument.addObject("Part::Feature","Fusion")
        obj.Shape = fshape
    else:
        obj = App.ActiveDocument.addObject("Part::Fuse","Fusion")
        obj.Base = object1
        obj.Tool = object2
    if App.GuiUp:
        object1.ViewObject.Visibility = False
        object2.ViewObject.Visibility = False
        gui_utils.format_object(obj,object1)
        gui_utils.select(obj)

    return obj

## @}
