# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
import unittest
import FreeCAD
from Path.Tool.machine.models.machine import Machine, MachineFeature
from Path.Tool.machine.models.axis import LinearAxis, AngularAxis
from Path.Tool.machine.models.mill import Mill
from Path.Tool.machine.models.lathe import Lathe
from Path.Tool.machine.models.spindle import Spindle


# Mock Spindle for testing
class MockSpindle(Spindle):
    def __init__(self, name="Mock Spindle", max_rpm=10000):
        super().__init__(
            name=name,
            max_power=FreeCAD.Units.Quantity("2 kW"),
            min_rpm=2000.0,
            max_rpm=float(max_rpm),
            max_torque=10.0,
            peak_torque_rpm=3000.0,
        )

    def validate(self):
        pass

    def _dump_self(self, indent_str=""):
        return f"{indent_str}MockSpindle: {self.name}\n"


class TestPathToolMachine(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Spindle 1", 10000)
        self.mock_spindle2 = MockSpindle("Spindle 2", 20000)
        self.default_machine = Machine(
            name="DefaultMachine",
            label="Default Machine",
        )

    def test_initialization_defaults(self):
        self.assertEqual(self.default_machine.name, "DefaultMachine")
        self.assertEqual(self.default_machine.label, "Default Machine")
        self.assertEqual(self.default_machine.post_processor, "generic")
        self.assertEqual(self.default_machine.post_processor_args, "")
        self.assertEqual(len(self.default_machine.get_children_by_type(Spindle)), 0)
        self.assertIsNotNone(self.default_machine.get_id())
        self.assertEqual(len(self.default_machine.get_children_by_type(LinearAxis)), 0)
        self.assertEqual(len(self.default_machine.get_children_by_type(AngularAxis)), 0)
        self.assertEqual(len(self.default_machine.feature_flags), 0)

    def test_initialization_custom_values(self):
        custom_machine = Machine(
            name="CustomMachine",
            label="Custom Machine",
            post_processor="linuxcnc",
            post_processor_args="--args",
            id="custom-id",
            feature_flags=[MachineFeature.MILLING_3D],
        )
        custom_machine.add(LinearAxis("X"))
        self.assertEqual(custom_machine.name, "CustomMachine")
        self.assertEqual(custom_machine.label, "Custom Machine")
        self.assertEqual(custom_machine.post_processor, "linuxcnc")
        self.assertEqual(custom_machine.post_processor_args, "--args")
        self.assertEqual(custom_machine.get_id(), "custom-id")
        self.assertEqual(len(custom_machine.get_children_by_type(Spindle)), 0)
        self.assertEqual(len(custom_machine.get_children_by_type(LinearAxis)), 1)
        self.assertEqual(custom_machine.feature_flags, [MachineFeature.MILLING_3D])

    def test_validation_valid(self):
        try:
            self.default_machine.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

    def test_validation_missing_label(self):
        self.default_machine.label = ""
        with self.assertRaisesRegex(AttributeError, "Machine name is required"):
            self.default_machine.validate()

    def test_spindle_management(self):
        # Test adding spindles
        self.default_machine.add(self.mock_spindle1)
        self.assertEqual(len(self.default_machine.get_children_by_type(Spindle)), 1)
        self.assertEqual(self.default_machine.get_children_by_type(Spindle)[0], self.mock_spindle1)

        # Test adding another spindle
        self.default_machine.add(self.mock_spindle2)
        self.assertEqual(len(self.default_machine.get_children_by_type(Spindle)), 2)
        self.assertEqual(self.default_machine.get_children_by_type(Spindle)[1], self.mock_spindle2)

        # Test validate with spindles
        try:
            self.default_machine.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

        # Test dump with spindles
        with self.assertRaises(NotImplementedError):
            self.default_machine.dump(False)

    def test_dump(self):
        with self.assertRaises(NotImplementedError):
            self.default_machine.dump(False)

    def test_get_attribute_configs(self):
        attrs = {a.name: a for a in self.default_machine.get_attribute_configs()}
        self.assertIn("name", attrs)
        self.assertEqual(attrs["name"].label, "Name")
        self.assertEqual(attrs["name"].property_type, "App::PropertyString")
        self.assertTrue(attrs["name"].readonly)

        self.assertIn("label", attrs)
        self.assertEqual(attrs["label"].label, "Label")
        self.assertEqual(attrs["label"].property_type, "App::PropertyString")
        self.assertTrue(attrs["label"].readonly)


class TestMill(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Mill Spindle 1", 10000)
        self.mock_spindle2 = MockSpindle("Mill Spindle 2", 20000)
        self.default_mill = Mill(
            name="DefaultMill",
            label="Default Mill",
        )
        self.default_mill.x_axis.max_feed = FreeCAD.Units.Quantity(1000, "mm/min")
        self.default_mill.y_axis.max_feed = FreeCAD.Units.Quantity(1200, "mm/min")
        self.default_mill.z_axis.max_feed = FreeCAD.Units.Quantity(800, "mm/min")

    def test_initialization(self):
        mill = self.default_mill

        # Check inherited properties
        self.assertEqual(mill.name, "DefaultMill")
        self.assertEqual(mill.label, "Default Mill")
        self.assertEqual(len(mill.get_children_by_type(Spindle)), 0)
        self.assertEqual(len(mill.find_children_by_type(Spindle)), 1)
        self.assertEqual(len(mill.get_children_by_type(LinearAxis)), 1)
        self.assertEqual(len(mill.find_children_by_type(LinearAxis)), 3)
        self.assertEqual(len(mill.get_children_by_type(AngularAxis)), 0)
        self.assertEqual(len(mill.find_children_by_type(AngularAxis)), 1)
        self.assertEqual(mill.spindle_axis.name, "A")
        self.assertEqual(mill.get_type(), "Mill")

        mill.add(self.mock_spindle2)
        self.assertEqual(len(mill.get_children_by_type(Spindle)), 1)
        self.assertEqual(len(mill.find_children_by_type(Spindle)), 2)
        self.assertEqual(mill.get_children_by_type(Spindle)[0].name, "Mill Spindle 2")

        # Check rigidity and feed rates
        self.assertAlmostEqual(mill.x_axis.max_feed.Value, 1000 / 60, 2)
        self.assertAlmostEqual(mill.y_axis.max_feed.Value, 1200 / 60, 2)
        self.assertAlmostEqual(mill.z_axis.max_feed.Value, 800 / 60, 2)
        self.assertEqual(mill.spindle_axis.angular_rigidity.UserString, "0.50°")
        self.assertEqual(mill.spindle_axis.rigidity_x.getValueAs("mm/N"), 0.001)
        self.assertEqual(mill.spindle_axis.rigidity_y.getValueAs("mm/N"), 0.001)

    def test_validation_valid(self):
        self.default_mill.validate()

    def test_dump(self):
        output = self.default_mill.dump(False)
        self.assertIn("Mill Default Mill", output)
        self.assertIn("LinearAxis: X", output)
        self.assertIn("Angular Rigidity: 0.50°/N", output)
        self.assertIn("Rigidity X: 0.001 mm/N", output)
        self.assertIn("Rigidity Y: 0.001 mm/N", output)
        self.assertIn("MainSpindle", output)

    def test_get_attribute_configs(self):
        attrs = {a.name: a for a in self.default_mill.x_axis.get_attribute_configs()}
        self.assertIn("name", attrs)
        self.assertEqual(attrs["name"].label, "Name")
        self.assertEqual(attrs["name"].property_type, "App::PropertyString")
        self.assertTrue(attrs["name"].readonly)

        self.assertIn("max_feed", attrs)
        self.assertEqual(attrs["max_feed"].label, "Max Feed")
        self.assertEqual(attrs["max_feed"].property_type, "App::PropertyVelocity")
        self.assertEqual(attrs["max_feed"].min_value, 0.0)
        self.assertFalse(attrs["max_feed"].readonly)

        attrs = {a.name: a for a in self.default_mill.spindle_axis.get_attribute_configs()}
        self.assertIn("name", attrs)
        self.assertEqual(attrs["name"].label, "Name")
        self.assertEqual(attrs["name"].property_type, "App::PropertyString")
        self.assertTrue(attrs["name"].readonly)

        self.assertIn("angular_rigidity", attrs)
        self.assertEqual(attrs["angular_rigidity"].label, "Angular rigidity")
        self.assertEqual(attrs["angular_rigidity"].property_type, "App::PropertyAngularRigidity")
        self.assertEqual(attrs["angular_rigidity"].min_value, 0.0)
        self.assertFalse(attrs["angular_rigidity"].readonly)

        self.assertIn("rigidity_x", attrs)
        self.assertEqual(attrs["rigidity_x"].label, "Rigidity X")
        self.assertEqual(attrs["rigidity_x"].property_type, "App::PropertyRigidity")
        self.assertEqual(attrs["rigidity_x"].min_value, 0.0)
        self.assertFalse(attrs["rigidity_x"].readonly)

        self.assertIn("rigidity_y", attrs)
        self.assertEqual(attrs["rigidity_y"].label, "Rigidity Y")
        self.assertEqual(attrs["rigidity_y"].property_type, "App::PropertyRigidity")
        self.assertEqual(attrs["rigidity_y"].min_value, 0.0)
        self.assertFalse(attrs["rigidity_y"].readonly)


class TestLathe(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Lathe Spindle 1", 5000)
        self.mock_spindle2 = MockSpindle("Lathe Spindle 2", 8000)
        self.default_lathe = Lathe(
            name="DefaultLathe",
            label="Default Lathe",
        )
        self.default_lathe.x_axis.max_feed = FreeCAD.Units.Quantity(1500, "mm/min")
        self.default_lathe.z_axis.max_feed = FreeCAD.Units.Quantity(1800, "mm/min")
        self.default_lathe.a_axis.angular_rigidity = FreeCAD.Units.Quantity(0.08, "°")
        self.default_lathe.a_axis.rigidity_x = FreeCAD.Units.Quantity(0.001, "mm/N")
        self.default_lathe.a_axis.rigidity_y = FreeCAD.Units.Quantity(0.001, "mm/N")

    def test_initialization(self):
        lathe = self.default_lathe

        # Check default properties
        self.assertEqual(lathe.name, "DefaultLathe")
        self.assertEqual(lathe.label, "Default Lathe")
        self.assertEqual(len(lathe.get_children_by_type(Spindle)), 1)
        self.assertEqual(lathe.get_children_by_type(Spindle)[0].name, "MainSpindle")
        self.assertEqual(len(lathe.get_children_by_type(LinearAxis)), 1)
        self.assertEqual(len(lathe.get_children_by_type(AngularAxis)), 0)
        self.assertEqual(lathe.get_type(), "Lathe")

        # Test adding another spindle
        self.default_lathe.add(self.mock_spindle2)
        self.assertEqual(len(lathe.get_children_by_type(Spindle)), 2)
        self.assertEqual(lathe.get_children_by_type(Spindle)[1].name, "Lathe Spindle 2")

        # Check rigidity and feed rates
        self.assertAlmostEqual(lathe.x_axis.max_feed.Value, 1500 / 60, 2)
        self.assertAlmostEqual(lathe.z_axis.max_feed.Value, 1800 / 60, 2)
        self.assertEqual(lathe.a_axis.angular_rigidity.UserString, "0.08°")
        self.assertEqual(lathe.a_axis.rigidity_x.getValueAs("mm/N"), 0.001)
        self.assertEqual(lathe.a_axis.rigidity_y.getValueAs("mm/N"), 0.001)

    def test_validation_valid(self):
        try:
            self.default_lathe.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

    def test_find_child_by_name(self):
        # Test finding a direct child
        z_axis = self.default_lathe.find_child_by_name("Z")
        self.assertIsNotNone(z_axis)
        self.assertEqual(z_axis.name, "Z")

        # Test finding a nested child
        x_axis = self.default_lathe.find_child_by_name("X")
        self.assertIsNotNone(x_axis)
        self.assertEqual(x_axis.name, "X")

        # Test finding a non-existent child
        non_existent_axis = self.default_lathe.find_child_by_name("Y")
        self.assertIsNone(non_existent_axis)

    def test_find_children_by_type(self):
        # Test finding all LinearAxis instances (Z and X)
        linear_axes = self.default_lathe.find_children_by_type(LinearAxis)
        self.assertEqual(len(linear_axes), 2)
        self.assertIn(self.default_lathe.z_axis, linear_axes)
        self.assertIn(self.default_lathe.x_axis, linear_axes)

        # Test finding all AngularAxis instances (A)
        angular_axes = self.default_lathe.find_children_by_type(AngularAxis)
        self.assertEqual(len(angular_axes), 1)
        self.assertIn(self.default_lathe.a_axis, angular_axes)

        # Test finding all Spindle instances
        spindles = self.default_lathe.find_children_by_type(Spindle)
        self.assertEqual(len(spindles), 1)
        self.assertIn(self.default_lathe.spindles[0], spindles)

        # Test finding a type that doesn't exist
        non_existent_type = self.default_lathe.find_children_by_type(Machine)
        self.assertEqual(len(non_existent_type), 0)

    def test_dump(self):
        output = self.default_lathe.dump(False)
        self.assertIn("Lathe Default Lathe", output)
        self.assertIn("LinearAxis: X", output)
        self.assertIn("Angular Rigidity: 0.08°/N", output)
        self.assertIn("Rigidity X: 0.001 mm/N", output)
        self.assertIn("Rigidity Y: 0.001 mm/N", output)
        self.assertIn("MainSpindle", output)

    def test_get_attribute_configs(self):
        attrs = {a.name: a for a in self.default_lathe.spindles[0].get_attribute_configs()}
        self.assertIn("name", attrs)
        self.assertEqual(attrs["name"].label, "Name")
        self.assertEqual(attrs["name"].property_type, "App::PropertyString")
        self.assertTrue(attrs["name"].readonly)

        self.assertIn("max_power", attrs)
        self.assertEqual(attrs["max_power"].label, "Max Power")
        self.assertEqual(attrs["max_power"].property_type, "App::PropertyPower")
        self.assertEqual(attrs["max_power"].min_value, 0.0)
        self.assertFalse(attrs["max_power"].readonly)

        self.assertIn("min_rpm", attrs)
        self.assertEqual(attrs["min_rpm"].label, "Min RPM")
        self.assertEqual(attrs["min_rpm"].property_type, "App::PropertyAngularSpeed")
        self.assertEqual(attrs["min_rpm"].min_value, 0.0)
        self.assertFalse(attrs["min_rpm"].readonly)

        self.assertIn("max_rpm", attrs)
        self.assertEqual(attrs["max_rpm"].label, "Max RPM")
        self.assertEqual(attrs["max_rpm"].property_type, "App::PropertyAngularSpeed")
        self.assertEqual(attrs["max_rpm"].min_value, 0.0)
        self.assertFalse(attrs["max_rpm"].readonly)

        self.assertIn("max_torque", attrs)
        self.assertEqual(attrs["max_torque"].label, "Max Torque")
        self.assertEqual(attrs["max_torque"].property_type, "App::PropertyTorque")
        self.assertEqual(attrs["max_torque"].min_value, 0.0)
        self.assertFalse(attrs["max_torque"].readonly)

        self.assertIn("peak_torque_rpm", attrs)
        self.assertEqual(attrs["peak_torque_rpm"].label, "Peak Torque RPM")
        self.assertEqual(attrs["peak_torque_rpm"].property_type, "App::PropertyAngularSpeed")
        self.assertEqual(attrs["peak_torque_rpm"].min_value, 0.0)
        self.assertFalse(attrs["peak_torque_rpm"].readonly)


if __name__ == "__main__":
    unittest.main()
