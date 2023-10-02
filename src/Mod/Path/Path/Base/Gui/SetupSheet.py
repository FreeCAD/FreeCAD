# -*- coding: utf-8 -*-
# ***************************************************************************
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

import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.IconViewProvider as PathIconViewProvider
import Path.Base.Gui.SetupSheetOpPrototype as PathSetupSheetOpPrototypeGui
import Path.Base.Gui.Util as PathGuiUtil
import Path.Base.SetupSheet as PathSetupSheet
import Path.Base.Util as PathUtil
import PathGui

from PySide import QtCore, QtGui

__title__ = "Setup Sheet Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Task panel editor for a SetupSheet"


LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ViewProvider:
    """ViewProvider for a SetupSheet.
    It's sole job is to provide an icon and invoke the TaskPanel on edit."""

    def __init__(self, vobj, name):
        Path.Log.track(name)
        vobj.Proxy = self
        self.icon = name
        # mode = 2
        self.obj = None
        self.vobj = None

    def attach(self, vobj):
        Path.Log.track()
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        return ":/icons/Path_SetupSheet.svg"

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getDisplayMode(self, mode):
        return "Default"

    def setEdit(self, vobj, mode=0):
        Path.Log.track()
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


class Delegate(QtGui.QStyledItemDelegate):
    PropertyRole = QtCore.Qt.UserRole + 1
    EditorRole = QtCore.Qt.UserRole + 2

    def createEditor(self, parent, option, index):
        if index.data(self.EditorRole) is None:
            editor = PathSetupSheetOpPrototypeGui.Editor(index.data(self.PropertyRole))
            index.model().setData(index, editor, self.EditorRole)
        return index.data(self.EditorRole).widget(parent)

    def setEditorData(self, widget, index):
        Path.Log.track(index.row(), index.column())
        index.data(self.EditorRole).setEditorData(widget)

    def setModelData(self, widget, model, index):
        Path.Log.track(index.row(), index.column())
        editor = index.data(self.EditorRole)
        editor.setModelData(widget)
        index.model().setData(index, editor.prop.displayString(), QtCore.Qt.DisplayRole)

    def updateEditorGeometry(self, widget, option, index):
        widget.setGeometry(option.rect)


class OpTaskPanel:
    """Editor for an operation's property default values.
    The implementation is a simplified generic property editor with basically 3 fields
     - checkbox - if set a default value for the given property is set
     - name     - a non-editable string with the property name
     - value    - the actual editor for the property's default value
    The specific editor classes for a given property type are implemented in
    PathSetupSheetOpPrototypeGui which also provides a factory function. The properties
    are displayed in a table, each field occypying a column and each row representing
    a single property."""

    def __init__(self, obj, name, op):
        self.name = name
        self.obj = obj
        self.op = op
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/SetupOp.ui")
        self.form.setWindowTitle("Op {}".format(self.name))
        self.props = sorted(op.properties())
        self.prototype = op.prototype(name)

        # initialized later
        self.delegate = None
        self.model = None

    def updateData(self, topLeft, bottomRight):
        if 0 == topLeft.column():
            isset = self.model.item(topLeft.row(), 0).checkState() == QtCore.Qt.Checked
            self.model.item(topLeft.row(), 1).setEnabled(isset)
            self.model.item(topLeft.row(), 2).setEnabled(isset)

    def setupUi(self):
        Path.Log.track()

        self.delegate = Delegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props), 3, self.form)
        self.model.setHorizontalHeaderLabels(["Set", "Property", "Value"])

        for i, name in enumerate(self.props):
            prop = self.prototype.getProperty(name)
            isset = hasattr(self.obj, self.propertyName(name))
            if isset:
                prop.setValue(getattr(self.obj, self.propertyName(name)))

            self.model.setData(self.model.index(i, 0), isset, QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 1), name, QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 2), prop, Delegate.PropertyRole)
            self.model.setData(
                self.model.index(i, 2), prop.displayString(), QtCore.Qt.DisplayRole
            )

            self.model.item(i, 0).setCheckable(True)
            self.model.item(i, 0).setText("")
            self.model.item(i, 1).setEditable(False)
            self.model.item(i, 1).setToolTip(prop.info)
            self.model.item(i, 2).setToolTip(prop.info)

            if isset:
                self.model.item(i, 0).setCheckState(QtCore.Qt.Checked)
            else:
                self.model.item(i, 0).setCheckState(QtCore.Qt.Unchecked)
                self.model.item(i, 1).setEnabled(False)
                self.model.item(i, 2).setEnabled(False)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegateForColumn(2, self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)

    def propertyName(self, prop):
        return PathSetupSheet.OpPropertyName(self.name, prop)

    def propertyGroup(self):
        return PathSetupSheet.OpPropertyGroup(self.name)

    def accept(self):
        propertiesCreatedRemoved = False
        for i, name in enumerate(self.props):
            prop = self.prototype.getProperty(name)
            propName = self.propertyName(name)
            enabled = self.model.item(i, 0).checkState() == QtCore.Qt.Checked
            if enabled and not prop.getValue() is None:
                if prop.setupProperty(
                    self.obj, propName, self.propertyGroup(), prop.getValue()
                ):
                    propertiesCreatedRemoved = True
            else:
                if hasattr(self.obj, propName):
                    self.obj.removeProperty(propName)
                    propertiesCreatedRemoved = True
        return propertiesCreatedRemoved


