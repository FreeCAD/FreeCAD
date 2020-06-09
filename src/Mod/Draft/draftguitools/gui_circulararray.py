# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provides the Draft CircularArray GuiCommand."""
## @package gui_circulararray
# \ingroup DRAFT
# \brief This module provides the Draft CircularArray tool.

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCADGui as Gui
import Draft_rc  # include resources, icons, ui files
from draftguitools import gui_base
from drafttaskpanels import task_circulararray

# The module is used to prevent complaints from code checkers (flake8)
bool(Draft_rc.__name__)


class CircularArray(gui_base.PolarCircularBase):
    """Gui command for the CircularArray tool.

    The Parent class PolarCircularBase sets up callbacks that
    allow the selection of edges and the object(s) to be duplicated.
    """
    def __init__(self):
        super(CircularArray, self).__init__()
        self.command_name = "Circular array"

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _tip = ("Creates copies of the selected object, "
                "and places the copies in a radial pattern\n"
                "creating various circular layers.\n"
                "\n"
                "The array can be turned into an orthogonal "
                "or a polar array by changing its type.")

        d = {'Pixmap': 'Draft_CircularArray',
             'MenuText': QT_TRANSLATE_NOOP("Draft", "Circular array"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft", _tip)}
        return d

    def Activated(self):
        """Execute when the command is called.

        We add callbacks that connect the 3D view with
        the widgets of the task panel.
        """
        self.ui = task_circulararray.TaskPanelCircularArray()
        # The calling class (this one) is saved in the object
        # of the interface, to be able to call a function from within it.
        super(CircularArray, self).Activated()


Gui.addCommand('Draft_CircularArray', CircularArray())
