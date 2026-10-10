# SPDX-License-Identifier: LGPL-2.1-or-later
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
import Path.Op.Deburring as PathDeburring
import Path.Op.Gui.Base as PathOpGui
from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Deburring Operation UI"
__url__ = "https://www.freecad.org"
__doc__ = "Deburring operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Deburring operation."""

    def initPage(self, obj):
        self.chamferAngleSpinBox = QuantitySpinBox(self.form.chamferAngle, obj, "Angle")
        self.chamferWidthSpinBox = QuantitySpinBox(self.form.chamferWidth, obj, "Width")
        self.radialSpinBox = QuantitySpinBox(self.form.radialOffset, obj, "RadialStockToLeave")
        self.extraDepthSpinBox = QuantitySpinBox(self.form.extraDepth, obj, "ExtraDepth")

    def getToolTipList(self):
        """getToolTipList() ... Collect list of tuples (widget_name: str, property_name: str)"""
        tuples = []
        tuples.append(("chamferAngle", "Angle"))
        tuples.append(("chamferWidth", "Width"))
        tuples.append(("direction", "Direction"))
        tuples.append(("extraDepth", "ExtraDepth"))
        tuples.append(("fromBottom", "StartFromBottom"))
        tuples.append(("processCircles", "ProcessCircles"))
        tuples.append(("processHoles", "ProcessHoles"))
        tuples.append(("processPerimeter", "ProcessPerimeter"))
        tuples.append(("radialOffset", "RadialStockToLeave"))
        tuples.append(("side", "Side"))
        tuples.append(("sorting", "SortingMode"))
        tuples.append(("splitArcs", "SplitArcs"))
        tuples.append(("strategy", "Strategy"))

        return tuples

    def getForm(self):
        """getForm() ... returns UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpDeburringEdit.ui")

        comboToPropertyMap = [
            ("direction", "Direction"),
            ("side", "Side"),
            ("sorting", "SortingMode"),
            ("strategy", "Strategy"),
        ]
        enumTups = PathDeburring.ObjectDeburring.propertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def updateQuantitySpinBoxes(self, index=None):
        self.chamferAngleSpinBox.updateWidget()
        self.chamferWidthSpinBox.updateWidget()
        self.radialSpinBox.updateWidget()
        self.extraDepthSpinBox.updateWidget()

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        self.chamferAngleSpinBox.updateProperty()
        self.chamferWidthSpinBox.updateProperty()
        self.radialSpinBox.updateProperty()
        self.extraDepthSpinBox.updateProperty()

        if obj.Strategy != str(self.form.strategy.currentData()):
            obj.Strategy = str(self.form.strategy.currentData())
        if obj.Direction != str(self.form.direction.currentData()):
            obj.Direction = str(self.form.direction.currentData())
        if obj.StartFromBottom != self.form.fromBottom.isChecked():
            obj.StartFromBottom = self.form.fromBottom.isChecked()
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
        if obj.SplitArcs != self.form.splitArcs.isChecked():
            obj.SplitArcs = self.form.splitArcs.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.updateQuantitySpinBoxes()

        self.selectInComboBox(obj.Strategy, self.form.strategy)
        self.selectInComboBox(obj.Direction, self.form.direction)
        self.selectInComboBox(obj.Side, self.form.side)
        self.selectInComboBox(obj.SortingMode, self.form.sorting)
        self.form.fromBottom.setChecked(obj.StartFromBottom)
        self.form.processCircles.setChecked(obj.ProcessCircles)
        self.form.processHoles.setChecked(obj.ProcessHoles)
        self.form.processPerimeter.setChecked(obj.ProcessPerimeter)
        self.form.splitArcs.setChecked(obj.SplitArcs)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.chamferAngle.valueChanged)
        signals.append(self.form.chamferWidth.valueChanged)
        signals.append(self.form.direction.currentIndexChanged)
        signals.append(self.form.radialOffset.valueChanged)
        signals.append(self.form.extraDepth.valueChanged)
        signals.append(self.form.fromBottom.checkStateChanged)
        signals.append(self.form.processCircles.checkStateChanged)
        signals.append(self.form.processHoles.checkStateChanged)
        signals.append(self.form.processPerimeter.checkStateChanged)
        signals.append(self.form.side.currentIndexChanged)
        signals.append(self.form.sorting.currentIndexChanged)
        signals.append(self.form.splitArcs.checkStateChanged)
        signals.append(self.form.strategy.currentIndexChanged)

        return signals

    def updateData(self, obj, prop):
        tools = ["ballend", "bullnose", "endmill"]
        toolShape = self.obj.ToolController.Tool.ShapeID

        if self.obj.Strategy == "Chamfer":
            self.form.label_chamferAngle.show()
            self.form.chamferAngle.show()
        else:
            self.form.label_chamferAngle.hide()
            self.form.chamferAngle.hide()

        if self.obj.Strategy == "Chamfer":
            self.form.label_chamferWidth.setText("Chamfer width")
        else:
            self.form.label_chamferWidth.setText("Fillet radius")

        geo = any(
            Path.Geom.isHorizontal(base.Shape.getElement(n))
            for base, names in self.obj.Base
            for n in names
            if n.startswith("Face")
        )
        self.form.processCircles.setEnabled(geo)
        self.form.processHoles.setEnabled(geo)
        self.form.processPerimeter.setEnabled(geo)

        self.form.chamferAngle.setEnabled(toolShape in tools)
        self.form.fromBottom.setEnabled(toolShape in tools)


Command = PathOpGui.SetupOperation(
    "Deburring",
    PathDeburring.Create,
    TaskPanelOpPage,
    "CAM_Deburr",
    QT_TRANSLATE_NOOP("CAM_Deburring", "Deburring"),
    QT_TRANSLATE_NOOP(
        "CAM_Deburring",
        "Creates a Deburring toolpath along Edges or around Faces"
        "\n\nSelect horizontal edges\nor horizontal face from shape\nor angled face of chamfer",
    ),
    PathDeburring.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathDeburring Gui... done\n")
