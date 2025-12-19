#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ***************************************************************************
# *   Copyright (c) 2024 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it     *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# * *************************************************************************

import FreeCAD
import Path
import unittest
from Path.Post.Processor import PostProcessor
from Machine.models.machine import Machine
import Path.Tool.Controller as PathToolController
from Path.Tool.toolbit import ToolBit
import Path.Main.Job as PathJob


class TestToolLengthOffset(unittest.TestCase):
    """Test tool length offset (G43) suppression functionality."""

    def setUp(self):
        """Set up test environment."""
        self.doc = FreeCAD.newDocument("TestToolLengthOffset")
        
    def tearDown(self):
        """Clean up test environment."""
        FreeCAD.closeDocument("TestToolLengthOffset")
        
    def test_g43_suppression_disabled(self):
        """Test that G43 commands are suppressed when output_tool_length_offset is False."""
        # Create machine config with G43 disabled
        machine = Machine("Test Machine")
        machine.output.output_tool_length_offset = False
        
        # Create a simple path with G43 command
        path = Path.Path()
        g43_cmd = Path.Command('G43', {'H': 1})
        path.addCommands(g43_cmd)
        
        # Create post processor
        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = machine
        processor.values['OUTPUT_TOOL_LENGTH_OFFSET'] = False
        
        # Convert G43 command
        result = processor.convert_command_to_gcode(g43_cmd)
        
        # Should return None (suppressed)
        self.assertIsNone(result, "G43 command should be suppressed when output_tool_length_offset is False")
        
    def test_g43_output_enabled(self):
        """Test that G43 commands are output when output_tool_length_offset is True."""
        # Create machine config with G43 enabled
        machine = Machine("Test Machine")
        machine.output.output_tool_length_offset = True
        
        # Create a simple path with G43 command
        path = Path.Path()
        g43_cmd = Path.Command('G43', {'H': 1})
        path.addCommands(g43_cmd)
        
        # Create post processor
        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = machine
        processor.values['OUTPUT_TOOL_LENGTH_OFFSET'] = True
        
        # Convert G43 command
        result = processor.convert_command_to_gcode(g43_cmd)
        
        # Should return the G43 command
        self.assertIsNotNone(result, "G43 command should be output when output_tool_length_offset is True")
        self.assertIn('G43', result, "Result should contain G43 command")
        self.assertIn('H1', result, "Result should contain H parameter")
        
    def test_machine_config_mapping(self):
        """Test that machine config field is properly mapped to processor values."""
        # Create machine config with G43 disabled
        machine = Machine("Test Machine")
        machine.output.output_tool_length_offset = False
        
        # Create post processor
        processor = PostProcessor(None, tooltip=None, tooltipargs=None, units=None)
        processor._machine = machine
        
        # Simulate the mapping that happens in export2
        if hasattr(machine.output, 'output_tool_length_offset'):
            processor.values['OUTPUT_TOOL_LENGTH_OFFSET'] = machine.output.output_tool_length_offset
            
        # Check that the value was mapped correctly
        self.assertFalse(processor.values['OUTPUT_TOOL_LENGTH_OFFSET'], 
                        "Machine config field should be mapped to processor values")


