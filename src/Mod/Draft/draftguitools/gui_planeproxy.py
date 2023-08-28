# ***************************************************************************
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides GUI tools to create WorkingPlaneProxy objects."""
## @package gui_planeproxy
# \ingroup draftguitools
# \brief Provides GUI tools to create WorkingPlaneProxy objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False

__title__ = "FreeCAD Draft Workbench GUI Tools - Working plane-related tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecad.org"


class Draft_WorkingPlaneProxy:
    """The Draft_WorkingPlaneProxy command definition."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        d = {'Pixmap': 'Draft_PlaneProxy',
             'MenuText': QT_TRANSLATE_NOOP("Draft_WorkingPlaneProxy","Create working plane proxy"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_WorkingPlaneProxy","Creates a proxy object from the current working plane.\nOnce the object is created double click it in the tree view to restore the camera position and objects' visibilities.\nThen you can use it to save a different camera position and objects' states any time you need.")}
        return d

    def IsActive(self):
        """Return True when this command should be available."""
        if Gui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):
        """Execute when the command is called."""
        if hasattr(App, "DraftWorkingPlane"):
            App.DraftWorkingPlane.setup()
            App.ActiveDocument.openTransaction("Create WP proxy")
            Gui.addModule("Draft")
            _cmd = "Draft.make_workingplaneproxy("
            _cmd += "FreeCAD.DraftWorkingPlane.getPlacement()"
            _cmd += ")"
            Gui.doCommand(_cmd)
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()


Gui.addCommand('Draft_WorkingPlaneProxy', Draft_WorkingPlaneProxy())

## @}
