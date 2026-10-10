# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 sliptonic <shopinthewoods@gmail.com>
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
import Path
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Pocket as PathPocket
from Path.Base.Gui.Util import QuantitySpinBox

__title__ = "CAM Pocket Base Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base page controller and command implementation for pocket operations."

if True:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

FeaturePocket = 0x01
FeatureFacing = 0x02
FeatureOutline = 0x04
FeatureRestMachining = 0x08


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for pocket operations, supports:
    FeaturePocket  ... used for pocketing operation
    FeatureFacing  ... used for face milling operation
    FeatureOutline ... used for pocket-shape operation
    """

    def pocketFeatures(self):
        """pocketFeatures() ... return which features of the UI are supported by the operation.
          FeaturePocket  ... used for pocketing operation
          FeatureFacing  ... used for face milling operation
          FeatureOutline ... used for pocket-shape operation
        Must be overwritten by subclasses"""

    def initPage(self, obj):
        self.extraOffsetSpinBox = QuantitySpinBox(self.form.extraOffset, obj, "ExtraOffset")
        self.thresholdSpinBox = QuantitySpinBox(self.form.threshold, obj, "RetractThreshold")
        self.finishingOffsetSpinBox = QuantitySpinBox(
            self.form.finishingOffset, obj, "FinishingOffset"
        )
        self.angleSpinBox = QuantitySpinBox(self.form.angle, obj, "Angle")

        FreeCADGui.ExpressionBinding(self.form.finishingPasses).bind(self.obj, "FinishingPasses")

    def getToolTipList(self):
        """getToolTipList() ... Collect list of tuples (widget_name: str, property_name: str)"""
        tuples = []
        tuples.append(("cutMode", "CutMode"))
        tuples.append(("clearingPattern", "ClearingPattern"))
        tuples.append(("startAt", "StartAt"))
        tuples.append(("sorting", "SortingMode"))
        tuples.append(("angle", "Angle"))
        tuples.append(("stepOver", "StepOver"))
        tuples.append(("finishingPasses", "FinishingPasses"))
        tuples.append(("finishingOffset", "FinishingOffset"))
        tuples.append(("extraOffset", "ExtraOffset"))
        tuples.append(("threshold", "RetractThreshold"))
        tuples.append(("finishingOneStepDown", "FinishingOneStepDown"))
        tuples.append(("finishingRampHelix", "FinishingRampHelix"))
        tuples.append(("useOutline", "UseOutline"))
        tuples.append(("clearEdges", "clearEdges"))
        tuples.append(("useRestMachining", "UseRestMachining"))
        tuples.append(("useStartPoint", "UseStartPoint"))

        return tuples

    def getForm(self):
        """getForm() ... returns UI, adapted to the results from pocketFeatures()"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketFullEdit.ui")

        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("clearingPattern", "ClearingPattern"),
            ("startAt", "StartAt"),
            ("sorting", "SortingMode"),
        ]
        enumTups = PathPocket.ObjectPocket.pocketPropertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)

        if not (FeatureFacing & self.pocketFeatures()):
            form.facingWidget.hide()
            form.clearEdges.hide()

        if not (FeatureOutline & self.pocketFeatures()):
            form.useOutline.hide()

        if not (FeatureRestMachining & self.pocketFeatures()):
            form.useRestMachining.hide()

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.extraOffsetSpinBox.updateWidget()
        self.thresholdSpinBox.updateWidget()
        self.finishingOffsetSpinBox.updateWidget()
        self.angleSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.ClearingPattern != str(self.form.clearingPattern.currentData()):
            obj.ClearingPattern = str(self.form.clearingPattern.currentData())
        if obj.StartAt != str(self.form.startAt.currentData()):
            obj.StartAt = str(self.form.startAt.currentData())
        if obj.SortingMode != str(self.form.sorting.currentData()):
            obj.SortingMode = str(self.form.sorting.currentData())

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        self.extraOffsetSpinBox.updateProperty()
        self.finishingOffsetSpinBox.updateProperty()
        self.thresholdSpinBox.updateProperty()
        self.angleSpinBox.updateProperty()

        obj.FinishingPasses = self.form.finishingPasses.value()

        if obj.FinishingOneStepDown != self.form.finishingOneStepDown.isChecked():
            obj.FinishingOneStepDown = self.form.finishingOneStepDown.isChecked()
        if obj.FinishingRampHelix != self.form.finishingRampHelix.isChecked():
            obj.FinishingRampHelix = self.form.finishingRampHelix.isChecked()

        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if obj.UseRestMachining != self.form.useRestMachining.isChecked():
            obj.UseRestMachining = self.form.useRestMachining.isChecked()

        if FeatureOutline & self.pocketFeatures():
            if obj.UseOutline != self.form.useOutline.isChecked():
                obj.UseOutline = self.form.useOutline.isChecked()

        if FeatureFacing & self.pocketFeatures():
            print(obj.BoundaryShape)
            print(self.form.boundaryShape.currentText())
            print(self.form.boundaryShape.currentData())
            if obj.BoundaryShape != str(self.form.boundaryShape.currentData()):
                obj.BoundaryShape = str(self.form.boundaryShape.currentData())
            if obj.ClearEdges != self.form.clearEdges.isChecked():
                obj.ClearEdges = self.form.clearEdges.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.updateQuantitySpinBoxes()

        self.form.finishingPasses.setValue(obj.FinishingPasses)
        self.form.stepOver.setValue(obj.StepOver)

        self.form.finishingOneStepDown.setChecked(obj.FinishingOneStepDown)
        self.form.finishingRampHelix.setChecked(obj.FinishingRampHelix)
        self.form.useStartPoint.setChecked(obj.UseStartPoint)
        self.form.useRestMachining.setChecked(obj.UseRestMachining)
        if FeatureOutline & self.pocketFeatures():
            self.form.useOutline.setChecked(obj.UseOutline)

        self.selectInComboBox(obj.ClearingPattern, self.form.clearingPattern)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.StartAt, self.form.startAt)
        self.selectInComboBox(obj.SortingMode, self.form.sorting)

        if FeatureFacing & self.pocketFeatures():
            self.selectInComboBox(obj.BoundaryShape, self.form.boundaryShape)
            self.form.clearEdges.setChecked(obj.ClearEdges)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.clearingPattern.currentIndexChanged)
        signals.append(self.form.startAt.currentIndexChanged)
        signals.append(self.form.sorting.currentIndexChanged)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.finishingOffset.editingFinished)
        signals.append(self.form.finishingPasses.editingFinished)
        signals.append(self.form.angle.editingFinished)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.threshold.editingFinished)
        signals.append(self.form.finishingOneStepDown.clicked)
        signals.append(self.form.finishingRampHelix.clicked)
        signals.append(self.form.useStartPoint.clicked)
        signals.append(self.form.useRestMachining.clicked)
        signals.append(self.form.useOutline.clicked)

        if FeatureFacing & self.pocketFeatures():
            signals.append(self.form.boundaryShape.currentIndexChanged)
            signals.append(self.form.clearEdges.clicked)

        return signals

    def registerSignalHandlers(self, obj):
        self.form.setStartPoint.clicked.connect(self.setStartPoint)
        self.form.thresholdToggle.clicked.connect(self.thresholdToggle)
        self.form.clearingPattern.currentIndexChanged.connect(self.updateVisibility)
        self.form.finishingPasses.editingFinished.connect(self.updateVisibility)

    def updateVisibility(self):
        self.form.startAt.setEnabled(self.obj.ClearingPattern in ("Offset", "Helix"))
        self.form.angle.setEnabled(self.obj.ClearingPattern not in ("Offset", "Helix"))
        self.form.finishingOffset.setEnabled(self.obj.FinishingPasses)
        self.form.finishingOneStepDown.setEnabled(self.obj.FinishingPasses)
        self.form.finishingRampHelix.setEnabled(self.obj.FinishingPasses)

    def setStartPoint(self):
        selEx = FreeCADGui.Selection.getSelectionEx()
        if selEx and selEx[0].PickedPoints:
            point = selEx[0].PickedPoints[0]
            self.obj.StartPoint = point
            self.setDirty()
            Path.Log.info(
                translate("CAM_Pocket", "Set start point: %s, %s")
                % (round(point.x, 3), round(point.y, 3))
            )

    def thresholdToggle(self):
        if self.obj.RetractThreshold == 0:
            self.obj.setExpression("RetractThreshold", "OpToolDiameter")
            self.thresholdSpinBox.refresh_expression_icon(True)
        else:
            self.obj.clearExpression("RetractThreshold")
            self.obj.RetractThreshold = 0
            self.thresholdSpinBox.refresh_expression_icon(False)
        self.updateQuantitySpinBoxes()
        self.setDirty()
