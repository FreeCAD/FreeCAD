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

import FreeCAD
import FreeCADGui
import Path
import Path.Tool
import os
from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CommandToolBitCreate:
    """
    Command used to create a new Tool.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Path_ToolBit",
            "MenuText": QT_TRANSLATE_NOOP("Path_ToolBitCreate", "Create Tool"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_ToolBitCreate", "Creates a new ToolBit object"
            ),
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        obj = Path.Tool.Bit.Factory.Create()
        obj.ViewObject.Proxy.setCreate(obj.ViewObject)


class CommandToolBitSave:
    """
    Command used to save an existing Tool to a file.
    """

    def __init__(self, saveAs):
        self.saveAs = saveAs

    def GetResources(self):
        if self.saveAs:
            menuTxt = QT_TRANSLATE_NOOP("Path_ToolBitSaveAs", "Save Tool as...")
        else:
            menuTxt = QT_TRANSLATE_NOOP("Path_ToolBitSave", "Save Tool")
        return {
            "Pixmap": "Path_ToolBit",
            "MenuText": menuTxt,
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_ToolBitSave", "Save an existing ToolBit object to a file"
            ),
        }

    def selectedTool(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if 1 == len(sel) and isinstance(sel[0].Object.Proxy, Path.Tool.Bit.ToolBit):
            return sel[0].Object
        return None

    def IsActive(self):
        tool = self.selectedTool()
        if tool:
            if tool.File:
                return True
            return self.saveAs
        return False

    def Activated(self):
        from PySide import QtGui

        tool = self.selectedTool()
        if tool:
            path = None
            if not tool.File or self.saveAs:
                if tool.File:
                    fname = tool.File
                else:
                    fname = os.path.join(
                        Path.Preferences.lastPathToolBit(),
                        tool.Label + ".fctb",
                    )
                foo = QtGui.QFileDialog.getSaveFileName(
                    QtGui.QApplication.activeWindow(), "Tool", fname, "*.fctb"
                )
                if foo:
                    path = foo[0]
            else:
                path = tool.File

            if path:
                if not path.endswith(".fctb"):
                    path += ".fctb"
                tool.Proxy.saveToFile(tool, path)
                Path.Preferences.setLastPathToolBit(os.path.dirname(path))


class CommandToolBitLoad:
    """
    Command used to load an existing Tool from a file into the current document.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Path_ToolBit",
            "MenuText": QT_TRANSLATE_NOOP("Path_ToolBitLoad", "Load Tool"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_ToolBitLoad", "Load an existing ToolBit object from a file"
            ),
        }

    def selectedTool(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if 1 == len(sel) and isinstance(sel[0].Object.Proxy, Path.Tool.Bit.ToolBit):
            return sel[0].Object
        return None

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        if Path.Tool.Bit.Gui.LoadTools():
            FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("Path_ToolBitCreate", CommandToolBitCreate())
    FreeCADGui.addCommand("Path_ToolBitLoad", CommandToolBitLoad())
    FreeCADGui.addCommand("Path_ToolBitSave", CommandToolBitSave(False))
    FreeCADGui.addCommand("Path_ToolBitSaveAs", CommandToolBitSave(True))

CommandList = [
    "Path_ToolBitCreate",
    "Path_ToolBitLoad",
    "Path_ToolBitSave",
    "Path_ToolBitSaveAs",
]

FreeCAD.Console.PrintLog("Loading PathToolBitCmd... done\n")
