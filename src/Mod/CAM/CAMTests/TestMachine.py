# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Brad Collette                                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify *
# *   it under the terms of the GNU Lesser General Public License (LGPL)   *
# *   as published by the Free Software Foundation; either version 2 of    *
# *   the License, or (at your option) any later version.                  *
# *   for detail see the LICENCE text file.                                *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import tempfile
import pathlib
import CAMTests.PathTestUtils as PathTestUtils
from Machine.models.machine import (
    Machine,
    Toolhead,
    OutputOptions,
    ProcessingOptions,
    MachineFactory,
)


class TestMachineDataclass(PathTestUtils.PathTestBase):
    """Test the unified Machine dataclass"""

    def setUp(self):
        """Set up test fixtures"""
        self.default_machine = Machine()

    def test_default_initialization(self):
        """Test that Machine initializes with sensible defaults"""
        machine = Machine()

        # Basic identification
        self.assertEqual(machine.name, "Default Machine")
        self.assertEqual(machine.manufacturer, "")
        self.assertEqual(machine.description, "")

        # Machine type is derived from axes configuration
        self.assertEqual(machine.machine_type, "custom")  # No axes configured yet

        # Add axes and verify machine type updates
        machine.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        machine.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        self.assertEqual(machine.machine_type, "custom")  # Still missing Z axis

        machine.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        self.assertEqual(machine.machine_type, "xyz")  # Now has XYZ axes

        # Add rotary axes and verify machine type updates
        machine.add_rotary_axis("A", FreeCAD.Vector(1, 0, 0), -120, 120)
        self.assertEqual(machine.machine_type, "xyza")

        machine.add_rotary_axis("C", FreeCAD.Vector(0, 0, 1), -360, 360)
        self.assertEqual(machine.machine_type, "xyzac")

        # Units and versioning
        self.assertEqual(machine.configuration_units, "metric")
        self.assertEqual(machine.version, 1)
        self.assertIsNotNone(machine.freecad_version)

        # Post-processor defaults
        self.assertIsInstance(machine.output, OutputOptions)
        self.assertIsInstance(machine.processing, ProcessingOptions)
        self.assertEqual(machine.postprocessor_file_name, "")
        self.assertIsInstance(machine.postprocessor_properties, dict)

    def test_custom_initialization(self):
        """Test Machine initialization with custom values and verify machine_type is derived"""
        # Create a 5-axis machine (XYZAC)
        machine = Machine(
            name="Test Mill",
            manufacturer="ACME Corp",
            description="5-axis mill",
            configuration_units="imperial",
        )

        # Add axes to make it a 5-axis machine
        machine.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        machine.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        machine.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        machine.add_rotary_axis("A", FreeCAD.Vector(1, 0, 0), -120, 120)
        machine.add_rotary_axis("C", FreeCAD.Vector(0, 0, 1), -360, 360)

        self.assertEqual(machine.name, "Test Mill")
        self.assertEqual(machine.manufacturer, "ACME Corp")
        self.assertEqual(machine.description, "5-axis mill")
        self.assertEqual(machine.machine_type, "xyzac")
        self.assertEqual(machine.configuration_units, "imperial")

    def test_configuration_units_property(self):
        """Test configuration_units property returns correct values"""
        metric_machine = Machine(configuration_units="metric")
        self.assertEqual(metric_machine.configuration_units, "metric")

        imperial_machine = Machine(configuration_units="imperial")
        self.assertEqual(imperial_machine.configuration_units, "imperial")


