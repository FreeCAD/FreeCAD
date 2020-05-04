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
"""This module provides the code for Draft mirror function.
"""
## @package mirror
# \ingroup DRAFT
# \brief This module provides the code for Draft mirror function.

import math

import FreeCAD as App

import DraftVecUtils

import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftutils.translate import _tr


def mirror(objlist, p1, p2):
    """mirror(objlist, p1, p2)

    Create a Part::Mirror of the given object(s) along a plane defined
    by the 2 given points and the draft working plane normal.

    TODO: Implement a proper Draft mirror tool that do not create a 
          Part object but works similar to offset

    Parameters
    ----------
    objlist :

    p1 : Base.Vector
        Point 1 of the mirror plane

    p2 : Base.Vector
        Point 1 of the mirror plane

    """

    if not objlist:
        _err = "No object given"
        App.Console.PrintError(_tr(_err) + "\n")
        return
    if p1 == p2:
        _err = "The two points are coincident"
        App.Console.PrintError(_tr(_err) + "\n")
        return
    if not isinstance(objlist,list):
        objlist = [objlist]

    if hasattr(App, "DraftWorkingPlane"):
        norm = App.DraftWorkingPlane.getNormal()
    elif App.GuiUp:
        norm = FreeCADGui.ActiveDocument.ActiveView.getViewDirection().negative()
    else:
        norm = App.Vector(0,0,1)
    
    pnorm = p2.sub(p1).cross(norm).normalize()

    result = []

    for obj in objlist:
        mir = App.ActiveDocument.addObject("Part::Mirroring","mirror")
        mir.Label = "Mirror of " + obj.Label
        mir.Source = obj
        mir.Base = p1
        mir.Normal = pnorm
        gui_utils.format_object(mir, obj)
        result.append(mir)

    if len(result) == 1:
        result = result[0]
        gui_utils.select(result)

    return result
