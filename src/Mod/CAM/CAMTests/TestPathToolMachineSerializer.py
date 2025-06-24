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
from typing import Mapping, cast

import FreeCAD
from Path.Tool.assets.asset import Asset
from Path.Tool.assets.uri import AssetUri
from Path.Tool.machine import Lathe, Mill
from Path.Tool.machine.models.machine import LinearAxis, AngularAxis
from Path.Tool.machine.serializers import MachineSerializer
from Path.Tool.spindle import Spindle


class TestPathToolMachineSerializer(unittest.TestCase):
    """
    Test case for the MachineSerializer.
    This class could inherit from PathTestWithAssets if asset loading is required.
    """

    def setUp(self):
        """Create machine and spindle objects for each test."""
        super().setUp()
        self.serializer_class = MachineSerializer

        # Create mock spindle objects
        self.spindle1 = Spindle(
            label="Main Spindle",
            max_power=FreeCAD.Units.Quantity("2.2 kW"),
            min_rpm=FreeCAD.Units.Quantity("1000 1/min"),
            max_rpm=FreeCAD.Units.Quantity("24000 1/min"),
            max_torque=FreeCAD.Units.Quantity("1.0 N*m"),
            peak_torque_rpm=FreeCAD.Units.Quantity("12000 1/min"),
            id="spindle-001",
        )

        self.spindle2 = Spindle(
            label="Secondary Spindle",
            max_power=FreeCAD.Units.Quantity("1.5 kW"),
            min_rpm=FreeCAD.Units.Quantity("500 1/min"),
            max_rpm=FreeCAD.Units.Quantity("12000 1/min"),
            max_torque=FreeCAD.Units.Quantity("0.8 N*m"),
            peak_torque_rpm=FreeCAD.Units.Quantity("6000 1/min"),
            id="spindle-002",
        )

        # Create a test Lathe object
        self.test_lathe = Lathe(
            label="MyTestLathe",
            axes={
                "X": LinearAxis(
                    start=FreeCAD.Units.Quantity("0 mm"),
                    end=FreeCAD.Units.Quantity("150 mm"),
                    rigidity=FreeCAD.Units.Quantity("0.001 mm"),
                ),
                "Z": LinearAxis(
                    start=FreeCAD.Units.Quantity("-300 mm"),
                    end=FreeCAD.Units.Quantity("0 mm"),
                    rigidity=FreeCAD.Units.Quantity("0.002 mm"),
                ),
                "spindle": AngularAxis(
                    rigidity_x=FreeCAD.Units.Quantity("0.15 deg"),
                    rigidity_y=FreeCAD.Units.Quantity("0.25 deg"),
                ),
            },
            max_workpiece_diameter=FreeCAD.Units.Quantity("200 mm"),
            post_processor="linuxcnc",
            id="lathe-test-01",
        )
        self.test_lathe.add_spindle(self.spindle1)

        # Create a test Mill object
        self.test_mill = Mill(
            label="MyTestMill",
            axes={
                "X": LinearAxis(
                    start=FreeCAD.Units.Quantity("-250 mm"),
                    end=FreeCAD.Units.Quantity("250 mm"),
                    rigidity=FreeCAD.Units.Quantity("0.0015 mm"),
                    max_feed=FreeCAD.Units.Quantity("2000 mm/min"),
                ),
                "Y": LinearAxis(
                    start=FreeCAD.Units.Quantity("-200 mm"),
                    end=FreeCAD.Units.Quantity("200 mm"),
                    rigidity=FreeCAD.Units.Quantity("0.0015 mm"),
                    max_feed=FreeCAD.Units.Quantity("2000 mm/min"),
                ),
                "Z": LinearAxis(
                    start=FreeCAD.Units.Quantity("-100 mm"),
                    end=FreeCAD.Units.Quantity("150 mm"),
                    rigidity=FreeCAD.Units.Quantity("0.0025 mm"),
                    max_feed=FreeCAD.Units.Quantity("2000 mm/min"),
                ),
                "spindle": AngularAxis(
                    rigidity_x=FreeCAD.Units.Quantity("0.35 deg"),
                    rigidity_y=FreeCAD.Units.Quantity("0.45 deg"),
                ),
            },
            post_processor="grbl",
            id="mill-test-01",
        )
        self.test_mill.spindles = [self.spindle1, self.spindle2]

    def test_serialize_lathe(self):
        """Test serialization of a Lathe object."""
        serialized_data = self.serializer_class.serialize(self.test_lathe)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["id"], "lathe-test-01")
        self.assertEqual(data["type"], "Lathe")
        self.assertEqual(data["label"], "MyTestLathe")
        self.assertEqual(data["max_workpiece_diameter"], "200.00 mm")
        self.assertEqual(data["axes"]["X"]["start"], "0.00 mm")
        self.assertEqual(data["axes"]["X"]["end"], "150.00 mm")
        self.assertEqual(data["axes"]["X"]["rigidity"], "1.00 µm/N")
        self.assertEqual(data["axes"]["Z"]["start"], "-300.00 mm")
        self.assertEqual(data["axes"]["Z"]["end"], "0.00 mm")
        self.assertEqual(data["axes"]["Z"]["rigidity"], "2.00 µm/N")
        self.assertEqual(data["axes"]["spindle"]["rigidity-x"], "0.15°/N")
        self.assertEqual(data["axes"]["spindle"]["rigidity-y"], "0.25°/N")
        self.assertIn(self.spindle1.get_id(), data["spindles"])

    def test_serialize_mill(self):
        """Test serialization of a Mill object."""
        serialized_data = self.serializer_class.serialize(self.test_mill)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["id"], "mill-test-01")
        self.assertEqual(data["type"], "Mill")
        self.assertEqual(data["label"], "MyTestMill")
        self.assertEqual(data["axes"]["X"]["start"], "-250.00 mm")
        self.assertEqual(data["axes"]["X"]["end"], "250.00 mm")
        self.assertEqual(data["axes"]["X"]["rigidity"], "1.50 µm/N")
        self.assertEqual(data["axes"]["X"]["max_feed"], "33.33 mm/s")
        self.assertEqual(data["axes"]["Y"]["start"], "-200.00 mm")
        self.assertEqual(data["axes"]["Y"]["end"], "200.00 mm")
        self.assertEqual(data["axes"]["Y"]["rigidity"], "1.50 µm/N")
        self.assertEqual(data["axes"]["Y"]["max_feed"], "33.33 mm/s")
        self.assertEqual(data["axes"]["Z"]["start"], "-100.00 mm")
        self.assertEqual(data["axes"]["Z"]["end"], "150.00 mm")
        self.assertEqual(data["axes"]["Z"]["rigidity"], "2.50 µm/N")
        self.assertEqual(data["axes"]["Z"]["max_feed"], "33.33 mm/s")
        self.assertEqual(data["axes"]["spindle"]["rigidity-x"], "0.35°/N")
        self.assertEqual(data["axes"]["spindle"]["rigidity-y"], "0.45°/N")
        self.assertEqual(len(data["spindles"]), 2)
        self.assertIn(self.spindle1.get_id(), data["spindles"])
        self.assertIn(self.spindle2.get_id(), data["spindles"])

    def test_extract_dependencies(self):
        """Test dependency extraction for a machine."""
        serialized_data = self.serializer_class.serialize(self.test_mill)
        dependencies = self.serializer_class.extract_dependencies(serialized_data)

        self.assertIsInstance(dependencies, list)
        self.assertEqual(len(dependencies), 2)
        self.assertIn(self.spindle1.get_uri(), dependencies)
        self.assertIn(self.spindle2.get_uri(), dependencies)

    def test_deserialize_lathe(self):
        """Test deserialization of a Lathe object."""
        lathe_yaml_str = """
id: lathe-test-01
label: MyTestLathe
max_workpiece_diameter: 200 mm
post_processor: linuxcnc
post_processor_args: ''
axes:
  X:
    end: 150 mm
    rigidity: 0.001 mm/N
    start: 0 mm
  Z:
    end: 0 mm
    rigidity: 0.002 mm/N
    start: -300 mm
  spindle:
    type: angular
    rigidity-x: 0.0001 deg/N
    rigidity-y: 0.0001°/N
spindles:
- spindle-001
type: Lathe
"""
        lathe_data = lathe_yaml_str.encode("utf-8")

        dependencies: Mapping[AssetUri, Asset] = {
            self.spindle1.get_uri(): self.spindle1,
        }

        deserialized_lathe = cast(
            Lathe,
            self.serializer_class.deserialize(
                lathe_data, id="lathe-test-01", dependencies=dependencies
            ),
        )

        self.assertIsInstance(deserialized_lathe, Lathe)
        self.assertEqual(deserialized_lathe.label, "MyTestLathe")
        self.assertEqual(str(deserialized_lathe.x_axis.end), "150.0 mm")
        self.assertEqual(str(deserialized_lathe.z_axis.end), "0.0 mm")
        self.assertEqual(str(deserialized_lathe.spindle_axis.rigidity_x), "0.0001 deg")
        self.assertEqual(str(deserialized_lathe.spindle_axis.rigidity_y), "0.0001 deg")
        self.assertEqual(len(deserialized_lathe.spindles), 1)
        self.assertEqual(deserialized_lathe.spindles[0].label, "Main Spindle")

    def test_deserialize_mill(self):
        """Test deserialization of a Mill object."""
        mill_yaml_str = """
id: mill-test-01
label: MyTestMill
post_processor: grbl
post_processor_args: --option=1
axes:
  X:
    end: 250 mm
    rigidity: 0.0015 mm/N
    start: -250 mm
  Y:
    end: 200 mm
    rigidity: 0.0015 mm/N
    start: -200 mm
  Z:
    end: 150 mm
    rigidity: 0.0025 mm/N
    start: -100 mm
    max_feed: 2000 mm/min
  spindle:
    type: angular
    rigidity-x: 0.0002°/N
    rigidity-y: 0.0002 deg/N
spindles:
- spindle-001
- spindle-002
type: Mill
"""
        mill_data = mill_yaml_str.encode("utf-8")

        dependencies: Mapping[AssetUri, Asset] = {
            self.spindle1.get_uri(): self.spindle1,
            self.spindle2.get_uri(): self.spindle2,
        }

        deserialized_mill = cast(
            Mill,
            self.serializer_class.deserialize(
                mill_data, id="mill-test-01", dependencies=dependencies
            ),
        )

        self.assertIsInstance(deserialized_mill, Mill)
        self.assertEqual(deserialized_mill.label, "MyTestMill")
        self.assertEqual(deserialized_mill.post_processor, "grbl")
        self.assertEqual(deserialized_mill.post_processor_args, "--option=1")
        self.assertEqual(str(deserialized_mill.x_axis.end), "250.0 mm")
        self.assertEqual(str(deserialized_mill.y_axis.end), "200.0 mm")
        self.assertEqual(str(deserialized_mill.z_axis.end), "150.0 mm")
        self.assertEqual(str(deserialized_mill.spindle_axis.rigidity_x), "0.0002 deg")
        self.assertEqual(str(deserialized_mill.spindle_axis.rigidity_y), "0.0002 deg")
        self.assertEqual(len(deserialized_mill.spindles), 2)
        spindle_labels = {s.label for s in deserialized_mill.spindles}
        self.assertIn("Main Spindle", spindle_labels)
        self.assertIn("Secondary Spindle", spindle_labels)


if __name__ == "__main__":
    unittest.main()
