# ***************************************************************************
# *   Copyright (c) 2023 edi <edi271@a1.net>                                *
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
"""Provides several TechDraw GuiCommands to create vertexes."""

__title__ = "TechDrawTools.CommandVertexCreations"
__author__ = "edi"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2023/12/05"


from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui

import TechDrawTools
import TechDrawTools.TDToolsUtil as Utils


import TechDraw

class CommandVertexCreationGroup:
    '''Create a drop down toolbar/menubar for vertex creating tools'''
    def Activated(self, index):
        if index == 0:
            Gui.runCommand("TechDraw_ExtensionVertexAtIntersection")
        elif index == 1:
            Gui.runCommand("TechDraw_CommandAddOffsetVertex")

    def GetCommands(self):
        return("TechDraw_ExtensionVertexAtIntersection",
               "TechDraw_CommandAddOffsetVertex")

    def GetDefaultCommand(self):
        return 0

    def GetResources(self):
        return {'Pixmap':'TechDraw_ExtensionVertexAtIntersection'}

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return Utils.havePage() and Utils.haveView()
        else:
            return False

class CommandAddOffsetVertex:
    """Creates a vertex offset to a selected vertex."""

    def __init__(self):
        """Initialize variables for the command that must exist at all times."""
        pass

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {'Pixmap': 'actions/TechDraw_AddOffsetVertex.svg',
                'Accel': "",
                'MenuText': QT_TRANSLATE_NOOP("TechDraw_AddOffsetVertex", "Add an offset vertex"),
                'ToolTip': QT_TRANSLATE_NOOP("TechDraw_AddOffsetVertex", "Create an offset vertex<br>\
                - select one vertex<br>\
                - start the tool<br>\
                - enter offset values in panel")}

    def Activated(self):
        """Run the following code when the command is activated (button pressed)."""
        if Utils.getSelView() and Utils.getSelVertexes():
            view = Utils.getSelView()
            vertexes = Utils.getSelVertexes()
            self.ui = TechDrawTools.TaskAddOffsetVertex(view, vertexes[0])
            Gui.Control.showDialog(self.ui)

    def IsActive(self):
        """Return True when the command should be active or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            return Utils.havePage() and Utils.haveView()
        else:
            return False

Gui.addCommand('TechDraw_CommandVertexCreationGroup',CommandVertexCreationGroup())
Gui.addCommand('TechDraw_CommandAddOffsetVertex',CommandAddOffsetVertex())