class TestOutputOptions(PathTestUtils.PathTestBase):
    """Test OutputOptions dataclass"""

    def test_default_initialization(self):
        """Test OutputOptions initialization with defaults"""
        opts = OutputOptions()

        # Default values - using current field names
        from Machine.models.machine import OutputUnits

        self.assertEqual(opts.output_units, OutputUnits.METRIC)
        self.assertEqual(opts.command_space, " ")
        self.assertEqual(opts.comment_symbol, "(")
        self.assertEqual(opts.end_of_line_chars, "\n")
        self.assertEqual(opts.line_increment, 10)
        self.assertEqual(opts.line_number_start, 100)
        self.assertFalse(opts.line_numbers)
        self.assertEqual(opts.line_number_prefix, "N")
        self.assertTrue(opts.output_comments)
        self.assertTrue(opts.output_blank_lines)
        self.assertTrue(opts.output_bcnc_comments)
        self.assertTrue(opts.output_header)
        self.assertFalse(opts.output_labels)
        self.assertTrue(opts.output_operation_labels)
        self.assertFalse(opts.list_tools_in_header)
        self.assertTrue(opts.list_fixtures_in_header)
        self.assertFalse(opts.machine_name_in_header)
        self.assertTrue(opts.description_in_header)
        self.assertTrue(opts.project_file_in_header)
        self.assertTrue(opts.output_units_in_header)
        self.assertTrue(opts.date_in_header)
        self.assertTrue(opts.document_name_in_header)
        self.assertTrue(opts.output_duplicate_parameters)
        self.assertTrue(opts.output_duplicate_commands)
        self.assertEqual(opts.axis_precision, 3)
        self.assertEqual(opts.feed_precision, 3)
        self.assertEqual(opts.spindle_precision, 0)

    def test_custom_initialization(self):
        """Test OutputOptions initialization with custom values"""
        from Machine.models.machine import OutputUnits

        opts = OutputOptions(
            output_units=OutputUnits.IMPERIAL,
            command_space="",
            comment_symbol=";",
            end_of_line_chars="\r\n",
            line_increment=5,
            line_number_start=10,
            line_numbers=True,
            line_number_prefix="L",
            output_comments=False,
            output_blank_lines=False,
            output_bcnc_comments=False,
            output_header=False,
            output_labels=True,
            output_operation_labels=False,
            list_tools_in_header=True,
            list_fixtures_in_header=False,
            machine_name_in_header=True,
            description_in_header=False,
            project_file_in_header=False,
            output_units_in_header=False,
            date_in_header=False,
            document_name_in_header=False,
            output_duplicate_parameters=False,
            output_duplicate_commands=False,
            axis_precision=4,
            feed_precision=2,
            spindle_precision=1,
        )

        # Verify custom values
        self.assertEqual(opts.output_units, OutputUnits.IMPERIAL)
        self.assertEqual(opts.command_space, "")
        self.assertEqual(opts.comment_symbol, ";")
        self.assertEqual(opts.end_of_line_chars, "\r\n")
        self.assertEqual(opts.line_increment, 5)
        self.assertEqual(opts.line_number_start, 10)
        self.assertTrue(opts.line_numbers)
        self.assertEqual(opts.line_number_prefix, "L")
        self.assertFalse(opts.output_comments)
        self.assertFalse(opts.output_blank_lines)
        self.assertFalse(opts.output_bcnc_comments)
        self.assertFalse(opts.output_header)
        self.assertTrue(opts.output_labels)
        self.assertFalse(opts.output_operation_labels)
        self.assertTrue(opts.list_tools_in_header)
        self.assertFalse(opts.list_fixtures_in_header)
        self.assertTrue(opts.machine_name_in_header)
        self.assertFalse(opts.description_in_header)
        self.assertFalse(opts.project_file_in_header)
        self.assertFalse(opts.output_units_in_header)
        self.assertFalse(opts.date_in_header)
        self.assertFalse(opts.document_name_in_header)
        self.assertFalse(opts.output_duplicate_parameters)
        self.assertFalse(opts.output_duplicate_commands)
        self.assertEqual(opts.axis_precision, 4)
        self.assertEqual(opts.feed_precision, 2)
        self.assertEqual(opts.spindle_precision, 1)

    def test_equality(self):
        """Test OutputOptions equality comparison"""
        opts1 = OutputOptions()
        opts2 = OutputOptions()
        self.assertEqual(opts1, opts2)

        opts2.output_comments = False
        self.assertNotEqual(opts1, opts2)


class TestProcessingOptions(PathTestUtils.PathTestBase):
    """Test ProcessingOptions dataclass"""

    def test_default_initialization(self):
        """Test ProcessingOptions initialization with defaults"""
        opts = ProcessingOptions()

        # Default values
        self.assertFalse(opts.early_tool_prep)
        self.assertFalse(opts.filter_inefficient_moves)
        self.assertFalse(opts.split_arcs)
        self.assertTrue(opts.tool_change)
        self.assertFalse(opts.translate_rapid_moves)
        self.assertIsNone(opts.return_to)

    def test_custom_initialization(self):
        """Test ProcessingOptions initialization with custom values"""
        opts = ProcessingOptions(
            early_tool_prep=True,
            filter_inefficient_moves=True,
            split_arcs=True,
            tool_change=False,
            translate_rapid_moves=True,
            return_to=(10.0, 20.0, 30.0),
        )

        # Verify custom values
        self.assertTrue(opts.early_tool_prep)
        self.assertTrue(opts.filter_inefficient_moves)
        self.assertTrue(opts.split_arcs)
        self.assertFalse(opts.tool_change)
        self.assertTrue(opts.translate_rapid_moves)
        self.assertEqual(opts.return_to, (10.0, 20.0, 30.0))

    def test_equality(self):
        """Test ProcessingOptions equality comparison"""
        opts1 = ProcessingOptions()
        opts2 = ProcessingOptions()
        self.assertEqual(opts1, opts2)

        opts2.filter_inefficient_moves = True
        self.assertNotEqual(opts1, opts2)


