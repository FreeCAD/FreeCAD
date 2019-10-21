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
import PathScripts.PathGui as PathGui
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitEdit as PathToolBitEdit
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

__title__ = "Tool Bit UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Task panel editor for a ToolBit"

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

class ViewProvider(object):
    '''ViewProvider for a ToolBit.
    It's sole job is to provide an icon and invoke the TaskPanel on edit.'''

    def __init__(self, vobj, name):
        PathLog.track(name, vobj.Object)
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
            pixmap.loadFromData(png, "PNG")
            return QtGui.QIcon(pixmap)
        return ':/icons/Path-ToolBit.svg'

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        # pylint: disable=unused-argument
        return None

    def getDisplayMode(self, mode):
        # pylint: disable=unused-argument
        return 'Default'

    def _openTaskPanel(self, vobj, deleteOnReject):
        PathLog.track()
        self.taskPanel = TaskPanel(vobj, deleteOnReject)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.taskPanel)
        self.taskPanel.setupUi()

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
        self.taskPanel = None
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
        FreeCAD.ActiveDocument.openTransaction(translate("PathToolBit", "Edit ToolBit"))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.editor.reject()
        FreeCADGui.Control.closeDialog()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate("PathToolBit", "Uncreate ToolBit"))
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

    def getFields(self):
        self.editor.getFields()

    def updateUI(self):
        self.editor.updateUI()

    def updateModel(self):
        self.editor.updateTool()
        FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.editor.setFields()

    def setupUi(self):
        self.editor.setupUI()

def Create(name = 'ToolBit'):
    '''Create(name = 'ToolBit') ... creates a new tool bit.
    It is assumed the tool will be edited immediately so the internal bit body is still attached.'''
    FreeCAD.ActiveDocument.openTransaction(translate("PathToolBit", "Create ToolBit"))
    tool = PathToolBit.Create(name)
    PathIconViewProvider.Attach(tool.ViewObject, name)
    FreeCAD.ActiveDocument.commitTransaction()
    return tool

def CreateFrom(path, name = 'ToolBit'):
    '''CreateFrom(path, name = 'ToolBit') ... creates an instance of a tool stored in path'''
    FreeCAD.ActiveDocument.openTransaction(translate('PathToolBit', 'Create ToolBit instance'))
    tool = PathToolBit.CreateFrom(path, name)
    PathIconViewProvider.Attach(tool.ViewObject, name)
    FreeCAD.ActiveDocument.commitTransaction()
    return tool

PathIconViewProvider.RegisterViewProvider('ToolBit', ViewProvider)
