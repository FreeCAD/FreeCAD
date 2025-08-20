# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                 2025 Samuel Abels <knipknap@gmail.com>                  *
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
from PySide.QtCore import QT_TRANSLATE_NOOP
from ...toolbit import ToolBit
from ...assets.ui import AssetSaveDialog
from ..serializers import all_serializers as toolbit_serializers
from .file import ToolBitOpenDialog

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
            "Pixmap": "CAM_ToolBit",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitCreate", "New Tool"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_ToolBitCreate", "Creates a new toolbit object"),
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        # Create a default endmill tool bit and attach it to a new DocumentObject
        toolbit = ToolBit.from_shape_id("endmill.fcstd")
        obj = toolbit.attach_to_doc(FreeCAD.ActiveDocument)
        obj.ViewObject.Proxy.setCreate(obj.ViewObject)


class CommandToolBitSave:
    """
    Command used to save an existing Tool to a file.
    """

    def __init__(self, saveAs):
        self.saveAs = saveAs

    def GetResources(self):
        if self.saveAs:
            menuTxt = QT_TRANSLATE_NOOP("CAM_ToolBitSaveAs", "Save Tool As…")
        else:
            menuTxt = QT_TRANSLATE_NOOP("CAM_ToolBitSave", "Save Tool")
        return {
            "Pixmap": "CAM_ToolBit",
            "MenuText": menuTxt,
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolBitSave", "Saves an existing toolbit object to a file"
            ),
        }

    def selectedTool(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if 1 == len(sel) and isinstance(sel[0].Object.Proxy, Path.Tool.ToolBit):
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
        tool_obj = self.selectedTool()
        if not tool_obj:
            return
        toolbit = tool_obj.Proxy

        dialog = AssetSaveDialog(ToolBit, toolbit_serializers, FreeCADGui.getMainWindow())
        dialog_result = dialog.exec_(toolbit)
        if not dialog_result:
            return


class CommandToolBitLoad:
    """
    Command used to load an existing Tool from a file into the current document.
    """

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "CAM_ToolBit",
            "MenuText": QT_TRANSLATE_NOOP("CAM_ToolBitLoad", "Load Tool"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_ToolBitLoad", "Loads an existing toolbit object from a file"
            ),
        }

    def selectedTool(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        if 1 == len(sel) and isinstance(sel[0].Object.Proxy, Path.Tool.ToolBit):
            return sel[0].Object
        return None

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        dialog = ToolBitOpenDialog(toolbit_serializers, FreeCADGui.getMainWindow())
        toolbits = dialog.exec()
        for toolbit in toolbits:
            toolbit.attach_to_doc(FreeCAD.ActiveDocument)
        if toolbits:
            FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_ToolBitCreate", CommandToolBitCreate())
    FreeCADGui.addCommand("CAM_ToolBitLoad", CommandToolBitLoad())
    FreeCADGui.addCommand("CAM_ToolBitSave", CommandToolBitSave(False))
    FreeCADGui.addCommand("CAM_ToolBitSaveAs", CommandToolBitSave(True))

CommandList = [
    "CAM_ToolBitCreate",
    "CAM_ToolBitLoad",
    "CAM_ToolBitSave",
    "CAM_ToolBitSaveAs",
]
