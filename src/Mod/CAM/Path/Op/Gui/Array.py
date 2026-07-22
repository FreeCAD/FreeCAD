# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2015 Yorik van Havre <yorik@uncreated.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import FreeCADGui
import Path.Op.Array as Array
from Path.Base.Gui.Util import IntegerSpinBox, QuantitySpinBox, updateInputField
from Path.Base.Util import toolControllerForOp
from PathPythonGui.simple_edit_panel import SimpleEditPanel

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore, QtGui

__doc__ = "UI and Command for Array object"

translate = FreeCAD.Qt.translate


class ViewProviderArray:
    def __init__(self, vobj):
        self.obj = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        if prop == "Visibility" and not self.obj.Document.Restoring:
            for op in self.obj.ArrayGroup:
                op.Visibility = vobj.Visibility

    def claimChildren(self):
        return [base for base in self.obj.ArrayGroup]

    def onDelete(self, vobj, args):
        for op in self.obj.ArrayGroup:
            op.Document.removeObject(op.Name)
        self.obj.Document.removeObject(self.obj.Name)

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_ArrayMTC.svg"
        else:
            return ":/icons/CAM_OpActive.svg"

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
        elif mode == 0:
            FreeCADGui.Control.closeDialog()
            panel = ArrayTaskPanel(vobj.Object, self)
            FreeCADGui.Control.showDialog(panel)
        return True

    def unsetEdit(self, vobj, mode=0):
        if mode == 0 and self.panel:
            self.panel.abort()

    def clearTaskPanel(self):
        self.panel = None


