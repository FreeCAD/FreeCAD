# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM constraint electric charge density task panel"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_constraint_electricchargedensity
#  \ingroup FEM
#  \brief task panel for constraint electric charge density object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets

from femtools import membertools
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):

    def __init__(self, obj):
        super().__init__(obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElectricChargeDensity.ui"
        )

        self.init_parameter_widget()

        QtCore.QObject.connect(
            self.parameter_widget.qsb_source_charge_density,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.source_charge_density_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_interface_charge_density,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.interface_charge_density_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_total_charge,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.total_charge_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.cb_mode,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.mode_changed,
        )

        # geometry selection widget
        # start with Solid in list!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge"], False, False
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

        analysis = obj.getParentGroup()
        self._mesh = None
        self._part = None
        if analysis is not None:
            self._mesh = membertools.get_single_member(analysis, "Fem::FemMeshObject")
        if self._mesh is not None:
            self._part = self._mesh.Shape
        self._partVisible = None
        self._meshVisible = None

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self.restore_visibility()
        self.selection_widget.finish_selection()
        return super().reject()

    def accept(self):
        self.obj.References = self.selection_widget.references
        self.obj.SourceChargeDensity = self.source_charge_density
        self.obj.InterfaceChargeDensity = self.interface_charge_density
        self.obj.TotalCharge = self.total_charge
        self.obj.Mode = self.mode

        self.selection_widget.finish_selection()
        self.restore_visibility()
        return super().accept()

    def restore_visibility(self):
        if self._mesh is not None and self._part is not None:
            if self._meshVisible:
                self._mesh.ViewObject.show()
            else:
                self._mesh.ViewObject.hide()
            if self._partVisible:
                self._part.ViewObject.show()
            else:
                self._part.ViewObject.hide()

    def init_parameter_widget(self):
        self.source_charge_density = self.obj.SourceChargeDensity
        self.interface_charge_density = self.obj.InterfaceChargeDensity
        self.total_charge = self.obj.TotalCharge
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_source_charge_density).bind(
            self.obj, "SourceChargeDensity"
        )
        self.parameter_widget.qsb_source_charge_density.setProperty(
            "value", self.source_charge_density
        )

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_interface_charge_density).bind(
            self.obj, "InterfaceChargeDensity"
        )
        self.parameter_widget.qsb_interface_charge_density.setProperty(
            "value", self.interface_charge_density
        )

        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_total_charge).bind(
            self.obj, "TotalCharge"
        )
        self.parameter_widget.qsb_total_charge.setProperty("value", self.total_charge)

        self.mode = self.obj.Mode
        self.mode_enum = self.obj.getEnumerationsOfProperty("Mode")
        self.parameter_widget.cb_mode.addItems(self.mode_enum)
        index = self.mode_enum.index(self.mode)
        self.parameter_widget.cb_mode.setCurrentIndex(index)
        self.mode_changed(index)

    def source_charge_density_changed(self, base_quantity_value):
        self.source_charge_density = base_quantity_value

    def interface_charge_density_changed(self, base_quantity_value):
        self.interface_charge_density = base_quantity_value

    def total_charge_changed(self, base_quantity_value):
        self.total_charge = base_quantity_value

    def mode_changed(self, index):
        self.mode = self.mode_enum[index]
        if self.mode in ["Total Interface", "Total Source"]:
            index = 2
        self.parameter_widget.sw_mode.setCurrentIndex(index)
