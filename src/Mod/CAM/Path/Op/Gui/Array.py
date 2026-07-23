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
import PathScripts.PathUtils as PathUtils
from Path.Base.Gui.Util import QuantitySpinBox
from Path.Base.Util import toolControllerForOp, coolantModeForOp
from PathPythonGui.simple_edit_panel import SimpleEditPanel

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtGui

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
        self.updateButtonsVisibility()

    def initPage(self):
        self.connectWidget("ReverseDirection", self.form.chk_reverse)
        self.connectWidget("SwapDirection", self.form.chk_swap)
        self.connectWidget("ExpandArray", self.form.chk_expand)
        self.connectWidget("Combine", self.form.chk_combine)
        self.connectWidget("UseJitter", self.form.chk_use_jitter)
        self.connectWidget("Type", self.form.cbo_type)

        self.copies = QuantitySpinBox(self.form.dsp_copies, self.obj, "Copies")
        self.copiesX = QuantitySpinBox(self.form.dsp_copies_x, self.obj, "CopiesX")
        self.copiesY = QuantitySpinBox(self.form.dsp_copies_y, self.obj, "CopiesY")
        self.offsetX = QuantitySpinBox(self.form.dsp_offset_x, self.obj, "Offset.x")
        self.offsetY = QuantitySpinBox(self.form.dsp_offset_y, self.obj, "Offset.y")

        self.polarAngle = QuantitySpinBox(self.form.dsp_polar_angle, self.obj, "Angle")
        self.polarCenterX = QuantitySpinBox(self.form.dsp_polar_center_x, self.obj, "Centre.x")
        self.polarCenterY = QuantitySpinBox(self.form.dsp_polar_center_y, self.obj, "Centre.y")

        self.jitterAngle = QuantitySpinBox(self.form.dsp_jitter_angle, self.obj, "JitterAngle")
        self.jitterSeed = QuantitySpinBox(self.form.dsp_jitter_seed, self.obj, "JitterSeed")
        self.jitterX = QuantitySpinBox(self.form.dsp_jitter_x, self.obj, "JitterMagnitude.x")
        self.jitterY = QuantitySpinBox(self.form.dsp_jitter_x, self.obj, "JitterMagnitude.y")

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
        self.form.chk_use_jitter.clicked.connect(self.updateVisibility)
        self.form.cbo_type.currentIndexChanged.connect(self.updateVisibility)
        self.form.baseList.clicked.connect(self.updateButtonsVisibility)
        self.form.pb_clear.clicked.connect(self.clearBaseList)
        self.form.pb_remove.clicked.connect(self.RemoveFromBaseList)
        self.form.pb_add.clicked.connect(self.addToBaseList)
        self.form.pb_up.clicked.connect(self.upInBaseList)
        self.form.pb_down.clicked.connect(self.downInBaseList)

    def getSignalsForUpdate(self):
        signals = []
        signals.append(self.form.dsp_copies.editingFinished)
        signals.append(self.form.dsp_copies_x.editingFinished)
        signals.append(self.form.dsp_copies_y.editingFinished)
        signals.append(self.form.dsp_offset_x.editingFinished)
        signals.append(self.form.dsp_offset_y.editingFinished)
        signals.append(self.form.dsp_polar_angle.editingFinished)
        signals.append(self.form.dsp_polar_center_x.editingFinished)
        signals.append(self.form.dsp_polar_center_y.editingFinished)
        signals.append(self.form.dsp_jitter_angle.editingFinished)
        signals.append(self.form.dsp_jitter_seed.editingFinished)
        signals.append(self.form.dsp_jitter_x.editingFinished)
        signals.append(self.form.dsp_jitter_y.editingFinished)
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

    def pageRegisterSignalHandlers(self):
        for signal in self.getSignalsForUpdate():
            signal.connect(self.pageGetFields)

    def updateVisibility(self):
        """Update visibility of widgets"""
        print("updateVisibility", self.form.chk_use_jitter.isChecked())
        if self.form.chk_use_jitter.isChecked():
            self.form.group_jitter.show()
        else:
            self.form.group_jitter.hide()

        if self.form.cbo_type.currentText() == "Points":
            self.form.group_copies.hide()
        else:
            self.form.group_copies.show()

        if self.form.cbo_type.currentText() in ("Linear1D", "Polar"):
            self.form.label_copies.show()
            self.form.label_polar_angle.show()
            self.form.dsp_copies.show()
            self.form.dsp_polar_angle.show()
        else:
            self.form.label_copies.hide()
            self.form.label_polar_angle.hide()
            self.form.dsp_copies.hide()
            self.form.dsp_polar_angle.hide()

        if self.form.cbo_type.currentText() == "Polar":
            self.form.label_polar_center_x.show()
            self.form.label_polar_center_y.show()
            self.form.dsp_polar_center_x.show()
            self.form.dsp_polar_center_y.show()
            self.form.dsp_polar_angle.setEnabled(True)
        else:
            self.form.label_polar_center_x.hide()
            self.form.label_polar_center_y.hide()
            self.form.dsp_polar_center_x.hide()
            self.form.dsp_polar_center_y.hide()
            self.form.dsp_polar_angle.setEnabled(False)

        if self.form.cbo_type.currentText() in ("Linear1D", "Linear2D"):
            self.form.label_offset_x.show()
            self.form.label_offset_y.show()
            self.form.dsp_offset_x.show()
            self.form.dsp_offset_y.show()
        else:
            self.form.label_offset_x.hide()
            self.form.label_offset_y.hide()
            self.form.dsp_offset_x.hide()
            self.form.dsp_offset_y.hide()

        if self.form.cbo_type.currentText() == "Linear2D":
            self.form.label_copies_x.show()
            self.form.label_copies_y.show()
            self.form.dsp_copies_x.show()
            self.form.dsp_copies_y.show()
            self.form.chk_swap.show()
        else:
            self.form.label_copies_x.hide()
            self.form.label_copies_y.hide()
            self.form.dsp_copies_x.hide()
            self.form.dsp_copies_y.hide()
            self.form.chk_swap.hide()

    def getOpIndex(self, op):
        print("getOpIndex")
        """Returns operation index from Operations group"""
        job = PathUtils.findParentJob(op)
        print("   job", job)
        if job and op in job.Operations.Group:
            print("   index", job.Operations.Group.index(op))
            return job.Operations.Group.index(op)
        else:
            return None

    def getWarningMessage(self):
        """Returns True if Base list order is correct"""
        string = translate("CAM_Array", "!!! Order of operations can be dangerous !!!")
        indexes = [self.getOpIndex(op) for op in self.obj.Base]
        if not indexes:
            return None
        if any(i is None for i in indexes):
            return string
        print("  warn", indexes, list(range(indexes[0], indexes[-1] + 1)))
        if indexes != list(range(indexes[0], indexes[-1] + 1)):
            return string

    def updateBaseList(self):
        """Update table with operations"""
        print("updateBaseList")
        # Column indices for baseList table
        COL_INDEX = 0
        COL_OP_NAME = 1
        COL_OP_TC = 2
        COL_OP_COOLANT = 3
        self.form.baseList.blockSignals(True)
        self.form.baseList.clearContents()
        self.form.baseList.setRowCount(0)
        for i, op in enumerate(self.obj.Base):
            print("   ", i, op.Label)
            self.form.baseList.insertRow(self.form.baseList.rowCount())

            index = self.getOpIndex(op)
            if index is not None:
                indexString = str(index + 1)
            else:
                indexString = "???"
            item = QtGui.QTableWidgetItem(indexString)
            self.form.baseList.setItem(i, COL_INDEX, item)

            item = QtGui.QTableWidgetItem(op.Label)
            self.form.baseList.setItem(i, COL_OP_NAME, item)

            if tc := toolControllerForOp(op):
                tcLabel = tc.Label
            else:
                tcLabel = "???"
            item = QtGui.QTableWidgetItem(tcLabel)
            self.form.baseList.setItem(i, COL_OP_TC, item)

            coolant = coolantModeForOp(op)
            coolantString = coolant if coolant != "None" else ""
            item = QtGui.QTableWidgetItem(coolantString)
            self.form.baseList.setItem(i, COL_OP_COOLANT, item)

        header = self.form.baseList.horizontalHeader()
        header.setSectionResizeMode(COL_INDEX, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_NAME, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_TC, QtGui.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(COL_OP_COOLANT, QtGui.QHeaderView.ResizeToContents)
        self.form.baseList.blockSignals(False)

        print("WARN", self.getWarningMessage())
        if string := self.getWarningMessage():
            self.form.label_message.setText(string)
            self.form.group_message.show()
        else:
            self.form.group_message.hide()

    def updateButtonsVisibility(self):
        """Update visibility of buttons"""
        print("updateButtonsVisibility", self.form.baseList.rowCount())
        self.form.pb_clear.setEnabled(self.form.baseList.rowCount())
        selectedRows = self.form.baseList.selectionModel().selectedRows()
        indexes = [row.row() for row in selectedRows]
        print("   indexes", indexes)
        self.form.pb_remove.setEnabled(bool(indexes))

        if len(indexes) == 1:
            index = self.form.baseList.currentRow()
            self.form.pb_up.setEnabled(index > 0)
            self.form.pb_down.setEnabled(index < self.form.baseList.rowCount() - 1)
        else:
            self.form.pb_up.setEnabled(False)
            self.form.pb_down.setEnabled(False)

        if indexes:  # select operations in 3d view
            FreeCADGui.Selection.clearSelection()
            operations = [op for i, op in enumerate(self.obj.Base) if i in indexes]
            for op in operations:
                FreeCADGui.Selection.addSelection(op)

    def clearBaseList(self):
        """Clear Base list"""
        print("clearBaseList")
        self.obj.Base = []
        self.updateBaseList()
        self.updateButtonsVisibility()
        self.form.focusWidget().clearFocus()

    def RemoveFromBaseList(self):
        """Remove selected operation from Base list"""
        print("RemoveFromBaseList")
        base = self.obj.Base
        selectedRows = self.form.baseList.selectionModel().selectedRows()
        indexes = [row.row() for row in selectedRows]
        print("  indexes", len(indexes), indexes)
        for index in sorted(indexes, reverse=True):
            print(f"    Remove op {index}")
            del base[index]
        self.obj.Base = base
        self.updateBaseList()
        self.form.baseList.selectRow(min(index, self.form.baseList.rowCount() - 1))
        self.updateButtonsVisibility()

    def addToBaseList(self):
        """Add selected operation to Base list"""
        print("addToBaseList")
        selection = FreeCADGui.Selection.getSelection()
        new = [sel for sel in selection if sel.isDerivedFrom("Path::Feature") and sel != self.obj]
        self.obj.Base = self.obj.Base + new
        self.updateBaseList()
        self.updateButtonsVisibility()

    def upInBaseList(self):
        """Move selected operation up in Base list"""
        print("upInBaseList")
        index = self.form.baseList.currentRow()
        if index > 0:
            base = self.obj.Base
            op = base.pop(index)
            base.insert(index - 1, op)
            self.obj.Base = base
            self.updateBaseList()
            self.form.baseList.selectRow(index - 1)
            self.updateButtonsVisibility()
            if not self.form.pb_up.isEnabled():
                self.form.focusWidget().clearFocus()

    def downInBaseList(self):
        """Move selected operation down in Base list"""
        print("downInBaseList")
        index = self.form.baseList.currentRow()
        if index < self.form.baseList.rowCount() - 1:
            base = self.obj.Base
            print("   ", [b.Label for b in base])
            op = base.pop(index)
            base.insert(index + 1, op)
            self.obj.Base = base
            self.updateBaseList()
            self.form.baseList.selectRow(index + 1)
            self.updateButtonsVisibility()
            if not self.form.pb_down.isEnabled():
                self.form.focusWidget().clearFocus()


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