class ViewProviderArrayChild:

    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        return

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_Array.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class ArrayTaskPanel(SimpleEditPanel):
    _transaction_name = "Edit Array"
    _ui_file = ":/panels/PageOpArray.ui"

    def setupUi(self):
        self.initPage()
        self.setFields()
        self.registerSignalHandlers()
        self.updateVisibility()
        self.pageRegisterSignalHandlers()
        self.updateBaseList()

    def initPage(self):
        self.connectWidget("ReverseDirection", self.form.chkReverse)
        self.connectWidget("SwapDirection", self.form.chkSwap)
        self.connectWidget("ExpandArray", self.form.chkExpand)
        self.connectWidget("Combine", self.form.chkCombine)
        self.connectWidget("UseJitter", self.form.chkUseJitter)
        self.connectWidget("Type", self.form.cboType)

        self.copies = IntegerSpinBox(self.form.dspCopies, self.obj, "Copies")
        self.copiesX = IntegerSpinBox(self.form.dspCopiesX, self.obj, "CopiesX")
        self.copiesY = IntegerSpinBox(self.form.dspCopiesY, self.obj, "CopiesY")
        self.offsetX = QuantitySpinBox(self.form.dspOffsetX, self.obj, "Offset.x")
        self.offsetY = QuantitySpinBox(self.form.dspOffsetY, self.obj, "Offset.y")

        self.polarAngle = QuantitySpinBox(self.form.dspPolarAngle, self.obj, "Angle")
        self.polarCenterX = QuantitySpinBox(self.form.dspPolarCenterX, self.obj, "Centre.x")
        self.polarCenterY = QuantitySpinBox(self.form.dspPolarCenterY, self.obj, "Centre.y")

        self.jitterAngle = QuantitySpinBox(self.form.dspJitterAngle, self.obj, "JitterAngle")
        self.jitterSeed = IntegerSpinBox(self.form.dspJitterSeed, self.obj, "JitterSeed")
        self.jitterX = QuantitySpinBox(self.form.dspJitterX, self.obj, "JitterMagnitude.x")
        self.jitterY = QuantitySpinBox(self.form.dspJitterY, self.obj, "JitterMagnitude.y")

        self.updateSpinBoxes()

    def updateSpinBoxes(self, index=None):
        self.copies.updateWidget()
        self.copiesX.updateWidget()
        self.copiesY.updateWidget()
        self.offsetX.updateWidget()
        self.offsetY.updateWidget()

        self.polarAngle.updateWidget()
        self.polarCenterX.updateWidget()
        self.polarCenterY.updateWidget()

        self.jitterAngle.updateWidget()
        self.jitterSeed.updateWidget()
        self.jitterX.updateWidget()
        self.jitterY.updateWidget()

    def registerSignalHandlers(self):
        self.form.chkUseJitter.clicked.connect(self.updateVisibility)
        self.form.cboType.currentIndexChanged.connect(self.updateVisibility)
        self.form.baseList.clicked.connect(self.updateButtonsVisibility)
        self.form.pbClear.clicked.connect(self.clearBaseList)
        self.form.pbRemove.clicked.connect(self.RemoveFromBaseList)
        self.form.pbAdd.clicked.connect(self.addToBaseList)
        self.form.pbUp.clicked.connect(self.upInBaseList)
        self.form.pbDown.clicked.connect(self.downInBaseList)

    def getSignalsForUpdate(self):
        signals = []
        signals.append(self.form.dspCopies.editingFinished)
        signals.append(self.form.dspCopiesX.editingFinished)
        signals.append(self.form.dspCopiesY.editingFinished)
        signals.append(self.form.dspOffsetX.editingFinished)
        signals.append(self.form.dspOffsetY.editingFinished)
        signals.append(self.form.dspPolarAngle.editingFinished)
        signals.append(self.form.dspPolarCenterX.editingFinished)
        signals.append(self.form.dspPolarCenterY.editingFinished)
        signals.append(self.form.dspJitterAngle.editingFinished)
        signals.append(self.form.dspJitterSeed.editingFinished)
        signals.append(self.form.dspJitterX.editingFinished)
        signals.append(self.form.dspJitterY.editingFinished)
        return signals

    def pageGetFields(self):
        print("pageGetFields: from UI to obj")
        self.copies.updateProperty()
        self.copiesX.updateProperty()
        self.copiesY.updateProperty()
        self.offsetX.updateProperty()
        self.offsetY.updateProperty()

        self.polarAngle.updateProperty()
        self.polarCenterX.updateProperty()
        self.polarCenterY.updateProperty()

        self.jitterAngle.updateProperty()
        self.jitterSeed.updateProperty()
        self.jitterX.updateProperty()
        self.jitterY.updateProperty()

        updateInputField(self.obj, "JitterMagnitude.x", self.form.dspJitterX)
        updateInputField(self.obj, "JitterMagnitude.y", self.form.dspJitterY)

    def pageRegisterSignalHandlers(self):
        for signal in self.getSignalsForUpdate():
            signal.connect(self.pageGetFields)

    def updateVisibility(self):
        print("updateVisibility", self.form.chkUseJitter.isChecked())
        if self.form.chkUseJitter.isChecked():
            self.form.groupJitter.show()
        else:
            self.form.groupJitter.hide()

        if self.form.cboType.currentText() == "Linear2D":
            self.form.groupCopies2D.show()
            self.form.chkSwap.show()
        else:
            self.form.groupCopies2D.hide()
            self.form.chkSwap.hide()

        if self.form.cboType.currentText() == "Polar":
            self.form.groupPolar.show()
            self.form.groupOffset.hide()
        else:
            self.form.groupPolar.hide()

        if self.form.cboType.currentText() in ("Linear1D", "Polar"):
            self.form.groupCopies1D.show()
        else:
            self.form.groupCopies1D.hide()

        if self.form.cboType.currentText() == "Points":
            self.form.groupCopies1D.hide()
            self.form.groupCopies2D.hide()
            self.form.groupOffset.hide()
            self.form.groupPolar.hide()

    def updateBaseList(self):
        print("updateBaseList")
        # Column indices for baseList table
        COL_ORDER = 0
        COL_OP_NAME = 1
        COL_OP_TC = 2
        COL_OP_COOLANT = 3
        self.form.baseList.blockSignals(True)
        self.form.baseList.clearContents()
        self.form.baseList.setRowCount(0)
        for i, op in enumerate(self.obj.Base):
            print("   ", i, op.Label)
            self.form.baseList.insertRow(self.form.baseList.rowCount())

            item = QtGui.QTableWidgetItem()
            item.setData(QtCore.Qt.DisplayRole, i + 1)
            self.form.baseList.setItem(i, COL_ORDER, item)

            item = QtGui.QTableWidgetItem(op.Label)
            self.form.baseList.setItem(i, COL_OP_NAME, item)

            item = QtGui.QTableWidgetItem(op.ToolController.Label)
            self.form.baseList.setItem(i, COL_OP_TC, item)

            item = QtGui.QTableWidgetItem(op.CoolantMode)
            self.form.baseList.setItem(i, COL_OP_COOLANT, item)

        header = self.form.baseList.horizontalHeader()
        header.setSectionResizeMode(COL_ORDER, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_NAME, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_TC, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_COOLANT, QtGui.QHeaderView.ResizeToContents)
        self.form.baseList.blockSignals(False)
        self.updateButtonsVisibility()

    def updateButtonsVisibility(self):
        print("baselist count", self.form.baseList.rowCount())
        self.form.pbClear.setEnabled(self.form.baseList.rowCount())
        selectedRows = self.form.baseList.selectionModel().selectedRows()
        indexes = [row.row() for row in selectedRows]
        self.form.pbRemove.setEnabled(bool(indexes))

        if len(indexes) == 1:
            index = self.form.baseList.currentRow()
            self.form.pbUp.setEnabled(index > 0)
            self.form.pbDown.setEnabled(index < self.form.baseList.rowCount() - 1)
        else:
            self.form.pbUp.setEnabled(False)
            self.form.pbDown.setEnabled(False)

        if indexes:  # select operations in 3d view
            FreeCADGui.Selection.clearSelection()
            operations = [op for i, op in enumerate(self.obj.Base) if i in indexes]
            for op in operations:
                FreeCADGui.Selection.addSelection(op)

    def clearBaseList(self):
        print("clearBaseList")
        self.obj.Base = []
        self.updateBaseList()

    def RemoveFromBaseList(self):
        print("RemoveFromBaseList")
        base = self.obj.Base
        selectedRows = self.form.baseList.selectionModel().selectedRows()
        indexes = [row.row() for row in selectedRows]
        for index in sorted(indexes, reverse=True):
            print(f"    Remove op {index}")
            del base[index]
        self.obj.Base = base
        self.updateBaseList()

    def addToBaseList(self):
        print("addToBaseList")
        selection = FreeCADGui.Selection.getSelection()
        new = [sel for sel in selection if sel.isDerivedFrom("Path::Feature") and sel != self.obj]
        self.obj.Base = self.obj.Base + new
        self.updateBaseList()

    def upInBaseList(self):
        print("upInBaseList")
        index = self.form.baseList.currentRow()
        if index > 0:
            base = self.obj.Base
            print("   ", [b.Label for b in base])
            op = base.pop(index)
            print("   ", index, op.Label)
            base.insert(index - 1, op)
            self.obj.Base = base
            self.updateBaseList()
            self.form.baseList.selectRow(index - 1)
            self.updateButtonsVisibility()

    def downInBaseList(self):
        print("downInBaseList")
        index = self.form.baseList.currentRow()
        if index < self.form.baseList.rowCount() - 1:
            base = self.obj.Base
            print("   ", [b.Label for b in base])
            op = base.pop(index)
            # index = max(index, 0)
            base.insert(index + 1, op)
            self.obj.Base = base
            self.updateBaseList()
            self.form.baseList.selectRow(index + 1)
            self.updateButtonsVisibility()


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_ArrayMTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Array", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Array",
                "Creates an array with multiple tool controllers and coolant modes",
            ),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False

        for sel in selection:
            if not sel.isDerivedFrom("Path::Feature"):
                return False

            if not toolControllerForOp(sel):
                # Active only for operations with tool controller
                return False

        return True

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()

        if any(not sel.isDerivedFrom("Path::Feature") for sel in selection):
            FreeCAD.Console.PrintError(
                translate("CAM_Array", "Arrays can be created only from toolpath operations.")
                + "\n"
            )
            return

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("Path.Op.Gui.Array")
        names = [sel.Name for sel in selection]
        FreeCADGui.doCommand(f"bases = [FreeCAD.ActiveDocument.getObject(n) for n in {names}]")
        FreeCADGui.doCommand("Path.Op.Gui.Array.Create(bases)")
        # FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


ObjectArray = Array.ObjectArray  # needed for backward compatibility


def Create(baseObjects, name="Array", mode=0):
    FreeCAD.ActiveDocument.openTransaction("Create an Array")
    obj = Array.Create(baseObjects, name)
    ViewProviderArray(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, mode)
    return obj


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Array", CommandPathArray())
