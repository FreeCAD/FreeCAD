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
import yaml
import unittest
from typing import cast
import FreeCAD
from Path.Tool.spindle import Spindle
from Path.Tool.spindle.serializers import SpindleSerializer


class TestPathToolSpindleSerializer(unittest.TestCase):
    """
    Test case for the SpindleSerializer.
    """

    def setUp(self):
        """Create spindle object for each test."""
        super().setUp()
        self.serializer_class = SpindleSerializer

        self.test_spindle = Spindle(
            id="spindle-test-01",
            label="Test Spindle 01",
            min_rpm=1000.0,
            max_rpm=24000.0,
            max_power=FreeCAD.Units.Quantity("2.2 kW"),
            max_torque=1.0,
            peak_torque_rpm=12000.0,
        )

    def test_serialize_spindle(self):
        """Test serialization of a Spindle object."""
        serialized_data = self.serializer_class.serialize(self.test_spindle)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["id"], "spindle-test-01")
        self.assertEqual(data["label"], "Test Spindle 01")
        self.assertEqual(data["min_rpm"], 1000.0)
        self.assertEqual(data["max_rpm"], 24000.0)
        self.assertEqual(data["max_power"], "2.20 kW")
        self.assertEqual(data["max_torque"], 1.0)
        self.assertEqual(data["peak_torque_rpm"], 12000.0)

    def test_deserialize_spindle(self):
        """Test deserialization of a Spindle object."""
        spindle_yaml_str = """
id: spindle-test-02
label: Test Spindle 02
min_rpm: 500.0
max_rpm: 12000.0
max_power: 1.5 kW
max_torque: 0.8
peak_torque_rpm: 6000.0
"""
        spindle_data = spindle_yaml_str.encode("utf-8")

        deserialized_spindle = cast(
            Spindle,
            self.serializer_class.deserialize(
                spindle_data, id="spindle-test-02", dependencies=None
            ),
        )

        self.assertIsInstance(deserialized_spindle, Spindle)
        self.assertEqual(deserialized_spindle.id, "spindle-test-02")
        self.assertEqual(deserialized_spindle.label, "Test Spindle 02")
        self.assertEqual(deserialized_spindle.min_rpm, 500.0)
        self.assertEqual(deserialized_spindle.max_rpm, 12000.0)
        self.assertEqual(deserialized_spindle.max_power.UserString, "1.50 kW")
        self.assertEqual(deserialized_spindle.max_torque, 0.8)
        self.assertEqual(deserialized_spindle.peak_torque_rpm, 6000.0)


if __name__ == "__main__":
    unittest.main()
