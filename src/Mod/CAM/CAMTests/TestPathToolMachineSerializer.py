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
            max_power="2.2 kW",
            min_rpm="1000 1/min",
            max_rpm="24000 1/min",
            max_torque="1.0 N*m",
            peak_torque_rpm="12000 1/min",
            id="spindle-001",
        )

        self.spindle2 = Spindle(
            label="Secondary Spindle",
            max_power="1.5 kW",
            min_rpm="500 1/min",
            max_rpm="12000 1/min",
            max_torque="0.8 N*m",
            peak_torque_rpm="6000 1/min",
            id="spindle-002",
        )

        # Create a test Lathe object
        self.test_lathe = Lathe(
            label="MyTestLathe",
            x_extent=(FreeCAD.Units.Quantity("0 mm"), FreeCAD.Units.Quantity("150 mm")),
            z_extent=(FreeCAD.Units.Quantity("-300 mm"), FreeCAD.Units.Quantity("0 mm")),
            max_feed=FreeCAD.Units.Quantity("5000 mm/min"),
            max_workpiece_diameter=FreeCAD.Units.Quantity("200 mm"),
            rigidity=(
                FreeCAD.Units.Quantity("0.001 mm/N"),
                FreeCAD.Units.Quantity("0.002 mm/N"),
                FreeCAD.Units.Quantity("0.0001 mm/rad"),
            ),
            post_processor="linuxcnc",
            id="lathe-test-01",
        )
        self.test_lathe.add_spindle(self.spindle1)

        # Create a test Mill object
        self.test_mill = Mill(
            label="MyTestMill",
            x_extent=(FreeCAD.Units.Quantity("-250 mm"), FreeCAD.Units.Quantity("250 mm")),
            y_extent=(FreeCAD.Units.Quantity("-200 mm"), FreeCAD.Units.Quantity("200 mm")),
            z_extent=(FreeCAD.Units.Quantity("-100 mm"), FreeCAD.Units.Quantity("150 mm")),
            max_feed=FreeCAD.Units.Quantity("8000 mm/min"),
            rigidity=(
                FreeCAD.Units.Quantity("0.0015 mm/N"),
                FreeCAD.Units.Quantity("0.0015 mm/N"),
                FreeCAD.Units.Quantity("0.0025 mm/N"),
                FreeCAD.Units.Quantity("0.0002 mm/rad"),
            ),
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
        self.assertEqual(data["max_feed"], "83.33 mm/s")
        self.assertEqual(data["max_workpiece_diameter"], "200.00 mm")
        self.assertEqual(data["x_extent"], ["0.00 mm", "150.00 mm"])
        self.assertIn(self.spindle1.get_id(), data["spindles"])

    def test_serialize_mill(self):
        """Test serialization of a Mill object."""
        serialized_data = self.serializer_class.serialize(self.test_mill)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["id"], "mill-test-01")
        self.assertEqual(data["type"], "Mill")
        self.assertEqual(data["label"], "MyTestMill")
        self.assertEqual(data["max_feed"], "133.33 mm/s")
        self.assertEqual(data["y_extent"], ["-200.00 mm", "200.00 mm"])
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
max_feed: 83.33 mm/s
max_workpiece_diameter: 200 mm
post_processor: linuxcnc
post_processor_args: ''
rigidity:
  rotational: 0.0001 mm/rad
  x: 0.001 mm/N
  z: 0.002 mm/N
spindles:
- spindle-001
type: Lathe
x_extent: [0 mm, 150 mm]
z_extent: [-300 mm, 0 mm]
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
        self.assertEqual(str(deserialized_lathe.max_feed), "83.33 mm/s")
        self.assertEqual(str(deserialized_lathe.x_max), "150.0 mm")
        self.assertEqual(len(deserialized_lathe.spindles), 1)
        self.assertEqual(deserialized_lathe.spindles[0].label, "Main Spindle")

    def test_deserialize_mill(self):
        """Test deserialization of a Mill object."""
        mill_yaml_str = """
id: mill-test-01
label: MyTestMill
max_feed: 8000 mm/min
post_processor: grbl
post_processor_args: --option=1
rigidity:
  rotational: 0.0002 mm/rad
  x: 0.0015 mm/N
  y: 0.0015 mm/N
  z: 0.0025 mm/N
spindles:
- spindle-001
- spindle-002
type: Mill
x_extent: [-250 mm, 250 mm]
y_extent: [-200 mm, 200 mm]
z_extent: [-100 mm, 150 mm]
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
        self.assertEqual(deserialized_mill.max_feed.UserString, "133.33 mm/s")
        self.assertEqual(str(deserialized_mill.y_max), "200.0 mm")
        self.assertEqual(len(deserialized_mill.spindles), 2)
        spindle_labels = {s.label for s in deserialized_mill.spindles}
        self.assertIn("Main Spindle", spindle_labels)
        self.assertIn("Secondary Spindle", spindle_labels)


if __name__ == "__main__":
    unittest.main()
