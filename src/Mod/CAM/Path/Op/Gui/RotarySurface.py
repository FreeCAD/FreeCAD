# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 <shopinthewoods@gmail.com>
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
import Path.Op.RotarySurface as PathRotarySurface

__title__ = "CAM Rotary Surface Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Rotary Surface operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller for the Rotary Surface operation."""

    def initPage(self, obj):
        self.startXSpinBox = PathGuiUtil.QuantitySpinBox(self.form.startX, obj, "StartX")
        self.stopXSpinBox = PathGuiUtil.QuantitySpinBox(self.form.stopX, obj, "StopX")
        self.startAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.startAngle, obj, "StartAngle"
        )
        self.stopAngleSpinBox = PathGuiUtil.QuantitySpinBox(self.form.stopAngle, obj, "StopAngle")
        self.stepOverSpinBox = PathGuiUtil.QuantitySpinBox(self.form.stepOver, obj, "StepOver")
        self.angularResolutionSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.angularResolution, obj, "AngularResolution"
        )
        self.radialStockToLeaveSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.radialStockToLeave, obj, "RadialStockToLeave"
        )
        self.maxFeedSpinBox = PathGuiUtil.QuantitySpinBox(self.form.maxFeed, obj, "MaxFeed")

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpRotarySurfaceEdit.ui")
        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("cutPattern", "CutPattern"),
            ("feedMode", "FeedMode"),
        ]
        enumTups = PathRotarySurface.ObjectRotarySurface.propertyEnumerations(dataType="raw")
        PathGuiUtil.populateCombobox(form, enumTups, comboToPropertyMap)
        # TODO(rings-visibility): when this page grows pattern-specific
        # fields, hide/show them based on form.cutPattern.currentData().
        # Today every CutPattern (Spiral, Parallel, Rings) shares the
        # same property surface, so no toggle is needed.
        return form

    def getFields(self, obj):
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.CutPattern != str(self.form.cutPattern.currentData()):
            obj.CutPattern = str(self.form.cutPattern.currentData())
        if obj.FeedMode != str(self.form.feedMode.currentData()):
            obj.FeedMode = str(self.form.feedMode.currentData())

        for sb in (
            self.startXSpinBox,
            self.stopXSpinBox,
            self.startAngleSpinBox,
            self.stopAngleSpinBox,
            self.stepOverSpinBox,
            self.angularResolutionSpinBox,
            self.radialStockToLeaveSpinBox,
            self.maxFeedSpinBox,
        ):
            sb.updateProperty()

        if hasattr(obj, "BoundaryFromFaces"):
            obj.BoundaryFromFaces = self.form.boundaryFromFaces.isChecked()

    def setFields(self, obj):
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.CutPattern, self.form.cutPattern)
        self.selectInComboBox(obj.FeedMode, self.form.feedMode)
        self.form.boundaryFromFaces.setChecked(bool(obj.BoundaryFromFaces))
        self.updateQuantitySpinBoxes()

    # TODO(parallel-visibility): If/when CutPattern-specific fields are
    # added (e.g. AngularStepover for Parallel), wire visibility through
    # the same mechanism Surface.py uses. The Rotary Surface task panel
    # currently shares one set of fields across all patterns, so no
    # per-pattern hide/show is required for Spiral/Parallel/Rings yet.

    def updateQuantitySpinBoxes(self, index=None):
        for sb in (
            self.startXSpinBox,
            self.stopXSpinBox,
            self.startAngleSpinBox,
            self.stopAngleSpinBox,
            self.stepOverSpinBox,
            self.angularResolutionSpinBox,
            self.radialStockToLeaveSpinBox,
            self.maxFeedSpinBox,
        ):
            sb.updateWidget()

    def getSignalsForUpdate(self, obj):
        signals = [
            self.form.toolController.currentIndexChanged,
            self.form.coolantController.currentIndexChanged,
            self.form.cutMode.currentIndexChanged,
            self.form.cutPattern.currentIndexChanged,
            self.form.feedMode.currentIndexChanged,
            self.form.startX.editingFinished,
            self.form.stopX.editingFinished,
            self.form.startAngle.editingFinished,
            self.form.stopAngle.editingFinished,
            self.form.stepOver.editingFinished,
            self.form.angularResolution.editingFinished,
            self.form.radialStockToLeave.editingFinished,
            self.form.maxFeed.editingFinished,
        ]
        if hasattr(self.form.boundaryFromFaces, "checkStateChanged"):
            signals.append(self.form.boundaryFromFaces.checkStateChanged)
        else:
            signals.append(self.form.boundaryFromFaces.stateChanged)
        return signals


Command = PathOpGui.SetupOperation(
    "RotarySurface",
    PathRotarySurface.Create,
    TaskPanelOpPage,
    "CAM_RotarySurface",
    QT_TRANSLATE_NOOP("CAM_RotarySurface", "Rotary Surface"),
    QT_TRANSLATE_NOOP(
        "CAM_RotarySurface",
        "Continuous 4-axis rotary surfacing on a part mounted on a single rotary.",
    ),
    PathRotarySurface.SetupProperties,
)


FreeCAD.Console.PrintLog("Loading PathRotarySurfaceGui... done\n")
