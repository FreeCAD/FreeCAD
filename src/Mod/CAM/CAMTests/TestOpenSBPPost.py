# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 sliptonic <shopinthewoods@gmail.com>

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
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Tests for the OpenSBP post-processor (opensbp_post.py).

OpenSBP is the native command dialect used by ShopBot CNC controllers.
It differs in some G-code: helix, prompt-messages, spindle-speed/tool change
Most G-code can be passed through (requires line-numbers).
"""

import re
import math
import unittest

import Path
import Constants
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory
from Path.Base.MachineState import MachineState
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits


# FIXME: use import when implemented
# from Path.Post.Processor import PPMachineState
def PPMachineState(init: dict):
    # FIXME: while still _modal_state
    return init


PPMachineState.Tracked = list("XYZABCUVWFS")

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

eol = "\n"  # for fstrings


class TestOpenSBPPost(PathTestUtils.PathTestBase):
    """Test OpenSBP-specific features of the opensbp_post.py postprocessor.

    OpenSBP uses native ShopBot commands instead of standard G-code.
    These tests verify command conversion for the major command categories.
    """

    _first_time = True

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = "opensbp"
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "opensbp")

    @classmethod
    def tearDownClass(cls):
        pass

    def setUp(self):
        self.maxDiff = None

        # Predictable output
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.output.duplicates.parameters = True
        self.post._machine.output.duplicates.commands = True
        self.post._machine.output.units = OutputUnits.METRIC

        self.post.apply_configuration_bundle()
        import json

        if self._first_time:  # FIXME: disable when dev is done
            self.__class__._first_time = False
            print(f"## _mach setup: { self.post._machine.__class__.__name__}")
            try:
                adump = json.dumps(self.post._machine.to_dict(), sort_keys=True, indent=2)
            except TypeError:
                adump = str(self.post._machine.to_dict())
            print("---_machine\n{adump}\n---")
            try:
                adump = json.dumps(
                    self.post._machine.postprocessor_properties, sort_keys=True, indent=2
                )
            except TypeError:
                adump = str(self.post._machine.postprocessor_properties)
            print(f"--.postprocessor_properties\n{adump}")

        self.post._machine.name = "Test ShopBot Machine"
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            min_rpm=0,
            max_rpm=18000,
            max_power_kw=3.0,
        )
        self.post._machine.toolheads = [toolhead]
        # Reset tracked speeds before each test
        self.post._current_move_speed_xy = None
        self.post._current_move_speed_z = None
        self.post._current_jog_speed_xy = None
        self.post._current_jog_speed_z = None

        if "machine_state" in dir(self.post):
            self.post.machine_state = None

    def tearDown(self):
        pass

    def any_line_startswith(self, gcode, lines):
        """Test if it has at least one"""

        # at least some lines w/gcode
        lines_with_gcode = [l for l in lines if l.startswith(gcode)]
        self.assertTrue(
            len(lines_with_gcode) >= 1,
            f"At least one line with {gcode} out of\n---\n{eol.join(lines)}\n---",
        )

    def no_unnumbered(self, gcode, lines):
        """Test if it has at least one, and all one of the `gcode` lines are numbered"""
        pattern = r"^(N\d+ +)?" + gcode + r"( |$)"  # N001 G01 ....

        # at least some lines w/gcode
        # numbered/un-numbered
        lines_with_gcode = [l for l in lines if re.match(pattern, l.strip())]
        self.assertTrue(
            len(lines_with_gcode) >= 1,
            f"At least one line with {gcode} out of\n---\n{eol.join(lines)}\n---",
        )

        # No unnumbered
        unnumbered = [
            l
            for l in lines_with_gcode
            if (m := re.match(pattern, l.strip())) and m.group(1) is None
        ]
        self.assertEqual(
            unnumbered,
            [],
            f"Unexpected bare G-code lines: {unnumbered} in\n---\n{eol.join(lines)}\n---",
        )

    # -------------------------------------------------------------------------
    # File extension and defaults
    # -------------------------------------------------------------------------

    def test_default_file_extension(self):
        """Default output file extension is 'sbp'."""
        schema = self.post.get_common_property_schema()
        ext = next((p["default"] for p in schema if p["name"] == "file_extension"), None)
        self.assertEqual(ext, "sbp")

    # -------------------------------------------------------------------------
    # Rapid move (G0) → Jog commands
    # -------------------------------------------------------------------------

    def test_rapid_x(self):
        """
        G0 X-only move
        """
        command = Path.Command("G0", {"X": 10.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X10.0", result)

    def test_rapid_y(self):
        """G0 Y-only"""
        command = Path.Command("G0", {"Y": 20.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 Y20.0", result)

    def test_rapid_z(self):
        """G0 Z-only"""
        command = Path.Command("G0", {"Z": 5.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 Z5.0", result)

    def test_rapid_xy(self):
        """
        G0 XY move
        """
        command = Path.Command("G0", {"X": 10.0, "Y": 20.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X10.000 Y20.000", result)

    def test_rapid_xyz(self):
        """
        G0 XYZ move
        """
        command = Path.Command("G0", {"X": 10.0, "Y": 20.0, "Z": 5.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X10.000 Y20.000 Z5.000", result)

    def test_rapid_xz(self):
        """
        G0 XZ move
        """
        command = Path.Command("G0", {"X": 10.0, "Z": 5.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X10.000 Z5.000", result)

    def test_rapid_yz(self):
        """
        G0 YZ move
        """
        command = Path.Command("G0", {"Y": 20.0, "Z": 5.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 Y20.000 Z5.000", result)

    # -------------------------------------------------------------------------
    # Linear move (G1) → Move commands
    # -------------------------------------------------------------------------

    def test_linear_x(self):
        """
        G1 X-only
        """
        command = Path.Command("G1", {"X": 13.1})
        result = self.post._convert_linear_move(command)
        self.assertIn(f"G1 X13.100", result)

    def test_linear_y(self):
        """G1 Y-only"""
        command = Path.Command("G1", {"Y": 20.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 Y20.000", result)

    def test_linear_z_only_produces_mz(self):
        """G1 Z-only"""
        command = Path.Command("G1", {"Z": -5.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 Z-5.000", result)

    def test_linear_xy(self):
        """
        G1 XY move
        """
        command = Path.Command("G1", {"X": 10.0, "Y": 20.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X10.000 Y20.000", result)

    def test_linear_xyz(self):
        """
        G1 XYZ move
        """
        command = Path.Command("G1", {"X": 10.0, "Y": 20.0, "Z": -5.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X10.000 Y20.000 Z-5.000", result)

    def test_linear_xz(self):
        """
        G1 XZ move
        """
        command = Path.Command("G1", {"X": 10.0, "Z": -5.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X10.000 Z-5.000", result)

    # -------------------------------------------------------------------------
    # Speed commands (MS / JS)
    # -------------------------------------------------------------------------

    def test_linear_with_feedrate(self):
        """
        G1 with F parameter

        F in Path.Command is in FreeCAD base units: mm/sec.
        ShopBot g-code expects {unit}/min — time-unit conversion needed.
        """
        command = Path.Command("G1", {"X": 10.0, "F": 500.0})
        result = self.post._convert_linear_move(command)
        self.assertEqual("G1 X10.000 F30000.000", result)

    def test_rapid_with_feedrate_outputs(self):
        """G0 with F parameter outputs"""
        command = Path.Command("G0", {"X": 10.0, "F": 500.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X10.000 F30000.000", result)

    def test_speed_value_imperial_conversion(self):
        """
        F value is divided by 25.4 for imperial output.

        F=25.4*2 mm/sec → 120.0 in/min for ShopBot imperial output.
        """
        self.post._machine.output.units = OutputUnits.IMPERIAL
        self.post.apply_configuration_bundle()

        command = Path.Command("G1", {"X": 25.4, "F": 25.4 * 2})
        result = self.post._convert_linear_move(command)
        self.assertIn(f"F{(25.4 * 2)/25.4 * 60:.3f}", result)

    def test_js_speed_value_metric_passthrough(self):
        """
        Jog speed to mm/min
        """
        command = Path.Command("G0", {"X": 10.0, "F": 300.0})
        result = self.post._convert_rapid_move(command)
        self.assertIn("F18000.000", result)

    def test_helical_arc_includes_plunge(self):
        """
        Helical arc (G2/G3 with Z) adds a plunge parameter as the 9th field.

        BEFORE: G2 X10 Y0 I5 J0 Z-5 (current Z=0)
        AFTER:  CG,,10.0000,0.0000,5.0000,0.0000,L,1,5.0000
        """

        # arc's need a previous Z, so psuedo track Z
        # FIXME: replace _modal_state when .machine_state implemented
        self.post._modal_state = PPMachineState({k: None for k in MachineState.Tracked})
        self.post._modal_state.update({"X": 0, "Y": 0, "Z": 0})
        # self.post.machine_state.addCommand( Path.Command("G0", {"X":0, "Y":0, "Z": 0}) )

        command = Path.Command("G2", {"F": 50, "X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -5.0})

        result = self.post._convert_arc_move(command)

        lines = result.strip().splitlines()
        cg_line = next(l for l in lines if l.startswith("CG"))
        self.assertEqual("CG,,10.000,0.000,5.000,0.000,T,1,5.000", cg_line)

    def test_arc_no_gcode_in_output(self):
        """Helix output must not contain G2 or G3."""

        # arc's need a previous Z, so psuedo track Z
        self.post._modal_state = PPMachineState({k: None for k in PPMachineState.Tracked})
        self.post._modal_state.update({"X": 0, "Y": 0, "Z": 0})
        # self.post.machine_state.addCommand( Path.Command("G0", {"X":0, "Y":0, "Z": 0}) )

        command = Path.Command("G2", {"F": 50, "X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -1.0})

        result = self.post._convert_arc_move(command)
        self.assertNotIn("G2", result)
        self.assertNotIn("G3", result)

    def test_helix_required_params(self):
        """Helix requires all params, either explicit or modal"""

        # Remember that Helix's need a previous state Z to plunge from, and XY for speed calc

        # Explicit is ok
        state = {k: None for k in PPMachineState.Tracked}
        state.update({"X": 1, "Y": 2, "Z": 0})
        self.post.machine_state = PPMachineState(state)
        command = Path.Command("G2 X10 Y9 I5 J4 Z-1 F50")
        # no exception
        self.post._convert_arc_move(command)

        # Modal XYF is ok
        state = {k: None for k in PPMachineState.Tracked}
        state.update({"Z": 0, "X": 10, "Y": 9, "F": 50})
        self.post._modal_state = PPMachineState(state)
        command = Path.Command("G2 I5 J4 Z-1")
        # no exception
        self.post._convert_arc_move(command)

        # No previous XYZ: exception
        for required in "XYZ":
            state = {k: None for k in PPMachineState.Tracked}
            state.update({k: i / 10 for i, k in enumerate("XYZ") if k != required})
            self.post._modal_state = PPMachineState(state)
            command = Path.Command("G2 X10 Y9 I5 J4 Z-1 F50")
            self.assertRaisesRegex(
                ValueError,
                r"Arcs require a previous " + required,
                self.post._convert_arc_move,
                command,
            )

        # Each required XYIJF (explicit)
        for missing in "XYIJF":
            state = {k: None for k in PPMachineState.Tracked}
            state.update({"Z": 0})
            self.post._modal_state = PPMachineState(state)
            command = Path.Command("G2 X10 Y9 I5 J4 Z-1 F50")
            # rebuild w/o missing
            command = Path.Command(
                command.Name, {p: v for p, v in command.Parameters.items() if p != missing}
            )
            self.assertRaisesRegex(
                ValueError,
                r"missing \['" + missing + r"'\]",
                self.post._convert_arc_move,
                command,
                # "failed to detect required Z in state"
            )

    def test_helix_bad_z(self):
        """Helix in negative direction is ok"""

        # Explicit is ok
        state = {k: None for k in PPMachineState.Tracked}
        state.update({"X": 0, "Y": 0, "Z": 0})
        self.post._modal_state = PPMachineState(state)
        command = Path.Command("G2 X10 Y9 I5 J4 Z1 F50")
        self.post._convert_arc_move(command)
        self.assertTrue(True, "No Crash")

    def test_helix_units(self):
        """Helix has to use units"""

        # arc's need a previous Z, so psuedo track Z
        state = {k: None for k in PPMachineState.Tracked}

        # convenient numbers: 1,10
        # a quarter arc, of arc-length 1
        # with a z of -1
        f = 10  # mm/sec
        # We want a circumference of 4, to get a 1/4 circumference == 1
        # 4 = 2piR, 1/R = pi/2, R = 2/pi
        radius = 2 / math.pi
        center = {"X": 1, "Y": 2}
        start = Path.Command(f"G0 X{center['X']-radius} Y{center['Y']-0} Z10 F999")
        helix = Path.Command(f"G2 X{center['X']} Y{center['Y']+radius} I{radius} J0 Z9 F{f}")
        # now we specified an xy travel distance of 1 above
        # and a z travel distance of 1
        # and a F of 10 mm/sec, so we should see that for XY and Z speed

        def expected_distance(mm=True):
            change = 1 if mm else 25.4
            plunge = (
                start.Parameters["Z"] - helix.Parameters["Z"]
            )  # plunge down is positive for opensbp
            return f"CG,,{center['X']/change:.3f},{(center['Y']+radius)/change:.3f},{radius/change:.3f},0.000,T,1,{plunge/change:.3f}"

        # mm

        self.post._modal_state = PPMachineState(state)
        self.post._machine.output.units = OutputUnits.METRIC
        self.post._modal_state.update(start.Parameters)
        self.post.apply_configuration_bundle()

        # self.post.machine_state.addCommand( start )
        result = self.post._convert_arc_move(helix)
        lines = result.split("\n")
        speed_line = next((l for l in lines if l.startswith("VS,")), None)
        self.assertIsNotNone(speed_line, "speed-line (VS) not found")
        helix_line = next((l for l in lines if l.startswith("CG,")), None)
        self.assertIsNotNone(speed_line, "helix-line (CG) not found")

        self.assertEqual(f"VS,{f:.3f},{f:.3f}", speed_line, "in mm/sec")
        self.assertEqual(expected_distance(mm=True), helix_line, "still in mm")

        # inch

        self.post._modal_state = PPMachineState(state)
        self.post._machine.output.units = OutputUnits.IMPERIAL
        self.post.apply_configuration_bundle()

        self.post._modal_state.update(start.Parameters)
        # self.post.machine_state.addCommand( start )
        result = self.post._convert_arc_move(helix)
        lines = result.split("\n")
        speed_line = next((l for l in lines if l.startswith("VS,")), None)
        self.assertIsNotNone(speed_line, "speed-line (VS) not found")
        helix_line = next((l for l in lines if l.startswith("CG,")), None)
        self.assertIsNotNone(speed_line, "helix-line (CG) not found")

        self.assertEqual(f"VS,{f/25.4:.3f},{f/25.4:.3f}", speed_line, "in in/sec")
        self.assertEqual(expected_distance(mm=False), helix_line, "changed to inch")

        # 3/4 of circle
        # which just swaps start and end
        start = Path.Command(f"G0 X{center['X']} Y{center['Y']+radius} Z10 F999")
        helix = Path.Command(f"G2 X{center['X']-radius} Y{center['Y']-0} I0 J{-radius} Z9 F{f}")

        self.post._modal_state = PPMachineState(state)
        self.post._machine.output.units = OutputUnits.METRIC
        self.post._modal_state.update(start.Parameters)
        # self.post.machine_state.addCommand( start )
        self.post.apply_configuration_bundle()

        result = self.post._convert_arc_move(helix)
        lines = result.split("\n")
        speed_line = next((l for l in lines if l.startswith("VS,")), None)
        self.assertIsNotNone(speed_line, "speed-line (VS) not found")
        helix_line = next((l for l in lines if l.startswith("CG,")), None)
        self.assertIsNotNone(speed_line, "helix-line (CG) not found")

        self.assertEqual(f"VS,{f:.3f},{f:.3f}", speed_line, "in in/sec")
        plunge = (
            start.Parameters["Z"] - helix.Parameters["Z"]
        )  # plunge down is positive for opensbp
        cg = (
            f"CG,,{center['X']-radius:.3f},{(center['Y']):.3f},0.000,{-radius:.3f},T,1,{plunge:.3f}"
        )
        # self.assertEqual("CG,,0.363,2.000,0.000,-0.637,T,1,1.000", helix_line, "changed to inch")
        self.assertEqual(cg, helix_line, "changed to inch")

        # CCW
        # which just swaps start and end (and change direction)
        start = Path.Command(f"G0 X{center['X']} Y{center['Y']+radius} Z10 F999")
        helix = Path.Command(f"G3 X{center['X']-radius} Y{center['Y']-0} I0 J{-radius} Z9 F{f}")

        self.post._modal_state = PPMachineState(state)
        self.post._machine.output.units = OutputUnits.METRIC
        self.post._modal_state.update(start.Parameters)
        # self.post.machine_state.addCommand( start )
        self.post.apply_configuration_bundle()

        result = self.post._convert_arc_move(helix)
        lines = result.split("\n")
        speed_line = next((l for l in lines if l.startswith("VS,")), None)
        self.assertIsNotNone(speed_line, "speed-line (VS) not found")
        helix_line = next((l for l in lines if l.startswith("CG,")), None)
        self.assertIsNotNone(speed_line, "helix-line (CG) not found")

        self.assertEqual(f"VS,{f:.3f},{f:.3f}", speed_line, "in in/sec")
        plunge = (
            start.Parameters["Z"] - helix.Parameters["Z"]
        )  # plunge down is positive for opensbp
        cg = f"CG,,{center['X']-radius:.3f},{(center['Y']):.3f},0.000,{-radius:.3f},T,-1,{plunge:.3f}"
        # self.assertEqual("CG,,0.363,2.000,0.000,-0.637,T,1,1.000", helix_line, "changed to inch")
        self.assertEqual(cg, helix_line, "changed to inch")

    # -------------------------------------------------------------------------
    # Tool change (M6)
    # -------------------------------------------------------------------------

    def test_tool_change_manual_includes_pause(self):
        """
        Manual tool change (no ATC) emits PAUSE for operator intervention.

        BEFORE: M6 T2
        AFTER:  'Manual tool change to T2
                >&ToolName=2
                >&Tool=2
                >PAUSE
        """
        self.post._machine.postprocessor_properties["automatic_tool_changer"] = False
        self.post.apply_configuration_bundle()
        command = Path.Command("M6", {"T": 2})
        result = self.post._convert_tool_change(command)
        self.assertIn("PAUSE", result)
        self.assertIn("&Tool=2", result)

    def test_tool_change_automatic_no_pause(self):
        """
        Automatic tool change (ATC enabled) does not emit PAUSE.

        BEFORE: M6 T3
        AFTER:  >&ToolName=3
                >&Tool=3
        """
        self.post._machine.postprocessor_properties["automatic_tool_changer"] = True
        self.post.apply_configuration_bundle()

        command = Path.Command("M6", {"T": 3})
        result = self.post._convert_tool_change(command)
        self.assertIn("&Tool=3", result)
        self.assertNotIn("PAUSE", result)

    def test_tool_change_sets_tool_name(self):
        """Tool change always sets >&ToolName variable."""
        command = Path.Command("M6", {"T": 5})
        result = self.post._convert_tool_change(command)
        self.assertIn("&ToolName=5", result)

    # -------------------------------------------------------------------------
    # Spindle commands (M3/M4/M5)
    # -------------------------------------------------------------------------

    def test_spindle_on_manual_emits_pause(self):
        """
        M3 without automatic spindle control emits manual prompt and PAUSE.

        BEFORE: M3 S18000
        AFTER:  'Set spindle to 18000 RPM and start manually
                >PAUSE
        """
        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertIn("PAUSE", result)
        self.assertIn("18000", result)

    def test_spindle_on_automatic_emits_tr(self):
        """
        M3 with automatic spindle control emits TR (speed), C6 (on), and PAUSE (wait).

        BEFORE: M3 S18000
        AFTER:  >TR,18000
                >PAUSE 2
        """
        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post.apply_configuration_bundle()

        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertIn("TR,18000", result)
        self.assertTrue(re.search(r"PAUSE \d+", result), f"expected pause in {result}")

    def test_spindle_off_manual_emits_pause(self):
        """
        M5 without automatic spindle emits manual prompt and PAUSE.

        BEFORE: M5
        AFTER:  'Turn spindle OFF manually
                >PAUSE
        """
        self.post._machine.postprocessor_properties["automatic_spindle"] = False
        self.post.apply_configuration_bundle()

        command = Path.Command("M5", {})
        result = self.post._convert_spindle_command(command)
        self.assertTrue(result.startswith("'"), f"expected a comment for the prompt in\n{result}")
        self.assertIn("PAUSE", result)

    def test_spindle_off_automatic_emits_tr0(self):
        """
        M5 with automatic spindle control emits TR,0.

        BEFORE: M5
        AFTER:  TR,0
        """
        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post.apply_configuration_bundle()

        command = Path.Command("M5", {})
        result = self.post._convert_spindle_command(command)
        self.assertIn("TR,0", result)

    def test_spindle_no_gcode_in_output(self):
        """Spindle output must not contain M3, M4, or M5."""
        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post.apply_configuration_bundle()

        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertNotIn("M3", result)
        self.assertNotIn("M4", result)
        self.assertNotIn("M5", result)

    # -------------------------------------------------------------------------
    # Suppressed commands
    # -------------------------------------------------------------------------

    def test_fixture_commands_suppressed(self):
        """
        G54–G59 fixture offsets are suppressed (return None).

        OpenSBP has no work coordinate system concept.
        Tolerate G54, others are illegal
        """
        for fixture in ["G54"]:
            command = Path.Command(fixture, {})
            self.profile_op.Path = Path.Path([command])
            result = self.post.export2()[0][1]
            # Can appear in comments
            self.assertFalse(
                re.search(r"^N\d+ +" + fixture, result, flags=re.M),
                f"{fixture} should be suppressed",
            )

    # -------------------------------------------------------------------------
    # Unit conversion (imperial output)
    # -------------------------------------------------------------------------

    def test_linear_move_imperial_conversion(self):
        """
        With imperial output units, coordinate values are divided by 25.4.

        BEFORE: G1 X25.4 Y50.8 (metric input)
        """
        self.post._machine.output.units = OutputUnits.IMPERIAL
        self.post.apply_configuration_bundle()

        command = Path.Command("G1", {"X": 25.4, "Y": 50.8})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X1.000 Y2.000", result)

    def test_rapid_move_imperial_conversion(self):
        """With imperial output, rapid move coordinates are divided by 25.4."""
        self.post._machine.output.units = OutputUnits.IMPERIAL
        self.post.apply_configuration_bundle()

        command = Path.Command("G0", {"X": 25.4, "Z": 25.4 * 2})
        result = self.post._convert_rapid_move(command)
        self.assertIn("G0 X1.000 Z2.000", result)

    # -------------------------------------------------------------------------
    # Full export sanity check
    # -------------------------------------------------------------------------

    def test_full_export_no_crash(self):
        """
        export2() doesn't crash
        Turn on all options for all code-paths
        """
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.output_header = True
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.processing.filter_inefficient_moves = True
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post._machine.postprocessor_properties["automatic_tool_changer"] = True
        self.post.apply_configuration_bundle()

        # basic stuff, + something that is shopbot specific
        # FIXME: all Test*Post should do this
        # FIXME: a test-util function for gcode-name to arbitrary-good-gcode, to generate this list from Constants, etc
        handled_gcode = [
            Path.Command(g)
            for g in
            # trying to list all handled gcodes, is checked below against opensbp list
            (
                "G0X1Y2Z3F110 G1X4Y5Z6F50 "  # with F
                "G2X7Y8I9J10 G3X11Y12I13J14 G2X7Y8I9J10Z11 G3X11Y12I13J14Z12 G4P2 "
                "G20 G21 G38.2X1Y2Z3 G54 G92X4Y5Z6 "
                # The drill params don't necessarily make sense in these, we just need certain params:
                "G98 G99 "
                "G73X1Y2Z7F100R91Q1 G80 G81X1Y2Z9F100R10 G82X1Y2Z10F100R11P12 G83X1Y2Z11F100R12Q2 "
                # G85Z1R2 is simple, soon FIXME
                "M0 M1 M3S1 M5 M6T2 M8 M9 "
                "(comment)"
            ).split(" ")
        ]

        self.profile_op.Path = Path.Path(handled_gcode)
        self.post.export2()
        self.assertTrue(True, "No Crash")

        # Did we cover all the opensbp_post supported gcodes?
        # remove the redundant x0n from known, we only test xn above
        all_supported = self.post.GCodeSupported - self.post.GCodeUnsupported
        untried = set([p for p in all_supported if not re.search(r"0\d$", p)]) - set(
            [p.Name for p in handled_gcode]
        )
        self.assertEqual(
            set(), untried, f"Untried but opensbp_post supported, add to list: {sorted(untried)}"
        )

        # Did we cover all the allowed gcodes?
        all_possible = (
            set(
                Constants.GCODE_SUPPORTED
                + Constants.MCODE_SUPPORTED
                + Constants.GCODE_NON_CONFORMING
            )
            - self.post.GCodeUnsupported
            - {"G90", "G91"}  # not a Path nor Post thing
            - {"G84", "G85", "G88", "G89"}  # tapping, boring
            - {"M7"}
        )
        untried = set([p for p in all_possible if not re.search(r"0\d$", p)]) - set(
            [p.Name for p in handled_gcode]
        )
        self.assertEqual(
            set(),
            untried,
            f"Untried but CAM/PostProcessing allowed, add to list: {sorted(untried)}",
        )

    def test_comments(self):
        comment = Path.Command("(The Comment)")

        self.post._machine.output.comments.enabled = True
        self.post.apply_configuration_bundle()

        gcode = self.post._convert_comment(comment)
        self.assertEqual(gcode, "' The Comment")

        self.profile_op.Path = Path.Path([comment])
        gcode = self.post.export2()[0][1]
        self.assertIn("' The Comment", gcode, f"in\n---\n{gcode}\n---")

        wrong_comments = [l for l in gcode.split("\n") if l.startswith("(")]
        self.assertEqual(wrong_comments, [], f"\nShould be no '(' lines in\n---\n{gcode}\n---")

    def test_line_numbering(self):
        """
        Native shopbot must not get line-numbers
        """
        self.profile_op.Path = Path.Path(
            [
                # gcode, line-numbered
                Path.Command("G0", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                # Comment, no line-numbe
                Path.Command("(comment-line)"),
                # shopbot, no line-number
                Path.Command("G2", {"F": 50, "X": 20.0, "Y": 1.0, "I": 5.0, "J": 0.0, "Z": -5.0}),
            ]
        )
        gcode = self.post.export2()[0][1]

        lines = gcode.split("\n")

        # comments (native) are not numbered
        self.assertFalse(
            any(l for l in lines if "comment-line" in l and re.match(r"^N\d+", l)),
            f"Native comments are not line-numbered: {gcode}",
        )

        # shopbot native must NOT get numbered
        self.assertFalse(
            any(l for l in lines if re.match(r"^N\d+ +CG,", l)),
            f"G2->CG (helix) and is not line-numbered: {gcode}",
        )

    def test_line_numbering2(self):
        """Certain pass-through MUST be line-numbered"""

        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post.apply_configuration_bundle()

        also_shop_bot = "M3 M5".split(" ")
        # all spindle so just
        self.profile_op.Path = Path.Path([Path.Command(c, {"S": 1}) for c in also_shop_bot])
        gcode = self.post.export2()[0][1]

        lines = gcode.split("\n")

        # gcode in also_shop_bot MUST get numbered
        # (actually, for M3 and M5, they are translated to TR)
        self.assertFalse(
            any(l for l in lines if re.match(r"^(M3|M5)", l)),
            f"M3|M5 must be line-numbered: {gcode}",
        )

    @unittest.expectedFailure
    def test_gcode_passthrough(self):
        # list all passthrough codes
        # check that they made it, and are line-numbered
        self.assertTrue(False)

    @unittest.expectedFailure
    def test_converted_to_native(self):
        # list all NON passthrough codes
        # check that they made it, and are not line-numbered
        self.assertTrue(False)

    def test_full_export_duplicates(self):
        """
        G01 is subject to duplicate removal, but CG is not (relative Z)
        And, should remove duplicates even if different line-numbers
        """

        self.post._machine.output.comments.enabled = True
        self.post._machine.output.output_header = True
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.processing.filter_inefficient_moves = True
        self.post._machine.output.formatting.line_numbers = True
        self.post.apply_configuration_bundle()

        # basic stuff, + something that is shopbot specific
        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                # identical G01's -> elide one
                Path.Command("G1", {"F": 55, "X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                # we know shopbot must translate a helix, using a relative Z
                # so these are identical output, but not "duplicates"
                Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -5.0}),
                Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -10.0}),
            ]
        )
        results = self.post.export2()

        lines = "".join((g for _, g in results))
        lines = lines.split("\n")

        g1s = [g for g in lines if "G1 " in g]
        self.assertEqual(1, len(g1s), f"Expected 1x G1's in\n{eol.join(lines)}\n---")

        arcs = [g for g in lines if "CG," in g]
        as_text = "\n".join(g for _, g in results)
        self.assertEqual(
            2,
            len(arcs),
            f"expected 2 (identical) CG's, because the Z is relative, saw:\n{as_text}\n---",
        )  # doesn't remove duplicate arc

    def test_G0_and_G1(self):
        """
        G0 and G1 are passed through
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                Path.Command("G0", {"Z": 5.0}),
            ]
        )
        gcode = self.post.export2()[0][1]

        self.any_line_startswith("G1", gcode.splitlines())
        self.any_line_startswith("G0", gcode.splitlines())

    def test_G4(self):
        """
        G4 passed through
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path([Path.Command("G4 P3")])
        gcode = self.post.export2()[0][1]

        self.any_line_startswith("G4", gcode.splitlines())
        self.assertIn(" P3.000", gcode)

    def test_G2_G3_noz(self):
        """
        G2 and G3 are passed through if no delta Z
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0 X0 Y0 Z10"),  # establish Z
                Path.Command("G2 X7 Y8 I9 J10 F50 Z10"),
                Path.Command("G3 X11 Y12 I13 J14 F50 Z10"),
            ]
        )
        gcode = self.post.export2()[0][1]

        self.any_line_startswith("G2", gcode.splitlines())
        self.any_line_startswith("G3", gcode.splitlines())

    def test_G20_G21(self):
        """
        G20/G21 are passed through
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path([Path.Command("G20"), Path.Command("G21")])
        gcode = self.post.export2()[0][1]

        self.assertIn("G20", gcode.splitlines())
        self.assertIn("G21", gcode.splitlines())

    def test_M0_and_M1(self):
        """
        stop lines are passed through
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command("M0"),
                Path.Command("M1"),
            ]
        )
        gcode = self.post.export2()[0][1]

        self.assertIn("M1", gcode.splitlines())
        self.assertIn("M0", gcode.splitlines())

    def test_M8_and_M9(self):
        """
        coolant control is passed through, but line-numbered
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command("M8"),
                Path.Command("M9"),
            ]
        )
        gcode = self.post.export2()[0][1]

        self.assertIn("M8", gcode.splitlines())
        self.assertIn("M9", gcode.splitlines())

    @unittest.expectedFailure  # FIXME: when duplicates.commands is lifted to Postable world
    def test_program_modal_numbered(self):
        """
        duplicate commands that leave Z as the head need to be numbered
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.output.duplicates.commands = False
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command("G1 X1 Y2 Z9"),  # "Z3"
                Path.Command("G1 X1 Y2 Z3"),  # "Z3" is a shopbot command
            ]
        )
        gcode = self.post.export2()[0][1]

        self.no_unnumbered("Z3.000", gcode.splitlines())

    def test_G82(self):
        """for the dwell"""
        self.profile_op.Path = Path.Path(
            [
                Path.Command(g)
                for g in ["G98", "G0 X0.0 Y0.0 Z10.0 F110", "G82 X90.0 Y90.0 F59 R9.9 Z0 L2 P9"]
            ]
        )
        gcode = self.post.export2()[0][1]

        self.assertIn("G4 ", gcode, gcode)

    def test_drill_cycles_translated(self):
        """by default, expanded"""

        drill_codes = Constants.GCODE_DRILL_EXTENDED + Constants.GCODE_MOVE_DRILL

        # self.post._machine.output.comments.enabled = False
        # self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path(
            [
                Path.Command(g)
                for g in [
                    "G98",
                    "G0 X0.0 Y0.0 Z10.0 F110",
                    # tweak the comment gcode so it can't match
                    "(g83)",
                    "G83 X10.0 Y10.0 Z0 F100 R9.0 Q4",
                    # move +xy, move z->R, drill Z, z->R,
                    "(g81)",
                    "G81 X10.0 Y10.0 F100 R9.0 Z0 L2",
                    "G0 X1.0 Y2.0 Z10.0",
                    "(g82)",
                    "G82 X10.0 Y10.0 F100 R9.0 Z0 L2 P3",
                    "G0 X3.0 Y4.0 Z10.0 F110",
                    "(g82 w/Q)",
                    "G81 X10.0 Y10.0 F100 R9.0 Z0",
                ]
            ]
        )
        gcode = self.post.export2()[0][1]

        # were they replaced?
        for drill_g in drill_codes:
            self.assertNotIn(drill_g, gcode, f"Should have expanded drills, but saw {drill_g}")

        # did we actually produce any replacement?

        # At least one G4 for the G81 Q
        self.assertIn("G4 ", gcode, gcode)

    def test_empty_path(self):
        self.post._machine.processing.tool_change = True
        self.post.apply_configuration_bundle()

        self.profile_op.Path = Path.Path([])
        gcode = self.post.export2()[0][1]

        # manual sequence
        expect = """&ToolName=1
&Tool=1
"""

        self.assertIn(
            expect, gcode, f"In\n---{gcode}\n---"
        )  # FIXME: a diff-like would be much better

    @unittest.expectedFailure
    def test_todo(self):
        self.assertTrue(False, "probe")
        self.assertTrue(False, "diff precision for mm|in")
        self.assertTrue(False, "test on machine G20/G21")
        self.assertTrue(False, "precision conversion should round +1 digit, then precision")
        self.assertTrue(
            False,
            "test_rapid_z should fail, should have 3 digits of precision? or test_rapid_xy should fail should have .0",
        )
        self.assertTrue(False, "do not like _convert_generic")
        self.assertTrue(False, "test in/sec conversion")
        self.assertTrue(False, "block-delete isn't spb compatible")
        self.assertTrue(False, "Add Must Line Number for modal resulting in Zn")
        # comments stripped, no-headers, etc.
