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
"""Provide the Draft ArrayTools command to group the other array tools."""
## @package gui_arrays
# \ingroup DRAFT
# \brief Provide the Draft ArrayTools command to group the other array tools.
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftguitools.gui_circulararray
import draftguitools.gui_polararray
import draftguitools.gui_orthoarray

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False
True if draftguitools.gui_circulararray.__name__ else False
True if draftguitools.gui_polararray.__name__ else False
True if draftguitools.gui_orthoarray.__name__ else False


class ArrayGroupCommand:
    """Gui command for the group of array tools."""

    def GetCommands(self):
        """Tuple of array commands."""
        return ("Draft_OrthoArray",
                "Draft_PolarArray", "Draft_CircularArray",
                "Draft_PathArray", "Draft_PathLinkArray",
                "Draft_PointArray")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tooltip = ("Create various types of arrays, "
                    "including rectangular, polar, circular, "
                    "path, and point")

        return {'Pixmap': 'Draft_Array',
                'MenuText': QT_TRANSLATE_NOOP("Draft", "Array tools"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft", _tooltip)}

    def IsActive(self):
        """Return True when this command should be available."""
        if App.activeDocument():
            return True
        else:
            return False


Gui.addCommand('Draft_ArrayTools', ArrayGroupCommand())