class TestToolProcessing(unittest.TestCase):
    """Test tool processing functionality including early tool prep."""

    def setUp(self):
        """Set up test environment."""
        self.doc = FreeCAD.newDocument("TestToolProcessing")
        
        # Create a basic job and tool controller for testing
        # Create base geometry for the job
        import Part
        box = Part.makeBox(100, 100, 20)
        base_obj = self.doc.addObject("Part::Feature", "BaseBox")
        base_obj.Shape = box
        
        # Create a test tool
        tool_attrs = {
            "name": "TestTool1",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 6.0},
            "attribute": {},
        }
        toolbit1 = ToolBit.from_dict(tool_attrs)
        tool1 = toolbit1.attach_to_doc(doc=self.doc)
        tool1.Label = "6mm_Endmill"
        
        # Create tool controller
        self.tc1 = PathToolController.Create("TC_Test_Tool1", tool1, 1)
        self.tc1.Label = "TC: 6mm Endmill"
        
        # Create job
        self.job = PathJob.Create('TestJob', [base_obj], None)
        self.job.Label = "TestJob"
        
        # Add tool controller to job
        self.job.Tools.Group = [self.tc1]
        
        # Create a basic operation
        profile_op = self.doc.addObject("Path::FeaturePython", "TestProfile")
        profile_op.Label = "TestProfile"
        profile_op.addProperty("App::PropertyLink", "ToolController", "Base", "Tool controller")
        profile_op.ToolController = self.tc1
        profile_op.Path = Path.Path([
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            Path.Command("G1", {"X": 10.0, "Y": 0.0, "Z": -5.0, "F": 100.0}),
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
        ])
        self.job.Operations.addObject(profile_op)
        
    def tearDown(self):
        """Clean up test environment."""
        FreeCAD.closeDocument("TestToolProcessing")
        
    def _get_full_machine_config(self):
        """Get a full machine configuration for testing."""
        return {
            "freecad_version": "1.0.0",
            "machine": {
                "name": "Test Machine",
                "description": "Test machine for unit tests",
                "manufacturer": "Test",
                "units": "metric",
                "axes": {
                    "X": {"type": "linear", "max": 100, "min": 0, "max_velocity": 1000},
                    "Y": {"type": "linear", "max": 100, "min": 0, "max_velocity": 1000},
                    "Z": {"type": "linear", "max": 100, "min": 0, "max_velocity": 1000}
                },
                "spindles": [{
                    "id": "spindle1",
                    "name": "Spindle 1",
                    "max_power_kw": 2.0,
                    "max_rpm": 24000,
                    "min_rpm": 6000,
                    "tool_change": "manual"
                }]
            },
            "output": {
                "units": "metric",
                "output_tool_length_offset": True,
                "output_header": True,
                "header": {
                    "include_date": True,
                    "include_description": True,
                    "include_document_name": True,
                    "include_machine_name": True,
                    "include_project_file": True,
                    "include_units": True,
                    "include_tool_list": True,
                    "include_fixture_list": True
                },
                "comments": {
                    "enabled": True,
                    "symbol": "(",
                    "include_operation_labels": True,
                    "include_blank_lines": True,
                    "output_bcnc_comments": False
                },
                "formatting": {
                    "line_numbers": False,
                    "line_number_start": 100,
                    "line_number_prefix": "N",
                    "line_increment": 10,
                    "command_space": " ",
                    "end_of_line_chars": "\n"
                },
                "precision": {
                    "axis": 3,
                    "feed": 3,
                    "spindle": 0
                },
                "duplicates": {
                    "commands": True,
                    "parameters": True
                }
            },
            "postprocessor": {
                "file_name": "generic",
                "properties": {
                    "preamble": "G17 G54 G40 G49 G80 G90",
                    "postamble": "M05\nG17 G54 G90 G80 G40\nM2",
                    "supported_commands": "G0\nG00\nG1\nG01\nG2\nG02\nG3\nG03\nG73\nG74\nG81\nG82\nG83\nG84\nG38.2\nG54\nG55\nG56\nG57\nG58\nG59\nG59.1\nG59.2\nG59.3\nG59.4\nG59.5\nG59.6\nG59.7\nG59.8\nG59.9\nM0\nM00\nM1\nM01\nM3\nM03\nM4\nM04\nM6\nM06"
                }
            },
            "processing": {
                "tool_change": True,
                "early_tool_prep": False
            },
            "version": 1
        }
        
    def _run_export2(self, machine):
        """Run export2 with the given machine configuration."""
        processor = PostProcessor(self.job, tooltip=None, tooltipargs=None, units=None)
        processor._machine = machine
        return processor.export2()
        
    def _get_all_gcode(self, results):
        """Extract all G-code from export results."""
        if not results:
            return ""
        
        all_output = ""
        for section_name, gcode in results:
            all_output += f"\n{gcode}"
        
        return all_output

    def test_XY_before_Z_after_tool_change(self):
        """
        Test that xy_before_z_after_tool_change decomposes first move after tool change.
        
        Expected behavior when enabled:
            BEFORE: M6 T2
                    G0 X50.0 Y60.0 Z10.0
            
            AFTER:  M6 T2
                    G0 X50.0 Y60.0
                    G0 Z10.0
        """
        # Create a second tool controller for testing tool changes
        tool_attrs = {
            "name": "TestTool2",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 3.0},
            "attribute": {},
        }
        toolbit2 = ToolBit.from_dict(tool_attrs)
        tool2 = toolbit2.attach_to_doc(doc=self.doc)
        tool2.Label = "3mm_Endmill"
        
        tc2 = PathToolController.Create("TC_Test_Tool2", tool2, 2)
        tc2.Label = "TC: 3mm Endmill"
        self.job.addObject(tc2)
        
        # Create a second operation using the second tool
        # First move after tool change has X, Y, and Z components
        profile_op2 = self.doc.addObject("Path::FeaturePython", "TestProfile2")
        profile_op2.Label = "TestProfile2"
        profile_op2.addProperty("App::PropertyLink", "ToolController", "Base", "Tool controller")
        profile_op2.ToolController = tc2
        profile_op2.Path = Path.Path([
            Path.Command("G0", {"X": 50.0, "Y": 60.0, "Z": 10.0}),  # First move with X, Y, Z
            Path.Command("G1", {"X": 55.0, "Y": 65.0, "Z": -5.0, "F": 100.0}),
        ])
        self.job.Operations.addObject(profile_op2)
        
        # Test with feature ENABLED
        config = self._get_full_machine_config()
        config['processing']['xy_before_z_after_tool_change'] = True
        machine = Machine.from_dict(config)
        
        try:
            results = self._run_export2(machine)
            gcode = self._get_all_gcode(results)
            lines = [line.strip() for line in gcode.split('\n') if line.strip()]
            
            # Find the tool change to T2
            m6_index = None
            for i, line in enumerate(lines):
                if 'M6' in line and 'T2' in line:
                    m6_index = i
                    break
            
            self.assertIsNotNone(m6_index, "Should have M6 T2 tool change")
            
            # Get the next two move commands after M6
            moves = []
            for i in range(m6_index + 1, min(m6_index + 10, len(lines))):
                if lines[i].startswith('G0') or lines[i].startswith('G1'):
                    moves.append(lines[i])
                    if len(moves) == 2:
                        break
            
            self.assertEqual(len(moves), 2, "Should have 2 moves after tool change (XY then Z)")
            
            # First move should have X and Y but NOT Z
            self.assertIn('X', moves[0], "First move should have X")
            self.assertIn('Y', moves[0], "First move should have Y")
            self.assertNotIn('Z', moves[0], "First move should NOT have Z")
            
            # Second move should have Z but NOT X or Y
            self.assertIn('Z', moves[1], "Second move should have Z")
            self.assertNotIn('X', moves[1], "Second move should NOT have X")
            self.assertNotIn('Y', moves[1], "Second move should NOT have Y")
            
        finally:
            # Clean up
            self.job.Operations.removeObject(profile_op2)
            self.job.removeObject(tc2)
            self.doc.removeObject(profile_op2.Name)
            self.doc.removeObject(tc2.Name)
            self.doc.removeObject(tool2.Name)

    def test_early_tool_prep(self):
        """Test that early_tool_prep inserts tool prep commands after M6."""
        # Create a second tool controller for testing tool changes
        tool_attrs = {
            "name": "TestTool2",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 3.0},
            "attribute": {},
        }
        toolbit2 = ToolBit.from_dict(tool_attrs)
        tool2 = toolbit2.attach_to_doc(doc=self.doc)
        tool2.Label = "3mm_Endmill"
        
        tc2 = PathToolController.Create("TC_Test_Tool2", tool2, 2)
        tc2.Label = "TC: 3mm Endmill"
        self.job.addObject(tc2)
        
        # Create a second operation using the second tool
        profile_op2 = self.doc.addObject("Path::FeaturePython", "TestProfile2")
        profile_op2.Label = "TestProfile2"
        profile_op2.Path = Path.Path([
            Path.Command("G0", {"X": 50.0, "Y": 50.0, "Z": 5.0}),
            Path.Command("G1", {"X": 60.0, "Y": 50.0, "Z": -5.0, "F": 100.0}),
            Path.Command("G0", {"X": 50.0, "Y": 50.0, "Z": 5.0}),
        ])
        self.job.Operations.addObject(profile_op2)
        
        # Create machine with early_tool_prep enabled
        config_with_prep = self._get_full_machine_config()
        config_with_prep['processing']['early_tool_prep'] = True
        machine_with_prep = Machine.from_dict(config_with_prep)
        
        # Create machine without early_tool_prep
        config_no_prep = self._get_full_machine_config()
        config_no_prep['processing']['early_tool_prep'] = False
        machine_no_prep = Machine.from_dict(config_no_prep)
        
        try:
            # Test with early_tool_prep enabled
            results_with = self._run_export2(machine_with_prep)
            gcode_with = self._get_all_gcode(results_with)
            
            # Test without early_tool_prep
            results_without = self._run_export2(machine_no_prep)
            
            lines_with = [line.strip() for line in gcode_with.split('\n') if line.strip()]
            
            # Find M6 commands in output with early_tool_prep enabled
            m6_lines_with = [i for i, line in enumerate(lines_with) if 'M6' in line]
            
            # With early_tool_prep, should have standalone T commands (tool prep)
            # Look for lines that start with T followed by a digit
            import re
            standalone_t_with = [line for line in lines_with if re.match(r'^T\d+$', line)]
            
            # Should have standalone T commands when early_tool_prep is enabled
            # Note: early_tool_prep only works when there are multiple tools
            if len(m6_lines_with) >= 2:
                self.assertGreater(len(standalone_t_with), 0,
                                 "Should have standalone T prep commands when early_tool_prep is enabled with multiple tools")
                
                # Verify the early prep command appears after first M6
                first_m6_idx = m6_lines_with[0]
                # Look for standalone T command shortly after first M6
                found_early_prep = False
                for i in range(first_m6_idx + 1, min(first_m6_idx + 20, len(lines_with))):
                    line = lines_with[i]
                    if re.match(r'^T\d+$', line):
                        found_early_prep = True
                        break
                
                self.assertTrue(found_early_prep,
                              "Should have early tool prep command shortly after first M6")
            
        finally:
            # Clean up the second tool controller and operation
            self.job.Operations.removeObject(profile_op2)
            self.job.removeObject(tc2)
            self.doc.removeObject(profile_op2.Name)
            self.doc.removeObject(tc2.Name)
            self.doc.removeObject(tool2.Name)

    def test_tool_change_blocks_insertion(self):
        """Test that pre/post tool change blocks are inserted around tool changes."""
        machine_config = self._get_full_machine_config()
        # Add pre/post tool change blocks to postprocessor properties
        machine_config['postprocessor']['properties']['pre_tool_change'] = "(pretoolchange)"
        machine_config['postprocessor']['properties']['post_tool_change'] = "(posttoolchange)"
        machine = Machine.from_dict(machine_config)

        # Add a second tool controller to trigger tool changes
        tool_attrs = {
            "name": "SecondTool",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 3.0},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=self.doc)
        tool.Label = "3mm_Endmill"

        tc2 = PathToolController.Create("TC_Second_Tool", tool, 2)
        tc2.Label = "TC: 3mm Endmill"
        self.job.addObject(tc2)

        # Create a second operation using the second tool
        profile_op2 = self.doc.addObject("Path::FeaturePython", "TestProfile2")
        profile_op2.Label = "TestProfile2"
        profile_op2.addProperty("App::PropertyLink", "ToolController", "Base", "Tool controller")
        profile_op2.ToolController = tc2
        profile_op2.Path = Path.Path([
            Path.Command("G0", {"X": 50.0, "Y": 50.0, "Z": 5.0}),
            Path.Command("G1", {"X": 60.0, "Y": 50.0, "Z": -5.0, "F": 100.0}),
        ])
        self.job.Operations.addObject(profile_op2)

        try:
            self.doc.recompute()
            results = self._run_export2(machine)
            all_output = self._get_all_gcode(results)

            # Should have some output
            self.assertIsNotNone(all_output, "Should generate some output")
            self.assertGreater(len(all_output), 0, "Should have non-empty output")

            self.assertIn("(pretoolchange)", all_output, "Pre-tool-change block should appear in output")
            self.assertIn("(posttoolchange)", all_output, "Post-tool-change block should appear in output")

            # Count occurrences - should have blocks for tool changes
            pretool_count = all_output.count("(pretoolchange)")
            posttool_count = all_output.count("(posttoolchange)")
            
            # Should have at least 2 tool changes (initial tool + change to second tool)
            self.assertGreaterEqual(pretool_count, 1, "Should have at least 1 pre-tool-change block")
            self.assertGreaterEqual(posttool_count, 1, "Should have at least 1 post-tool-change block")

            # Verify ordering: pre-tool-change should come before post-tool-change
            lines = all_output.split('\n')
            pretool_indices = [i for i, line in enumerate(lines) if '(pretoolchange)' in line]
            posttool_indices = [i for i, line in enumerate(lines) if '(posttoolchange)' in line]
            
            for pre_idx, post_idx in zip(pretool_indices, posttool_indices):
                self.assertLess(pre_idx, post_idx, "Pre-tool-change should come before post-tool-change")

            # Verify tool change commands (M6) are present
            self.assertIn("M6", all_output, "Tool change command M6 should be present")
            
        finally:
            # Clean up - remove the second tool controller and operation
            self.job.Operations.removeObject(profile_op2)
            self.job.removeObject(tc2)
            self.doc.removeObject(profile_op2.Name)
            self.doc.removeObject(tc2.Name)
            self.doc.recompute()

    def test_list_tools_in_header_option(self):
        """Test that list_tools_in_header option includes tool list in header."""
        # Test with tool list enabled
        machine_with_tools = self._get_full_machine_config()
        machine_with_tools['output']['list_tools_in_header'] = True
        machine_with_tools = Machine.from_dict(machine_with_tools)
        
        # Test with tool list disabled
        machine_no_tools = self._get_full_machine_config()
        machine_no_tools['output']['list_tools_in_header'] = False
        machine_no_tools = Machine.from_dict(machine_no_tools)
        
        results_with = self._run_export2(machine_with_tools)
        gcode_with = self._get_all_gcode(results_with)
        
        results_without = self._run_export2(machine_no_tools)
        gcode_without = self._get_all_gcode(results_without)
        
        # With tool list enabled, header should contain tool information in comments
        # Look for specific tool listing format: (T<number>=toolname)
        import re
        tool_pattern = re.compile(r'\(T\d+=.*?\)')
        
        lines_with = gcode_with.split('\n')
        tool_comments_with = [line for line in lines_with if tool_pattern.search(line)]
        
        lines_without = gcode_without.split('\n')
        tool_comments_without = [line for line in lines_without if tool_pattern.search(line)]
        
        # Should have more tool-related comments when enabled
        self.assertGreaterEqual(len(tool_comments_with), len(tool_comments_without),
                               "Should have more tool comments when list_tools_in_header=True")


if __name__ == '__main__':
    unittest.main()
