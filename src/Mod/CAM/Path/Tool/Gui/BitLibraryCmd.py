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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class CommandToolBitSelectorOpen:
    """
    Command to toggle the ToolBitSelector Dock
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ToolTable",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "ToolBit Dock"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_ToolBitDock", "Toggle the Toolbit Dock"),
            "Accel": "P, T",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import Path.Tool.Gui.BitLibrary as PathToolBitLibraryGui

        dock = PathToolBitLibraryGui.ToolBitSelector()
        dock.open()


class CommandToolBitLibraryOpen:
    """
    Command to open ToolBitLibrary editor.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ToolTable",
            "MenuText": QT_TRANSLATE_NOOP(
                "CAM_ToolBitLibraryOpen", "ToolBit Library editor"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolBitLibraryOpen", "Open an editor to manage ToolBit libraries"
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import Path.Tool.Gui.BitLibrary as PathToolBitLibraryGui

        library = PathToolBitLibraryGui.ToolBitLibrary()

        library.open()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_ToolBitLibraryOpen", CommandToolBitLibraryOpen())
    FreeCADGui.addCommand("CAM_ToolBitDock", CommandToolBitSelectorOpen())

BarList = ["CAM_ToolBitDock"]
MenuList = ["CAM_ToolBitLibraryOpen", "CAM_ToolBitDock"]

FreeCAD.Console.PrintLog("Loading PathToolBitLibraryCmd... done\n")