class OpsDefaultEditor:
    """Class to collect and display default property editors for all registered operations.
    If a form is given at creation time it will integrate with that form and provide an interface to switch
    between the editors of different operations. If no form is provided the class assumes that the UI is
    taken care of somehow else and just serves as an interface to all operation editors."""

    def __init__(self, obj, form):
        self.form = form
        self.obj = obj
        self.ops = sorted(
            [
                OpTaskPanel(self.obj, name, op)
                for name, op in PathSetupSheet._RegisteredOps.items()
            ],
            key=lambda op: op.name,
        )
        if form:
            parent = form.tabOpDefaults
            for op in self.ops:
                form.opDefaultOp.addItem(op.form.windowTitle(), op)
                op.form.setParent(parent)
                parent.layout().addWidget(op.form)
                op.form.hide()
        self.currentOp = None

    def reject(self):
        pass

    def accept(self):
        if any([op.accept() for op in self.ops]):
            Path.Log.track()

    def getFields(self):
        pass

    def updateUI(self):
        if self.currentOp:
            self.currentOp.form.hide()
            self.currentOp = None
        if self.form:
            current = self.form.opDefaultOp.currentIndex()
            if current < 0:
                current = 0
            self.currentOp = self.form.opDefaultOp.itemData(current)
            self.currentOp.form.show()

    def updateModel(self, recomp=True):
        Path.Log.track()
        self.getFields()
        self.updateUI()
        if recomp:
            FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.updateUI()

    def setupUi(self):
        for op in self.ops:
            op.setupUi()
        self.updateUI()
        if self.form:
            self.form.opDefaultOp.currentIndexChanged.connect(self.updateUI)