class TestToolhead(PathTestUtils.PathTestBase):
    """Test Toolhead dataclass"""

    def test_toolhead_initialization(self):
        """Test Toolhead initialization with defaults"""
        toolhead = Toolhead(
            name="Main Toolhead",
            max_power_kw=5.5,
            max_rpm=24000,
            min_rpm=1000,
            tool_change="automatic",
        )

        self.assertEqual(toolhead.name, "Main Toolhead")
        self.assertEqual(toolhead.max_power_kw, 5.5)
        self.assertEqual(toolhead.max_rpm, 24000)
        self.assertEqual(toolhead.min_rpm, 1000)
        self.assertEqual(toolhead.tool_change, "automatic")
        # Default toolhead_wait should be 0.0
        self.assertEqual(toolhead.toolhead_wait, 0.0)

    def test_toolhead_serialization(self):
        """Test to_dict and from_dict"""
        toolhead = Toolhead(
            name="Test Toolhead",
            id="toolhead-001",
            max_power_kw=3.0,
            max_rpm=18000,
            min_rpm=500,
            tool_change="manual",
            toolhead_wait=1.5,
        )

        data = toolhead.to_dict()
        self.assertEqual(data["name"], "Test Toolhead")
        self.assertEqual(data["id"], "toolhead-001")
        self.assertEqual(data["max_power_kw"], 3.0)
        self.assertEqual(data["toolhead_wait"], 1.5)

        restored = Toolhead.from_dict(data)
        self.assertEqual(restored.name, toolhead.name)
        self.assertEqual(restored.id, toolhead.id)
        self.assertEqual(restored.max_power_kw, toolhead.max_power_kw)
        self.assertEqual(restored.toolhead_wait, toolhead.toolhead_wait)


