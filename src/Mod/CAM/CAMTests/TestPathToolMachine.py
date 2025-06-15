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
from Path.Tool.machine.models.machine import Machine, Axis, LinearAxis, AngularAxis
from Path.Tool.machine.models.mill import Mill
from Path.Tool.machine.models.lathe import Lathe
from Path.Tool.spindle import Spindle


# Mock Spindle for testing
class MockSpindle(Spindle):
    def __init__(self, label="Mock Spindle", max_rpm=10000, id="spindle1"):
        super().__init__(
            label=label,
            max_power=2,
            min_rpm=2000,
            max_rpm=max_rpm,
            max_torque=10,
            peak_torque_rpm=3000,
            id=id,
        )

    def validate(self):
        pass

    def dump(self, do_print=True):
        return f"MockSpindle: {self.label}"


class TestPathToolMachine(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Spindle 1", 10000)
        self.mock_spindle2 = MockSpindle("Spindle 2", 20000)
        self.default_machine = Machine(
            label="Default Machine",
            axes={},
            max_feed=FreeCAD.Units.Quantity("2000 mm/min"),
        )

    def test_initialization_defaults(self):
        self.assertEqual(self.default_machine.label, "Default Machine")
        self.assertAlmostEqual(self.default_machine.max_feed.Value, 33.33, 2)
        self.assertEqual(self.default_machine.post_processor, "generic")
        self.assertEqual(self.default_machine.post_processor_args, "")
        self.assertEqual(len(self.default_machine.spindles), 0)
        self.assertIsNotNone(self.default_machine.get_id())
        self.assertEqual(len(self.default_machine.axes), 0)

    def test_initialization_custom_values(self):
        custom_machine = Machine(
            label="Custom Machine",
            axes={"x": LinearAxis()},
            max_feed=FreeCAD.Units.Quantity(5000, "mm/min"),
            post_processor="linuxcnc",
            post_processor_args="--args",
            id="custom-id",
        )
        self.assertEqual(custom_machine.label, "Custom Machine")
        self.assertAlmostEqual(custom_machine.max_feed.Value, 83.33, 2)
        self.assertEqual(custom_machine.post_processor, "linuxcnc")
        self.assertEqual(custom_machine.post_processor_args, "--args")
        self.assertEqual(custom_machine.get_id(), "custom-id")
        self.assertEqual(len(custom_machine.spindles), 0)
        self.assertEqual(len(custom_machine.axes), 1)

    def test_validation_valid(self):
        try:
            self.default_machine.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

    def test_validation_missing_label(self):
        self.default_machine.label = ""
        with self.assertRaisesRegex(AttributeError, "Machine name is required"):
            self.default_machine.validate()

    def test_validation_non_positive_max_feed(self):
        self.default_machine.max_feed = FreeCAD.Units.Quantity("0 mm/min")
        with self.assertRaisesRegex(AttributeError, "Max feed rate must be positive"):
            self.default_machine.validate()

    def test_spindle_management(self):
        # Test adding spindles
        self.default_machine.add_spindle(self.mock_spindle1)
        self.assertEqual(len(self.default_machine.spindles), 1)
        self.assertEqual(self.default_machine.spindles[0], self.mock_spindle1)

        # Test adding another spindle
        self.default_machine.add_spindle(self.mock_spindle2)
        self.assertEqual(len(self.default_machine.spindles), 2)
        self.assertEqual(self.default_machine.spindles[1], self.mock_spindle2)

        # Test validate with spindles
        try:
            self.default_machine.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

        # Test dump with spindles
        output = self.default_machine.dump(False)
        self.assertIn("Spindle 1", output)
        self.assertIn("Spindle 2", output)

    def test_dump(self):
        output = self.default_machine.dump(False)
        self.assertIn("Machine Default Machine", output)
        self.assertIn("Max Feed Rate: 33.33 mm/s", output)


class TestMill(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Mill Spindle 1", 10000)
        self.mock_spindle2 = MockSpindle("Mill Spindle 2", 20000)
        self.default_mill = Mill(
            label="Default Mill",
            axes={
                "X": LinearAxis(
                    start=FreeCAD.Units.Quantity(1, "mm"),
                    end=FreeCAD.Units.Quantity(100, "mm"),
                    rigidity=FreeCAD.Units.Quantity(0.01, "mm"),
                ),
                "Y": LinearAxis(
                    start=FreeCAD.Units.Quantity(2, "mm"),
                    end=FreeCAD.Units.Quantity(200, "mm"),
                    rigidity=FreeCAD.Units.Quantity(0.02, "mm"),
                ),
                "Z": LinearAxis(
                    start=FreeCAD.Units.Quantity(3, "mm"),
                    end=FreeCAD.Units.Quantity(300, "mm"),
                    rigidity=FreeCAD.Units.Quantity("0.03 mm"),
                ),
                "spindle": AngularAxis(
                    rigidity_x=FreeCAD.Units.Quantity("0.04 °"),
                    rigidity_y=FreeCAD.Units.Quantity("0.05 °"),
                ),
            },
            max_feed=FreeCAD.Units.Quantity(5000, "mm/min"),
        )

    def test_initialization(self):
        mill = self.default_mill

        # Check inherited properties
        self.assertEqual(mill.label, "Default Mill")
        self.assertAlmostEqual(mill.max_feed.Value, 83.33, 2)
        self.assertEqual(len(mill.spindles), 1)
        self.assertEqual(mill.spindles[0].label, "Spindle 1")
        self.assertEqual(len(mill.axes), 4)

        self.default_mill.add_spindle(self.mock_spindle2)
        self.assertEqual(len(mill.spindles), 2)
        self.assertEqual(mill.spindles[1].label, "Mill Spindle 2")

        # Check rigidity
        self.assertEqual(mill.x_axis.rigidity.Value, 0.01)
        self.assertEqual(mill.y_axis.rigidity.Value, 0.02)
        self.assertEqual(mill.z_axis.rigidity.Value, 0.03)
        self.assertEqual(mill.spindle_axis.rigidity_x.Value, 0.04, mill.spindle_axis.rigidity_x)
        self.assertEqual(mill.spindle_axis.rigidity_y.Value, 0.05)

        # Check axis extents
        self.assertEqual(mill.x_axis.start.Value, 1)
        self.assertEqual(mill.x_axis.end and mill.x_axis.end.Value, 100)
        self.assertEqual(mill.y_axis.start.Value, 2)
        self.assertEqual(mill.y_axis.end and mill.y_axis.end.Value, 200)
        self.assertEqual(mill.z_axis.start.Value, 3)
        self.assertEqual(mill.z_axis.end and mill.z_axis.end.Value, 300)

    def test_validation_valid(self):
        self.default_mill.validate()

    def test_validation_negative_rigidity(self):
        self.default_mill.x_axis.rigidity = FreeCAD.Units.Quantity(-0.01, "mm/N")
        with self.assertRaisesRegex(
            AttributeError, "Axis X: Linear axis rigidity cannot be negative"
        ):
            self.default_mill.validate()

    def test_validation_invalid_extents(self):
        # X min >= max
        self.default_mill.x_axis.start = FreeCAD.Units.Quantity(100, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis X: Linear axis end must be larger than axis start"
        ):
            self.default_mill.validate()

        # Reset and test Y
        self.default_mill.x_axis.start = FreeCAD.Units.Quantity(1, "mm")
        self.default_mill.y_axis.start = FreeCAD.Units.Quantity(200, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis Y: Linear axis end must be larger than axis start"
        ):
            self.default_mill.validate()

        # Reset and test Z
        self.default_mill.y_axis.start = FreeCAD.Units.Quantity(2, "mm")
        self.default_mill.z_axis.start = FreeCAD.Units.Quantity(300, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis Z: Linear axis end must be larger than axis start"
        ):
            self.default_mill.validate()

    def test_dump(self):
        output = self.default_mill.dump(False)
        self.assertIn("Mill Default Mill", output)
        self.assertIn("Max Feed Rate: 83.33 mm/s", output)
        self.assertIn("X-Axis:", output)
        self.assertIn("Start=1.00 mm", output)
        self.assertIn("End=100.00 mm", output)
        self.assertIn("Rigidity=10.00 µm/N", output)
        self.assertIn("Spindle 1", output)


class TestLathe(unittest.TestCase):
    def setUp(self):
        self.mock_spindle1 = MockSpindle("Lathe Spindle 1", 5000)
        self.mock_spindle2 = MockSpindle("Lathe Spindle 2", 8000)
        self.default_lathe = Lathe(
            label="Default Lathe",
            axes={
                "X": LinearAxis(
                    start=FreeCAD.Units.Quantity(1, "mm"),
                    end=FreeCAD.Units.Quantity(50, "mm"),
                    rigidity=FreeCAD.Units.Quantity(0.05, "mm"),
                ),
                "Z": LinearAxis(
                    start=FreeCAD.Units.Quantity(0, "mm"),
                    end=FreeCAD.Units.Quantity(400, "mm"),
                    rigidity=FreeCAD.Units.Quantity(0.06, "mm"),
                ),
                "spindle": AngularAxis(
                    rigidity_x=FreeCAD.Units.Quantity(0.07, "°"),
                    rigidity_y=FreeCAD.Units.Quantity(0.08, "°"),
                ),
            },
            max_feed=FreeCAD.Units.Quantity(3000, "mm/min"),
            max_workpiece_diameter=FreeCAD.Units.Quantity(100, "mm"),
        )

    def test_initialization(self):
        lathe = self.default_lathe

        # Check inherited properties
        self.assertEqual(lathe.label, "Default Lathe")
        self.assertEqual(lathe.max_feed.Value, 50)
        self.assertEqual(len(lathe.spindles), 1)
        self.assertEqual(lathe.spindles[0].label, "Lathe spindle")
        self.assertEqual(len(lathe.axes), 3)

        # Check multiple spinndles not allowed
        self.default_lathe.add_spindle(self.mock_spindle2)
        self.assertEqual(len(lathe.spindles), 1)
        self.assertEqual(lathe.spindles[0].label, "Lathe Spindle 2")

        # Check rigidity
        self.assertEqual(lathe.x_axis.rigidity.Value, 0.05)
        self.assertEqual(lathe.z_axis.rigidity.Value, 0.06)
        self.assertEqual(lathe.spindle_axis.rigidity_x.Value, 0.07)
        self.assertEqual(lathe.spindle_axis.rigidity_y.Value, 0.08)

        # Check axis extents
        self.assertEqual(lathe.x_axis.start.Value, 1)
        self.assertEqual(lathe.x_axis.end and lathe.x_axis.end.Value, 50)
        self.assertEqual(lathe.z_axis.start.Value, 0)
        self.assertEqual(lathe.z_axis.end and lathe.z_axis.end.Value, 400)

        # Check workpiece diameter
        self.assertEqual(lathe.max_workpiece_diameter.Value, 100)

    def test_validation_valid(self):
        try:
            self.default_lathe.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

    def test_validation_negative_rigidity(self):
        self.default_lathe.x_axis.rigidity = FreeCAD.Units.Quantity(-0.01, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis X: Linear axis rigidity cannot be negative"
        ):
            self.default_lathe.validate()

    def test_validation_invalid_extents(self):
        # X min >= max
        self.default_lathe.x_axis.start = FreeCAD.Units.Quantity(50, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis X: Linear axis end must be larger than axis start"
        ):
            self.default_lathe.validate()

        # Reset and test Z
        self.default_lathe.x_axis.start = FreeCAD.Units.Quantity(1, "mm")
        self.default_lathe.z_axis.start = FreeCAD.Units.Quantity(400, "mm")
        with self.assertRaisesRegex(
            AttributeError, "Axis Z: Linear axis end must be larger than axis start"
        ):
            self.default_lathe.validate()

    def test_validation_invalid_diameter(self):
        self.default_lathe.max_workpiece_diameter = FreeCAD.Units.Quantity(0, "mm")
        with self.assertRaisesRegex(AttributeError, "Maximum workpiece diameter must be positive"):
            self.default_lathe.validate()

    def test_dump(self):
        output = self.default_lathe.dump(False)
        self.assertIn("Lathe Default Lathe", output)
        self.assertIn("Max Feed Rate: 50.00 mm/s", output)
        self.assertIn("X-Axis:", output)
        self.assertIn("Start=1.00 mm", output)
        self.assertIn("End=50.00 mm", output)
        self.assertIn("Rigidity=50.00 µm/N", output)
        self.assertIn("Max Workpiece Diameter: 100.00 mm", output)
        self.assertIn("Lathe spindle", output)


if __name__ == "__main__":
    unittest.main()
