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
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.CircularHoleBase as PathCircularHoleBaseGui
import Path.Op.Helix as PathHelix
from PySide.QtCore import QT_TRANSLATE_NOOP

translate = FreeCAD.Qt.translate


__doc__ = "Helix operation page controller and command implementation."

LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.NOTICE, Path.Log.thisModule())


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Page controller class for Helix operations."""

    def initPage(self, obj):
        self.helixMaxPitchSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.helixMaxPitch, obj, "HelixMaxPitch", setToolTip=True
        )
        self.helixMaxRampAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.helixMaxRampAngle, obj, "HelixMaxRampAngle", setToolTip=True
        )
        self.radialStockToLeaveOuterSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.radialStockToLeaveOuter, obj, "RadialStockToLeaveOuter", setToolTip=True
        )
        self.radialStockToLeaveInnerSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.radialStockToLeaveInner, obj, "RadialStockToLeaveInner", setToolTip=True
        )
        self.coneAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.coneAngle, obj, "HelixConeAngle", setToolTip=True
        )
        self.rotationAngleSpinBox = PathGuiUtil.QuantitySpinBox(
            self.form.rotationAngle, obj, "RotationAngle", setToolTip=True
        )

        self.form.cutMode.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("CutMode"))
        )
        self.form.side.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("Side"))
        )
        self.form.startAt.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("StartAt"))
        )
        self.form.stepOver.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("StepOver"))
        )
        self.form.spiralMill.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("SpiralMill"))
        )
        self.form.singleHelix.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("SingleHelix"))
        )
        self.form.startBottom.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("StartConeFromBottom"))
        )
        self.form.retractFromWall.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("RetractFromWall"))
        )
        self.form.overrideArcFeed.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("OverrideArcFeedRate"))
        )

    def getForm(self):
        """getForm() ... return UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpHelixEdit.ui")

        comboToPropertyMap = [("side", "Side"), ("startAt", "StartAt"), ("cutMode", "CutMode")]
        enumTups = PathHelix.ObjectHelix.helixOpPropertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.helixMaxPitchSpinBox.updateWidget()
        self.helixMaxRampAngleSpinBox.updateWidget()
        self.radialStockToLeaveOuterSpinBox.updateWidget()
        self.radialStockToLeaveInnerSpinBox.updateWidget()
        self.coneAngleSpinBox.updateWidget()
        self.rotationAngleSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        Path.Log.track()
        self.helixMaxPitchSpinBox.updateProperty()
        self.helixMaxRampAngleSpinBox.updateProperty()
        self.radialStockToLeaveOuterSpinBox.updateProperty()
        self.radialStockToLeaveInnerSpinBox.updateProperty()
        self.coneAngleSpinBox.updateProperty()
        self.rotationAngleSpinBox.updateProperty()

        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.StartAt != str(self.form.startAt.currentData()):
            obj.StartAt = str(self.form.startAt.currentData())
        if obj.Side != str(self.form.side.currentData()):
            obj.Side = str(self.form.side.currentData())

        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()

        if obj.SpiralMill != self.form.spiralMill.isChecked():
            obj.SpiralMill = self.form.spiralMill.isChecked()
        if obj.SingleHelix != self.form.singleHelix.isChecked():
            obj.SingleHelix = self.form.singleHelix.isChecked()
        if obj.StartConeFromBottom != self.form.startBottom.isChecked():
            obj.StartConeFromBottom = self.form.startBottom.isChecked()
        if obj.RetractFromWall != self.form.retractFromWall.isChecked():
            obj.RetractFromWall = self.form.retractFromWall.isChecked()
        if obj.OverrideArcFeedRate != self.form.overrideArcFeed.isChecked():
            obj.OverrideArcFeedRate = self.form.overrideArcFeed.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        Path.Log.track()
        self.updateQuantitySpinBoxes()

        self.form.stepOver.setValue(obj.StepOver)

        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.selectInComboBox(obj.StartAt, self.form.startAt)
        self.selectInComboBox(obj.Side, self.form.side)

        self.form.spiralMill.setChecked(obj.SpiralMill)
        self.form.singleHelix.setChecked(obj.SingleHelix)
        self.form.startBottom.setChecked(obj.StartConeFromBottom)
        self.form.retractFromWall.setChecked(obj.RetractFromWall)
        self.form.overrideArcFeed.setChecked(obj.OverrideArcFeedRate)

        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.helixMaxPitch.editingFinished)
        signals.append(self.form.helixMaxRampAngle.editingFinished)
        signals.append(self.form.radialStockToLeaveOuter.editingFinished)
        signals.append(self.form.radialStockToLeaveInner.editingFinished)
        signals.append(self.form.coneAngle.editingFinished)
        signals.append(self.form.rotationAngle.editingFinished)
        signals.append(self.form.stepOver.editingFinished)

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.startAt.currentIndexChanged)
        signals.append(self.form.side.currentIndexChanged)

        signals.append(self.form.spiralMill.checkStateChanged)
        signals.append(self.form.singleHelix.checkStateChanged)
        signals.append(self.form.startBottom.checkStateChanged)
        signals.append(self.form.retractFromWall.checkStateChanged)
        signals.append(self.form.overrideArcFeed.checkStateChanged)

        return signals

    def updateVisibility(self):
        if self.form.coneAngle.property("rawValue"):
            self.form.startBottom.show()
        else:
            self.form.startBottom.hide()

        if self.form.spiralMill.isChecked():
            self.form.singleHelix.hide()
        else:
            self.form.singleHelix.show()

    def registerSignalHandlers(self, obj):
        self.form.coneAngle.editingFinished.connect(self.updateVisibility)
        self.form.spiralMill.checkStateChanged.connect(self.updateVisibility)
        self.form.autoConeAngle.clicked.connect(self.autoConeAngle)

    def autoConeAngle(self):
        angle = self.obj.Proxy.coneAngle(self.obj, verbose=True)
        if angle is not None:
            self.obj.clearExpression("HelixConeAngle")
            self.obj.HelixConeAngle = angle
            self.coneAngleSpinBox.refresh_expression_icon(False)
            self.updateQuantitySpinBoxes()
            self.setDirty()
        self.updateVisibility()


Command = PathOpGui.SetupOperation(
    "Helix",
    PathHelix.Create,
    TaskPanelOpPage,
    "CAM_Helix",
    QT_TRANSLATE_NOOP("CAM_Helix", "Helix"),
    QT_TRANSLATE_NOOP("CAM_Helix", "Creates a Helical toolpath from the features of a base object"),
    PathHelix.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathHelixGui... done\n")
