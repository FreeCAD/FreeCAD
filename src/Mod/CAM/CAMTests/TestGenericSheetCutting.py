# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 sliptonic <shopinthewoods@gmail.com>               *
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


import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine
import Constants

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def CompValue(val):
    # 5 significant digits should be precise enough (for sheet cutting)
    return round(val, 5)


class TestGenericSheetCutting(PathTestUtils.PathTestBase):
    """Test the GenericSheetCutting postprocessor unique functionality."""

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

        # Create GenericSheetCutting postprocessor using the mock job
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "generic_sheet_cutting")

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

        # Create machine with postprocessor properties
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.name = "Test Generic Sheet Cutting"

    def _set_postprocessor_properties(self, **kwargs):
        """Have to do this before any method that checks .values"""
        for k, v in kwargs.items():
            self.post._machine.postprocessor_properties[k] = v
        self.post.apply_configuration_bundle()

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def test00_property_schema(self):
        """
        Test that GenericSheetCutting has the correct property schema with sheet cutting-specific properties.

        INPUT:
        - Function: get_property_schema()
        - Parameters: None
        - Input data: GenericSheetCutting postprocessor instance

        EXPECTED OUTPUT:
        - Returns schema with pierce_delay, cooling_delay, marking_delay, torch_zaxis_control, force_rapid_feeds
        - Properties should have correct types, defaults, and help text
        - This ensures the machine configuration editor can properly configure sheet cutting features
        """
        schema = self.post.get_property_schema()

        # Check that we have the expected number of properties
        # 7 machine-config properties + 1 runtime-only (mark_entry_only)
        self.assertEqual(len(schema), 8)

        # Check pierce_delay property
        pierce_delay = next(prop for prop in schema if prop["name"] == "pierce_delay")
        self.assertEqual(pierce_delay["type"], "integer")
        self.assertEqual(pierce_delay["default"], 1000)
        self.assertEqual(pierce_delay["min"], 0)
        self.assertEqual(pierce_delay["max"], 10000)

        # Check cooling_delay property
        cooling_delay = next(prop for prop in schema if prop["name"] == "cooling_delay")
        self.assertEqual(cooling_delay["type"], "integer")
        self.assertEqual(cooling_delay["default"], 500)
        self.assertEqual(cooling_delay["min"], 0)
        self.assertEqual(cooling_delay["max"], 10000)

        # Check marking_delay property
        marking_delay = next(prop for prop in schema if prop["name"] == "marking_delay")
        self.assertEqual(marking_delay["type"], "integer")
        self.assertEqual(marking_delay["default"], 100)
        self.assertEqual(marking_delay["min"], 0)
        self.assertEqual(marking_delay["max"], 10000)

        # Check cutter_control property
        cutter_control = next(prop for prop in schema if prop["name"] == "cutter_control")
        self.assertEqual(cutter_control["type"], "choice")
        self.assertEqual(cutter_control["default"], "Z_Control")

        # Check STRIP_Z property
        STRIP_Z = next(prop for prop in schema if prop["name"] == "STRIP_Z")
        self.assertEqual(STRIP_Z["type"], "bool")
        self.assertEqual(STRIP_Z["default"], False)

        # Check mark_entry_only property
        mark_entry_only = next(prop for prop in schema if prop["name"] == "mark_entry_only")
        self.assertEqual(mark_entry_only["type"], "bool")
        self.assertEqual(mark_entry_only["default"], False)

        # Check STRIP_F property
        STRIP_F = next(prop for prop in schema if prop["name"] == "STRIP_F")
        self.assertEqual(STRIP_F["type"], "bool")
        self.assertEqual(STRIP_F["default"], False)

    def test01_pierce_delay_injection(self):
        """
        Test that pierce delay is correctly injected after M3/M4 commands.

        INPUT:
        - Function: _inject_pierce_delay()
        - Parameters: postables with M3 command
        - Input data: Path containing M3 torch ignition command

        EXPECTED OUTPUT:
        - G4 dwell command inserted after M3 with correct P parameter
        - Delay duration matches pierce_delay property value in seconds
        - This ensures proper torch ignition delay for sheet cutting cutting
        """
        # Create a simple path with M3 command
        commands = [
            Path.Command("G0", {"Z": 5.0}),
            Path.Command("M3"),  # Torch ignition
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
        ]
        self.profile_op.Path = Path.Path(commands)

        # Set pierce delay to 2000ms (should become 2.0 seconds in G4)
        self._set_postprocessor_properties(pierce_delay=2000)

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_pierce_delay(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        # Should have G4 inserted after M3
        m3_idx = cmd_names.index("M3")
        self.assertEqual(cmd_names[m3_idx + 1], "G4", "G4 should follow M3")
        self.assertAlmostEqual(
            result_cmds[m3_idx + 1].Parameters["P"], 2.0, msg="G4 should have 2.0 second delay"
        )

    def test02_cooling_delay_injection(self):
        """
        Test that cooling delay is correctly injected after M5 commands.

        INPUT:
        - Function: _inject_cooling_delay()
        - Parameters: postables with M5 command
        - Input data: Path containing M5 torch extinguish command

        EXPECTED OUTPUT:
        - G4 dwell command inserted after M5 with correct P parameter
        - Delay duration matches cooling_delay property value in seconds
        - This ensures proper torch cooling delay before next movement
        """
        # Create a simple path with M5 command
        commands = [
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("M5"),  # Torch extinguish
            Path.Command("G0", {"Z": 10.0}),
        ]
        self.profile_op.Path = Path.Path(commands)

        # Set cooling delay to 500ms (should become 0.5 seconds in G4)
        self._set_postprocessor_properties(cooling_delay=500)

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_cooling_delay(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        # Should have G4 inserted after M5
        m5_idx = cmd_names.index("M5")
        self.assertEqual(cmd_names[m5_idx + 1], "G4", "G4 should follow M5")
        self.assertAlmostEqual(
            result_cmds[m5_idx + 1].Parameters["P"], 0.5, msg="G4 should have 0.5 second delay"
        )

    def test03_cutter_control_z_control(self):
        """
        Test cutter_control when set to Z_Control - M3/M5 inserted based on Z movement.

        INPUT:
        - Function: _inject_torch_control()
        - Parameters: postables with Z movements
        - Input data: Path with Z- movement to cut height, then Z+ retraction

        EXPECTED OUTPUT:
        - M3 inserted before Z- movement when torch_zaxis_control=True
        - M5 inserted after Z+ movement when torch is active
        - This demonstrates automatic torch control based on Z-axis movement
        """
        # Set up operation heights
        self.profile_op.StartDepth = 2.0  # Pierce height
        self.profile_op.FinalDepth = 0.0  # Cut height

        # Create path with Z movements
        commands = [
            Path.Command("G0", {"Z": 10.0}),  # Start at clearance
            Path.Command("G0", {"Z": 2.0}),  # Move to pierce height
            Path.Command("G1", {"Z": 0.0, "F": 500}),  # Move to cut height (should trigger M3)
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),  # Cut
            Path.Command("G0", {"Z": 10.0}),  # Retract (should trigger M5)
        ]
        self.profile_op.Path = Path.Path(commands)

        # Enable torch Z-axis control
        self._set_postprocessor_properties(cutter_control="Z_Control")

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_torch_control(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        # Should have M3 inserted for torch ignition
        self.assertIn("M3", cmd_names, "M3 should be inserted for torch ignition")
        # Should have M5 inserted for torch extinguish
        self.assertIn("M5", cmd_names, "M5 should be inserted for torch extinguish")

        # M3 should appear before the Z- cut move
        m3_idx = cmd_names.index("M3")
        # Find the G1 Z0.0 command (cut height move)
        cut_idx = None
        for i, cmd in enumerate(result_cmds):
            if cmd.Name == "G1" and "Z" in cmd.Parameters and cmd.Parameters["Z"] == 0.0:
                cut_idx = i
                break
        self.assertIsNotNone(cut_idx, "G1 Z0.0 cut move should be present")
        self.assertLess(m3_idx, cut_idx, "M3 should appear before Z- cut move")

    def test04_cutter_control_spindle_control(self):
        """
        Test when cutter_control is set to Spindle_Control.

        INPUT:
        - Function: _inject_torch_control()
        - Parameters: postables with Z movements
        - Input data: Path with Z movements, torch_zaxis_control=False

        EXPECTED OUTPUT:
        - No M3/M5 commands automatically inserted based on Z movement
        - Original path commands pass through unchanged
        - This allows manual torch control when automatic control is disabled
        """
        # Set up operation heights
        self.profile_op.StartDepth = 2.0
        self.profile_op.FinalDepth = 0.0

        # Create path with Z movements (no manual M3/M5)
        commands = [
            Path.Command("G0", {"Z": 10.0}),
            Path.Command("G0", {"Z": 2.0}),
            Path.Command("G1", {"Z": 0.0, "F": 500}),
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("G0", {"Z": 10.0}),
        ]
        self.profile_op.Path = Path.Path(commands)
        original_cmd_count = len(commands)

        # Disable torch Z-axis control
        self._set_postprocessor_properties(cutter_control="Spindle_Control")

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_torch_control(postables)

        # Verify the path is unchanged
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        self.assertEqual(
            len(result_cmds),
            original_cmd_count,
            "Path should be unchanged when torch control is disabled",
        )
        self.assertNotIn("M3", cmd_names, "No M3 should be injected when torch control is disabled")
        self.assertNotIn("M5", cmd_names, "No M5 should be injected when torch control is disabled")

    def test04_cutter_control_G0_control(self):
        """
        Test that G0_Control adds M3 commands when switching between G0 and G1 and M5 commands when switching between G1 and G0.

        INPUT:
        - Function: _inject_torch_control()
        - Parameters: postables with G0 and movements
        - Input data: Path with Z movements, torch_zaxis_control=False

        EXPECTED OUTPUT:
        - M3 inserted before Z- movement when torch_zaxis_control=True
        - M5 inserted after Z+ movement when torch is active
        - This demonstrates automatic torch control based on G0 and G1
        """

        # Create path with Z movements (no manual M3/M5)
        commands = [
            Path.Command("G0", {"Z": 10.0}),
            Path.Command("G0", {"Z": 2.0}),
            Path.Command("G1", {"Z": 0.0, "F": 500}),
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("G0", {"Z": 10.0}),
        ]
        self.profile_op.Path = Path.Path(commands)

        # Enable torch G0 control
        self._set_postprocessor_properties(cutter_control="G0_Control")

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_torch_control(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        # Should have M3 inserted for torch ignition
        self.assertIn("M3", cmd_names, "M3 should be inserted for torch ignition")
        # Should have M5 inserted for torch extinguish
        self.assertIn("M5", cmd_names, "M5 should be inserted for torch extinguish")

        # M3 should appear before the G1 cut move
        m3_idx = cmd_names.index("M3")
        # Find the G1 Z0.0 command (cut height move)
        cut_idx = None
        for i, cmd in enumerate(result_cmds):
            if cmd.Name == "G1":
                cut_idx = i
                break
        self.assertIsNotNone(cut_idx, "G1 Z0.0 cut move should be present")
        self.assertLess(m3_idx, cut_idx, "M3 should appear before Z- cut move")

    def test06_mark_entry_only_mode(self):
        """
        Test mark entry only mode - only the first entry point is marked.
        INPUT:
        - Function: _inject_mark_entry_only()
        - Parameters: postables with multiple Z- movements
        - Input data: Path with multiple cutting passes, mark_entry_only=True
        EXPECTED OUTPUT:
        - First Z- movement to cut height is retained
        - G4 marking delay is inserted after the first entry
        - Lateral cutting moves are removed
        - Subsequent Z- entry movements are removed
        - Z+ retractions are retained
        - Existing torch commands are removed
        """
        self.profile_op.StartDepth = 2.0
        self.profile_op.FinalDepth = 0.0
        self.profile_op.ClearanceHeight = 10.0

        commands = [
            Path.Command("G0", {"Z": 10.0}),  # Start at clearance
            Path.Command("G1", {"Z": 0.0, "F": 500}),  # First entry (should be marked)
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),  # Cut - remove
            Path.Command("G0", {"Z": 10.0}),  # Retract - keep
            Path.Command("G0", {"X": 20.0, "Y": 20.0}),  # Positioning
            Path.Command("G1", {"Z": 0.0, "F": 500}),  # Second entry - remove
            Path.Command("G1", {"X": 30.0, "Y": 30.0, "F": 1000}),  # Cut - remove
            Path.Command("G0", {"Z": 10.0}),  # Final retract - keep
        ]

        self.profile_op.Path = Path.Path(commands)

        self._set_postprocessor_properties(
            mark_entry_only=True,
            marking_delay=100,
        )

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._inject_mark_entry_only(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        # First entry must be retained.
        z_moves = [
            cmd
            for cmd in result_cmds
            if cmd.Name in Constants.GCODE_MOVE_LINE + Constants.GCODE_MOVE_ARC
            and "Z" in cmd.Parameters
        ]

        self.assertEqual(
            len(z_moves),
            3,
            "Expected initial positioning, first entry, and two retractions",
        )

        self.assertEqual(z_moves[0].Name, "G0")
        self.assertEqual(CompValue(z_moves[0].Parameters["Z"]), 10.0)

        self.assertEqual(z_moves[1].Name, "G1")
        self.assertEqual(CompValue(z_moves[1].Parameters["Z"]), 0.0)

        self.assertEqual(z_moves[2].Name, "G0")
        self.assertEqual(CompValue(z_moves[2].Parameters["Z"]), 10.0)

        # A marking delay must have been inserted after the first entry.
        g4_commands = [cmd for cmd in result_cmds if cmd.Name == "G4"]

        self.assertEqual(len(g4_commands), 1)
        self.assertEqual(
            CompValue(g4_commands[0].Parameters["P"]),
            0.1,
        )

        # No lateral cutting moves should remain.
        lateral_moves = [
            cmd
            for cmd in result_cmds
            if cmd.Name in Constants.GCODE_MOVE_LINE + Constants.GCODE_MOVE_ARC
            and "Z" not in cmd.Parameters
        ]

        self.assertEqual(
            lateral_moves,
            [],
            "Lateral cutting moves should be removed in mark-entry-only mode",
        )

        # Only one Z-down move should remain.
        z_down_moves = [
            cmd
            for cmd in result_cmds
            if (
                cmd.Name in Constants.GCODE_MOVE_LINE + Constants.GCODE_MOVE_ARC
                and "Z" in cmd.Parameters
                and CompValue(cmd.Parameters["Z"]) <= 0.0
            )
        ]

        self.assertEqual(
            len(z_down_moves),
            1,
            "Only the first Z-down entry should remain",
        )

    def test07_force_rapid_feeds(self):
        """
        Test force rapid feeds functionality - removes F parameters from movement commands.

        INPUT:
        - Function: _force_rapid_feeds()
        - Parameters: postables with movement commands containing F parameters
        - Input data: Path with G0/G1/G2/G3 commands having feed rates

        EXPECTED OUTPUT:
        - All F parameters removed from movement commands
        - Non-movement commands unchanged
        - This enables dry run mode for path verification without cutting
        """
        # Create path with various movement commands and feed rates
        commands = [
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 10.0, "F": 3000}),  # Rapid with feed
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "Z": 0.0, "F": 1000}),  # Linear move
            Path.Command("G2", {"X": 20.0, "Y": 10.0, "I": 5.0, "F": 800}),  # Arc move
            Path.Command("G3", {"X": 30.0, "Y": 20.0, "J": 5.0, "F": 600}),  # Arc move
            Path.Command("M3", {"S": 1000}),  # Non-movement command
        ]
        self.profile_op.Path = Path.Path(commands)

        # Enable force rapid feeds
        self._set_postprocessor_properties(force_rapid_feeds=True)

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._force_rapid_feeds(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands

        # Check that no movement commands have F parameters
        for cmd in result_cmds:
            if cmd.Name in ["G0", "G1", "G2", "G3"]:
                self.assertNotIn(
                    "F",
                    cmd.Parameters,
                    f"{cmd.Name} should not have F parameter after force rapid feeds",
                )

        # Check that non-movement commands are unchanged
        m3_cmd = next(cmd for cmd in result_cmds if cmd.Name == "M3")
        self.assertIn("S", m3_cmd.Parameters, "M3 should retain S parameter")
        self.assertAlmostEqual(
            m3_cmd.Parameters["S"], 1000.0, msg="M3 S parameter should be unchanged"
        )

    def test08_common_property_overrides(self):
        """
        Test that GenericSheetCutting correctly overrides common postprocessor properties.

        INPUT:
        - Function: get_common_property_schema()
        - Parameters: None
        - Input data: GenericSheetCutting postprocessor instance

        EXPECTED OUTPUT:
        - file_extension defaults to "nc"
        - supports_tool_radius_compensation defaults to True
        - preamble and postamble have sheet cutting-specific defaults
        - This ensures proper defaults for sheet cutting cutting controllers
        """
        common_props = self.post.get_common_property_schema()

        # Check file extension override
        file_ext = next(prop for prop in common_props if prop["name"] == "file_extension")
        self.assertEqual(file_ext["default"], "nc")

        # Check tool radius compensation override
        trc = next(
            prop for prop in common_props if prop["name"] == "supports_tool_radius_compensation"
        )
        self.assertEqual(trc["default"], True)

        # Check preamble override
        preamble = next(prop for prop in common_props if prop["name"] == "preamble")
        self.assertEqual(preamble["default"], "G17 G54 G40 G49 G80 G90")

        # Check postamble override
        postamble = next(prop for prop in common_props if prop["name"] == "postamble")
        self.assertEqual(postamble["default"], "M05\nG17 G54 G90 G80 G40\nM2")

    def test09_zero_delay_values(self):
        """
        Test that zero or negative delay values don't inject G4 commands.

        INPUT:
        - Function: _inject_pierce_delay() and _inject_cooling_delay()
        - Parameters: postables with M3/M5 commands
        - Input data: pierce_delay=0, cooling_delay=-100

        EXPECTED OUTPUT:
        - No G4 commands injected when delay values are <= 0
        - M3/M5 commands pass through unchanged
        - This prevents unnecessary dwell commands when delays are disabled
        """
        # Create path with M3 and M5 commands
        commands = [
            Path.Command("G0", {"Z": 5.0}),
            Path.Command("M3"),  # Torch ignition
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("M5"),  # Torch extinguish
            Path.Command("G0", {"Z": 10.0}),
        ]
        self.profile_op.Path = Path.Path(commands)
        original_cmd_count = len(commands)

        # Set zero/negative delays
        self._set_postprocessor_properties(
            force_rapid_feeds=0,
            cooling_delay=-100,
            pierce_delay=0,
        )

        # Build postables and call both injection methods directly
        postables = [("section", [self.profile_op])]
        self.post._inject_pierce_delay(postables)
        self.post._inject_cooling_delay(postables)

        # Verify the path is unchanged (no G4 commands added)
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        # Should have no G4 commands
        self.assertNotIn(
            "G4", cmd_names, "No G4 commands should be injected for zero/negative delays"
        )

        # Path should be unchanged
        self.assertEqual(
            len(result_cmds),
            original_cmd_count,
            "Path length should be unchanged when delays are zero/negative",
        )

        # Should still have M3 and M5
        self.assertIn("M3", cmd_names, "M3 should be present")
        self.assertIn("M5", cmd_names, "M5 should be present")

    def test10_strip_F_commands(self):
        """
        Test strip F parameters functionality - removes F parameters from movement commands.

        INPUT:
        - Function: _strip_f_parameterts()
        - Parameters: postables with movement commands containing F parameters
        - Input data: Path with G0/G1/G2/G3 commands having feed rates

        EXPECTED OUTPUT:
        - All F parameters removed from movement commands
        - Non-movement commands unchanged
        - This dissables feed rate commands for machines that do not support them
        """
        # Create path with various movement commands and feed rates
        commands = [
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 10.0, "F": 3000}),  # Rapid with feed
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "Z": 0.0, "F": 1000}),  # Linear move
            Path.Command("G2", {"X": 20.0, "Y": 10.0, "I": 5.0, "F": 800}),  # Arc move
            Path.Command("G3", {"X": 30.0, "Y": 20.0, "J": 5.0, "F": 600}),  # Arc move
            Path.Command("M3", {"S": 1000}),  # Non-movement command
        ]
        self.profile_op.Path = Path.Path(commands)

        # Enable strip F Parameters
        self._set_postprocessor_properties(STRIP_F=True)

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._force_rapid_feeds(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands

        # Check that no movement commands have F parameters
        for cmd in result_cmds:
            if cmd.Name in ["G0", "G1", "G2", "G3"]:
                self.assertNotIn(
                    "F",
                    cmd.Parameters,
                    f"{cmd.Name} should not have F parameter after strip F parameters",
                )

        # Check that non-movement commands are unchanged
        m3_cmd = next(cmd for cmd in result_cmds if cmd.Name == "M3")
        self.assertIn("S", m3_cmd.Parameters, "M3 should retain S parameter")
        self.assertAlmostEqual(
            m3_cmd.Parameters["S"], 1000.0, msg="M3 S parameter should be unchanged"
        )

    def test11_strip_Z_commands(self):
        """
        Test strip Z parameters functionality - removes Z parameters from movement commands.

        INPUT:
        - Function: _strip_z_parameterts()
        - Parameters: postables with movement commands containing Z parameters
        - Input data: Path with G0/G1/G2/G3 commands having Z parameters

        EXPECTED OUTPUT:
        - All Z parameters removed from movement commands
        - Non-movement commands unchanged
        - This dissables feed rate commands for machines that do not support them
        """
        # Create path with various movement commands and feed rates
        commands = [
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 10.0, "F": 3000}),  # Rapid with feed
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "Z": 0.0, "F": 1000}),  # Linear move
            Path.Command("G2", {"X": 20.0, "Y": 10.0, "I": 5.0, "F": 800}),  # Arc move
            Path.Command("G3", {"X": 30.0, "Y": 20.0, "J": 5.0, "F": 600}),  # Arc move
            Path.Command("M3", {"S": 1000}),  # Non-movement command
        ]
        self.profile_op.Path = Path.Path(commands)

        # Enable strip Z
        self._set_postprocessor_properties(STRIP_Z=True)

        # Build postables and call injection method directly
        postables = [("section", [self.profile_op])]
        self.post._strip_z_parameterts(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands

        # Check that no movement commands have Z parameters
        for cmd in result_cmds:
            if cmd.Name in ["G0", "G1", "G2", "G3"]:
                self.assertNotIn(
                    "Z",
                    cmd.Parameters,
                    f"{cmd.Name} should not have Z parameter after strip Z parameters",
                )

        # Check that non-movement commands are unchanged
        m3_cmd = next(cmd for cmd in result_cmds if cmd.Name == "M3")
        self.assertIn("S", m3_cmd.Parameters, "M3 should retain S parameter")
        self.assertAlmostEqual(
            m3_cmd.Parameters["S"], 1000.0, msg="M3 S parameter should be unchanged"
        )

    def test12_reset_cutter_state_active_torch_and_clearance_move(self):
        """
        Test that _reset_cutter_state() extinguishes an active torch and
        moves to the operation clearance height when required.

        INPUT:
        - Torch is active
        - Last Z position differs from ClearanceHeight

        EXPECTED OUTPUT:
        - M5 is emitted
        - G0 to ClearanceHeight is emitted
        - Torch state is reset
        - Last Z position is updated to ClearanceHeight
        """
        self.profile_op.ClearanceHeight = 10.0

        self.post._torch_active = True
        self.post._last_z = 2.0

        reset_commands = self.post._reset_cutter_state(self.profile_op)

        self.assertEqual(
            [cmd.Name for cmd in reset_commands],
            ["M5", "G0"],
        )

        self.assertEqual(
            reset_commands[1].Parameters["Z"],
            10.0,
        )

        self.assertFalse(
            self.post._torch_active,
            "Torch should be inactive after state reset",
        )

        self.assertEqual(
            CompValue(self.post._last_z),
            CompValue(10.0),
        )

    def test13_reset_cutter_state_already_at_clearance(self):
        """
        Test that _reset_cutter_state() does not emit an unnecessary Z move
        when already at the clearance height.

        INPUT:
        - Torch is active
        - Last Z position equals ClearanceHeight

        EXPECTED OUTPUT:
        - M5 is emitted
        - No G0 Z move is emitted
        - Torch state is reset
        """
        self.profile_op.ClearanceHeight = 10.0

        self.post._torch_active = True
        self.post._last_z = 10.0

        reset_commands = self.post._reset_cutter_state(self.profile_op)

        self.assertEqual(
            [cmd.Name for cmd in reset_commands],
            ["M5"],
        )

        self.assertFalse(self.post._torch_active)
        self.assertEqual(
            CompValue(self.post._last_z),
            CompValue(10.0),
        )

    def test14_reset_cutter_state_inactive_torch(self):
        """
        Test that _reset_cutter_state() does not emit M5 when the torch is
        already inactive.

        INPUT:
        - Torch is inactive
        - Last Z position differs from ClearanceHeight

        EXPECTED OUTPUT:
        - No M5 is emitted
        - G0 to ClearanceHeight is emitted
        - Torch remains inactive
        """
        self.profile_op.ClearanceHeight = 10.0

        self.post._torch_active = False
        self.post._last_z = 2.0

        reset_commands = self.post._reset_cutter_state(self.profile_op)

        self.assertEqual(
            [cmd.Name for cmd in reset_commands],
            ["G0"],
        )

        self.assertEqual(
            reset_commands[0].Parameters["Z"],
            10.0,
        )

        self.assertFalse(self.post._torch_active)
        self.assertEqual(
            CompValue(self.post._last_z),
            CompValue(10.0),
        )

    def test15_reset_cutter_state_no_previous_z(self):
        """
        Test that _reset_cutter_state() does not emit a clearance move when
        there is no previous Z position to compare against.

        INPUT:
        - Torch is inactive
        - Last Z position is None

        EXPECTED OUTPUT:
        - No reset commands are emitted
        - Last Z remains None
        """
        self.profile_op.ClearanceHeight = 10.0

        self.post._torch_active = False
        self.post._last_z = None

        reset_commands = self.post._reset_cutter_state(self.profile_op)

        self.assertEqual(reset_commands, [])
        self.assertFalse(self.post._torch_active)
        self.assertIsNone(self.post._last_z)

    def test16_reset_cutter_state_zero_clearance(self):
        """
        Test that _reset_cutter_state() does not emit a clearance move when
        ClearanceHeight is zero.

        INPUT:
        - Last Z position differs from clearance
        - ClearanceHeight is zero

        EXPECTED OUTPUT:
        - No G0 clearance move is emitted
        - Torch is still extinguished if active
        """
        self.profile_op.ClearanceHeight = 0.0

        self.post._torch_active = True
        self.post._last_z = 2.0

        reset_commands = self.post._reset_cutter_state(self.profile_op)

        self.assertEqual(
            [cmd.Name for cmd in reset_commands],
            ["M5"],
        )

        self.assertFalse(self.post._torch_active)
        self.assertEqual(
            CompValue(self.post._last_z),
            CompValue(2.0),
        )

    def test_actual_machine(self):
        """Our specific `postprocessor_properties` were seen"""
        self.post = PostProcessorFactory.get_post_processor(self.job, "generic_sheet_cutting")
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.name = "Test Generic Sheet Cutting"
        self.post.apply_configuration_bundle()

        self.assertIsNotNone(self.post.values["PIERCE_DELAY"])
