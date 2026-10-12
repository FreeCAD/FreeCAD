# SPDX-License-Identifier: LGPL-2.1-or-later

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
"""Provides functions to create Block objects."""

## @package make_block
# \ingroup draftmake
# \brief Provides functions to create Block objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
from draftobjects.block import Block
from draftutils import gui_utils
from freecad.deprecation import deprecated

if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraftPart


def make_block(objectslist):
    """make_block(objectslist)

    Creates a Draft Block from the given objects.

    Parameters
    ----------
    objectslist : list
        Major radius of the ellipse.

    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    obj = App.ActiveDocument.addObject("Part::FeaturePython", "Block")
    obj.addExtension("Part::AttachExtensionPython")
    Block(obj)
    obj.Components = objectslist
    if App.GuiUp:
        ViewProviderDraftPart(obj.ViewObject)
        for o in objectslist:
            o.ViewObject.Visibility = False
        gui_utils.select(obj)
    return obj


@deprecated(
    deprecated_in="26.3",
    removed_in="28.3",
    replacement="Draft.make_block()",
)
def makeBlock(*args, **kwarg):
    """DEPRECATED. Use 'make_block'."""
    return make_block(*args, **kwarg)


## @}
