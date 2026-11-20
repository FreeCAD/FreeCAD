# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Valentin Pohu
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

__title__ = "FEM material task panel Gui unit tests"

import unittest

import FreeCAD
from FreeCAD import Units

import ObjectsFem
from femtest.app.support_utils import fcc_print

# The field of each property and a value no default material has
TYPED = {
    "Density": ("qsb_density", "7000 kg/m^3"),
    "YoungsModulus": ("qsb_young_modulus", "150 GPa"),
    "PoissonRatio": ("qsb_poisson_ratio", "0.27"),
    "ThermalConductivity": ("qsb_thermal_conductivity", "45 W/m/K"),
    "ThermalExpansionCoefficient": ("qsb_expansion_coefficient", "3e-5 1/K"),
    "ThermalExpansionReferenceTemperature": ("qsb_expansion_reference_temperature", "310 K"),
    "SpecificHeat": ("qsb_specific_heat", "470 J/kg/K"),
    "KinematicViscosity": ("qsb_kinematic_viscosity", "2e-6 m^2/s"),
}
THERMAL = (
    "Density",
    "ThermalConductivity",
    "ThermalExpansionCoefficient",
    "ThermalExpansionReferenceTemperature",
    "SpecificHeat",
)
SOLID = THERMAL + ("YoungsModulus", "PoissonRatio")
FLUID = THERMAL + ("KinematicViscosity",)
# A material already filled in, each value different from the others and from TYPED
OPENED = {
    "Density": "7900 kg/m^3",
    "YoungsModulus": "210 GPa",
    "PoissonRatio": "0.3",
    "ThermalConductivity": "16 W/m/K",
    "ThermalExpansionCoefficient": "1.2e-5 1/K",
    "ThermalExpansionReferenceTemperature": "293.15 K",
    "SpecificHeat": "500 J/kg/K",
    "KinematicViscosity": "1e-6 m^2/s",
}


class TestMaterialTaskPanel(unittest.TestCase):
    fcc_print("import TestMaterialTaskPanel")

    def setUp(self):
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def test_each_field_of_a_solid_writes_its_own_property(self):
        # The 2 expansion fields were wired to one slot, the coefficient typed in was never writen
        material = ObjectsFem.makeMaterialSolid(self.document, "Material")
        panel = self.open_panel(material)
        expected = self.type_each(panel, SOLID)
        self.accept_and_read(panel, material, expected)

    def test_each_field_of_a_fluid_writes_its_own_property(self):
        material = ObjectsFem.makeMaterialFluid(self.document, "Fluid")
        panel = self.open_panel(material)
        expected = self.type_each(panel, FLUID)
        self.accept_and_read(panel, material, expected)

    def test_an_opened_material_shows_and_keeps_its_values(self):
        # A value shown in the wrong field is saved there once editing is allowed
        for make, names in (
            (ObjectsFem.makeMaterialSolid, SOLID),
            (ObjectsFem.makeMaterialFluid, FLUID),
        ):
            material = make(self.document)
            opened = {name: OPENED[name] for name in names}
            material.Material = dict({"Name": "Opened"}, **opened)
            with self.subTest(category=material.Category):
                panel = self.open_panel(material, shown=opened)
                self.accept_and_read(panel, material, opened)

    def open_panel(self, material, shown=None):
        # The panel of the material, its fields checked against what it shows, editing allowed
        from femtaskpanels import task_material_common

        panel = task_material_common._TaskPanel(material)
        self.addCleanup(panel.selectionWidget.finish_selection)
        fields = panel.parameterWidget
        for name, want in (shown or {}).items():
            with self.subTest(property=name, read="field"):
                self.assert_quantity(
                    getattr(fields, TYPED[name][0]).property("value").UserString, want
                )
        fields.chbu_allow_edit.click()
        return panel

    def type_each(self, panel, names):
        # Every property read after each edit : a slot writing a neighbour of its own would show
        fields = panel.parameterWidget
        expected = {name: panel.material[name] for name in names}
        for name in names:
            field = getattr(fields, TYPED[name][0])
            self.assertTrue(field.isEnabled(), name)
            field.setProperty("value", Units.Quantity(TYPED[name][1]))
            field.editingFinished.emit()
            expected[name] = TYPED[name][1]
            for other, want in expected.items():
                with self.subTest(edited=name, property=other):
                    self.assert_quantity(panel.material[other], want)
        return expected

    def accept_and_read(self, panel, material, expected):
        self.assertTrue(panel.accept())
        self.assertEqual(set(material.Material), {"Name", *expected})
        for name, want in expected.items():
            with self.subTest(property=name, read="object"):
                self.assert_quantity(material.Material[name], want)

    def assert_quantity(self, stored, expected):
        # The solver writers convert to a unit, a bare number fails there
        got = Units.Quantity(stored)
        want = Units.Quantity(expected)
        self.assertEqual(got.Unit, want.Unit, stored)
        # Relative : the internal units make 7000 kg/m^3 a 7e-6
        self.assertAlmostEqual(got.Value, want.Value, delta=abs(want.Value) * 1e-6)
