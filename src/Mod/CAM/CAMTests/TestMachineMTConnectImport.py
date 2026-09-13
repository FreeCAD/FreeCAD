# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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
#   See the GNU Lesser General Public License for more details.               #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses      #
#                                                                              #
################################################################################

"""Tests for building a Machine from an MTConnect probe document.

The LinuxCNC fixtures (probe_xyz_mm.xml, probe_xyz_inch.xml,
probe_xyzac_trt.xml) were generated with `python3 -m mtc.device_model` from
LinuxCNC sim configurations and include the LinuxCNC vendor extension block,
which the generic importer must ignore.  probe_foreign_vendor.xml and
probe_multi_device.xml are hand-written to model third-party agents.
"""

import pathlib
import unittest

from Machine.models.machine import AxisRole, Machine
from Machine.models.mtconnect_import import (
    ProbeParseError,
    list_devices,
    machine_from_probe,
)

FIXTURE_DIR = pathlib.Path(__file__).parent / "Fixtures" / "mtconnect"


def fixture(name):
    return (FIXTURE_DIR / name).read_text(encoding="utf-8")


class TestProbeParsing(unittest.TestCase):
    def test_rejects_malformed_xml(self):
        with self.assertRaises(ProbeParseError):
            machine_from_probe("<not-even-xml")

    def test_rejects_non_probe_document(self):
        with self.assertRaises(ProbeParseError):
            machine_from_probe("<SomeOtherDocument/>")

    def test_rejects_probe_without_devices(self):
        xml = '<MTConnectDevices xmlns="urn:mtconnect.org:MTConnectDevices:1.7"><Devices/></MTConnectDevices>'
        with self.assertRaises(ProbeParseError):
            machine_from_probe(xml)


class TestDeviceSelection(unittest.TestCase):
    def test_list_devices_excludes_agent(self):
        devices = list_devices(fixture("probe_multi_device.xml"))
        self.assertEqual([d["name"] for d in devices], ["mill-1", "mill-2"])

    def test_multiple_devices_require_a_name(self):
        with self.assertRaises(ProbeParseError):
            machine_from_probe(fixture("probe_multi_device.xml"))

    def test_select_device_by_name(self):
        machine, _ = machine_from_probe(fixture("probe_multi_device.xml"), device_name="mill-2")
        self.assertEqual(machine.name, "mill-2")
        self.assertEqual(machine.linear_axes["X"].max_limit, 800)

    def test_unknown_device_name(self):
        with self.assertRaises(ProbeParseError):
            machine_from_probe(fixture("probe_multi_device.xml"), device_name="lathe-9")


class TestLinuxCNC3Axis(unittest.TestCase):
    """Generated from the LinuxCNC axis_mm sim config."""

    @classmethod
    def setUpClass(cls):
        cls.machine, cls.report = machine_from_probe(fixture("probe_xyz_mm.xml"))

    def test_identity(self):
        self.assertEqual(self.machine.name, "LinuxCNC-HAL-SIM-AXIS")
        self.assertEqual(self.machine.manufacturer, "LinuxCNC")

    def test_axes_and_limits(self):
        self.assertEqual(set(self.machine.linear_axes), {"X", "Y", "Z"})
        self.assertEqual(self.machine.rotary_axes, {})
        self.assertEqual(self.machine.linear_axes["X"].min_limit, -254)
        self.assertEqual(self.machine.linear_axes["X"].max_limit, 254)
        self.assertEqual(self.machine.linear_axes["Z"].min_limit, -50.8)
        self.assertEqual(self.machine.linear_axes["Z"].max_limit, 101.6)

    def test_units_are_metric(self):
        self.assertEqual(self.machine.configuration_units, "metric")

    def test_machine_type(self):
        self.assertEqual(self.machine.machine_type, "xyz")

    def test_z_is_head_linear(self):
        self.assertEqual(self.machine.linear_axes["Z"].role, AxisRole.HEAD_LINEAR)
        self.assertEqual(self.machine.linear_axes["X"].role, AxisRole.TABLE_LINEAR)

    def test_spindle_toolhead_created(self):
        self.assertEqual(len(self.machine.toolheads), 1)
        self.assertTrue(self.machine.toolheads[0].is_rotary())

    def test_coolant_detected(self):
        self.assertTrue(self.machine.toolheads[0].coolant_flood)
        self.assertTrue(self.machine.toolheads[0].coolant_mist)

    def test_chain_is_valid(self):
        self.assertEqual(self.machine.validate_kinematic_chain(), [])

    def test_provenance_recorded(self):
        self.assertIn("MTConnect", self.machine.description)
        self.assertIn("linuxcnc-0001", self.machine.description)


