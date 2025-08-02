# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "Toolbit Dock"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "Toggles the toolbit dock"),
            "Accel": "P, T",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return True

    def Activated(self):
        dock = ToolBitLibraryDock()
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
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitLibraryOpen", "Toolbit Library Editor"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolBitLibraryOpen", "Opens an editor to manage toolbit libraries"
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return True

    def Activated(self):
        library = LibraryEditor()
        library.open()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_ToolBitLibraryOpen", CommandLibraryEditorOpen())
    FreeCADGui.addCommand("CAM_ToolBitDock", CommandToolBitLibraryDockOpen())

BarList = ["CAM_ToolBitDock"]
MenuList = ["CAM_ToolBitLibraryOpen", "CAM_ToolBitDock"]

FreeCAD.Console.PrintLog("Loading PathToolBitLibraryCmd… done\n")
