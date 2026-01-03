# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger    <stefantroeger@gmx.net>           *
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

__title__ = "FreeCAD FEM mesh shape refinement task panel"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_mesh_region
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from . import base_femtaskpanel


class _TaskPanelShape(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of FemMeshRegion objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshShape.ui"
        )
        self.parameter_widget.setWindowTitle("Mesh size")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshShape.svg"))
        self._init_parameter_widget()

        # shape definition widget
        self.shape_widget = obj.ViewObject.createControlWidget()
        self.parameter_widget.setWindowTitle("Shape definition")
        self.shape_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshShape.svg"))
        self._init_shape_widget()

        self.parameter_widget.layout().addWidget(self.shape_widget)

        # form made from param and selection widget
        self.form = [self.parameter_widget]

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        ui.Shape.setCurrentText(self.obj.ShapeType)
        ui.Shape.currentTextChanged.connect(self.shapeChanged)

        ui.Shape.setItemIcon(0, FreeCADGui.getIcon(":icons/FEM_MeshBox.svg"))
        ui.Shape.setItemIcon(1, FreeCADGui.getIcon(":icons/FEM_MeshSphere.svg"))
        ui.Shape.setItemIcon(2, FreeCADGui.getIcon(":icons/FEM_MeshCylinder.svg"))

        ui.SizeIn.setProperty("value", self.obj.SizeIn)
        FreeCADGui.ExpressionBinding(ui.SizeIn).bind(self.obj, "SizeIn")
        ui.SizeIn.valueChanged.connect(self.sizeInChanged)

        ui.SizeOut.setProperty("value", self.obj.SizeOut)
        FreeCADGui.ExpressionBinding(ui.SizeOut).bind(self.obj, "SizeOut")
        ui.SizeOut.valueChanged.connect(self.sizeOutChanged)

        ui.Thickness.setProperty("value", self.obj.Thickness)
        FreeCADGui.ExpressionBinding(ui.Thickness).bind(self.obj, "Thickness")
        ui.Thickness.valueChanged.connect(self.thicknessChanged)

        if self.obj.ShapeType == "Cylinder":
            ui.Thickness.hide()
            ui.ThicknessLabel.hide()

    def _init_shape_widget(self):

        for i in range(self.shape_widget.count()):
            w = self.shape_widget.widget(i)

            if w.objectName() == f"ViewProvider{self.obj.ShapeType}ExtensionPython":
                self.shape_widget.setCurrentIndex(i)
                return

    @QtCore.Slot(str)
    def shapeChanged(self, value):
        self.obj.ShapeType = value
        self._init_shape_widget()

        if self.obj.ShapeType == "Cylinder":
            self.parameter_widget.Thickness.hide()
            self.parameter_widget.ThicknessLabel.hide()
        else:
            self.parameter_widget.Thickness.show()
            self.parameter_widget.ThicknessLabel.show()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeInChanged(self, value):
        self.obj.SizeIn = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeOutChanged(self, value):
        self.obj.SizeOut = value

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def thicknessChanged(self, value):
        self.obj.Thickness = value
