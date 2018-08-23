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
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathSetupSheetOpPrototype as PathSetupSheetOpPrototype
import PathScripts.PathUtil as PathUtil

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

class PropertyEditorDelegate(QtGui.QStyledItemDelegate):

    def paint(self, painter, option, index):
        #PathLog.track(index.column(), type(option))
        if False and 2 == index.column():
            PathLog.track(index.row(), index.data().toString())
            painter.drawText(option.rect, index.data().toString())
        else:
            QtGui.QStyledItemDelegate.paint(self, painter, option, index)

    def createEditor(self, parent, option, index):
        PathLog.track(index.row(), index.column())
        if 0 == index.column():
            return QtGui.QStyledItemDelegate.createEditor(self, parent, option, index)
        return None

    def setEditorData(self, widget, index):
        PathLog.track(index.row(), index.column())
        if 0 == index.column():
            QtGui.QStyledItemDelegate.setEditorData(self, widget, index)
            isset = widget.isChecked()
            index.model().item(index.row(), 1).setEnabled(isset)
            index.model().item(index.row(), 2).setEnabled(isset)
        print("setEditorData(%s)" % widget)

    def setModelData(self, widget, model, index):
        PathLog.track(index.row(), index.column())
        if 0 == index.column():
            return QtGui.QStyledItemDelegate.setModelData(self, widget, model, index)
	print("setModelData(%s)" % widget)

    def updateEditorGeometry(self, widget, option, index):
	#print("is this even called")
	if widget:
	    widget.setGeometry(option.rect)
	else:
	    print("so sad")

class OpTaskPanel:

    def __init__(self, obj, name, op):
        self.name = name
        self.prefix = PathSetupSheet.OpNamePrefix(name)
        self.obj = obj
        self.op = op
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/SetupOp.ui")
        self.form.setWindowTitle(self.name)
        self.props = sorted(op.properties())
        self.prototype = PathSetupSheetOpPrototype.OpPrototype()
        op.factory("OpPrototype.%s" % name, self.prototype)

    def updateData(self, topLeft, bottomRight):
        if 0 == topLeft.column():
            isset = self.model.item(topLeft.row(), 0).checkState() == QtCore.Qt.Checked
            self.model.item(topLeft.row(), 1).setEnabled(isset)
            self.model.item(topLeft.row(), 2).setEnabled(isset)


    def setupUi(self):
        PathLog.track()

        self.delegate = PropertyEditorDelegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props), 3, self.form)
        self.model.setHorizontalHeaderLabels(['Set', 'Property', 'Value'])

        for i,name in enumerate(self.props):
            prop = self.prototype.getProperty(name)
            isset = hasattr(self.obj, self.propertyName(name))
            if isset:
                prop.setValue(getattr(self.obj, self.propertyName(name)))

            self.model.setData(self.model.index(i, 0), isset, QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 1), name,  QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 2), prop,  QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 2), prop.toString(),  QtCore.Qt.DisplayRole)

            self.model.item(i, 0).setCheckable(True)
            self.model.item(i, 0).setText('')
            self.model.item(i, 1).setEditable(False)
            if not isset:
                self.model.item(i, 1).setEnabled(False)
                self.model.item(i, 2).setEnabled(False)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegate(self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)

    def propertyName(self, prop):
        return "%{}_%{}".format(self.prefix, prop)

class TaskPanel:
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataKey = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        PathLog.track(self.obj.Label)
        self.globalForm = FreeCADGui.PySideUic.loadUi(":/panels/SetupGlobal.ui")
        self.ops = [OpTaskPanel(self.obj, name, op) for name, op in PathUtil.keyValueIter(PathSetupSheet._RegisteredOps)]
        self.form = [self.globalForm] + [op.form for op in self.ops]
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

        updateExpression('StartDepthExpression',        self.globalForm.startDepthExpr)
        updateExpression('FinalDepthExpression',        self.globalForm.finalDepthExpr)
        updateExpression('StepDownExpression',          self.globalForm.stepDownExpr)
        updateExpression('ClearanceHeightExpression',   self.globalForm.clearanceHeightExpr)
        updateExpression('SafeHeightExpression',        self.globalForm.safeHeightExpr)
        self.clearanceHeightOffs.updateProperty()
        self.safeHeightOffs.updateProperty()
        self.rapidVertical.updateProperty()
        self.rapidHorizontal.updateProperty()

    def updateUI(self):
        self.globalForm.startDepthExpr.setText(       self.obj.StartDepthExpression)
        self.globalForm.finalDepthExpr.setText(       self.obj.FinalDepthExpression)
        self.globalForm.stepDownExpr.setText(         self.obj.StepDownExpression)
        self.globalForm.clearanceHeightExpr.setText(  self.obj.ClearanceHeightExpression)
        self.globalForm.safeHeightExpr.setText(       self.obj.SafeHeightExpression)
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
        self.clearanceHeightOffs = PathGui.QuantitySpinBox(self.globalForm.clearanceHeightOffs, self.obj, 'ClearanceHeightOffset')
        self.safeHeightOffs = PathGui.QuantitySpinBox(self.globalForm.safeHeightOffs, self.obj, 'SafeHeightOffset')
        self.rapidHorizontal = PathGui.QuantitySpinBox(self.globalForm.rapidHorizontal, self.obj, 'HorizRapid')
        self.rapidVertical = PathGui.QuantitySpinBox(self.globalForm.rapidVertical, self.obj, 'VertRapid')
        self.setFields()

        for op in self.ops:
            op.setupUi()

def Create(name = 'SetupSheet'):
    '''Create(name = 'SetupSheet') ... creates a new setup sheet'''
    FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
    ssheet = SetupSheet.Create(name)
    PathIconViewProvider.Attach(ssheet)
    return ssheet

PathIconViewProvider.RegisterViewProvider('SetupSheet', ViewProvider)