class TestMachineFactory(PathTestUtils.PathTestBase):
    """Test MachineFactory class for loading/saving configurations"""

    def setUp(self):
        """Set up test fixtures with temporary directory"""
        self.temp_dir = tempfile.mkdtemp()
        self.temp_path = pathlib.Path(self.temp_dir)
        MachineFactory.set_config_directory(self.temp_dir)

    def tearDown(self):
        """Clean up temporary directory"""
        import shutil

        if self.temp_path.exists():
            shutil.rmtree(self.temp_path)

    def test_set_and_get_config_directory(self):
        """Test setting and getting configuration directory"""
        test_dir = self.temp_path / "test_configs"
        MachineFactory.set_config_directory(test_dir)

        config_dir = MachineFactory.get_config_directory()
        self.assertEqual(config_dir, test_dir)
        self.assertTrue(config_dir.exists())

    def test_save_and_load_configuration(self):
        """Test saving and loading a machine configuration"""
        # Create a test machine
        machine = Machine(
            name="Test Machine",
            manufacturer="Test Corp",
            description="Test description",
            configuration_units="metric",
        )

        # Add axes to make it an XYZ machine
        machine.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        machine.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        machine.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))

        # Add a toolhead
        toolhead = Toolhead(
            name="Main Toolhead",
            max_power_kw=5.5,
            max_rpm=24000,
            min_rpm=1000,
        )
        machine.toolheads.append(toolhead)

        # Save configuration
        filepath = MachineFactory.save_configuration(machine, "test_machine.fcm")
        self.assertTrue(filepath.exists())

        # Load configuration
        loaded_machine = MachineFactory.load_configuration("test_machine.fcm")

        # Verify loaded data
        self.assertEqual(loaded_machine.name, "Test Machine")
        self.assertEqual(loaded_machine.manufacturer, "Test Corp")
        self.assertEqual(loaded_machine.description, "Test description")
        self.assertEqual(loaded_machine.machine_type, "xyz")
        self.assertEqual(loaded_machine.configuration_units, "metric")
        self.assertEqual(len(loaded_machine.toolheads), 1)
        self.assertEqual(loaded_machine.toolheads[0].name, "Main Toolhead")

    def test_save_configuration_auto_filename(self):
        """Test saving with automatic filename generation"""
        machine = Machine(name="My Test Machine")

        filepath = MachineFactory.save_configuration(machine)

        # Should create file with sanitized name
        self.assertTrue(filepath.exists())
        self.assertEqual(filepath.name, "My_Test_Machine.fcm")

    def test_load_nonexistent_file(self):
        """Test loading a file that doesn't exist"""
        with self.assertRaises(FileNotFoundError):
            MachineFactory.load_configuration("nonexistent.fcm")

    def test_create_default_machine_data(self):
        """Test creating default machine data dictionary"""
        data = MachineFactory.create_default_machine_data()

        self.assertIsInstance(data, dict)
        # The data structure has nested "machine" key
        self.assertIn("machine", data)
        self.assertEqual(data["machine"]["name"], "New Machine")
        self.assertIn("toolheads", data["machine"])

    def test_list_configuration_files(self):
        """Test listing available configuration files"""
        # Create some test configurations
        machine1 = Machine(name="Machine 1")
        machine2 = Machine(name="Machine 2")

        MachineFactory.save_configuration(machine1, "machine1.fcm")
        MachineFactory.save_configuration(machine2, "machine2.fcm")

        # List configurations
        configs = MachineFactory.list_configuration_files()

        # Should include <any> plus our two machines
        self.assertGreaterEqual(len(configs), 3)
        self.assertEqual(configs[0][0], "<any>")

        # Check that our machines are in the list (by display name, not filename)
        names = [name for name, path in configs]
        self.assertIn("Machine 1", names)
        self.assertIn("Machine 2", names)

    def test_list_configurations(self):
        """Test listing configuration names"""
        machine = Machine(name="Test Machine")
        MachineFactory.save_configuration(machine, "test.fcm")

        configs = MachineFactory.list_configurations()

        self.assertIsInstance(configs, list)
        self.assertIn("<any>", configs)
        # Returns display name from JSON, not filename
        self.assertIn("Test Machine", configs)

    def test_delete_configuration(self):
        """Test deleting a configuration file"""
        machine = Machine(name="To Delete")
        filepath = MachineFactory.save_configuration(machine, "delete_me.fcm")

        self.assertTrue(filepath.exists())

        # Delete the configuration
        result = MachineFactory.delete_configuration("delete_me.fcm")
        self.assertTrue(result)
        self.assertFalse(filepath.exists())

        # Try deleting again (should return False)
        result = MachineFactory.delete_configuration("delete_me.fcm")
        self.assertFalse(result)

    def test_get_builtin_config(self):
        """Test getting built-in machine configurations"""
        # Test each built-in config type
        config_types = ["XYZ", "XYZAC", "XYZBC", "XYZA", "XYZB"]

        for config_type in config_types:
            machine = MachineFactory.get_builtin_config(config_type)
            self.assertIsInstance(machine, Machine)
            self.assertIsNotNone(machine.name)

    def test_get_builtin_config_invalid_type(self):
        """Test getting built-in config with invalid type"""
        with self.assertRaises(ValueError):
            MachineFactory.get_builtin_config("INVALID")

    def test_serialization_roundtrip(self):
        """Test full serialization roundtrip with complex machine"""
        # Create a complex machine with all components
        machine = Machine(
            name="Complex Machine",
            manufacturer="Test Mfg",
            description="Full featured machine",
            configuration_units="metric",
        )

        # Add toolhead
        machine.toolheads.append(
            Toolhead(
                name="Main",
                max_power_kw=7.5,
                max_rpm=30000,
            )
        )

        # Configure post-processor settings
        machine.output.comments.enabled = False
        machine.output.precision.axis = 4
        machine.output.formatting.line_increment = 5

        # line_increment is set to default 10 in OutputOptions

        # Save and load
        filepath = MachineFactory.save_configuration(machine, "complex.fcm")
        loaded = MachineFactory.load_configuration(filepath)

        # Verify all components
        self.assertEqual(loaded.name, machine.name)
        self.assertEqual(loaded.manufacturer, machine.manufacturer)
        self.assertEqual(len(loaded.toolheads), 1)
        self.assertFalse(loaded.output.comments.enabled)
        self.assertEqual(loaded.output.precision.axis, 4)
        self.assertEqual(loaded.output.formatting.line_increment, 5)
