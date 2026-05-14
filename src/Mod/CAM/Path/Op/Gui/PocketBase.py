# SPDX-License-Identifier: LGPL-2.1-or-later

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
FeatureExtension = 0x10


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for pocket operations, supports:
    FeaturePocket  ... used for pocketing operation
    FeatureFacing  ... used for face milling operation
    FeatureOutline ... used for pocket-shape operation
    FeatureExtension ... used for pocket-shape operation
    """

    def pocketFeatures(self):
        """pocketFeatures() ... return which features of the UI are supported by the operation.
          FeaturePocket  ... used for pocketing operation
          FeatureFacing  ... used for face milling operation
          FeatureOutline ... used for pocket-shape operation
          FeatureExtension ... used for pocket-shape operation
        Must be overwritten by subclasses"""
        pass

    def initPage(self, obj):
        self.form.extraOffset.setProperty("unit", obj.ExtraOffset.getUserPreferred()[2])
        if FeatureExtension & self.pocketFeatures():
            self.form.extensionOffset.setProperty("unit", obj.ExtensionOffset.getUserPreferred()[2])

    def getForm(self):
        """getForm() ... returns UI, adapted to the results from pocketFeatures()"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpPocketFullEdit.ui")

        comboToPropertyMap = [
            ("cutMode", "CutMode"),
            ("clearingPattern", "ClearingPattern"),
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

        if not (FeatureExtension & self.pocketFeatures()):
            form.extensionOffset.hide()
            form.extensionOffset_label.hide()

        return form

    def updateMinTravel(self, obj, setModel=True):
        if setModel and obj.MinTravel != self.form.minTravel.isChecked():
            obj.MinTravel = self.form.minTravel.isChecked()

    def updateAngle(self, obj, setModel=True):
        if obj.ClearingPattern == "Offset":
            self.form.angle.setEnabled(False)
        else:
            self.form.angle.setEnabled(True)

        if setModel and getattr(obj.Angle, "Value", obj.Angle) != self.form.angle.value():
            obj.Angle = self.form.angle.value()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.CutMode != str(self.form.cutMode.currentData()):
            obj.CutMode = str(self.form.cutMode.currentData())
        if obj.StepOver != self.form.stepOver.value():
            obj.StepOver = self.form.stepOver.value()
        if obj.ClearingPattern != str(self.form.clearingPattern.currentData()):
            obj.ClearingPattern = str(self.form.clearingPattern.currentData())

        PathGuiUtil.updateInputField(obj, "ExtraOffset", self.form.extraOffset)
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)
        self.updateAngle(obj)

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

        if FeatureExtension & self.pocketFeatures():
            PathGuiUtil.updateInputField(obj, "ExtensionOffset", self.form.extensionOffset)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.form.stepOver.setValue(obj.StepOver)
        self.form.extraOffset.setProperty("rawValue", obj.ExtraOffset.Value)
        self.form.useStartPoint.setChecked(obj.UseStartPoint)
        self.form.useRestMachining.setChecked(obj.UseRestMachining)
        if FeatureOutline & self.pocketFeatures():
            self.form.useOutline.setChecked(obj.UseOutline)

        self.form.angle.setValue(getattr(obj.Angle, "Value", obj.Angle))
        self.updateAngle(obj, False)

        self.form.minTravel.setChecked(obj.MinTravel)
        self.updateMinTravel(obj, False)

        self.selectInComboBox(obj.ClearingPattern, self.form.clearingPattern)
        self.selectInComboBox(obj.CutMode, self.form.cutMode)
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        if FeatureFacing & self.pocketFeatures():
            self.selectInComboBox(obj.BoundaryShape, self.form.boundaryShape)
            self.form.clearEdges.setChecked(obj.ClearEdges)

        if FeatureExtension & self.pocketFeatures():
            self.form.extensionOffset.setProperty("rawValue", obj.ExtensionOffset.Value)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []

        signals.append(self.form.cutMode.currentIndexChanged)
        signals.append(self.form.clearingPattern.currentIndexChanged)
        signals.append(self.form.stepOver.editingFinished)
        signals.append(self.form.angle.editingFinished)
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

        if FeatureExtension & self.pocketFeatures():
            signals.append(self.form.extensionOffset.editingFinished)

        return signals

    def registerSignalHandlers(self, obj):
        self.form.setStartPoint.clicked.connect(self.setStartPoint)

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
