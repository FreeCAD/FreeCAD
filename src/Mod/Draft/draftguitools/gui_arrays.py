# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to create parametric Array objects. Grouping command."""
## @package gui_arrays
# \ingroup draftguitools
# \brief Provides GUI tools to create parametric Array objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_circulararray
import draftguitools.gui_orthoarray
import draftguitools.gui_patharray
import draftguitools.gui_pointarray
import draftguitools.gui_polararray
import draftguitools.gui_pathtwistedarray

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)
bool(draftguitools.gui_circulararray.__name__)
bool(draftguitools.gui_orthoarray.__name__)
bool(draftguitools.gui_patharray.__name__)
bool(draftguitools.gui_pointarray.__name__)
bool(draftguitools.gui_polararray.__name__)


class ArrayGroup:
    """Gui command for the group of array tools."""

    def GetCommands(self):
        """Tuple of array commands."""
        return ("Draft_OrthoArray",
                "Draft_PolarArray", "Draft_CircularArray",
                "Draft_PathArray", "Draft_PathLinkArray",
                "Draft_PointArray", "Draft_PointLinkArray",
                "Draft_PathTwistedArray", "Draft_PathTwistedLinkArray")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Array',
                'MenuText': QT_TRANSLATE_NOOP("Draft_ArrayTools", "Array tools"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_ArrayTools", "Create various types of arrays, including rectangular, polar, circular, path, and point")}

    def IsActive(self):
        """Return True when this command should be available."""
        if Gui.activeDocument():
            return True
        else:
            return False


Gui.addCommand('Draft_ArrayTools', ArrayGroup())

## @}
