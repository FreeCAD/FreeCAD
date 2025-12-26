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

## @package task_mesh_restrict
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
    The TaskPanel for editing References property of FemMeshRegion objects
    """

    def __init__(self, obj):

        base_femtaskpanel._BaseTaskPanel.__init__(self, obj)
        base_fempreviewpanel._TaskPanel.__init__(self, obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshManipulate.ui"
        )
        self.parameter_widget.setWindowTitle("Manipulation settings")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshManipulate.svg"))
        self._init_parameter_widget()

        # geometry selection widget
        # only allow valid restriction objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Edge", "Vertex", "Solid"], True, False
        )
        self.selection_widget.setWindowTitle("Reference geometries for restriction")
        self.selection_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshManipulate.svg"))

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        options = self.obj.getEnumerationsOfProperty("Type")
        ui.Widgets.setCurrentIndex(options.index(self.obj.Type))
        ui.Type.setCurrentText(self.obj.Type)
        ui.Type.currentTextChanged.connect(self.typeChanged)

        # Restrict
        # ########
        ui.Boundary.setChecked(self.obj.IncludeBoundary)
        ui.Boundary.toggled.connect(self.boundaryChanged)

        info = FreeCADGui.getIcon("info.svg")
        ui.Icon.setPixmap(info.pixmap(QtCore.QSize(32,32)))

        # Threshold
        # #########

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

        ui.SizeMin.setProperty("value", self.obj.SizeMinimum)
        FreeCADGui.ExpressionBinding(ui.SizeMin).bind(self.obj, "SizeMinimum")
        ui.SizeMin.valueChanged.connect(self.sizeMinChanged)

        ui.SizeMax.setProperty("value", self.obj.SizeMaximum)
        FreeCADGui.ExpressionBinding(ui.SizeMax).bind(self.obj, "SizeMaximum")
        ui.SizeMax.valueChanged.connect(self.sizeMaxChanged)

        ui.DistMin.setProperty("value", self.obj.InputMinimum)
        FreeCADGui.ExpressionBinding(ui.DistMin).bind(self.obj, "InputMinimum")
        ui.DistMin.valueChanged.connect(self.distMinChanged)

        ui.DistMax.setProperty("value", self.obj.InputMaximum)
        FreeCADGui.ExpressionBinding(ui.DistMax).bind(self.obj, "InputMaximum")
        ui.DistMax.valueChanged.connect(self.distMaxChanged)

        ui.Linear.setChecked(self.obj.LinearInterpolation)
        ui.Linear.toggled.connect(self.linearChanged)

        ui.Stop.setChecked(self.obj.StopAtInputMax)
        ui.Stop.toggled.connect(self.stopChanged)

        # gradient + mean  + Curvature + Laplacian
        # ########################################
        self._block_delta_update = False

        ui.Gradient_Kind.setCurrentText(self.obj.Kind)
        ui.Gradient_Kind.currentTextChanged.connect(self.kindChanged)

        ui.Mean_Delta.setProperty("value", self.obj.Delta)
        FreeCADGui.ExpressionBinding(ui.Mean_Delta).bind(self.obj, "Delta")
        ui.Mean_Delta.valueChanged.connect(self.deltaChanged)

        ui.Gradient_Delta.setProperty("value", self.obj.Delta)
        FreeCADGui.ExpressionBinding(ui.Gradient_Delta).bind(self.obj, "Delta")
        ui.Gradient_Delta.valueChanged.connect(self.deltaChanged)

        ui.Curvature_Delta.setProperty("value", self.obj.Delta)
        FreeCADGui.ExpressionBinding(ui.Curvature_Delta).bind(self.obj, "Delta")
        ui.Curvature_Delta.valueChanged.connect(self.deltaChanged)

        ui.Laplacian_Delta.setProperty("value", self.obj.Delta)
        FreeCADGui.ExpressionBinding(ui.Laplacian_Delta).bind(self.obj, "Delta")
        ui.Laplacian_Delta.valueChanged.connect(self.deltaChanged)

        # add the preview widget
        ui.layout().addWidget(self.preview_widget())

    def accept(self):
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().reject()

    @QtCore.Slot(str)
    def typeChanged(self, value):
        self.obj.Type = value
        self.update_preview()

    @QtCore.Slot(bool)
    def boundaryChanged(self, value):
        self.obj.IncludeBoundary = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMaxChanged(self, value):
        self.obj.InputMaximum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMinChanged(self, value):
        self.obj.InputMinimum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMaxChanged(self, value):
        self.obj.SizeMaximum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMinChanged(self, value):
        self.obj.SizeMinimum = value
        self.update_preview()

    @QtCore.Slot(bool)
    def linearChanged(self, value):
        self.obj.LinearInterpolation = value
        self.update_preview()

    @QtCore.Slot(bool)
    def stopChanged(self, value):
        self.obj.StopAtInputMax = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def deltaChanged(self, value):
        if not self._block_delta_update:
            self.obj.Delta = value

            self._block_delta_update = True
            self.parameter_widget.Mean_Delta.setProperty("value", value)
            self.parameter_widget.Gradient_Delta.setProperty("value", value)
            self.parameter_widget.Curvature_Delta.setProperty("value", value)
            self.parameter_widget.Laplacian_Delta.setProperty("value", value)
            self._block_delta_update = False
            self.update_preview()

    @QtCore.Slot(str)
    def kindChanged(self, value):
        self.obj.Kind = value
        self.update_preview()
