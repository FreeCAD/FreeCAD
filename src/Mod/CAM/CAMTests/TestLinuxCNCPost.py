# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
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

import FreeCAD

import Path
import CAMTests.PathTestUtils as PathTestUtils
import CAMTests.PostTestMocks as PostTestMocks
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine, Toolhead, ToolheadType


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestLinuxCNCPost(PathTestUtils.PathTestBase):
    """Test LinuxCNC-specific features of the linuxcnc_post.py postprocessor.

    This test suite focuses on LinuxCNC-specific functionality such as path blending modes.
    Generic postprocessor functionality is tested in TestGenericPost.
    """

    @classmethod
    def setUpClass(cls):
        """setUpClass()...

        This method is called upon instantiation of this test class.  Add code
        and objects here that are needed for the duration of the test() methods
        in this class.  In other words, set up the 'global' test environment
        here; use the `setUp()` method to set up a 'local' test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """

        # Create mock job with default operation and tool controller
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )

        # Create postprocessor using the mock job
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "linuxcnc")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does not
        have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # No cleanup needed for mock objects
        pass

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        # reinitialize the postprocessor data structures between tests
        self.post.reinitialize()
        # Create a machine configuration for each test
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.name = "Test LinuxCNC Machine"
        # Add a default toolhead (required by export2)
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            min_rpm=0,
            max_rpm=24000,
            max_power_kw=1.0,
        )
        self.post._machine.toolheads = [toolhead]

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def test_blend_mode_exact_path(self):
        """Test EXACT_PATH blend mode outputs G61."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "EXACT_PATH"
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # G61 should be in the preamble
        self.assertIn("G61", gcode)
        # Should not have G64
        self.assertNotIn("G64", gcode)
        # Should not have G61.1
        self.assertNotIn("G61.1", gcode)

    def test_blend_mode_exact_stop(self):
        """Test EXACT_STOP blend mode outputs G61.1."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "EXACT_STOP"
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # G61.1 should be in the preamble
        self.assertIn("G61.1", gcode)
        # Should not have G64
        self.assertNotIn("G64", gcode)

    def test_blend_mode_blend_default(self):
        """Test BLEND mode with default tolerance (0) outputs G64."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.postprocessor_properties["blend_tolerance"] = 0.0
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # G64 should be in the preamble (without P parameter)
        lines = gcode.splitlines()
        has_g64 = any("G64" in line and "P" not in line for line in lines)
        self.assertTrue(has_g64, "Expected G64 without P parameter")

    def test_blend_mode_blend_with_tolerance(self):
        """Test BLEND mode with tolerance outputs G64 P<tolerance>."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.postprocessor_properties["blend_tolerance"] = 0.05
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # G64 P0.05 should be in the preamble
        self.assertIn("G64 P0.0500", gcode)

    def test_blend_mode_blend_with_custom_tolerance(self):
        """Test BLEND mode with custom tolerance value."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.postprocessor_properties["blend_tolerance"] = 0.02
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # G64 P0.02 should be in the preamble
        self.assertIn("G64 P0.0200", gcode)

    def test_blend_mode_in_preamble_position(self):
        """Test that blend mode command appears in correct position in preamble."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.postprocessor_properties["blend_tolerance"] = 0.1
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]
        lines = gcode.splitlines()

        # Find G64 P line
        g64_line_idx = None
        for i, line in enumerate(lines):
            if "G64 P" in line:
                g64_line_idx = i
                break

        self.assertIsNotNone(g64_line_idx, "G64 P command not found")
        # Should be early in output (within first few lines of preamble)
        self.assertLess(g64_line_idx, 5, "G64 command should be in preamble")

    def test_blend_tolerance_zero_equals_no_tolerance(self):
        """Test that blend tolerance of 0 outputs G64 without P parameter."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.postprocessor_properties["blend_tolerance"] = 0
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]

        # Should have G64 without P
        lines = gcode.splitlines()
        has_g64_without_p = any("G64" in line and "P" not in line for line in lines)
        self.assertTrue(has_g64_without_p, "Expected G64 without P parameter when tolerance is 0")

    def test_blend_interaction_with_preamble_argument(self):
        """Test blend mode appears after units command in preamble."""
        self.profile_op.Path = Path.Path([])
        # Set blend mode via machine configuration
        self.post._machine.postprocessor_properties["blend_mode"] = "BLEND"
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        gcode = self.post.export2()[0][1]
        lines = gcode.splitlines()
        # G64 should appear early in the output
        self.assertIn("G64", gcode)
        # Find G64 line
        g64_idx = None
        for i, line in enumerate(lines):
            if "G64" in line:
                g64_idx = i
                break
        self.assertIsNotNone(g64_idx)
        self.assertLess(g64_idx, 5, "G64 should be in preamble")

    def test_rigid_tapping_g84_basic(self):
        """
        Test G84 rigid tapping conversion to G33.1 sequence.

        Expected behavior:
            BEFORE: G84 Z-10 F1.5 (rigid=True)

            AFTER:  G33.1 K1.5000 Z-10.0000
                    M4
                    G33.1 K1.5000 Z0.0000
                    M3
        """
        # Setup - create G84 command with rigid annotation
        command = Path.Command("G84", {"Z": -10.0, "F": 1.5})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("G33.1", result)
        self.assertIn("K1.5000", result)
        self.assertIn("Z-10.0000", result)
        self.assertIn("M4", result)
        self.assertIn("M3", result)

    def test_rigid_tapping_g74_basic(self):
        """
        Test G74 rigid tapping conversion to G33.1 sequence.

        Expected behavior:
            BEFORE: G74 Z-10 F1.5 (rigid=True)

            AFTER:  G33.1 K1.5000 Z-10.0000
                    M3
                    G33.1 K1.5000 Z0.0000
                    M4
        """
        # Setup - create G74 command with rigid annotation
        command = Path.Command("G74", {"Z": -10.0, "F": 1.5})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("G33.1", result)
        self.assertIn("K1.5000", result)
        self.assertIn("Z-10.0000", result)
        self.assertIn("M3", result)
        self.assertIn("M4", result)

    def test_rigid_tapping_pitch_conversion(self):
        """
        Test pitch (F) parameter conversion to K parameter.

        Expected behavior:
            BEFORE: G84 Z-10 F1.25 (rigid=True)

            AFTER:  G33.1 K1.2500 Z-10.0000
        """
        # Setup
        command = Path.Command("G84", {"Z": -10.0, "F": 1.25})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("K1.2500", result)

    def test_rigid_tapping_with_retract_height(self):
        """
        Test rigid tapping with retract height (R parameter).

        Expected behavior:
            BEFORE: G84 Z-15 R5 F1.25 (rigid=True)

            AFTER:  G33.1 K1.2500 Z-15.0000
                    M4
                    G33.1 K1.2500 Z5.0000
                    M3
        """
        # Setup
        command = Path.Command("G84", {"Z": -15.0, "R": 5.0, "F": 1.25})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("Z-15.0000", result)  # Tap depth
        self.assertIn("Z5.0000", result)  # Retract height

    def test_rigid_tapping_with_coordinates(self):
        """
        Test rigid tapping preserves X and Y coordinates.

        Expected behavior:
            BEFORE: G84 X10 Y20 Z-10 F1.5 (rigid=True)

            AFTER:  G33.1 K1.5000 X10.0000 Y20.0000 Z-10.0000
        """
        # Setup
        command = Path.Command("G84", {"X": 10.0, "Y": 20.0, "Z": -10.0, "F": 1.5})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("X10.0000", result)
        self.assertIn("Y20.0000", result)

    def test_rigid_tapping_with_dwell(self):
        """
        Test rigid tapping with dwell (P parameter).

        Expected behavior:
            BEFORE: G84 Z-10 F1.5 P0.5 (rigid=True)

            AFTER:  G33.1 K1.5000 Z-10.0000
                    M5
                    G04 P0.50
                    M4
                    G33.1 K1.5000 Z0.0000
                    M3
        """
        # Setup
        command = Path.Command("G84", {"Z": -10.0, "F": 1.5, "P": 0.5})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify
        self.assertIn("M5", result)
        self.assertIn("G04 P0.50", result)

    def test_rigid_tapping_imperial_units(self):
        """
        Test rigid tapping unit conversion to imperial.

        Expected behavior:
            BEFORE: G84 Z-10 F1.5 (rigid=True) in imperial units

            AFTER:  G33.1 K0.0591 Z-0.3937
        """
        # Setup - set imperial units
        from Machine.models.machine import OutputUnits

        self.post._machine.output.units = OutputUnits.IMPERIAL

        command = Path.Command("G84", {"Z": -10.0, "F": 1.5})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify - converted values (mm to inches)
        self.assertIn("Z-0.3937", result)  # -10mm / 25.4
        self.assertIn("K0.0591", result)  # 1.5mm / 25.4

    def test_rigid_tapping_block_delete(self):
        """
        Test rigid tapping with block delete annotation.

        Expected behavior:
            BEFORE: G84 Z-10 F1.5 (rigid=True, blockdelete=True)

            AFTER:  /G33.1 K1.5000 Z-10.0000
                    /M4
                    /G33.1 K1.5000 Z0.0000
                    /M3
        """
        # Setup
        command = Path.Command("G84", {"Z": -10.0, "F": 1.5})
        command.Annotations = {"rigid": "True", "operation": "tapping", "blockdelete": True}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify - all commands should have '/' prefix
        lines = result.split("\n")
        for line in lines:
            if line.strip():
                self.assertTrue(line.startswith("/"), f"Line missing block delete: {line}")

    def test_rigid_tapping_missing_pitch_fallback(self):
        """
        Test rigid tapping falls back to parent when pitch missing.

        Expected behavior:
            BEFORE: G84 Z-10 (rigid=True, no F parameter)

            AFTER:  [standard G84 conversion, not G33.1]
        """
        # Setup - command without F (pitch) parameter
        command = Path.Command("G84", {"Z": -10.0})
        command.Annotations = {"rigid": "True", "operation": "tapping"}

        # Execute
        result = self.post._convert_drill_cycle(command)

        # Verify - should not contain G33.1 (fallback to parent)
        self.assertNotIn("G33.1", result)

    def test_rigid_tapping_suppresses_g80(self):
        """
        Test G80 is suppressed for rigid tapping operations.

        Expected behavior:
            BEFORE: G80 (operation=tapping, rigid=True)

            AFTER:  None (command suppressed)
        """
        # Setup
        command = Path.Command("G80", {})
        command.Annotations = {"operation": "tapping", "rigid": "True"}

        # Execute
        result = self.post._convert_modal_command(command)

        # Verify - should return None (suppressed)
        self.assertIsNone(result)

    def test_rigid_tapping_suppresses_g98(self):
        """
        Test G98 is suppressed for rigid tapping operations.

        Expected behavior:
            BEFORE: G98 (operation=tapping, rigid=True)

            AFTER:  None (command suppressed)
        """
        # Setup
        command = Path.Command("G98", {})
        command.Annotations = {"operation": "tapping", "rigid": "True"}

        # Execute
        result = self.post._convert_modal_command(command)

        # Verify - should return None (suppressed)
        self.assertIsNone(result)

    def test_rigid_tapping_suppresses_g99(self):
        """
        Test G99 is suppressed for rigid tapping operations.

        Expected behavior:
            BEFORE: G99 (operation=tapping, rigid=True)

            AFTER:  None (command suppressed)
        """
        # Setup
        command = Path.Command("G99", {})
        command.Annotations = {"operation": "tapping", "rigid": "True"}

        # Execute
        result = self.post._convert_modal_command(command)

        # Verify - should return None (suppressed)
        self.assertIsNone(result)

    def test_non_rigid_tapping_not_suppressed(self):
        """
        Test G80/G98/G99 are not suppressed for non-rigid tapping.

        Expected behavior:
            BEFORE: G80 (operation=tapping, rigid=False)

            AFTER:  G80 (command not suppressed)
        """
        # Setup
        command = Path.Command("G80", {})
        command.Annotations = {"operation": "tapping", "rigid": "False"}

        # Execute
        result = self.post._convert_modal_command(command)

        # Verify - should not be None (not suppressed)
        self.assertIsNotNone(result)
