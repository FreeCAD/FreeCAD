# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitEdit as PathToolBitEdit
import os

from PySide import QtCore, QtGui

__title__ = "Tool Bit UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Task panel editor for a ToolBit"

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

#PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

class ViewProvider(object):
    '''ViewProvider for a ToolBit.
    It's sole job is to provide an icon and invoke the TaskPanel on edit.'''

    def __init__(self, vobj, name):
        PathLog.track(name, vobj.Object)
        self.panel = None
        self.icon = name
        self.obj = vobj.Object
        self.vobj = vobj
        vobj.Proxy = self

    def attach(self, vobj):
        PathLog.track(vobj.Object)
        self.vobj = vobj
        self.obj  = vobj.Object

    def getIcon(self):
        png = self.obj.Proxy.getBitThumbnail(self.obj)
        if png:
            pixmap = QtGui.QPixmap()
            pixmap.loadFromData(png, 'PNG')
            return QtGui.QIcon(pixmap)
        return ':/icons/Path-ToolBit.svg'

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        # pylint: disable=unused-argument
        return None

    def onDelete(self, vobj, arg2=None):
        PathLog.track(vobj.Object.Label)
        vobj.Object.Proxy.onDelete(vobj.Object)

    def getDisplayMode(self, mode):
        # pylint: disable=unused-argument
        return 'Default'

    def _openTaskPanel(self, vobj, deleteOnReject):
        PathLog.track()
        self.panel = TaskPanel(vobj, deleteOnReject)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.panel)
        self.panel.setupUi()

    def setCreate(self, vobj):
        PathLog.track()
        self._openTaskPanel(vobj, True)

    def setEdit(self, vobj, mode=0):
        # pylint: disable=unused-argument
        self._openTaskPanel(vobj, False)
        return True

    def unsetEdit(self, vobj, mode):
        # pylint: disable=unused-argument
        FreeCADGui.Control.closeDialog()
        self.panel = None
        return

    def claimChildren(self):
        if self.obj.BitBody:
            return [self.obj.BitBody]
        return []

    def doubleClicked(self, vobj):
        self.setEdit(vobj)

class TaskPanel:
    '''TaskPanel for the SetupSheet - if it is being edited directly.'''

    def __init__(self, vobj, deleteOnReject):
        PathLog.track(vobj.Object.Label)
        self.vobj = vobj
        self.obj = vobj.Object
        self.editor = PathToolBitEdit.ToolBitEditor(self.obj)
        self.form = self.editor.form
        self.deleteOnReject = deleteOnReject
        FreeCAD.ActiveDocument.openTransaction(translate('PathToolBit', 'Edit ToolBit'))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.editor.reject()
        FreeCADGui.Control.closeDialog()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate('PathToolBit', 'Uncreate ToolBit'))
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
        self.editor.updateUI()

    def updateModel(self):
        self.editor.updateTool()
        FreeCAD.ActiveDocument.recompute()

    def setupUi(self):
        self.editor.setupUI()


