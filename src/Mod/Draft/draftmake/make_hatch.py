# SPDX-License-Identifier: LGPL-2.1-or-later

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


def make_hatch(selection, filename, pattern, scale, rotation, translate=True):
    """make_hatch(selection, filename, pattern, scale, rotation, translate)

    Creates a hatch object by applying a pattern from a PAT file to faces.

    Parameters
    ----------
    selection: single object / list of objects / selection set / LinkSubList-like iterable
        Examples:
        App.ActiveDocument.Box
        Gui.Selection.getSelection()
        Gui.Selection.getSelectionEx("", 0)
        [(App.ActiveDocument.Box, ("Face1",))]
        [[App.ActiveDocument.Box, ["Face1", "Face2"]]]

    filename: string
        Filename of the PAT file with relative or absolute path.
        To use a relative path the FreeCAD document must have been saved.

    pattern: string
        Case-sensitive name of the hatch pattern. Must exist in the PAT file.

    scale: float
        The pattern scale.

    rotation: float
        The pattern rotation in degrees.

    translate: Bool
        Specifies if the faces are temporarily translated to the global
        XY-plane during the hatching process. Setting it to `False` may
        give wrong results for non-XY faces.
        See https://wiki.freecad.org/Draft_Hatch.

    Returns
    -------
    Part::FeaturePython object
        A scripted object of the Proxy type `"Hatch"`.
    """

    if not FreeCAD.ActiveDocument:
        return
    if not isinstance(selection, list):
        selection = [selection]
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Hatch")
    Hatch(obj)
    obj.File = filename
    obj.Pattern = pattern
    obj.Scale = scale
    obj.Rotation = rotation
    obj.Translate = translate
    obj.Proxy.add_faces(obj, selection)
    if FreeCAD.GuiUp:
        ViewProviderDraftHatch(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)
    return obj
