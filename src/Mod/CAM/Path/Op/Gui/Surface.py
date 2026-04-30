# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Surface as PathSurface

__title__ = "CAM Surface 3D Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Surface 3D operation page controller and command implementation."

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Surface operation."""

    def initPage(self, obj):
        """initPage(obj) ... initialize the task panel page"""
        self.setTitle("3D Surface - " + obj.Label)
        self.updateVisibility()
        self.form.accuracySlider.setPageStep(1)

    def getForm(self):
        """getForm() ... returns UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpSurfaceEdit.ui")
        comboToPropertyMap = [
            ("strategySelect", "Strategy"),
            ("boundBoxSelect", "BoundBox"),
            ("layerMode", "LayerMode"),
            ("cutPattern", "CutPattern"),
            ("cutPatternZLevel", "CutPatternZLevel"),
            ("profileEdges", "ProfileEdges"),
        ]
        enumTups = PathSurface.ObjectSurface.propertyEnumerations(dataType="raw")
        PathGuiUtil.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.Strategy != str(self.form.strategySelect.currentData()):
            obj.Strategy = str(self.form.strategySelect.currentData())

        if obj.BoundBox != str(self.form.boundBoxSelect.currentData()):
            obj.BoundBox = str(self.form.boundBoxSelect.currentData())

        if obj.LayerMode != str(self.form.layerMode.currentData()):
            obj.LayerMode = str(self.form.layerMode.currentData())

        obj.CutPattern = self.form.cutPattern.currentData()

        obj.CutPatternZLevel = self.form.cutPatternZLevel.currentData()

        if obj.AvoidLastX_Faces != self.form.avoidLastX_Faces.value():
            obj.AvoidLastX_Faces = self.form.avoidLastX_Faces.value()

        obj.ProfileEdges = self.form.profileEdges.currentData()

        PathGuiUtil.updateInputField(obj, "DepthOffset", self.form.depthOffset)
        PathGuiUtil.updateInputField(obj, "StockToLeave", self.form.stockToLeave)
        PathGuiUtil.updateInputField(obj, "BoundaryAdjustment", self.form.boundaryAdjustment)
        PathGuiUtil.updateInputField(obj, "SampleInterval", self.form.sampleInterval)
        PathGuiUtil.updateInputField(obj, "MinSampleInterval", self.form.minSampleInterval)

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if obj.AdaptiveSampling != self.form.adaptiveSampling.isChecked():
            obj.AdaptiveSampling = self.form.adaptiveSampling.isChecked()

        if obj.OptimizeLinearPaths != self.form.optimizeEnabled.isChecked():
            obj.OptimizeLinearPaths = self.form.optimizeEnabled.isChecked()

        if obj.KeepToolDown != self.form.keepToolDown.isChecked():
            obj.KeepToolDown = self.form.keepToolDown.isChecked()

        if obj.ClearPlanarOnly != self.form.clearPlanarOnly.isChecked():
            obj.ClearPlanarOnly = self.form.clearPlanarOnly.isChecked()

        if obj.CutPatternReversed != self.form.cutPatternReversed.isChecked():
            obj.CutPatternReversed = self.form.cutPatternReversed.isChecked()

        if obj.IgnoreOuter != self.form.ignoreOuter.isChecked():
            obj.IgnoreOuter = self.form.ignoreOuter.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.selectInComboBox(obj.Strategy, self.form.strategySelect)
        self.selectInComboBox(obj.BoundBox, self.form.boundBoxSelect)
        self.selectInComboBox(obj.LayerMode, self.form.layerMode)
        self.selectInComboBox(obj.CutPattern, self.form.cutPattern)
        self.selectInComboBox(obj.CutPatternZLevel, self.form.cutPatternZLevel)

        self.form.avoidLastX_Faces.setValue(obj.AvoidLastX_Faces)
        self.selectInComboBox(obj.ProfileEdges, self.form.profileEdges)
        self.form.depthOffset.setText(
            FreeCAD.Units.Quantity(obj.DepthOffset.Value, FreeCAD.Units.Length).UserString
        )

        self.form.boundaryAdjustment.setText(
            FreeCAD.Units.Quantity(obj.BoundaryAdjustment.Value, FreeCAD.Units.Length).UserString
        )
        self.form.stockToLeave.setText(
            FreeCAD.Units.Quantity(obj.StockToLeave.Value, FreeCAD.Units.Length).UserString
        )
        self.form.stepOver.setValue(obj.StepOver)
        self.form.sampleInterval.setText(
            FreeCAD.Units.Quantity(obj.SampleInterval.Value, FreeCAD.Units.Length).UserString
        )
        self.form.minSampleInterval.setText(
            FreeCAD.Units.Quantity(obj.MinSampleInterval.Value, FreeCAD.Units.Length).UserString
        )

        if obj.UseStartPoint:
            self.form.useStartPoint.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.useStartPoint.setCheckState(QtCore.Qt.Unchecked)

        if obj.AdaptiveSampling:
            self.form.adaptiveSampling.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.adaptiveSampling.setCheckState(QtCore.Qt.Unchecked)

        if obj.OptimizeLinearPaths:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.optimizeEnabled.setCheckState(QtCore.Qt.Unchecked)

        if obj.KeepToolDown:
            self.form.keepToolDown.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.keepToolDown.setCheckState(QtCore.Qt.Unchecked)

        if obj.ClearPlanarOnly:
            self.form.clearPlanarOnly.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.clearPlanarOnly.setCheckState(QtCore.Qt.Unchecked)

        if obj.CutPatternReversed:
            self.form.cutPatternReversed.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.cutPatternReversed.setCheckState(QtCore.Qt.Unchecked)

        if obj.IgnoreOuter:
            self.form.ignoreOuter.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.ignoreOuter.setCheckState(QtCore.Qt.Unchecked)

        self._syncAccuracyLabel()

        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.strategySelect.currentIndexChanged)
        signals.append(self.form.boundBoxSelect.currentIndexChanged)
        signals.append(self.form.layerMode.currentIndexChanged)
        signals.append(self.form.cutPattern.currentIndexChanged)
        signals.append(self.form.cutPatternZLevel.currentIndexChanged)
        signals.append(self.form.avoidLastX_Faces.editingFinished)
        signals.append(self.form.profileEdges.currentIndexChanged)
        signals.append(self.form.depthOffset.editingFinished)
        signals.append(self.form.boundaryAdjustment.editingFinished)
        signals.append(self.form.stockToLeave.editingFinished)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.sampleInterval.editingFinished)
        signals.append(self.form.minSampleInterval.editingFinished)
        signals.append(self.form.accuracySlider.valueChanged)

        if hasattr(self.form.useStartPoint, "checkStateChanged"):  # Qt version >= 6.7.0
            signals.append(self.form.useStartPoint.checkStateChanged)
            signals.append(self.form.adaptiveSampling.checkStateChanged)
            signals.append(self.form.optimizeEnabled.checkStateChanged)
            signals.append(self.form.keepToolDown.checkStateChanged)
            signals.append(self.form.clearPlanarOnly.checkStateChanged)
            signals.append(self.form.cutPatternReversed.checkStateChanged)
            signals.append(self.form.ignoreOuter.checkStateChanged)
        else:  # Qt version < 6.7.0
            signals.append(self.form.useStartPoint.stateChanged)
            signals.append(self.form.adaptiveSampling.stateChanged)
            signals.append(self.form.optimizeEnabled.stateChanged)
            signals.append(self.form.keepToolDown.stateChanged)
            signals.append(self.form.clearPlanarOnly.stateChanged)
            signals.append(self.form.cutPatternReversed.stateChanged)
            signals.append(self.form.ignoreOuter.stateChanged)

        return signals

    def _onAccuracySliderChanged(self, level):
        """Populate UI fields and non-UI properties from the selected accuracy preset."""
        presets = PathSurface.ObjectSurface.ACCURACY_PRESETS
        preset = presets.get(level, presets[4])
        self.form.sampleInterval.setText(
            FreeCAD.Units.Quantity(preset["sample_interval"], FreeCAD.Units.Length).UserString
        )
        self.form.minSampleInterval.setText(
            FreeCAD.Units.Quantity(preset["min_sample_interval"], FreeCAD.Units.Length).UserString
        )
        self.form.accuracyDescription.setText(
            "{} - {}".format(preset["name"], preset["description"])
        )
        obj = self.obj
        if hasattr(obj, "AngularDeflection"):
            obj.AngularDeflection = preset["angular_deflection"]
        if hasattr(obj, "LinearDeflection"):
            obj.LinearDeflection = preset["linear_deflection"]
        if hasattr(obj, "MeshSimplification"):
            obj.MeshSimplification = preset["mesh_simplification"]

        self.updateVisibility()

    def _syncAccuracyLabel(self):
        """Check if current UI values match a preset; update slider and label accordingly."""
        presets = PathSurface.ObjectSurface.ACCURACY_PRESETS
        try:
            current_si = FreeCAD.Units.Quantity(self.form.sampleInterval.text()).Value
        except Exception:
            current_si = None

        for lvl, preset in presets.items():
            if current_si is not None and abs(current_si - preset["sample_interval"]) < 0.001:
                self.form.accuracySlider.blockSignals(True)
                self.form.accuracySlider.setValue(lvl)
                self.form.accuracySlider.blockSignals(False)
                self.form.accuracyDescription.setText(
                    "{} - {}".format(preset["name"], preset["description"])
                )
                return

        self.form.accuracyDescription.setText("Custom")

    def updateVisibility(self, sentObj=None):
        """Main visibility controller. Acts as a conductor, gathering the current state
        and dispatching control to specialized helper functions for each UI group.
        """
        # 1. Gather state
        strategy = self.form.strategySelect.currentData()
        cut_pattern = self.form.cutPattern.currentData()
        cut_pattern_zlevel = self.form.cutPatternZLevel.currentData()

        try:
            sample_interval = FreeCAD.Units.Quantity(self.form.sampleInterval.text()).Value
        except Exception:
            sample_interval = 1.0  # Default to a safe value

        # Dispatch to helpers
        self._updateStrategyWidgets(strategy, cut_pattern, cut_pattern_zlevel)
        self._updatePerformanceWidgets(strategy, sample_interval)
        self._updateBoundaryWidgets(strategy)
        self._updateClearingWidgets(strategy, cut_pattern, cut_pattern_zlevel)
        self._updateOptimizationWidgets(strategy)

    def _updateStrategyWidgets(self, strategy, cut_pattern, cut_pattern_zlevel):
        """Manages widgets in the 'Strategy' group."""
        is_surface_pattern = strategy == "SurfacePattern"
        is_zlevel = strategy == "ZLevelHybrid"

        self.form.cutPattern.setVisible(is_surface_pattern)
        self.form.cutPattern_label.setVisible(is_surface_pattern)

        self.form.cutPatternZLevel.setVisible(is_zlevel)
        self.form.cutPatternZLevel_label.setVisible(is_zlevel)

        self.form.layerMode.setVisible(is_surface_pattern)
        self.form.layerMode_label.setVisible(is_surface_pattern)

    def _updatePerformanceWidgets(self, strategy, sample_interval):
        """Manages widgets in the 'Performance and Accuracy' group."""
        is_zlevel = strategy == "ZLevelHybrid"
        is_waterline = strategy == "Waterline"

        self.form.performanceAccuracyGroup.setVisible(not is_zlevel)

        adaptive_threshold = 0.3
        is_adaptive_useful = sample_interval >= adaptive_threshold

        # The checkbox itself is only visible for strategies that can use it
        can_show_adaptive = strategy == "SurfacePattern" or is_waterline
        self.form.adaptiveSampling.setVisible(can_show_adaptive)

        # It's only ENABLED if it's both visible and useful
        self.form.adaptiveSampling.setEnabled(can_show_adaptive and is_adaptive_useful)

        # The Min Sample Interval field is only visible and enabled if adaptive is checked and active
        is_min_sample_visible = (
            self.form.adaptiveSampling.isVisible() and self.form.adaptiveSampling.isChecked()
        )
        self.form.minSampleInterval.setVisible(is_min_sample_visible)
        self.form.minSampleInterval_label.setVisible(is_min_sample_visible)

        is_min_sample_enabled = (
            self.form.adaptiveSampling.isEnabled() and self.form.adaptiveSampling.isChecked()
        )
        self.form.minSampleInterval.setEnabled(is_min_sample_enabled)
        self.form.minSampleInterval_label.setEnabled(is_min_sample_enabled)

    def _updateBoundaryWidgets(self, strategy):
        """Manages widgets in the 'Boundary Control' group."""
        is_waterline = strategy == "Waterline"
        is_zlevel = strategy == "ZLevelHybrid"

        self.form.boundaryGroup.setVisible(not is_waterline)
        self.form.stockToLeave.setVisible(is_zlevel)
        self.form.stockToLeave_label.setVisible(is_zlevel)

    def _updateClearingWidgets(self, strategy, cut_pattern, cut_pattern_zlevel):
        """Manages widgets in the 'Clearing Options' group."""
        is_surface_pattern = strategy == "SurfacePattern"
        is_zlevel = strategy == "ZLevelHybrid"

        self.form.clearingOptionsGroup.setVisible(is_surface_pattern or is_zlevel)

        # Pattern Angle is enabled for linear patterns in either strategy
        is_linear_surface = is_surface_pattern and cut_pattern in ["Line", "ZigZag"]
        is_linear_zlevel = is_zlevel and cut_pattern_zlevel in ["Line", "ZigZag", "Grid"]
        self.form.cutPatternAngle.setEnabled(is_linear_surface or is_linear_zlevel)
        self.form.cutPatternAngle_label.setEnabled(is_linear_surface or is_linear_zlevel)

        # Step Over is enabled if any pattern is chosen
        has_surface_pattern = is_surface_pattern and cut_pattern is not None
        has_zlevel_pattern = is_zlevel and cut_pattern_zlevel != "None"
        self.form.stepOver.setEnabled(has_surface_pattern or has_zlevel_pattern)
        self.form.stepOver_label.setEnabled(has_surface_pattern or has_zlevel_pattern)

        # Z-Level specific checkboxes
        self.form.clearPlanarOnly.setVisible(is_zlevel)
        self.form.ignoreOuter.setVisible(is_zlevel)

    def _updateOptimizationWidgets(self, strategy):
        """Manages widgets in the 'Optimization' group."""
        is_zlevel = strategy == "ZLevelHybrid"
        self.form.optimizationGroup.setVisible(not is_zlevel)

    def registerSignalHandlers(self, obj):
        self.form.strategySelect.currentIndexChanged.connect(self.updateVisibility)
        self.form.cutPattern.currentIndexChanged.connect(self.updateVisibility)
        self.form.cutPatternZLevel.currentIndexChanged.connect(self.updateVisibility)

        if hasattr(self.form.adaptiveSampling, "checkStateChanged"):
            self.form.adaptiveSampling.checkStateChanged.connect(self.updateVisibility)
        else:
            self.form.adaptiveSampling.stateChanged.connect(self.updateVisibility)

        self.form.accuracySlider.valueChanged.connect(self._onAccuracySliderChanged)
        self.form.sampleInterval.editingFinished.connect(self.updateVisibility)
        self.form.performanceAccuracyGroup.toggled.connect(self.updateVisibility)


Command = PathOpGui.SetupOperation(
    "Surface",
    PathSurface.Create,
    TaskPanelOpPage,
    "CAM_3DSurface",
    QT_TRANSLATE_NOOP("CAM_Surface", "3D Surface"),
    QT_TRANSLATE_NOOP("CAM_Surface", "Create a 3D Surface Operation from a model"),
    PathSurface.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathSurfaceGui... done\n")
