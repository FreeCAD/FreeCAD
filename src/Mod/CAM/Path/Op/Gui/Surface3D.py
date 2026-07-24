# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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


from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui

import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Surface3D as PathSurface3D

__title__ = "CAM 3D Surface Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Surface3D operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller for the Surface3D operation."""

    def initPage(self, obj):
        self.stepOverSpinBox = PathGuiUtil.QuantitySpinBox(self.form.stepOver, obj, "StepOver")
        self.sampleIntervalSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.sampleInterval, obj, "SampleInterval"
        )
        self.minSampleIntervalSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.minSampleInterval, obj, "MinSampleInterval"
        )
        self.stockToLeaveSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.stockToLeave, obj, "StockToLeave"
        )
        self.depthOffsetSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.depthOffset, obj, "DepthOffset"
        )
        self.boundaryAdjustmentSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.boundaryAdjustment, obj, "BoundaryAdjustment"
        )
        self.cutPatternAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.cutPatternAngle, obj, "CutPatternAngle"
        )
        self.linearDeflectionSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.linearDeflection, obj, "LinearDeflection"
        )
        self.angularDeflectionSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.angularDeflection, obj, "AngularDeflection"
        )

    def _allSpinBoxes(self):
        return (
            self.stepOverSpinBox,
            self.sampleIntervalSpinBox,
            self.minSampleIntervalSpinBox,
            self.stockToLeaveSpinBox,
            self.depthOffsetSpinBox,
            self.boundaryAdjustmentSpinBox,
            self.cutPatternAngleSpinBox,
            self.linearDeflectionSpinBox,
            self.angularDeflectionSpinBox,
        )

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpSurface3DEdit.ui")
        comboToPropertyMap = [
            ("strategy", "Strategy"),
            ("cutPattern", "CutPattern"),
            ("cutPatternZLevel", "CutPatternZLevel"),
            ("cutMode", "CutMode"),
            ("handleMultiple", "HandleMultipleFeatures"),
            ("samplingAccuracy", "SamplingAccuracy"),
        ]
        enumTups = PathSurface3D.ObjectSurface3D.propertyEnumerations(dataType="raw")
        PathGuiUtil.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def _applyVisibility(self, obj):
        """Hide/show strategy-specific widget groups based on current Strategy."""
        strategy = str(self.form.strategy.currentData()) or obj.Strategy
        self.form.waterlineOptions.setVisible(strategy == "Waterline")
        self.form.zlevelOptions.setVisible(strategy == "ZLevelHybrid")
        is_pattern = strategy == "SurfacePattern"
        self.form.patternOptions.setVisible(is_pattern or strategy == "ZLevelHybrid")
        self.form.cutPattern_label.setVisible(is_pattern)
        self.form.cutPattern.setVisible(is_pattern)

    def getFields(self, obj):
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.Strategy != str(self.form.strategy.currentData()):
            obj.Strategy = str(self.form.strategy.currentData())
        if obj.CutPattern != str(self.form.cutPattern.currentData()):
            obj.CutPattern = str(self.form.cutPattern.currentData())
        if obj.CutPatternZLevel != str(self.form.cutPatternZLevel.currentData()):
            obj.CutPatternZLevel = str(self.form.cutPatternZLevel.currentData())
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.HandleMultipleFeatures != str(self.form.handleMultiple.currentData()):
            obj.HandleMultipleFeatures = str(self.form.handleMultiple.currentData())
        if obj.SamplingAccuracy != str(self.form.samplingAccuracy.currentData()):
            obj.SamplingAccuracy = str(self.form.samplingAccuracy.currentData())

        if obj.CutPatternReversed != self.form.cutPatternReversed.isChecked():
            obj.CutPatternReversed = self.form.cutPatternReversed.isChecked()
        if obj.OptimizeLinearPaths != self.form.optimizeLinearPaths.isChecked():
            obj.OptimizeLinearPaths = self.form.optimizeLinearPaths.isChecked()
        if obj.ClearPlanarOnly != self.form.clearPlanarOnly.isChecked():
            obj.ClearPlanarOnly = self.form.clearPlanarOnly.isChecked()
        if obj.IgnoreOuter != self.form.ignoreOuter.isChecked():
            obj.IgnoreOuter = self.form.ignoreOuter.isChecked()

        if obj.AccuracyLevel != self.form.accuracyLevel.value():
            obj.AccuracyLevel = self.form.accuracyLevel.value()
        if obj.MeshSimplification != self.form.meshSimplification.value():
            obj.MeshSimplification = self.form.meshSimplification.value()

        for sb in self._allSpinBoxes():
            sb.updateProperty()

        self._applyVisibility(obj)

    def setFields(self, obj):
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.selectInComboBox(obj.Strategy, self.form.strategy)
        self.selectInComboBox(obj.CutPattern, self.form.cutPattern)
        self.selectInComboBox(obj.CutPatternZLevel, self.form.cutPatternZLevel)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.HandleMultipleFeatures, self.form.handleMultiple)
        self.selectInComboBox(obj.SamplingAccuracy, self.form.samplingAccuracy)
        self.form.cutPatternReversed.setChecked(bool(obj.CutPatternReversed))
        self.form.optimizeLinearPaths.setChecked(bool(obj.OptimizeLinearPaths))
        self.form.clearPlanarOnly.setChecked(bool(obj.ClearPlanarOnly))
        self.form.ignoreOuter.setChecked(bool(obj.IgnoreOuter))
        self.form.accuracyLevel.setValue(int(obj.AccuracyLevel))
        self.form.meshSimplification.setValue(int(obj.MeshSimplification))
        self.updateQuantitySpinBoxes()
        self._applyVisibility(obj)

    def updateQuantitySpinBoxes(self, index=None):
        for sb in self._allSpinBoxes():
            sb.updateWidget()

    def getSignalsForUpdate(self, obj):
        signals = [
            self.form.toolController.currentIndexChanged,
            self.form.coolantController.currentIndexChanged,
            self.form.strategy.currentIndexChanged,
            self.form.cutPattern.currentIndexChanged,
            self.form.cutPatternZLevel.currentIndexChanged,
            self.form.cutMode.currentIndexChanged,
            self.form.handleMultiple.currentIndexChanged,
            self.form.samplingAccuracy.currentIndexChanged,
            self.form.stepOver.editingFinished,
            self.form.sampleInterval.editingFinished,
            self.form.minSampleInterval.editingFinished,
            self.form.stockToLeave.editingFinished,
            self.form.depthOffset.editingFinished,
            self.form.boundaryAdjustment.editingFinished,
            self.form.cutPatternAngle.editingFinished,
            self.form.linearDeflection.editingFinished,
            self.form.angularDeflection.editingFinished,
            self.form.accuracyLevel.valueChanged,
            self.form.meshSimplification.valueChanged,
        ]
        for cb in (
            self.form.cutPatternReversed,
            self.form.optimizeLinearPaths,
            self.form.clearPlanarOnly,
            self.form.ignoreOuter,
        ):
            if hasattr(cb, "checkStateChanged"):
                signals.append(cb.checkStateChanged)
            else:
                signals.append(cb.stateChanged)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.strategy.currentIndexChanged.connect(lambda *_: self._applyVisibility(obj))


Command = PathOpGui.SetupOperation(
    "Surface3D",
    PathSurface3D.Create,
    TaskPanelOpPage,
    "CAM_3DSurface",
    QT_TRANSLATE_NOOP("CAM_Surface3D", "3D Surface (new)"),
    QT_TRANSLATE_NOOP(
        "CAM_Surface3D",
        "Creates a 3D surface finishing toolpath on selected faces.",
    ),
    PathSurface3D.SetupProperties,
)


FreeCAD.Console.PrintLog("Loading PathSurface3DGui... done\n")
