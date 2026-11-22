# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2019 sliptonic
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
from Path.Tool.library.ui.dock import ToolBitLibraryDock
from Path.Tool.library.ui.editor import LibraryEditor

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class CommandToolBitLibraryDockOpen:
    """
    Command to toggle the ToolBitLibraryDock
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ToolTable",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "Add Toolbit…"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "Opens the toolbit selection dialog"),
            "Accel": "P, T",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return True

    def Activated(self):
        dock = ToolBitLibraryDock(askToolNumber=True)
        dock.open()


class CommandLibraryEditorOpen:
    """
    Command to open ToolBitLibrary editor.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ToolTable",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitLibraryOpen", "Toolbit Library Manager"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolBitLibraryOpen", "Opens an editor to manage toolbit libraries"
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return True

    def Activated(self):
        library = LibraryEditor(parent=FreeCADGui.getMainWindow())
        library.open()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_ToolBitLibraryOpen", CommandLibraryEditorOpen())
    FreeCADGui.addCommand("CAM_ToolBitDock", CommandToolBitLibraryDockOpen())

BarList = ["CAM_ToolBitDock"]
MenuList = ["CAM_ToolBitLibraryOpen", "CAM_ToolBitDock"]

FreeCAD.Console.PrintLog("Loading PathToolBitLibraryCmd… done\n")
