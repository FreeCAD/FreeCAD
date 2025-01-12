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

        # start with vector inputs hidden if no vector is set
        if (
            self.obj.AV_re_1_Disabled
            and self.obj.AV_re_2_Disabled
            and self.obj.AV_re_3_Disabled
            and self.obj.AV_im_Disabled
            and self.obj.AV_im_1_Disabled
            and self.obj.AV_im_2_Disabled
            and self.obj.AV_im_3_Disabled
        ):
            self._vectorField_visibility(False)
            self.parameter_widget.vectorFieldBox.setChecked(False)

        QtCore.QObject.connect(
            self.parameter_widget.vectorFieldBox,
            QtCore.SIGNAL("toggled(bool)"),
            self._vectorField_visibility,
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
            self.parameter_widget.ckb_av_re,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_re_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_re_1,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_re_1_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_re_2,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_re_2_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_re_3,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_re_3_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_re_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_re_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_im,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_im_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_im_1,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_im_1_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_im_2,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_im_2_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_av_im_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.av_im_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_av_im_3,
            QtCore.SIGNAL("toggled(bool)"),
            self.av_im_3_enabled_changed,
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
            self.parameter_widget.ckb_electric_forcecalculation,
            QtCore.SIGNAL("toggled(bool)"),
            self.electric_forcecalculation_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_surface_charge_density,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.surface_charge_density_changed,
        )

        self.init_parameter_widget()

    def _vectorField_visibility(self, visible):
        self.parameter_widget.vectorFieldGB.setVisible(visible)

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

        self.av_re_1 = self.obj.AV_re_1
        self.av_re_2 = self.obj.AV_re_2
        self.av_re_3 = self.obj.AV_re_3
        self.av_im = self.obj.AV_im
        self.av_im_1 = self.obj.AV_im_1
        self.av_im_2 = self.obj.AV_im_2
        self.av_im_3 = self.obj.AV_im_3

        self.av_re_enabled = not self.obj.PotentialEnabled
        self.av_re_1_enabled = not self.obj.AV_re_1_Disabled
        self.av_re_2_enabled = not self.obj.AV_re_2_Disabled
        self.av_re_3_enabled = not self.obj.AV_re_3_Disabled
        self.av_im_enabled = not self.obj.AV_im_Disabled
        self.av_im_1_enabled = not self.obj.AV_im_1_Disabled
        self.av_im_2_enabled = not self.obj.AV_im_2_Disabled
        self.av_im_3_enabled = not self.obj.AV_im_3_Disabled

        self.boundary_condition = self.obj.BoundaryCondition
        self.potential_constant = self.obj.PotentialConstant
        self.electric_infinity = self.obj.ElectricInfinity
        self.electric_forcecalculation = self.obj.ElectricForcecalculation
        self.capacitance_body_enabled = self.obj.CapacitanceBodyEnabled
        self.capacitance_body = self.obj.CapacitanceBody

        self.surface_charge_density = self.obj.SurfaceChargeDensity

    def _set_params(self):
        self.obj.Potential = self.potential
        self.obj.PotentialEnabled = self.potential_enabled

        self.obj.AV_re_1 = self.av_re_1
        self.obj.AV_re_2 = self.av_re_2
        self.obj.AV_re_3 = self.av_re_3
        self.obj.AV_im = self.av_im
        self.obj.AV_im_1 = self.av_im_1
        self.obj.AV_im_2 = self.av_im_2
        self.obj.AV_im_3 = self.av_im_3

        self.obj.AV_re_1_Disabled = not self.av_re_1_enabled
        self.obj.AV_re_2_Disabled = not self.av_re_2_enabled
        self.obj.AV_re_3_Disabled = not self.av_re_3_enabled
        self.obj.AV_im_Disabled = not self.av_im_enabled
        self.obj.AV_im_1_Disabled = not self.av_im_1_enabled
        self.obj.AV_im_2_Disabled = not self.av_im_2_enabled
        self.obj.AV_im_3_Disabled = not self.av_im_3_enabled

        self.obj.BoundaryCondition = self.boundary_condition
        self.obj.PotentialConstant = self.potential_constant
        self.obj.ElectricInfinity = self.electric_infinity
        self.obj.ElectricForcecalculation = self.electric_forcecalculation
        self.obj.CapacitanceBodyEnabled = self.capacitance_body_enabled
        self.obj.CapacitanceBody = self.capacitance_body

        self.obj.SurfaceChargeDensity = self.surface_charge_density

    def init_parameter_widget(self):
        self._get_params()

        self.parameter_widget.qsb_potential.setProperty("value", self.potential)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_potential).bind(
            self.obj, "Potential"
        )
        self.parameter_widget.ckb_potential.setChecked(self.potential_enabled)

        # the vector potentials
        self.parameter_widget.qsb_av_re.setProperty("value", self.potential)
        self.parameter_widget.qsb_av_re.setEnabled(self.av_re_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re).bind(self.obj, "Potential")

        self.parameter_widget.qsb_av_re_1.setProperty("value", self.av_re_1)
        self.parameter_widget.qsb_av_re_1.setEnabled(self.av_re_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_1).bind(self.obj, "AV_re_1")
        self.parameter_widget.qsb_av_re_2.setProperty("value", self.av_re_2)
        self.parameter_widget.qsb_av_re_2.setEnabled(self.av_re_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_2).bind(self.obj, "AV_re_2")
        self.parameter_widget.qsb_av_re_3.setProperty("value", self.av_re_3)
        self.parameter_widget.qsb_av_re_3.setEnabled(self.av_re_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_re_3).bind(self.obj, "AV_re_3")
        self.parameter_widget.qsb_av_im.setProperty("value", self.av_im)
        self.parameter_widget.qsb_av_im.setEnabled(self.av_im_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im).bind(self.obj, "AV_im")
        self.parameter_widget.qsb_av_im_1.setProperty("value", self.av_im_1)
        self.parameter_widget.qsb_av_im_1.setEnabled(self.av_im_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_1).bind(self.obj, "AV_im_1")
        self.parameter_widget.qsb_av_im_2.setProperty("value", self.av_im_2)
        self.parameter_widget.qsb_av_im_2.setEnabled(self.av_im_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_2).bind(self.obj, "AV_im_2")
        self.parameter_widget.qsb_av_im_3.setProperty("value", self.av_im_3)
        self.parameter_widget.qsb_av_im_3.setEnabled(self.av_im_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_av_im_3).bind(self.obj, "AV_im_3")

        self.parameter_widget.ckb_av_re_1.setChecked(self.av_re_1_enabled)
        self.parameter_widget.ckb_av_re_2.setChecked(self.av_re_2_enabled)
        self.parameter_widget.ckb_av_re_3.setChecked(self.av_re_3_enabled)
        self.parameter_widget.ckb_av_im.setChecked(self.av_im_enabled)
        self.parameter_widget.ckb_av_im_1.setChecked(self.av_im_1_enabled)
        self.parameter_widget.ckb_av_im_2.setChecked(self.av_im_2_enabled)
        self.parameter_widget.ckb_av_im_3.setChecked(self.av_im_3_enabled)

        self.parameter_widget.ckb_potential_constant.setChecked(self.potential_constant)

        self.parameter_widget.ckb_electric_infinity.setChecked(self.electric_infinity)

        self.parameter_widget.ckb_electric_forcecalculation.setChecked(
            self.electric_forcecalculation
        )

        self.parameter_widget.ckb_capacitance_body.setChecked(self.capacitance_body_enabled)
        self.parameter_widget.spb_capacitance_body.setValue(self.capacitance_body)

        self.parameter_widget.qsb_surface_charge_density.setProperty(
            "value", self.surface_charge_density
        )
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_surface_charge_density).bind(
            self.obj, "SurfaceChargeDensity"
        )

        self.bc_enum = self.obj.getEnumerationsOfProperty("BoundaryCondition")
        self.parameter_widget.cb_boundary_condition.addItems(self.bc_enum)
        index = self.bc_enum.index(self.boundary_condition)
        self.parameter_widget.cb_boundary_condition.setCurrentIndex(index)
        self.boundary_condition_changed(index)

    def potential_changed(self, value):
        self.potential = value

    def potential_enabled_changed(self, value):
        self.potential_enabled = value
        self.parameter_widget.qsb_potential.setEnabled(value)

    def av_re_enabled_changed(self, value):
        self.av_re_enabled = value
        self.parameter_widget.qsb_av_re.setEnabled(value)

    def av_re_1_enabled_changed(self, value):
        self.av_re_1_enabled = value
        self.parameter_widget.qsb_av_re_1.setEnabled(value)

    def av_re_2_enabled_changed(self, value):
        self.av_re_2_enabled = value
        self.parameter_widget.qsb_av_re_2.setEnabled(value)

    def av_re_3_enabled_changed(self, value):
        self.av_re_3_enabled = value
        self.parameter_widget.qsb_av_re_3.setEnabled(value)

    def av_im_enabled_changed(self, value):
        self.av_im_enabled = value
        self.parameter_widget.qsb_av_im.setEnabled(value)

    def av_im_1_enabled_changed(self, value):
        self.av_im_1_enabled = value
        self.parameter_widget.qsb_av_im_1.setEnabled(value)

    def av_im_2_enabled_changed(self, value):
        self.av_im_2_enabled = value
        self.parameter_widget.qsb_av_im_2.setEnabled(value)

    def av_im_3_enabled_changed(self, value):
        self.av_im_3_enabled = value
        self.parameter_widget.qsb_av_im_3.setEnabled(value)

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

    def electric_forcecalculation_changed(self, value):
        self.electric_forcecalculation = value

    def capacitance_body_enabled_changed(self, value):
        self.capacitance_body_enabled = value
        self.parameter_widget.spb_capacitance_body.setEnabled(value)

    def capacitance_body_changed(self, value):
        self.capacitance_body = value
        self.parameter_widget.spb_capacitance_body.setValue(value)

    def surface_charge_density_changed(self, value):
        self.surface_charge_density = value

    def boundary_condition_changed(self, index):
        self.boundary_condition = self.bc_enum[index]
        if self.boundary_condition == "Dirichlet":
            self.parameter_widget.gb_neumann.setEnabled(False)
            self.parameter_widget.gb_dirichlet.setEnabled(True)
        elif self.boundary_condition == "Neumann":
            self.parameter_widget.gb_neumann.setEnabled(True)
            self.parameter_widget.gb_dirichlet.setEnabled(False)

    def _applyPotentialChanges(self, enabledBox, potentialQSB):
        enabled = enabledBox.isChecked()
        potential = None
        try:
            potential = potentialQSB.property("value")
        except ValueError:
            FreeCAD.Console.PrintMessage(
                "Wrong input. Not recognised input: '{}' "
                "Potential has not been set.\n".format(potentialQSB.text())
            )
            potential = "0.0 mm^2*kg/(s^3*A)"
        return enabled, potential

    def _applyWidgetChanges(self):
        # apply the voltages and their enabled state
        self.obj.AV_re_1_Disabled, self.obj.AV_re_1 = self._applyPotentialChanges(
            self.parameter_widget.reXunspecBox, self.parameter_widget.realXQSB
        )
        self.obj.AV_re_2_Disabled, self.obj.AV_re_2 = self._applyPotentialChanges(
            self.parameter_widget.reYunspecBox, self.parameter_widget.realYQSB
        )
        self.obj.AV_re_3_Disabled, self.obj.AV_re_3 = self._applyPotentialChanges(
            self.parameter_widget.reZunspecBox, self.parameter_widget.realZQSB
        )
        self.obj.AV_im_Disabled, self.obj.AV_im = self._applyPotentialChanges(
            self.parameter_widget.imScalarunspecBox, self.parameter_widget.imagScalarQSB
        )
        self.obj.AV_im_1_Disabled, self.obj.AV_im_1 = self._applyPotentialChanges(
            self.parameter_widget.imXunspecBox, self.parameter_widget.imagXQSB
        )
        self.obj.AV_im_2_Disabled, self.obj.AV_im_2 = self._applyPotentialChanges(
            self.parameter_widget.imYunspecBox, self.parameter_widget.imagYQSB
        )
        self.obj.AV_im_3_Disabled, self.obj.AV_im_3 = self._applyPotentialChanges(
            self.parameter_widget.imZunspecBox, self.parameter_widget.imagZQSB
        )
        # because this is an enable the others are disabled, reverse
        self.obj.PotentialEnabled = not self.obj.PotentialEnabled

        self.obj.PotentialConstant = self.parameter_widget.potentialConstantBox.isChecked()

        self.obj.ElectricInfinity = self.parameter_widget.electricInfinityBox.isChecked()

        calc_is_checked = self.parameter_widget.electricForcecalculationBox.isChecked()
        self.obj.ElectricForcecalculation = calc_is_checked  # two lines because max line length

        self.obj.SurfaceChargeDensity = self.parameter_widget.surfacechargedensityQSB.property(
            "value"
        )