class TestLinuxCNCInchMachine(unittest.TestCase):
    """An inch-native LinuxCNC machine still publishes canonical millimetres."""

    def test_imports_as_metric_with_mm_values(self):
        machine, _ = machine_from_probe(fixture("probe_xyz_inch.xml"))
        self.assertEqual(machine.configuration_units, "metric")
        self.assertEqual(machine.linear_axes["X"].max_limit, 254)
        self.assertEqual(machine.linear_axes["Z"].min_limit, -203.2)


class TestLinuxCNC5Axis(unittest.TestCase):
    """Generated from the LinuxCNC xyzac-trt sim config (extension ignored)."""

    @classmethod
    def setUpClass(cls):
        cls.machine, cls.report = machine_from_probe(fixture("probe_xyzac_trt.xml"))

    def test_machine_type(self):
        self.assertEqual(self.machine.machine_type, "xyzac")

    def test_rotary_limits(self):
        self.assertEqual(self.machine.rotary_axes["A"].min_limit, -100)
        self.assertEqual(self.machine.rotary_axes["A"].max_limit, 50)
        self.assertEqual(self.machine.rotary_axes["C"].min_limit, -36000)
        self.assertEqual(self.machine.rotary_axes["C"].max_limit, 36000)

    def test_chain_matches_factory_config(self):
        factory = Machine.create_AC_table_config()
        for letter in ("A", "C"):
            imported = self.machine.rotary_axes[letter]
            reference = factory.rotary_axes[letter]
            self.assertEqual(imported.role, reference.role, letter)
            self.assertEqual(imported.parent, reference.parent, letter)
            self.assertEqual(imported.sequence, reference.sequence, letter)
            self.assertEqual(
                (
                    imported.rotation_vector.x,
                    imported.rotation_vector.y,
                    imported.rotation_vector.z,
                ),
                (
                    reference.rotation_vector.x,
                    reference.rotation_vector.y,
                    reference.rotation_vector.z,
                ),
                letter,
            )
        self.assertEqual(self.machine.primary_rotary_axis, factory.primary_rotary_axis)
        self.assertEqual(self.machine.secondary_rotary_axis, factory.secondary_rotary_axis)

    def test_chain_assumption_reported(self):
        self.assertTrue(any("table" in item.lower() for item in self.report.assumed))
        self.assertIn("Import assumptions:", self.machine.kinematics.notes)

    def test_spindle_not_imported_as_axis(self):
        self.assertEqual(set(self.machine.rotary_axes), {"A", "C"})

    def test_chain_is_valid(self):
        self.assertEqual(self.machine.validate_kinematic_chain(), [])

    def test_serialization_roundtrip(self):
        data = self.machine.to_dict()
        restored = Machine.from_dict(data)
        self.assertEqual(restored.machine_type, "xyzac")
        self.assertEqual(restored.rotary_axes["A"].parent, "C")


class TestForeignVendor(unittest.TestCase):
    """A third-party agent: INCH units, suffixed names, no extension block."""

    @classmethod
    def setUpClass(cls):
        cls.machine, cls.report = machine_from_probe(fixture("probe_foreign_vendor.xml"))

    def test_identity(self):
        self.assertEqual(self.machine.name, "VMC-24")
        self.assertEqual(self.machine.manufacturer, "Example Machine Works")

    def test_imperial_units_kept(self):
        self.assertEqual(self.machine.configuration_units, "imperial")
        self.assertEqual(self.machine.linear_axes["X"].max_limit, 24)
        self.assertEqual(self.machine.linear_axes["Y"].max_limit, 16)
        self.assertEqual(self.machine.linear_axes["Z"].max_limit, 20)

    def test_suffixed_names_normalized(self):
        self.assertEqual(set(self.machine.linear_axes), {"X", "Y", "Z"})

    def test_velocity_converted_to_per_minute(self):
        # 10 in/s -> 600 in/min in an imperial configuration
        self.assertEqual(self.machine.linear_axes["X"].max_velocity, 600)

    def test_spindle_detected_by_rotary_velocity(self):
        self.assertEqual(self.machine.rotary_axes, {})
        toolhead = self.machine.toolheads[0]
        self.assertEqual(toolhead.min_rpm, 100)
        self.assertEqual(toolhead.max_rpm, 8100)

    def test_no_coolant(self):
        self.assertFalse(self.machine.toolheads[0].coolant_flood)


if __name__ == "__main__":
    unittest.main()
