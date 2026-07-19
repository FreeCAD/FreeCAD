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
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Profile as PathProfile
import Path
from Path.Base.Gui.Util import QuantitySpinBox
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Profile Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Profile operation page controller and command implementation."

translate = FreeCAD.Qt.translate

FeatureSide = 0x01
FeatureProcessing = 0x02


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Base class for profile operation page controllers. Two sub features are supported:
    FeatureSide       ... Is the Side property exposed in the UI
    FeatureProcessing ... Are the processing check boxes supported by the operation
    """

    def initPage(self, obj):
        # self.setTitle("Profile - " + obj.Label)
        # self.updateVisibility()
        self.extraOffsetSpinBox = QuantitySpinBox(self.form.extraOffset, obj, "OffsetExtra")
        self.thresholdSpinBox = QuantitySpinBox(self.form.threshold, obj, "RetractThreshold")
        self.stepoverSpinBox = QuantitySpinBox(self.form.stepover, obj, "Stepover")

        FreeCADGui.ExpressionBinding(self.form.numPasses).bind(self.obj, "NumPasses")

    def profileFeatures(self):
        """profileFeatures() ... return which of the optional profile features are supported.
        Currently two features are supported and returned:
            FeatureSide       ... Is the Side property exposed in the UI
            FeatureProcessing ... Are the processing check boxes supported by the operation
        ."""
        return FeatureSide | FeatureProcessing

    def getForm(self):
        """getForm() ... returns UI customized according to profileFeatures()"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpProfileFullEdit.ui")

        comboToPropertyMap = [("cutSide", "Side"), ("direction", "Direction")]
        enumTups = PathProfile.ObjectProfile.areaOpPropertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)
        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.extraOffsetSpinBox.updateWidget()
        self.thresholdSpinBox.updateWidget()
        self.stepoverSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

        if obj.Side != str(self.form.cutSide.currentData()):
            obj.Side = str(self.form.cutSide.currentData())
        if obj.Direction != str(self.form.direction.currentData()):
            obj.Direction = str(self.form.direction.currentData())

        self.extraOffsetSpinBox.updateProperty()
        self.thresholdSpinBox.updateProperty()
        self.stepoverSpinBox.updateProperty()

        obj.NumPasses = self.form.numPasses.value()

        if obj.UseComp != self.form.useCompensation.isChecked():
            obj.UseComp = self.form.useCompensation.isChecked()
        if obj.UseStartPoint != self.form.useStartPoint.isChecked():
            obj.UseStartPoint = self.form.useStartPoint.isChecked()

        if obj.processHoles != self.form.processHoles.isChecked():
            obj.processHoles = self.form.processHoles.isChecked()
        if obj.processPerimeter != self.form.processPerimeter.isChecked():
            obj.processPerimeter = self.form.processPerimeter.isChecked()
        if obj.processCircles != self.form.processCircles.isChecked():
            obj.processCircles = self.form.processCircles.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.updateQuantitySpinBoxes()
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

        self.selectInComboBox(obj.Side, self.form.cutSide)
        self.selectInComboBox(obj.Direction, self.form.direction)
        self.form.numPasses.setValue(obj.NumPasses)
        self.form.useCompensation.setChecked(obj.UseComp)
        self.form.useStartPoint.setChecked(obj.UseStartPoint)
        self.form.processHoles.setChecked(obj.processHoles)
        self.form.processPerimeter.setChecked(obj.processPerimeter)
        self.form.processCircles.setChecked(obj.processCircles)

        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        signals.append(self.form.cutSide.currentIndexChanged)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.extraOffset.editingFinished)
        signals.append(self.form.threshold.editingFinished)
        signals.append(self.form.numPasses.editingFinished)
        signals.append(self.form.stepover.editingFinished)
        if hasattr(self.form.useCompensation, "checkStateChanged"):  # Qt version >= 6.7.0
            signals.append(self.form.useCompensation.checkStateChanged)
            signals.append(self.form.useStartPoint.checkStateChanged)
            signals.append(self.form.processHoles.checkStateChanged)
            signals.append(self.form.processPerimeter.checkStateChanged)
            signals.append(self.form.processCircles.checkStateChanged)
        else:  # Qt version < 6.7.0
            signals.append(self.form.useCompensation.stateChanged)
            signals.append(self.form.useStartPoint.stateChanged)
            signals.append(self.form.processHoles.stateChanged)
            signals.append(self.form.processPerimeter.stateChanged)
            signals.append(self.form.processCircles.stateChanged)

        return signals

    def updateVisibility(self):
        hasFace = False
        objBase = list()

        if hasattr(self.obj, "Base"):
            objBase = self.obj.Base

        if objBase.__len__() > 0:
            for base, subsList in objBase:
                for sub in subsList:
                    if sub[:4] == "Face":
                        hasFace = True
                        break

        if hasFace:
            self.form.processCircles.show()
            self.form.processHoles.show()
            self.form.processPerimeter.show()
        else:
            self.form.processCircles.hide()
            self.form.processHoles.hide()
            self.form.processPerimeter.hide()

        self.form.stepover.setEnabled(self.obj.NumPasses > 1)

    def registerSignalHandlers(self, obj):
        if hasattr(self.form.useCompensation, "checkStateChanged"):  # Qt version >= 6.7.0
            self.form.useCompensation.checkStateChanged.connect(self.updateVisibility)
        else:  # Qt version < 6.7.0
            self.form.useCompensation.stateChanged.connect(self.updateVisibility)
        self.form.numPasses.editingFinished.connect(self.updateVisibility)

        self.form.setStartPoint.clicked.connect(self.setStartPoint)
        self.form.thresholdToggle.clicked.connect(self.thresholdToggle)

    def setStartPoint(self):
        selEx = FreeCADGui.Selection.getSelectionEx()
        if selEx and selEx[0].PickedPoints:
            point = selEx[0].PickedPoints[0]
            self.obj.StartPoint = point
            self.setDirty()
            Path.Log.info(
                translate("CAM_Profile", "Set start point: %s, %s")
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


# Eclass


Command = PathOpGui.SetupOperation(
    "Profile",
    PathProfile.Create,
    TaskPanelOpPage,
    "CAM_Profile",
    QT_TRANSLATE_NOOP("CAM_Profile", "Profile"),
    QT_TRANSLATE_NOOP("CAM_Profile", "Profile entire model, selected face(s) or selected edge(s)"),
    PathProfile.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathProfileGui ... done\n")
