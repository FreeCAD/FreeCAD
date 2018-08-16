# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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

import Draft
import DraftVecUtils
import FreeCAD
import FreeCADGui
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathIconViewProvider as PathIconViewProvider

from PySide import QtCore, QtGui

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class ViewProvider:

    def __init__(self, vobj, name):
        PathLog.track(name)
        vobj.Proxy = self
        self.icon = name
        mode = 2
        #vobj.setEditorMode('BoundingBox', mode)
        #vobj.setEditorMode('DisplayMode', mode)
        #vobj.setEditorMode('Selectable', mode)
        #vobj.setEditorMode('ShapeColor', mode)
        #vobj.setEditorMode('Transparency', mode)

    def attach(self, vobj):
        PathLog.track()
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        PathLog.track()
        return ":/icons/Path-SetupSheet.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getDisplayMode(self, mode):
        return 'Default'

    def setEdit(self, vobj, mode=0):
        PathLog.track()
        taskPanel = TaskPanel(vobj)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(taskPanel)
        taskPanel.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def claimChildren(self):
        return []

    def doubleClicked(self, vobj):
        self.setEdit(vobj)

class TaskPanel:
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataKey = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        PathLog.track(self.obj.Label)
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/SetupGlobal.ui")
        FreeCAD.ActiveDocument.openTransaction(translate("Path_SetupSheet", "Edit SetupSheet"))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        #FreeCADGui.Selection.removeObserver(self.s)
        #FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        def updateExpression(name, widget):
            value = str(widget.text())
            val = PathGui.getProperty(self.obj, name)
            if val != value:
                PathGui.setProperty(self.obj, name, value)

        updateExpression('StartDepthExpression',        self.form.startDepthExpr)
        updateExpression('FinalDepthExpression',        self.form.finalDepthExpr)
        updateExpression('StepDownExpression',          self.form.stepDownExpr)
        updateExpression('ClearanceHeightExpression',   self.form.clearanceHeightExpr)
        updateExpression('SafeHeightExpression',        self.form.safeHeightExpr)
        self.clearanceHeightOffs.updateProperty()
        self.safeHeightOffs.updateProperty()
        self.rapidVertical.updateProperty()
        self.rapidHorizontal.updateProperty()

    def updateUI(self):
        self.form.startDepthExpr.setText(       self.obj.StartDepthExpression)
        self.form.finalDepthExpr.setText(       self.obj.FinalDepthExpression)
        self.form.stepDownExpr.setText(         self.obj.StepDownExpression)
        self.form.clearanceHeightExpr.setText(  self.obj.ClearanceHeightExpression)
        self.form.safeHeightExpr.setText(       self.obj.SafeHeightExpression)
        self.clearanceHeightOffs.updateSpinBox()
        self.safeHeightOffs.updateSpinBox()
        self.rapidVertical.updateSpinBox()
        self.rapidHorizontal.updateSpinBox()

    def updateModel(self):
        self.getFields()
        self.updateUI()
        FreeCAD.ActiveDocument.recompute()

    def setupCombo(self, combo, text, items):
        if items and len(items) > 0:
            for i in range(combo.count(), -1, -1):
                combo.removeItem(i)
            combo.addItems(items)
        index = combo.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.setCurrentIndex(index)

    def setFields(self):
        self.updateUI()

    def setupUi(self):
        self.clearanceHeightOffs = PathGui.QuantitySpinBox(self.form.clearanceHeightOffs, self.obj, 'ClearanceHeightOffset')
        self.safeHeightOffs = PathGui.QuantitySpinBox(self.form.safeHeightOffs, self.obj, 'SafeHeightOffset')
        self.rapidHorizontal = PathGui.QuantitySpinBox(self.form.rapidHorizontal, self.obj, 'HorizRapid')
        self.rapidVertical = PathGui.QuantitySpinBox(self.form.rapidVertical, self.obj, 'VertRapid')
        self.setFields()

def Create(name = 'SetupSheet'):
    '''Create(name = 'SetupSheet') ... creates a new setup sheet'''
    FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
    ssheet = SetupSheet.Create(name)
    PathIconViewProvider.Attach(ssheet)
    return ssheet

PathIconViewProvider.RegisterViewProvider('SetupSheet', ViewProvider)
