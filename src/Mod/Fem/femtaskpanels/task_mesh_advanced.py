# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM mesh refinement task panel for the document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_mesh_math
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel
from . import base_fempreviewpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel, base_fempreviewpanel._TaskPanel):
    """
    The TaskPanel for editing References property of FemMeshMath objects
    """

    def __init__(self, obj):

        base_femtaskpanel._BaseTaskPanel.__init__(self, obj)
        base_fempreviewpanel._TaskPanel.__init__(self, obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshAdvanced.ui"
        )
        self.parameter_widget.setWindowTitle("Advanced refinement settings")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshAdvanced.svg"))
        self._init_parameter_widget()

        # geometry selection widget
        # only allow valid restriction objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Edge", "Vertex"], True, False
        )
        self.selection_widget.setWindowTitle("Reference geometries for AnisoCurve and Distance")
        self.selection_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshAdvanced.svg"))

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

        FreeCAD.addDocumentObserver(self)

    def _update_field_list(self):
        # update the available math fields

        self.parameter_widget.FieldList.clear()
        self.parameter_widget.FieldList_Aniso.clear()

        cnt = 1
        for obj in self.obj.Refinements:
            if not obj.Suppressed:
                self.parameter_widget.FieldList.addItem(f"F{cnt}: {obj.Label}")
                self.parameter_widget.FieldList_Aniso.addItem(f"F{cnt}: {obj.Label}")
                cnt += 1

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        # Type
        options = self.obj.getEnumerationsOfProperty("Type")
        ui.Widgets.setCurrentIndex(options.index(self.obj.Type))
        ui.Type.setCurrentText(self.obj.Type)
        ui.Type.currentTextChanged.connect(self.typeChanged)

        # AttractorAnisoCurve
        stylesheet = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").GetString("StyleSheet")
        if "dark" in stylesheet.lower():
            lightness = "Light"
        elif "light" in stylesheet.lower():
            lightness = "Dark"
        else:
            # use the qt style background and text color to detect the image to use
            palette = ui.palette()
            if palette.color(QtGui.QPalette.Text).lightness() > palette.color(QtGui.QPalette.Window).lightness():
                lightness = "Light"
            else:
                lightness = "Dark"

        ui.Diagram.setPixmap(QtGui.QPixmap(f":images/FEM_MeshDistanceThreshold{lightness}.svg"))

        ui.SizeMinNormal.setProperty("value", self.obj.SizeMinNormal)
        FreeCADGui.ExpressionBinding(ui.SizeMinNormal).bind(self.obj, "SizeMinNormal")
        ui.SizeMinNormal.valueChanged.connect(self.sizeMinNormalChanged)

        ui.SizeMaxNormal.setProperty("value", self.obj.SizeMaxNormal)
        FreeCADGui.ExpressionBinding(ui.SizeMaxNormal).bind(self.obj, "SizeMaxNormal")
        ui.SizeMaxNormal.valueChanged.connect(self.sizeMaxNormalChanged)

        ui.SizeMinTangent.setProperty("value", self.obj.SizeMinTangent)
        FreeCADGui.ExpressionBinding(ui.SizeMinTangent).bind(self.obj, "SizeMinTangent")
        ui.SizeMinTangent.valueChanged.connect(self.sizeMinTangentChanged)

        ui.SizeMaxTangent.setProperty("value", self.obj.SizeMaxTangent)
        FreeCADGui.ExpressionBinding(ui.SizeMaxTangent).bind(self.obj, "SizeMaxTangent")
        ui.SizeMaxTangent.valueChanged.connect(self.sizeMaxTangentChanged)

        ui.DistMin.setProperty("value", self.obj.DistanceMin)
        FreeCADGui.ExpressionBinding(ui.DistMin).bind(self.obj, "DistanceMin")
        ui.DistMin.valueChanged.connect(self.distMinChanged)

        ui.DistMax.setProperty("value", self.obj.DistanceMax)
        FreeCADGui.ExpressionBinding(ui.DistMax).bind(self.obj, "DistanceMax")
        ui.DistMax.valueChanged.connect(self.distMaxChanged)

        self._block_sampling_update = False
        ui.Sampling_Aniso.setProperty("value", self.obj.Sampling)
        FreeCADGui.ExpressionBinding(ui.Sampling_Aniso).bind(self.obj, "Sampling")
        ui.Sampling_Aniso.valueChanged.connect(self.samplingChanged)

        warning = FreeCADGui.getIcon("Warning.svg")
        ui.WarnIcon_Curve.setPixmap(warning.pixmap(QtCore.QSize(32,32)))

        # math
        ui.Equation.setText(self.obj.Equation)
        ui.Equation.editingFinished.connect(self.equationChanged)

        info = FreeCADGui.getIcon("info.svg")
        ui.Icon.setPixmap(info.pixmap(QtCore.QSize(32,32)))
        ui.EqIcon.setPixmap(info.pixmap(QtCore.QSize(32,32)))

        # math ansio
        ui.Icon_Aniso.setPixmap(info.pixmap(QtCore.QSize(32,32)))

        ui.M11.setText(self.obj.M11)
        ui.M11.editingFinished.connect(self.anisoEquationChanged)
        ui.M12.setText(self.obj.M12)
        ui.M12.editingFinished.connect(self.anisoEquationChanged)
        ui.M13.setText(self.obj.M13)
        ui.M13.editingFinished.connect(self.anisoEquationChanged)

        ui.M22.setText(self.obj.M22)
        ui.M22.editingFinished.connect(self.anisoEquationChanged)
        ui.M23.setText(self.obj.M23)
        ui.M23.editingFinished.connect(self.anisoEquationChanged)
        ui.M33.setText(self.obj.M33)
        ui.M33.editingFinished.connect(self.anisoEquationChanged)

        ui.WarnIcon_Math.setPixmap(warning.pixmap(QtCore.QSize(32,32)))
        self._update_field_list()

        # Distance
        ui.Sampling_Dist.setProperty("value", self.obj.Sampling)
        FreeCADGui.ExpressionBinding(ui.Sampling_Dist).bind(self.obj, "Sampling")
        ui.Sampling_Dist.valueChanged.connect(self.samplingChanged)


        # Result

        # find all possible result objects
        ui.ResultObject.addItem(FreeCADGui.getIcon("button_invalid.svg"), "None selected", None)
        for res_obj in self.obj.Document.Objects:
            if res_obj.isDerivedFrom("Fem::FemPostObject"):
                icon = res_obj.ViewObject.Icon
                ui.ResultObject.addItem(icon, res_obj.Label, res_obj)

        # set selection
        if self.obj.ResultObject:
            ui.ResultObject.setCurrentText(self.obj.ResultObject.Label)
            ui.ResultField.addItems(self.obj.ResultObject.ViewObject.getEnumerationsOfProperty("Field"))
            ui.ResultField.setCurrentText(self.obj.ResultField)
        else:
            ui.ResultField.addItem(FreeCADGui.getIcon("button_invalid.svg"), "No object selected")


        ui.ResultObject.currentIndexChanged.connect(self.resultObjectChanged)
        ui.ResultField.currentTextChanged.connect(self.resultFieldChanged)

        # add the preview widget
        ui.layout().addWidget(self.preview_widget())


    def slotChangedObject(self, obj, prop):
        # callback of document observer for changed property
        if (obj == self.obj) and (prop == "Refinements"):
            self._update_field_list()

        # if some childrens suppress value is changed we also need to recreate the list
        if (obj in self.obj.Refinements) and (prop == "Suppressed"):
            self._update_field_list()

    def accept(self):
        FreeCAD.removeDocumentObserver(self)
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().accept()

    def reject(self):
        FreeCAD.removeDocumentObserver(self)
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().reject()

    @QtCore.Slot(str)
    def typeChanged(self, value):
        self.obj.Type = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMaxChanged(self, value):
        self.obj.DistanceMax = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMinChanged(self, value):
        self.obj.DistanceMin = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMaxNormalChanged(self, value):
        self.obj.SizeMaxNormal = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMinNormalChanged(self, value):
        self.obj.SizeMinNormal = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMaxTangentChanged(self, value):
        self.obj.SizeMaxTangent = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMinTangentChanged(self, value):
        self.obj.SizeMinTangent = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def samplingChanged(self, value):
        if not self._block_sampling_update:
            self.obj.Sampling = int(value)
            self.update_preview()

            self._block_sampling_update = True
            self.parameter_widget.Sampling_Aniso.setValue(value)
            self.parameter_widget.Sampling_Dist.setValue(value)
            self._block_sampling_update = False

    @QtCore.Slot()
    def equationChanged(self):
        self.obj.Equation = self.parameter_widget.Equation.text()
        self.update_preview()

    @QtCore.Slot()
    def anisoEquationChanged(self):
        self.obj.M11 = self.parameter_widget.M11.text()
        self.obj.M12 = self.parameter_widget.M12.text()
        self.obj.M13 = self.parameter_widget.M13.text()
        self.obj.M22 = self.parameter_widget.M22.text()
        self.obj.M23 = self.parameter_widget.M23.text()
        self.obj.M33 = self.parameter_widget.M33.text()
        self.update_preview()

    @QtCore.Slot(int)
    def resultObjectChanged(self, value):

        old_field = self.obj.ResultField
        self.obj.ResultObject = self.parameter_widget.ResultObject.itemData(value)
        fields = self.obj.ResultObject.ViewObject.getEnumerationsOfProperty("Field")

        self.parameter_widget.ResultField.clear()
        self.parameter_widget.ResultField.addItems(fields)
        if old_field in fields:
            self.parameter_widget.ResultField.setCurrentText(old_field)

        self.update_preview()

    @QtCore.Slot(int)
    def resultFieldChanged(self, value):
        self.obj.ResultField = value
        self.update_preview()

