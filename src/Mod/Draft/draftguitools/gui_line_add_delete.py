# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
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
"""Provides GUI tools to do certain add and remove line operations.

These GuiCommands aren't really used anymore, as the same actions
are implemented directly in the Draft_Edit command.
"""
## @package gui_line_add_delete
# \ingroup draftguitools
# \brief Provides GUI tools to do certain add and remove line operations.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import DraftTools
import draftutils.utils as utils

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class AddPoint(DraftTools.Modifier):
    """GuiCommand to add a point to a line being drawn."""

    def __init__(self):
        self.running = False

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_AddPoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_AddPoint", "Add point"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_AddPoint", "Adds a point to an existing Wire or B-spline.")}

    def IsActive(self):
        """Return True when there is selection and the command is active."""
        if Gui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        """Execute when the command is called."""
        selection = Gui.Selection.getSelection()
        if selection:
            if (utils.get_type(selection[0]) in ['Wire', 'BSpline']):
                Gui.runCommand("Draft_Edit")
                Gui.draftToolBar.vertUi(True)


Gui.addCommand('Draft_AddPoint', AddPoint())


class DelPoint(DraftTools.Modifier):
    """GuiCommand to delete a point to a line being drawn."""

    def __init__(self):
        self.running = False

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_DelPoint',
                'MenuText': QT_TRANSLATE_NOOP("Draft_DelPoint", "Remove point"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_DelPoint", "Removes a point from an existing Wire or B-spline.")}

    def IsActive(self):
        """Return True when there is selection and the command is active."""
        if Gui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        """Execute when the command is called."""
        selection = Gui.Selection.getSelection()
        if selection:
            if (utils.get_type(selection[0]) in ['Wire', 'BSpline']):
                Gui.runCommand("Draft_Edit")
                Gui.draftToolBar.vertUi(False)


Gui.addCommand('Draft_DelPoint', DelPoint())

## @}
