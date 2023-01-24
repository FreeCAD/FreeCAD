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
"""Provides functions to create Shape2DView objects."""
## @package make_shape2dview
# \ingroup draftmake
# \brief Provides functions to create Shape2DView objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftobjects.shape2dview import Shape2DView

if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraftAlt


def make_shape2dview(baseobj,projectionVector=None,facenumbers=[]):
    """make_shape2dview(object, [projectionVector], [facenumbers])

    Add a 2D shape to the document, which is a 2D projection of the given object.

    Parameters
    ----------
    object :
        TODO: Describe

    projectionVector : Base.Vector
        Custom vector for the projection

    facenumbers : [] TODO: Describe
        A list of face numbers to be considered in individual faces mode.
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython","Shape2DView")
    Shape2DView(obj)
    if App.GuiUp:
        ViewProviderDraftAlt(obj.ViewObject)
    obj.Base = baseobj
    if projectionVector:
        obj.Projection = projectionVector
    if facenumbers:
        obj.FaceNumbers = facenumbers
    gui_utils.select(obj)

    return obj


makeShape2DView = make_shape2dview

## @}
