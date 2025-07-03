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
from Path.Tool.machine.models.machine import MachineFeatureFlags
from Path.Tool.machine.serializers import MachineSerializer
from Path.Tool.machine.models.spindle import Spindle


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
            name="A",
            label="Main Spindle",
            max_power=FreeCAD.Units.Quantity("2.2 kW"),
            min_rpm=1000.0,
            max_rpm=24000.0,
            max_torque=1.0,
            peak_torque_rpm=12000.0,
            icon=None,
        )

        self.spindle2 = Spindle(
            name="B",
            label="Secondary Spindle",
            max_power=FreeCAD.Units.Quantity("1.5 kW"),
            min_rpm=500.0,
            max_rpm=12000.0,
            max_torque=0.8,
            peak_torque_rpm=6000.0,
            icon=None,
        )

        # Create a test Lathe object
        self.test_lathe = Lathe(
            name="MyTestLathe",
            label="My Test Lathe",
            post_processor="linuxcnc",
            feature_flags=[MachineFeatureFlags.TURNING],
            id="lathe-test-01",
        )
        self.test_lathe.x_axis.max_feed = FreeCAD.Units.Quantity("1000 mm/min")
        self.test_lathe.z_axis.max_feed = FreeCAD.Units.Quantity("1000 mm/min")
        self.test_lathe.a_axis.angular_rigidity = FreeCAD.Units.Quantity("0.25 deg")
        self.test_lathe.a_axis.rigidity_x = FreeCAD.Units.Quantity("0.001 mm/N")
        self.test_lathe.a_axis.rigidity_y = FreeCAD.Units.Quantity("0.001 mm/N")

        # Create a test Mill object
        self.test_mill = Mill(
            name="MyTestMill",
            label="My Test Mill",
            post_processor="grbl",
            feature_flags=[MachineFeatureFlags.MILLING_3D, MachineFeatureFlags.RIGID_TAPPING],
            id="mill-test-01",
        )
        self.test_mill.x_axis.max_feed = FreeCAD.Units.Quantity("2000 mm/min")
        self.test_mill.y_axis.max_feed = FreeCAD.Units.Quantity("2000 mm/min")
        self.test_mill.z_axis.max_feed = FreeCAD.Units.Quantity("1500 mm/min")
        self.test_mill.spindle_axis.angular_rigidity = FreeCAD.Units.Quantity("0.45 deg")
        self.test_mill.spindle_axis.rigidity_x = FreeCAD.Units.Quantity("0.001 mm/N")
        self.test_mill.spindle_axis.rigidity_y = FreeCAD.Units.Quantity("0.001 mm/N")

        self.lathe_yaml_str = """
type: Lathe
name: StandardLathe
label: Standard Lathe
icon: lathe
feature_flags:
  - TURNING
children:
  - type: Spindle
    name: MainSpindle
    max_rpm: 5000 rpm
    max_power: 3 kW
    children:
      - type: AngularAxis
        name: A
        angular_rigidity: 12 deg/N
        rigidity_x: 0.001 mm/N
        rigidity_y: 0.001 mm/N
        children: []
  - type: LinearAxis
    name: Z
    max_feed: 1000 mm/min
    children:
      - type: LinearAxis
        name: X
        max_feed: 1000 mm/min
        children:
          - type: MachineComponent
            name: ToolHolder
"""

        self.mill_yaml_str = """
type: Mill
name: Standard3AxisMill
label: Standard 3-Axis Mill
icon: mill
post_processor: grbl
feature_flags:
  - 3DMILLING
  - RIGID_TAPPING
children:
  - type: LinearAxis
    name: X
    max_feed: 2000 mm/min
    children:
      - type: LinearAxis
        name: Y
        max_feed: 2000 mm/min
        children:
          - type: LinearAxis
            name: Z
            max_feed: 1500 mm/min
            children:
              - type: Spindle
                name: MainSpindle
                max_rpm: 24000 rpm
                max_power: 5 kW
                children:
                  - type: AngularAxis
                    name: A
                    angular_rigidity: 12 deg/N
                    rigidity_x: 0.001 mm/N
                    rigidity_y: 0.001 mm/N
                    children: []
"""

    def test_serialize_lathe(self):
        """Test serialization of a Lathe object."""
        serialized_data = self.serializer_class.serialize(self.test_lathe)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["name"], "MyTestLathe")
        self.assertEqual(data["icon"], None)
        self.assertIn("TURNING", data["feature_flags"])
        self.assertEqual(data["post_processor"], "linuxcnc")
        self.assertEqual(data["post_processor_args"], "")

        # Check axes properties
        z_axis = data["children"][1]
        self.assertEqual(z_axis["name"], "Z")
        self.assertEqual(z_axis["max_feed"], f"{1000/60:.2f} mm/s")

        x_axis = z_axis["children"][0]
        self.assertEqual(x_axis["name"], "X")
        self.assertEqual(x_axis["max_feed"], f"{1000/60:.2f} mm/s")

        spindle_axis = data["children"][0]["children"][0]
        self.assertEqual(spindle_axis["name"], "A")
        self.assertEqual(spindle_axis["angular_rigidity"], "0.25°/N")
        self.assertEqual(spindle_axis["rigidity_x"], "0.001 mm/N")
        self.assertEqual(spindle_axis["rigidity_y"], "0.001 mm/N")

        # Check spindle properties
        spindle = data["children"][0]
        self.assertEqual(spindle["name"], "MainSpindle")
        self.assertEqual(spindle["max_rpm"], 60000)

    def test_serialize_mill(self):
        """Test serialization of a Mill object."""
        serialized_data = self.serializer_class.serialize(self.test_mill)
        self.assertIsInstance(serialized_data, bytes)

        data = yaml.safe_load(serialized_data.decode("utf-8"))
        self.assertEqual(data["name"], "MyTestMill")
        self.assertEqual(data["icon"], None)
        self.assertIn("3DMILLING", data["feature_flags"])
        self.assertIn("RIGID_TAPPING", data["feature_flags"])
        self.assertEqual(data["post_processor"], "grbl")
        self.assertEqual(data["post_processor_args"], "")

        # Check axes properties
        x_axis = data["children"][0]
        self.assertEqual(x_axis["name"], "X")
        self.assertEqual(x_axis["max_feed"], f"{2000/60:.2f} mm/s")

        y_axis = x_axis["children"][0]
        self.assertEqual(y_axis["name"], "Y")
        self.assertEqual(y_axis["max_feed"], f"{2000/60:.2f} mm/s")

        z_axis = y_axis["children"][0]
        self.assertEqual(z_axis["name"], "Z")
        self.assertEqual(z_axis["max_feed"], f"{1500/60:.2f} mm/s")

        spindle_a_axis = z_axis["children"][0]
        self.assertEqual(spindle_a_axis["name"], "A")
        self.assertEqual(spindle_a_axis["angular_rigidity"], "0.45°/N")
        self.assertEqual(spindle_a_axis["rigidity_x"], "0.001 mm/N")
        self.assertEqual(spindle_a_axis["rigidity_y"], "0.001 mm/N")

        main_spindle = spindle_a_axis["children"][0]
        self.assertEqual(main_spindle["name"], "MainSpindle")
        self.assertEqual(main_spindle["max_rpm"], 60000.0)

    def test_extract_dependencies(self):
        """Test dependency extraction for a machine."""
        serialized_data = self.serializer_class.serialize(self.test_mill)
        dependencies = self.serializer_class.extract_dependencies(serialized_data)

        self.assertIsInstance(dependencies, list)
        self.assertEqual(len(dependencies), 0)

    def test_deserialize_lathe(self):
        """Test deserialization of a Lathe object."""
        lathe_data = self.lathe_yaml_str.encode("utf-8")

        dependencies: Mapping[AssetUri, Asset] = {}

        deserialized_lathe = cast(
            Lathe,
            self.serializer_class.deserialize(
                lathe_data, id="lathe-test-01", dependencies=dependencies
            ),
        )

        self.assertIsInstance(deserialized_lathe, Lathe)
        self.assertEqual(deserialized_lathe.label, "Standard Lathe")
        self.assertEqual(deserialized_lathe.icon, "lathe")
        self.assertIn(MachineFeatureFlags.TURNING, deserialized_lathe.feature_flags)

        # Check axes properties
        self.assertEqual(deserialized_lathe.x_axis.name, "X")
        self.assertEqual(str(deserialized_lathe.x_axis.max_feed), f"{1000.0/60} mm/s")

        self.assertEqual(deserialized_lathe.z_axis.name, "Z")
        self.assertEqual(str(deserialized_lathe.z_axis.max_feed), f"{1000.0/60} mm/s")

        self.assertEqual(deserialized_lathe.a_axis.name, "A")
        self.assertEqual(str(deserialized_lathe.a_axis.angular_rigidity), "12.0 deg")
        self.assertEqual(deserialized_lathe.a_axis.rigidity_x.getValueAs("mm/N"), 0.001)
        self.assertEqual(deserialized_lathe.a_axis.rigidity_y.getValueAs("mm/N"), 0.001)

        # Check spindle properties
        self.assertEqual(len(deserialized_lathe.spindles), 1)
        self.assertEqual(deserialized_lathe.spindles[0].name, "MainSpindle")
        self.assertEqual(str(deserialized_lathe.spindles[0].max_rpm), "5000 rpm")

    def test_deserialize_mill(self):
        """Test deserialization of a Mill object."""
        mill_data = self.mill_yaml_str.encode("utf-8")

        dependencies: Mapping[AssetUri, Asset] = {}

        deserialized_mill = cast(
            Mill,
            self.serializer_class.deserialize(
                mill_data, id="mill-test-01", dependencies=dependencies
            ),
        )

        self.assertIsInstance(deserialized_mill, Mill)
        self.assertEqual(deserialized_mill.label, "Standard 3-Axis Mill")
        self.assertEqual(deserialized_mill.icon, "mill")
        self.assertIn(MachineFeatureFlags.MILLING_3D, deserialized_mill.feature_flags)
        self.assertIn(MachineFeatureFlags.RIGID_TAPPING, deserialized_mill.feature_flags)
        self.assertEqual(deserialized_mill.post_processor, "grbl")
        self.assertEqual(deserialized_mill.post_processor_args, "")

        # Check axes properties
        self.assertEqual(deserialized_mill.x_axis.name, "X")
        self.assertEqual(str(deserialized_mill.x_axis.max_feed), f"{2000/60} mm/s")

        self.assertEqual(deserialized_mill.y_axis.name, "Y")
        self.assertEqual(str(deserialized_mill.y_axis.max_feed), f"{2000/60} mm/s")

        self.assertEqual(deserialized_mill.z_axis.name, "Z")
        self.assertEqual(str(deserialized_mill.z_axis.max_feed), f"{1500/60} mm/s")

        self.assertEqual(deserialized_mill.spindle_axis.name, "A")
        self.assertEqual(str(deserialized_mill.spindle_axis.angular_rigidity), "12.0 deg")
        self.assertEqual(deserialized_mill.spindle_axis.rigidity_x.getValueAs("mm/N"), 0.001)
        self.assertEqual(deserialized_mill.spindle_axis.rigidity_y.getValueAs("mm/N"), 0.001)

        # Check spindle properties
        self.assertEqual(len(deserialized_mill.spindle_axis.children), 0)
        main_spindle = cast(Spindle, deserialized_mill.spindles[0])
        self.assertEqual(main_spindle.name, "MainSpindle")
        self.assertEqual(str(main_spindle.max_rpm), "24000 rpm")


if __name__ == "__main__":
    unittest.main()
