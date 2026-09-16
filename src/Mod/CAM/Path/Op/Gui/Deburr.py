# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 sliptonic <shopinthewoods@gmail.com>
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
from Path.Base.Gui.Util import QuantitySpinBox
import Path.Op.Deburr as PathDeburr
import Path.Op.Gui.Base as PathOpGui
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Deburr Operation UI"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "https://www.freecad.org"
__doc__ = "Deburr operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Deburr operation."""

    def initPage(self, obj):
        self.chamferWidthSpinBox = QuantitySpinBox(self.form.chamferWidth, obj, "Width")
        self.extraDepthSpinBox = QuantitySpinBox(self.form.extraDepth, obj, "ExtraDepth")

    def getToolTipList(self):
        """getToolTipList() ... Collect list of tuples (widget_name: str, property_name: str)"""
        tuples = []
        tuples.append(("chamferWidth", "Width"))
        tuples.append(("direction", "Direction"))
        tuples.append(("extraDepth", "ExtraDepth"))
        tuples.append(("processCircles", "ProcessCircles"))
        tuples.append(("processHoles", "ProcessHoles"))
        tuples.append(("processPerimeter", "ProcessPerimeter"))
        tuples.append(("side", "Side"))
        tuples.append(("sorting", "SortingMode"))

        return tuples

    def getForm(self):
        """getForm() ... returns UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpDeburrEdit.ui")

        comboToPropertyMap = [
            ("direction", "Direction"),
            ("side", "Side"),
            ("sorting", "SortingMode"),
        ]
        enumTups = PathDeburr.ObjectDeburr.propertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.chamferWidthSpinBox.updateWidget()
        self.extraDepthSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.chamferWidthSpinBox.updateProperty()
        self.extraDepthSpinBox.updateProperty()

        if obj.Direction != str(self.form.direction.currentData()):
            obj.Direction = str(self.form.direction.currentData())
        if obj.ProcessCircles != self.form.processCircles.isChecked():
            obj.ProcessCircles = self.form.processCircles.isChecked()
        if obj.ProcessHoles != self.form.processHoles.isChecked():
            obj.ProcessHoles = self.form.processHoles.isChecked()
        if obj.ProcessPerimeter != self.form.processPerimeter.isChecked():
            obj.ProcessPerimeter = self.form.processPerimeter.isChecked()
        if obj.Side != str(self.form.side.currentData()):
            obj.Side = str(self.form.side.currentData())
        if obj.SortingMode != str(self.form.sorting.currentData()):
            obj.SortingMode = str(self.form.sorting.currentData())

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.updateQuantitySpinBoxes()

        self.selectInComboBox(obj.Direction, self.form.direction)
        self.selectInComboBox(obj.Side, self.form.side)
        self.selectInComboBox(obj.SortingMode, self.form.sorting)
        self.form.processCircles.setChecked(obj.ProcessCircles)
        self.form.processHoles.setChecked(obj.ProcessHoles)
        self.form.processPerimeter.setChecked(obj.ProcessPerimeter)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.chamferWidth.valueChanged)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.extraDepth.valueChanged)
        signals.append(self.form.processCircles.checkStateChanged)
        signals.append(self.form.processHoles.checkStateChanged)
        signals.append(self.form.processPerimeter.checkStateChanged)
        signals.append(self.form.side.currentIndexChanged)
        signals.append(self.form.sorting.currentIndexChanged)

        return signals


Command = PathOpGui.SetupOperation(
    "Deburr",
    PathDeburr.Create,
    TaskPanelOpPage,
    "CAM_Deburr",
    QT_TRANSLATE_NOOP("CAM_Deburr", "Deburr"),
    QT_TRANSLATE_NOOP(
        "CAM_Deburr",
        "Creates a Deburr toolpath along Edges or around Faces"
        "\n\nSelect horizontal edges\nor horizontal face from shape\nor angled face of chamfer",
    ),
    PathDeburr.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathDeburrGui... done\n")
