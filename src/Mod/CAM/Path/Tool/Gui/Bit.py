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

import os
import pathlib
from typing import Mapping, Optional
from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.IconViewProvider as PathIconViewProvider
import Path.Tool.Gui.BitEdit as PathToolBitEdit
from Path.Tool import ToolBitFactory


__title__ = "Tool Bit UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Task panel editor for a ToolBit"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ViewProvider(object):
    """ViewProvider for a ToolBit.
    It's sole job is to provide an icon and invoke the TaskPanel on edit."""

    def __init__(self, vobj, name):
        Path.Log.track(name, vobj.Object)
        self.panel = None
        self.icon = name
        self.obj = vobj.Object
        self.vobj = vobj
        vobj.Proxy = self

    def attach(self, vobj):
        Path.Log.track(vobj.Object)
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        png = self.obj.Proxy.getBitThumbnail(self.obj)
        if png:
            pixmap = QtGui.QPixmap()
            pixmap.loadFromData(png, "PNG")
            return QtGui.QIcon(pixmap)
        return ":/icons/CAM_ToolBit.svg"

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, vobj, arg2=None):
        Path.Log.track(vobj.Object.Label)
        vobj.Object.Proxy.onDelete(vobj.Object)

    def getDisplayMode(self, mode):
        return "Default"

    def _openTaskPanel(self, vobj, deleteOnReject):
        Path.Log.track()
        self.panel = TaskPanel(vobj, deleteOnReject)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.panel)
        self.panel.setupUi()

    def setCreate(self, vobj):
        Path.Log.track()
        self._openTaskPanel(vobj, True)

    def setEdit(self, vobj, mode=0):
        self._openTaskPanel(vobj, False)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        self.panel = None
        return

    def claimChildren(self):
        if self.obj.BitBody:
            return [self.obj.BitBody]
        return []

    def doubleClicked(self, vobj):
        pass

    def setupContextMenu(self, vobj, menu):
        # Override the base class method to prevent adding the "Edit" action
        # for ToolBit objects.
        pass  # TODO: call setEdit here once we have a new editor panel


class TaskPanel:
    """TaskPanel for the SetupSheet - if it is being edited directly."""

    def __init__(self, vobj, deleteOnReject):
        Path.Log.track(vobj.Object.Label)
        self.vobj = vobj
        self.obj = vobj.Object
        self.editor = PathToolBitEdit.ToolBitEditor(self.obj)
        self.form = self.editor.form
        self.deleteOnReject = deleteOnReject
        FreeCAD.ActiveDocument.openTransaction("Edit ToolBit")

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.editor.reject()
        FreeCADGui.Control.closeDialog()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction("Uncreate ToolBit")
            self.editor.reject()
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.editor.accept()

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def updateUI(self):
        Path.Log.track()
        self.editor.updateUI()

    def updateModel(self):
        self.editor.updateTool()
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
        self.editor.setupUI()


class ToolBitGuiFactory(Path.Tool.toolbit.base.ToolBitFactory):
    def create_bit_from_dict(self, attrs: Mapping, filepath: Optional[pathlib.Path]=None):
        """
        Creates a new tool bit from attributes.
        This method is overridden to attach the ViewProvider.
        """
        Path.Log.track(attrs)

        # Use the base class factory to create the tool bit object
        tool = super().create_bit_from_dict(attrs, filepath)
        if not tool:
            raise Exception("failed to create tool from {attrs} ({filepath})")

        # Attach the ViewProvider if the tool object has a ViewObject
        if hasattr(tool, "ViewObject") and tool.ViewObject:
            PathIconViewProvider.Attach(tool.ViewObject, tool.Label)
        else:
            FreeCAD.Console.PrintWarning(
                f"WARNING: ViewObject not available for tool {tool.Label}. "
                "ViewProvider not attached.\n"
            )

        return tool

    def create_bit(self, path: str, shape_path=None):
        """
        Creates a new tool bit.
        It is assumed the tool will be edited immediately so the internal bit
        body is still attached.
        """
        Path.Log.track(path, shape_path)
        return super().create_bit(path, shape_path)


def isValidFileName(filename):
    print(filename)
    try:
        with open(filename, "w"):
            return True
    except Exception:
        return False


def GetNewToolFile(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()

    bitdir = Path.Preferences.getToolBitPath()
    bitfile = QtGui.QFileDialog.getSaveFileName(
        parent, translate("CAM_Toolbit", "Tool"), str(bitdir), "*.fctb"
    )
    if bitfile and bitfile[0]:
        if not isValidFileName(bitfile[0]):
            msgBox = QtGui.QMessageBox()
            msg = translate("CAM_Toolbit", "Failed to open file for writing")
            msgBox.setText(msg)
            msgBox.exec_()
        else:
            return bitfile[0]
    return None


def GetToolFile(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()

    bitdir = Path.Preferences.getToolBitPath()
    bitfile = QtGui.QFileDialog.getOpenFileName(
        parent, "Tool", str(bitdir), "*.fctb"
    )
    if bitfile and bitfile[0]:
        return bitfile[0]
    return None


def GetToolFiles(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()
    bitdir = Path.Preferences.getToolBitPath()
    foo = QtGui.QFileDialog.getOpenFileNames(
        parent, "Tool", str(bitdir), "*.fctb"
    )
    if foo and foo[0]:
        return foo[0]
    return []


def LoadTool(parent=None):
    """
    LoadTool(parent=None) ... Open a file dialog to load a tool from a file.
    """
    foo = GetToolFile(parent)
    return ToolBitFactory.create_bit_from_file(foo) if foo else foo


def LoadTools(parent=None):
    """
    LoadTool(parent=None) ... Open a file dialog to load a tool from a file.
    """
    return [ToolBitFactory.create_bit_from_file(foo) for foo in GetToolFiles(parent)]


# Set the factory so all tools are created with UI
Path.Tool.toolbit.base.Factory = ToolBitGuiFactory()

PathIconViewProvider.RegisterViewProvider("ToolBit", ViewProvider)