class ToolBitSelector(object):
    ToolRole = QtCore.Qt.UserRole + 1

    def __init__(self):
        self.buttons = None
        self.editor = None
        self.dialog = None
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/ToolBitSelector.ui')
        self.setupUI()

    def updateTools(self, selected=None):
        PathLog.track()
        selItem = None
        self.form.tools.setUpdatesEnabled(False)
        if selected is None and self.form.tools.currentItem():
            selected = self.form.tools.currentItem().text()
        self.form.tools.clear()
        for tool in sorted(self.loadedTools(), key=lambda t: t.Label):
            icon = None
            if tool.ViewObject and tool.ViewObject.Proxy:
                icon = tool.ViewObject.Proxy.getIcon()
            if icon and isinstance(icon, QtGui.QIcon):
                item = QtGui.QListWidgetItem(icon, tool.Label)
            else:
                item = QtGui.QListWidgetItem(tool.Label)
            item.setData(self.ToolRole, tool)
            if selected == tool.Label:
                selItem = item
            self.form.tools.addItem(item)
        if selItem:
            self.form.tools.setCurrentItem(selItem)
        self.updateSelection()
        self.form.tools.setUpdatesEnabled(True)

    def getTool(self):
        PathLog.track()
        self.updateTools()
        res = self.form.exec_()
        if 1 == res and self.form.tools.currentItem():
            return self.form.tools.currentItem().data(self.ToolRole)
        return None

    def loadedTools(self):
        PathLog.track()
        if FreeCAD.ActiveDocument:
            return [o for o in FreeCAD.ActiveDocument.Objects if hasattr(o, 'Proxy') and isinstance(o.Proxy, PathToolBit.ToolBit)]
        return []

    def loadTool(self):
        PathLog.track()
        tool = LoadTool(self.form)
        if tool:
            self.updateTools(tool.Label)

    def createTool(self):
        PathLog.track()
        tool = PathToolBit.Factory.Create()

        def accept():
            self.editor.accept()
            self.dialog.done(1)
            self.updateTools(tool.Label)

        def reject():
            FreeCAD.ActiveDocument.openTransaction(translate('PathToolBit', 'Uncreate ToolBit'))
            self.editor.reject()
            self.dialog.done(0)
            FreeCAD.ActiveDocument.removeObject(tool.Name)
            FreeCAD.ActiveDocument.commitTransaction()

        self.dialog = QtGui.QDialog(self.form)
        layout = QtGui.QVBoxLayout(self.dialog)
        self.editor = PathToolBitEdit.ToolBitEditor(tool, self.dialog)
        self.editor.setupUI()
        self.buttons = QtGui.QDialogButtonBox(
                QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel,
                QtCore.Qt.Horizontal, self.dialog)
        layout.addWidget(self.buttons)
        self.buttons.accepted.connect(accept)
        self.buttons.rejected.connect(reject)
        print(self.dialog.exec_())

    def updateSelection(self):
        PathLog.track()
        if self.form.tools.selectedItems():
            self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(True)
        else:
            self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok).setEnabled(False)

    def setupUI(self):
        PathLog.track()
        self.form.toolCreate.clicked.connect(self.createTool)
        self.form.toolLoad.clicked.connect(self.loadTool)
        self.form.tools.itemSelectionChanged.connect(self.updateSelection)
        self.form.tools.doubleClicked.connect(self.form.accept)

class ToolBitGuiFactory(PathToolBit.ToolBitFactory):

    def Create(self, name='ToolBit', shapeFile=None):
        '''Create(name = 'ToolBit') ... creates a new tool bit.
        It is assumed the tool will be edited immediately so the internal bit body is still attached.'''
        FreeCAD.ActiveDocument.openTransaction(translate('PathToolBit', 'Create ToolBit'))
        tool = PathToolBit.ToolBitFactory.Create(self, name, shapeFile)
        PathIconViewProvider.Attach(tool.ViewObject, name)
        FreeCAD.ActiveDocument.commitTransaction()
        return tool

def GetToolFile(parent = None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()
    foo = QtGui.QFileDialog.getOpenFileName(parent, 'Tool', PathPreferences.lastPathToolBit(), '*.fctb')
    if foo and foo[0]:
        PathPreferences.setLastPathToolBit(os.path.dirname(foo[0]))
        return foo[0]
    return None

def GetToolFiles(parent = None):
    if parent is None:
        parent = QtGui.QApplication.activeWindow()
    foo = QtGui.QFileDialog.getOpenFileNames(parent, 'Tool', PathPreferences.lastPathToolBit(), '*.fctb')
    if foo and foo[0]:
        PathPreferences.setLastPathToolBit(os.path.dirname(foo[0][0]))
        return foo[0]
    return []


def LoadTool(parent = None):
    '''LoadTool(parent=None) ... Open a file dialog to load a tool from a file.'''
    foo = GetToolFile(parent)
    return PathToolBit.Factory.CreateFrom(foo) if foo else foo

def LoadTools(parent = None):
    '''LoadTool(parent=None) ... Open a file dialog to load a tool from a file.'''
    return [PathToolBit.Factory.CreateFrom(foo) for foo in GetToolFiles(parent)]

# Set the factory so all tools are created with UI
PathToolBit.Factory = ToolBitGuiFactory()

PathIconViewProvider.RegisterViewProvider('ToolBit', ViewProvider)