class GlobalEditor(object):
    """Editor for the global properties which affect almost every operation."""

    def __init__(self, obj, form):
        self.form = form
        self.obj = obj

        # initialized later
        self.clearanceHeightOffs = None
        self.safeHeightOffs = None
        self.rapidHorizontal = None
        self.rapidVertical = None

    def reject(self):
        pass

    def accept(self):
        self.getFields()

    def getFields(self):
        def updateExpression(name, widget):
            value = str(widget.text())
            val = PathUtil.getProperty(self.obj, name)
            if val != value:
                PathUtil.setProperty(self.obj, name, value)

        updateExpression("StartDepthExpression", self.form.setupStartDepthExpr)
        updateExpression("FinalDepthExpression", self.form.setupFinalDepthExpr)
        updateExpression("StepDownExpression", self.form.setupStepDownExpr)
        updateExpression(
            "ClearanceHeightExpression", self.form.setupClearanceHeightExpr
        )
        updateExpression("SafeHeightExpression", self.form.setupSafeHeightExpr)
        self.clearanceHeightOffs.updateProperty()
        self.safeHeightOffs.updateProperty()
        self.rapidVertical.updateProperty()
        self.rapidHorizontal.updateProperty()
        self.obj.CoolantMode = self.form.setupCoolantMode.currentText()

    def selectInComboBox(self, name, combo):
        """selectInComboBox(name, combo) ... helper function to select a specific value in a combo box."""
        index = combo.findText(name, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.blockSignals(True)
            combo.setCurrentIndex(index)
            combo.blockSignals(False)

    def updateUI(self):
        Path.Log.track()
        self.form.setupStartDepthExpr.setText(self.obj.StartDepthExpression)
        self.form.setupFinalDepthExpr.setText(self.obj.FinalDepthExpression)
        self.form.setupStepDownExpr.setText(self.obj.StepDownExpression)
        self.form.setupClearanceHeightExpr.setText(self.obj.ClearanceHeightExpression)
        self.form.setupSafeHeightExpr.setText(self.obj.SafeHeightExpression)
        self.clearanceHeightOffs.updateSpinBox()
        self.safeHeightOffs.updateSpinBox()
        self.rapidVertical.updateSpinBox()
        self.rapidHorizontal.updateSpinBox()
        self.selectInComboBox(self.obj.CoolantMode, self.form.setupCoolantMode)

    def updateModel(self, recomp=True):
        Path.Log.track()
        self.getFields()
        self.updateUI()
        if recomp:
            FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.updateUI()

    def setupUi(self):
        self.clearanceHeightOffs = PathGuiUtil.QuantitySpinBox(
            self.form.setupClearanceHeightOffs, self.obj, "ClearanceHeightOffset"
        )
        self.safeHeightOffs = PathGuiUtil.QuantitySpinBox(
            self.form.setupSafeHeightOffs, self.obj, "SafeHeightOffset"
        )
        self.rapidHorizontal = PathGuiUtil.QuantitySpinBox(
            self.form.setupRapidHorizontal, self.obj, "HorizRapid"
        )
        self.rapidVertical = PathGuiUtil.QuantitySpinBox(
            self.form.setupRapidVertical, self.obj, "VertRapid"
        )
        self.form.setupCoolantMode.addItems(self.obj.CoolantModes)
        self.setFields()


class TaskPanel:
    """TaskPanel for the SetupSheet - if it is being edited directly."""

    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        Path.Log.track(self.obj.Label)
        self.globalForm = FreeCADGui.PySideUic.loadUi(":/panels/SetupGlobal.ui")
        self.globalEditor = GlobalEditor(self.obj, self.globalForm)
        self.opsEditor = OpsDefaultEditor(self.obj, None)
        self.form = [op.form for op in self.opsEditor.ops] + [self.globalForm]
        FreeCAD.ActiveDocument.openTransaction("Edit SetupSheet")

    def reject(self):
        self.globalEditor.reject()
        self.opsEditor.reject()
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.globalEditor.accept()
        self.opsEditor.accept()

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.globalEditor.getFields()
        self.opsEditor.getFields()

    def updateUI(self):
        self.globalEditor.updateUI()
        self.opsEditor.updateUI()

    def updateModel(self):
        self.globalEditor.updateModel(False)
        self.opsEditor.updateModel(False)
        FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.globalEditor.setFields()
        self.opsEditor.setFields()

    def setupUi(self):
        self.globalEditor.setupUi()
        self.opsEditor.setupUi()


def Create(name="SetupSheet"):
    """Create(name='SetupSheet') ... creates a new setup sheet"""
    FreeCAD.ActiveDocument.openTransaction("Create Job")
    ssheet = PathSetupSheet.Create(name)
    PathIconViewProvider.Attach(ssheet, name)
    return ssheet


PathIconViewProvider.RegisterViewProvider("SetupSheet", ViewProvider)
