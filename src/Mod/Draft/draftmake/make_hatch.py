# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains FreeCAD commands for the Draft workbench"""

import FreeCAD
from draftobjects.hatch import Hatch

if FreeCAD.GuiUp:
    from draftviewproviders.view_hatch import ViewProviderDraftHatch
    import draftutils.gui_utils as gui_utils


def make_hatch(baseobject, filename, pattern, scale, rotation, translate=True):
    """make_hatch(baseobject, filename, pattern, scale, rotation, translate): Creates and returns a
    hatch object made by applying the given pattern of the given PAT file to the faces of
    the given base object. Given scale, rotation and translate factors are applied to the hatch object.
    The result is a Part-based object created in the active document."""

    if not FreeCAD.ActiveDocument:
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Hatch")
    Hatch(obj)
    obj.Base = baseobject
    obj.File = filename
    obj.Pattern = pattern
    obj.Scale = scale
    obj.Rotation = rotation
    obj.Translate = translate
    if FreeCAD.GuiUp:
        ViewProviderDraftHatch(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.autogroup(obj)
    return obj
