# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM constraint electrostatic potential task panel for the document object"
__author__ = "Markus Hovorka, Bernd Hahnebach, Uwe Stöhr, André Kapelrud, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_constraint_electrostaticpotential
#  \ingroup FEM
#  \brief task panel for constraint electrostatic potential object

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
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElectrostaticPotential.ui"
        )

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge", "Vertex"], True, False
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self._selectionWidget]

        analysis = obj.getParentGroup()
        self._mesh = None
        self._part = None
        if analysis is not None:
            self._mesh = membertools.get_single_member(analysis, "Fem::FemMeshObject")
        if self._mesh is not None:
            self._part = self._mesh.Shape
        self._partVisible = None
        self._meshVisible = None

        QtCore.QObject.connect(
            self.parameter_widget.ckb_electromagnetic,
            QtCore.SIGNAL("toggled(bool)"),
            self.electromagnetic_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.cb_boundary_condition,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.boundary_condition_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_potential,
            QtCore.SIGNAL("toggled(bool)"),
            self.potential_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_potential,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.potential_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_1,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_1_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_2,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_2_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_3,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_3_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_capacitance_body,
            QtCore.SIGNAL("toggled(bool)"),
            self.capacitance_body_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.spb_capacitance_body,
            QtCore.SIGNAL("valueChanged(int)"),
            self.capacitance_body_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_potential_constant,
            QtCore.SIGNAL("toggled(bool)"),
            self.potential_constant_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_electric_infinity,
            QtCore.SIGNAL("toggled(bool)"),
            self.electric_infinity_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_electric_flux_density,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.electric_flux_density_changed,
        )

        self.init_parameter_widget()

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self._restoreVisibility()
        self._selectionWidget.finish_selection()
        return super().reject()

    def accept(self):
        if self.obj.References != self._selectionWidget.references:
            self.obj.References = self._selectionWidget.references
        self._set_params()
        self._selectionWidget.finish_selection()
        self._restoreVisibility()
        return super().accept()

    def _restoreVisibility(self):
        if self._mesh is not None and self._part is not None:
            if self._meshVisible:
                self._mesh.ViewObject.show()
            else:
                self._mesh.ViewObject.hide()
            if self._partVisible:
                self._part.ViewObject.show()
            else:
                self._part.ViewObject.hide()

    def _get_params(self):
        self.potential = self.obj.Potential
        self.potential_enabled = self.obj.PotentialEnabled

        self.av_re = self.obj.AV_re
        self.av_re_1 = self.obj.AV_re_1
        self.av_re_2 = self.obj.AV_re_2
        self.av_re_3 = self.obj.AV_re_3
        self.av_im = self.obj.AV_im
        self.av_im_1 = self.obj.AV_im_1
        self.av_im_2 = self.obj.AV_im_2
        self.av_im_3 = self.obj.AV_im_3

        self.av_enabled = self.obj.EnableAV
        self.av_1_enabled = self.obj.EnableAV_1
        self.av_2_enabled = self.obj.EnableAV_2
        self.av_3_enabled = self.obj.EnableAV_3

        self.boundary_condition = self.obj.BoundaryCondition
        self.potential_constant = self.obj.PotentialConstant
        self.electric_infinity = self.obj.ElectricInfinity
        self.capacitance_body_enabled = self.obj.CapacitanceBodyEnabled
        self.capacitance_body = self.obj.CapacitanceBody
        self.electric_flux_density = self.obj.ElectricFluxDensity

    def _set_params(self):
        self.obj.Potential = self.potential
        self.obj.PotentialEnabled = self.potential_enabled

        self.obj.AV_re = self.av_re
        self.obj.AV_re_1 = self.av_re_1
        self.obj.AV_re_2 = self.av_re_2
        self.obj.AV_re_3 = self.av_re_3
        self.obj.AV_im = self.av_im
        self.obj.AV_im_1 = self.av_im_1
        self.obj.AV_im_2 = self.av_im_2
        self.obj.AV_im_3 = self.av_im_3

        self.obj.EnableAV = self.av_enabled
        self.obj.EnableAV_1 = self.av_1_enabled
        self.obj.EnableAV_2 = self.av_2_enabled
        self.obj.EnableAV_3 = self.av_3_enabled

        self.obj.BoundaryCondition = self.boundary_condition
        self.obj.PotentialConstant = self.potential_constant
        self.obj.ElectricInfinity = self.electric_infinity
        self.obj.CapacitanceBodyEnabled = self.capacitance_body_enabled
        self.obj.CapacitanceBody = self.capacitance_body

        self.obj.ElectricFluxDensity = self.electric_flux_density

    def init_parameter_widget(self):
        self._get_params()

        self.parameter_widget.qsb_potential.setProperty("value", self.potential)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_potential).bind(
            self.obj, "Potential"
        )
        self.parameter_widget.ckb_potential.setChecked(self.potential_enabled)

        # scalar potential
        self.parameter_widget.qsb_av_re.setProperty("value", self.av_re)
        self.parameter_widget.qsb_av_re.setEnabled(self.av_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re).bind(self.obj, "AV_re")
        self.parameter_widget.qsb_av_im.setProperty("value", self.av_im)
        self.parameter_widget.qsb_av_im.setEnabled(self.av_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im).bind(self.obj, "AV_im")

        # vector potential
        self.parameter_widget.qsb_av_re_1.setProperty("value", self.av_re_1)
        self.parameter_widget.qsb_av_re_1.setEnabled(self.av_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_1).bind(self.obj, "AV_re_1")
        self.parameter_widget.qsb_av_re_2.setProperty("value", self.av_re_2)
        self.parameter_widget.qsb_av_re_2.setEnabled(self.av_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_2).bind(self.obj, "AV_re_2")
        self.parameter_widget.qsb_av_re_3.setProperty("value", self.av_re_3)
        self.parameter_widget.qsb_av_re_3.setEnabled(self.av_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_3).bind(self.obj, "AV_re_3")

        self.parameter_widget.qsb_av_im_1.setProperty("value", self.av_im_1)
        self.parameter_widget.qsb_av_im_1.setEnabled(self.av_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_1).bind(self.obj, "AV_im_1")
        self.parameter_widget.qsb_av_im_2.setProperty("value", self.av_im_2)
        self.parameter_widget.qsb_av_im_2.setEnabled(self.av_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_2).bind(self.obj, "AV_im_2")
        self.parameter_widget.qsb_av_im_3.setProperty("value", self.av_im_3)
        self.parameter_widget.qsb_av_im_3.setEnabled(self.av_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_3).bind(self.obj, "AV_im_3")

        self.parameter_widget.ckb_av.setChecked(self.av_enabled)
        self.parameter_widget.ckb_av_1.setChecked(self.av_1_enabled)
        self.parameter_widget.ckb_av_2.setChecked(self.av_2_enabled)
        self.parameter_widget.ckb_av_3.setChecked(self.av_3_enabled)

        self.parameter_widget.ckb_potential_constant.setChecked(self.potential_constant)

        self.parameter_widget.ckb_electric_infinity.setChecked(self.electric_infinity)

        self.parameter_widget.ckb_capacitance_body.setChecked(self.capacitance_body_enabled)
        self.parameter_widget.spb_capacitance_body.setProperty("value", self.capacitance_body)
        FreeCADGui.ExpressionBinding(self.parameter_widget.spb_capacitance_body).bind(
            self.obj, "CapacitanceBody"
        )

        self.parameter_widget.qsb_electric_flux_density.setProperty(
            "value", self.electric_flux_density
        )
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_electric_flux_density).bind(
            self.obj, "ElectricFluxDensity"
        )

        self.bc_enum = self.obj.getEnumerationsOfProperty("BoundaryCondition")
        index = self.bc_enum.index(self.boundary_condition)
        self.parameter_widget.cb_boundary_condition.addItems(self.bc_enum)
        self.parameter_widget.cb_boundary_condition.setCurrentIndex(index)

        # start with electromagnetic inputs hidden if no field is set
        if not (self.av_enabled or self.av_1_enabled or self.av_2_enabled or self.av_3_enabled):
            self.parameter_widget.ckb_electromagnetic.setChecked(False)

    def potential_changed(self, value):
        self.potential = value

    def potential_enabled_changed(self, value):
        self.potential_enabled = value
        self.parameter_widget.qsb_potential.setEnabled(value)

    def electromagnetic_enabled_changed(self, value):
        self.parameter_widget.gb_electromagnetic.setVisible(value)

    def av_enabled_changed(self, value):
        self.av_enabled = value
        self.parameter_widget.qsb_av_re.setEnabled(value)
        self.parameter_widget.qsb_av_im.setEnabled(value)

    def av_1_enabled_changed(self, value):
        self.av_1_enabled = value
        self.parameter_widget.qsb_av_re_1.setEnabled(value)
        self.parameter_widget.qsb_av_im_1.setEnabled(value)

    def av_2_enabled_changed(self, value):
        self.av_2_enabled = value
        self.parameter_widget.qsb_av_re_2.setEnabled(value)
        self.parameter_widget.qsb_av_im_2.setEnabled(value)

    def av_3_enabled_changed(self, value):
        self.av_3_enabled = value
        self.parameter_widget.qsb_av_re_3.setEnabled(value)
        self.parameter_widget.qsb_av_im_3.setEnabled(value)

    def av_re_changed(self, value):
        self.av_re = value

    def av_re_1_changed(self, value):
        self.av_re_1 = value

    def av_re_2_changed(self, value):
        self.av_re_2 = value

    def av_re_3_changed(self, value):
        self.av_re_3 = value

    def av_im_changed(self, value):
        self.av_im = value

    def av_im_1_changed(self, value):
        self.av_im_1 = value

    def av_im_2_changed(self, value):
        self.av_im_2 = value

    def av_im_3_changed(self, value):
        self.av_im_3 = value

    def potential_constant_changed(self, value):
        self.potential_constant = value

    def electric_infinity_changed(self, value):
        self.electric_infinity = value

    def capacitance_body_enabled_changed(self, value):
        self.capacitance_body_enabled = value
        self.parameter_widget.spb_capacitance_body.setEnabled(value)

    def capacitance_body_changed(self, value):
        self.capacitance_body = value
        self.parameter_widget.spb_capacitance_body.setValue(value)

    def electric_flux_density_changed(self, value):
        self.electric_flux_density = value

    def boundary_condition_changed(self, index):
        self.boundary_condition = self.bc_enum[index]
        if self.boundary_condition == "Dirichlet":
            self.parameter_widget.gb_neumann.setEnabled(False)
            self.parameter_widget.gb_dirichlet.setEnabled(True)
        elif self.boundary_condition == "Neumann":
            self.parameter_widget.gb_neumann.setEnabled(True)
            self.parameter_widget.gb_dirichlet.setEnabled(False)
