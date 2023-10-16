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

from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.IconViewProvider as PathIconViewProvider
import Path.Tool.Bit as PathToolBit
import Path.Tool.Gui.BitEdit as PathToolBitEdit
import os

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
        return ":/icons/Path_ToolBit.svg"

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
        if os.path.exists(vobj.Object.BitShape):
            self.setEdit(vobj)
        else:
            msg = translate(
                "PathToolBit", "Toolbit cannot be edited: Shapefile not found"
            )
            diag = QtGui.QMessageBox(QtGui.QMessageBox.Warning, "Error", msg)
            diag.setWindowModality(QtCore.Qt.ApplicationModal)
            diag.exec_()


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


class ToolBitGuiFactory(PathToolBit.ToolBitFactory):
    def Create(self, name="ToolBit", shapeFile=None, path=None):
        """Create(name = 'ToolBit') ... creates a new tool bit.
        It is assumed the tool will be edited immediately so the internal bit body is still attached."""

        Path.Log.track(name, shapeFile, path)
        FreeCAD.ActiveDocument.openTransaction("Create ToolBit")
        tool = PathToolBit.ToolBitFactory.Create(self, name, shapeFile, path)
        PathIconViewProvider.Attach(tool.ViewObject, name)
        FreeCAD.ActiveDocument.commitTransaction()
        return tool


def isValidFileName(filename):
    print(filename)
    try:
        with open(filename, "w") as tempfile:
            return True
    except Exception:
        return False


def GetNewToolFile(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()

    foo = QtGui.QFileDialog.getSaveFileName(
        parent, "Tool", Path.Preferences.lastPathToolBit(), "*.fctb"
    )
    if foo and foo[0]:
        if not isValidFileName(foo[0]):
            msgBox = QtGui.QMessageBox()
            msg = translate("Path", "Invalid Filename")
            msgBox.setText(msg)
            msgBox.exec_()
        else:
            Path.Preferences.setLastPathToolBit(os.path.dirname(foo[0]))
            return foo[0]
    return None


def GetToolFile(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()
    foo = QtGui.QFileDialog.getOpenFileName(
        parent, "Tool", Path.Preferences.lastPathToolBit(), "*.fctb"
    )
    if foo and foo[0]:
        Path.Preferences.setLastPathToolBit(os.path.dirname(foo[0]))
        return foo[0]
    return None


def GetToolFiles(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()
    foo = QtGui.QFileDialog.getOpenFileNames(
        parent, "Tool", Path.Preferences.lastPathToolBit(), "*.fctb"
    )
    if foo and foo[0]:
        Path.Preferences.setLastPathToolBit(os.path.dirname(foo[0][0]))
        return foo[0]
    return []


def GetToolShapeFile(parent=None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()

    location = Path.Preferences.lastPathToolShape()
    if os.path.isfile(location):
        location = os.path.split(location)[0]
    elif not os.path.isdir(location):
        location = Path.Preferences.filePath()

    fname = QtGui.QFileDialog.getOpenFileName(
        parent, "Select Tool Shape", location, "*.fcstd"
    )
    if fname and fname[0]:
        if fname != location:
            newloc = os.path.dirname(fname[0])
            Path.Preferences.setLastPathToolShape(newloc)
        return fname[0]
    else:
        return None


def LoadTool(parent=None):
    """
    LoadTool(parent=None) ... Open a file dialog to load a tool from a file.
    """
    foo = GetToolFile(parent)
    return PathToolBit.Factory.CreateFrom(foo) if foo else foo


def LoadTools(parent=None):
    """
    LoadTool(parent=None) ... Open a file dialog to load a tool from a file.
    """
    return [PathToolBit.Factory.CreateFrom(foo) for foo in GetToolFiles(parent)]


# Set the factory so all tools are created with UI
PathToolBit.Factory = ToolBitGuiFactory()

PathIconViewProvider.RegisterViewProvider("ToolBit", ViewProvider)
