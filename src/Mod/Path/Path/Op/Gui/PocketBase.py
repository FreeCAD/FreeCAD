# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Pocket as PathPocket
import PathGui

__title__ = "Path Pocket Base Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base page controller and command implementation for path pocket operations."

if False:
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
        pass

    def getForm(self):
        """getForm() ... returns UI, adapted to the results from pocketFeatures()"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketFullEdit.ui")

        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("offsetPattern", "OffsetPattern"),
        ]
        enumTups = PathPocket.ObjectPocket.pocketPropertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)

        if not FeatureFacing & self.pocketFeatures():
            form.facingWidget.hide()
            form.clearEdges.hide()

        if FeaturePocket & self.pocketFeatures():
            form.extraOffset_label.setText(translate("PathPocket", "Pass Extension"))
            form.extraOffset.setToolTip(
                translate(
                    "PathPocket",
                    "The distance the facing operation will extend beyond the boundary shape.",
                )
            )

        if not (FeatureOutline & self.pocketFeatures()):
            form.useOutline.hide()

        if not (FeatureRestMachining & self.pocketFeatures()):
            form.useRestMachining.hide()

        # if True:
        #     # currently doesn't have an effect or is experimental
        #     form.minTravel.hide()

        return form

    def updateMinTravel(self, obj, setModel=True):
        if obj.UseStartPoint:
            self.form.minTravel.setEnabled(True)
        else:
            self.form.minTravel.setChecked(False)
            self.form.minTravel.setEnabled(False)

        if setModel and obj.MinTravel != self.form.minTravel.isChecked():
            obj.MinTravel = self.form.minTravel.isChecked()

    def updateZigZagAngle(self, obj, setModel=True):
        if obj.OffsetPattern in ["Offset"]:
            self.form.zigZagAngle.setEnabled(False)
        else:
            self.form.zigZagAngle.setEnabled(True)

        if setModel:
            PathGuiUtil.updateInputField(obj, "ZigZagAngle", self.form.zigZagAngle)

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.StepOver != self.form.stepOverPercent.value():
            obj.StepOver = self.form.stepOverPercent.value()
        if obj.OffsetPattern != str(self.form.offsetPattern.currentData()):
            obj.OffsetPattern = str(self.form.offsetPattern.currentData())

        PathGuiUtil.updateInputField(obj, "ExtraOffset", self.form.extraOffset)
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)
        self.updateZigZagAngle(obj)

        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if obj.UseRestMachining != self.form.useRestMachining.isChecked():
            obj.UseRestMachining = self.form.useRestMachining.isChecked()

        if FeatureOutline & self.pocketFeatures():
            if obj.UseOutline != self.form.useOutline.isChecked():
                obj.UseOutline = self.form.useOutline.isChecked()

        self.updateMinTravel(obj)

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
        self.form.stepOverPercent.setValue(obj.StepOver)
        self.form.extraOffset.setText(
            FreeCAD.Units.Quantity(
                obj.ExtraOffset.Value, FreeCAD.Units.Length
            ).UserString
        )
        self.form.useStartPoint.setChecked(obj.UseStartPoint)
        self.form.useRestMachining.setChecked(obj.UseRestMachining)
        if FeatureOutline & self.pocketFeatures():
            self.form.useOutline.setChecked(obj.UseOutline)

        self.form.zigZagAngle.setText(
            FreeCAD.Units.Quantity(obj.ZigZagAngle, FreeCAD.Units.Angle).UserString
        )
        self.updateZigZagAngle(obj, False)

        self.form.minTravel.setChecked(obj.MinTravel)
        self.updateMinTravel(obj, False)

        self.selectInComboBox(obj.OffsetPattern, self.form.offsetPattern)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        if FeatureFacing & self.pocketFeatures():
            self.selectInComboBox(obj.BoundaryShape, self.form.boundaryShape)
            self.form.clearEdges.setChecked(obj.ClearEdges)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.offsetPattern.currentIndexChanged)
        signals.append(self.form.stepOverPercent.editingFinished)
        signals.append(self.form.zigZagAngle.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.useStartPoint.clicked)
        signals.append(self.form.useRestMachining.clicked)
        signals.append(self.form.useOutline.clicked)
        signals.append(self.form.minTravel.clicked)
        signals.append(self.form.coolantController.currentIndexChanged)

        if FeatureFacing & self.pocketFeatures():
            signals.append(self.form.boundaryShape.currentIndexChanged)
            signals.append(self.form.clearEdges.clicked)

        return signals
