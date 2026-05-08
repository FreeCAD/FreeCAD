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
            ("cutMode", "CutMode"),
            ("handleMultiple", "HandleMultipleFeatures"),
        ]
        enumTups = PathSurface3D.ObjectSurface3D.propertyEnumerations(dataType="raw")
        PathGuiUtil.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def getFields(self, obj):
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.Strategy != str(self.form.strategy.currentData()):
            obj.Strategy = str(self.form.strategy.currentData())
        if obj.CutPattern != str(self.form.cutPattern.currentData()):
            obj.CutPattern = str(self.form.cutPattern.currentData())
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.HandleMultipleFeatures != str(self.form.handleMultiple.currentData()):
            obj.HandleMultipleFeatures = str(self.form.handleMultiple.currentData())

        if obj.CutPatternReversed != self.form.cutPatternReversed.isChecked():
            obj.CutPatternReversed = self.form.cutPatternReversed.isChecked()
        if obj.OptimizeLinearPaths != self.form.optimizeLinearPaths.isChecked():
            obj.OptimizeLinearPaths = self.form.optimizeLinearPaths.isChecked()

        for sb in self._allSpinBoxes():
            sb.updateProperty()

    def setFields(self, obj):
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.selectInComboBox(obj.Strategy, self.form.strategy)
        self.selectInComboBox(obj.CutPattern, self.form.cutPattern)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.HandleMultipleFeatures, self.form.handleMultiple)
        self.form.cutPatternReversed.setChecked(bool(obj.CutPatternReversed))
        self.form.optimizeLinearPaths.setChecked(bool(obj.OptimizeLinearPaths))
        self.updateQuantitySpinBoxes()

    def updateQuantitySpinBoxes(self, index=None):
        for sb in self._allSpinBoxes():
            sb.updateWidget()

    def getSignalsForUpdate(self, obj):
        signals = [
            self.form.toolController.currentIndexChanged,
            self.form.coolantController.currentIndexChanged,
            self.form.strategy.currentIndexChanged,
            self.form.cutPattern.currentIndexChanged,
            self.form.cutMode.currentIndexChanged,
            self.form.handleMultiple.currentIndexChanged,
            self.form.stepOver.editingFinished,
            self.form.sampleInterval.editingFinished,
            self.form.depthOffset.editingFinished,
            self.form.boundaryAdjustment.editingFinished,
            self.form.cutPatternAngle.editingFinished,
            self.form.linearDeflection.editingFinished,
            self.form.angularDeflection.editingFinished,
        ]
        for cb in (self.form.cutPatternReversed, self.form.optimizeLinearPaths):
            if hasattr(cb, "checkStateChanged"):
                signals.append(cb.checkStateChanged)
            else:
                signals.append(cb.stateChanged)
        return signals


Command = PathOpGui.SetupOperation(
    "Surface3D",
    PathSurface3D.Create,
    TaskPanelOpPage,
    "CAM_3DSurface",
    QT_TRANSLATE_NOOP("CAM_Surface3D", "3D Surface (new)"),
    QT_TRANSLATE_NOOP(
        "CAM_Surface3D",
        "3D surface finishing operation built on the surface_* generators.",
    ),
    PathSurface3D.SetupProperties,
)


FreeCAD.Console.PrintLog("Loading PathSurface3DGui... done\n")
