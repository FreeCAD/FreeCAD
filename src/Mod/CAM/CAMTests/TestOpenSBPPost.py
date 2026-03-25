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
import unittest

import Path
import Constants
import CAMTests.PathTestUtils as PathTestUtils
import CAMTests.PostTestMocks as PostTestMocks
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

eol = "\n"  # for fstrings


class TestOpenSBPPost(PathTestUtils.PathTestBase):
    """Test OpenSBP-specific features of the opensbp_post.py postprocessor.

    OpenSBP uses native ShopBot commands instead of standard G-code.
    These tests verify command conversion for the major command categories.
    """

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "opensbp")

    @classmethod
    def tearDownClass(cls):
        pass

    def setUp(self):
        self.maxDiff = None
        print("## assign Machine.create_3axis_config")
        self.post._machine = Machine.create_3axis_config()
        # FIXME: is this right? I want the class's config to apply. and shouldn't all Test*Post do this?
        self.post._merge_machine_config()
        self.post._apply_schema_defaults()
        self.post._apply_job_property_overrides()
        self.post.reinitialize()
        print(f"## _mach setup: { self.post._machine.__class__.__name__}")
        import json

        print(
            f"---_machine\n",
            # json.dumps(self.post._machine.to_dict()["output"], sort_keys=True, indent=2),
            json.dumps(self.post._machine.to_dict(), sort_keys=True, indent=2),
            "\n---",
        )
        print(
            f"--.postprocessor_properties\n{json.dumps(self.post._machine.postprocessor_properties, sort_keys=True, indent=2)}"
        )

        self.post._machine.name = "Test ShopBot Machine"
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            min_rpm=0,
            max_rpm=24000,
            max_power_kw=3.0,
        )
        self.post._machine.toolheads = [toolhead]
        # Reset tracked speeds before each test
        self.post._current_move_speed_xy = None
        self.post._current_move_speed_z = None
        self.post._current_jog_speed_xy = None
        self.post._current_jog_speed_z = None

    def tearDown(self):
        pass

    # -------------------------------------------------------------------------
    # File extension and defaults
    # -------------------------------------------------------------------------

    def test_default_file_extension(self):
        """Default output file extension is 'sbp'."""
        schema = self.post.get_common_property_schema()
        ext = next((p["default"] for p in schema if p["name"] == "file_extension"), None)
        self.assertEqual(ext, "sbp")

    def test_default_preamble_contains_opensbp_comment(self):
        """Default preamble starts with an OpenSBP comment line."""
        schema = self.post.get_common_property_schema()
        preamble = next((p["default"] for p in schema if p["name"] == "preamble"), None)
        self.assertIsNotNone(preamble)
        self.assertIn("OpenSBP", preamble)

    def test_default_postamble_contains_spindle_off(self):
        """Default postamble ends program with spindle-off command C7."""
        schema = self.post.get_common_property_schema()
        postamble = next((p["default"] for p in schema if p["name"] == "postamble"), None)
        self.assertIsNotNone(postamble)
        self.assertIn("C7", postamble)

    # -------------------------------------------------------------------------
    # Comment conversion
    # -------------------------------------------------------------------------

    def test_comment_parentheses_converted(self):
        """
        G-code comments in parentheses are converted to OpenSBP single-quote style.

        BEFORE: (This is a comment)
        AFTER:  'This is a comment
        """
        command = Path.Command("(This is a comment)")
        result = self.post._convert_comment(command)
        self.assertEqual(result, "'This is a comment")

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
        command = Path.Command("G1", {"X": 10.0})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X10.000", result)

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

    # -------------------------------------------------------------------------
    # Arc moves (G2/G3) w/o Z → G-Code
    # -------------------------------------------------------------------------

    def test_arc_cw_g2(self):
        """
        CW arc (G2)
        """
        command = Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0})
        result = self.post._convert_arc_move(command)
        self.assertEqual("G2 X10.000 Y0.000 I5.000 J0.000", result)

    def test_arc_ccw_g3(self):
        """
        CCW arc (G3)
        """
        command = Path.Command("G3", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0})
        result = self.post._convert_arc_move(command)
        self.assertEqual("G3 X10.000 Y0.000 I5.000 J0.000", result)

    def test_helical_arc_includes_plunge(self):
        """
        Helical arc (G2/G3 with Z) adds a plunge parameter as the 9th field.

        BEFORE: G2 X10 Y0 I5 J0 Z-5 (current Z=0)
        AFTER:  CG,,10.0000,0.0000,5.0000,0.0000,L,1,5.0000
        """
        self.post._modal_state = {"Z": 0.0}
        command = Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -5.0})
        result = self.post._convert_arc_move(command)
        lines = result.strip().splitlines()
        cg_line = next(l for l in lines if l.startswith("CG"))
        self.assertEqual("CG,,10.000,0.000,5.000,0.000,T,1,5.000", cg_line)

    def test_arc_no_gcode_in_output(self):
        """Helix output must not contain G2 or G3."""
        self.post._convert_rapid_move(Path.Command("G0", {"Z": 0}))
        command = Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -1.0})
        result = self.post._convert_arc_move(command)
        self.assertNotIn("G2", result)
        self.assertNotIn("G3", result)

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
        self.post.values["AUTOMATIC_TOOL_CHANGER"] = False
        command = Path.Command("M6", {"T": 2})
        result = self.post._convert_tool_change(command)
        self.assertIn(">PAUSE", result)
        self.assertIn(">&Tool=2", result)

    def test_tool_change_automatic_no_pause(self):
        """
        Automatic tool change (ATC enabled) does not emit PAUSE.

        BEFORE: M6 T3
        AFTER:  >&ToolName=3
                >&Tool=3
        """
        self.post.values["AUTOMATIC_TOOL_CHANGER"] = True
        command = Path.Command("M6", {"T": 3})
        result = self.post._convert_tool_change(command)
        self.assertIn(">&Tool=3", result)
        self.assertNotIn(">PAUSE", result)

    def test_tool_change_sets_tool_name(self):
        """Tool change always sets >&ToolName variable."""
        self.post.values["AUTOMATIC_TOOL_CHANGER"] = False
        command = Path.Command("M6", {"T": 5})
        result = self.post._convert_tool_change(command)
        self.assertIn(">&ToolName=5", result)

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
        self.post.values["AUTOMATIC_SPINDLE"] = False
        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertIn(">PAUSE", result)
        self.assertIn("18000", result)

    def test_spindle_on_automatic_emits_tr_and_c6(self):
        """
        M3 with automatic spindle control emits TR (speed), C6 (on), and PAUSE (wait).

        BEFORE: M3 S18000
        AFTER:  >TR,18000
                C6
                >PAUSE 2
        """
        self.post.values["AUTOMATIC_SPINDLE"] = True
        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertIn(">TR,18000", result)
        self.assertIn("C6", result)
        self.assertIn(">PAUSE 2", result)

    def test_spindle_off_manual_emits_pause(self):
        """
        M5 without automatic spindle emits manual prompt and PAUSE.

        BEFORE: M5
        AFTER:  'Turn spindle OFF manually
                >PAUSE
        """
        self.post.values["AUTOMATIC_SPINDLE"] = False
        command = Path.Command("M5", {})
        result = self.post._convert_spindle_command(command)
        self.assertIn(">PAUSE", result)

    def test_spindle_off_automatic_emits_tr0_and_c7(self):
        """
        M5 with automatic spindle control emits TR,0 and C7 (spindle off).

        BEFORE: M5
        AFTER:  >TR,0
                C7
        """
        self.post.values["AUTOMATIC_SPINDLE"] = True
        command = Path.Command("M5", {})
        result = self.post._convert_spindle_command(command)
        self.assertIn(">TR,0", result)
        self.assertIn("C7", result)

    def test_spindle_no_gcode_in_output(self):
        """Spindle output must not contain M3, M4, or M5."""
        self.post.values["AUTOMATIC_SPINDLE"] = True
        command = Path.Command("M3", {"S": 18000})
        result = self.post._convert_spindle_command(command)
        self.assertNotIn("M3", result)
        self.assertNotIn("M4", result)
        self.assertNotIn("M5", result)

    # -------------------------------------------------------------------------
    # Dwell (G4)
    # -------------------------------------------------------------------------

    def test_dwell_produces_pause_with_time(self):
        """
        G4 dwell → >PAUSE <seconds>

        BEFORE: G4 P2.5
        AFTER:  >PAUSE 2.50
        """
        command = Path.Command("G4", {"P": 2.5})
        result = self.post._convert_dwell(command)
        self.assertIn(">PAUSE", result)
        self.assertIn("2.50", result)

    def test_dwell_no_gcode_in_output(self):
        """Dwell output must not contain G4."""
        command = Path.Command("G4", {"P": 1.0})
        result = self.post._convert_dwell(command)
        self.assertNotIn("G4", result)

    # -------------------------------------------------------------------------
    # Suppressed commands
    # -------------------------------------------------------------------------

    def test_fixture_commands_suppressed(self):
        """
        G54–G59 fixture offsets are suppressed (return None).

        OpenSBP has no work coordinate system concept.
        """
        for fixture in ["G54", "G55", "G56", "G57", "G58", "G59"]:
            command = Path.Command(fixture, {})
            result = self.post._convert_fixture(command)
            self.assertIsNone(result, f"{fixture} should be suppressed")

    def test_modal_commands_suppressed(self):
        """
        Standard G-code modal setup commands are suppressed (return None).

        OpenSBP doesn't use G20/G21 (units), G43, G80, G90, etc.
        """
        for modal in ["G20", "G21", "G43", "G80", "G90", "G91"]:
            command = Path.Command(modal, {})
            result = self.post._convert_modal_command(command)
            # FIXME: wrong
            self.assertIsNone(result, f"{modal} should be suppressed")

    # -------------------------------------------------------------------------
    # Unit conversion (imperial output)
    # -------------------------------------------------------------------------

    def test_linear_move_imperial_conversion(self):
        """
        With imperial output units, coordinate values are divided by 25.4.

        BEFORE: G1 X25.4 Y50.8 (metric input)
        """
        self.post._machine.output.units = OutputUnits.IMPERIAL
        command = Path.Command("G1", {"X": 25.4, "Y": 50.8})
        result = self.post._convert_linear_move(command)
        self.assertIn("G1 X1.000 Y2.000", result)

    def test_rapid_move_imperial_conversion(self):
        """With imperial output, rapid move coordinates are divided by 25.4."""
        self.post._machine.output.units = OutputUnits.IMPERIAL
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
        # FIXME: what's the right way to do the above? inconsistent use of VALUES[] and ._machine.*
        self.post._merge_machine_config()

        # basic stuff, + something that is shopbot specific
        # FIXME: all Test*Post should do this
        # FIXME: a test-util function for gcode-name to arbitrary-good-gcode, to generate this list from Constants, etc
        handled_gcode = [ Path.Command(g) for g in
                # trying to list all handled gcodes, is checked below against opensbp list
                ("G0X1Y2Z3 G1X4Y5Z6 G2X7Y8I9J10 G3X11Y12I13J14 G2X7Y8I9J10Z11 G3X11Y12I13J14Z12 G4P2 "
                "G20 G21 G38.2X1Y2Z3 G54 G90 G91 G92X4Y5Z6 "
                # The drill params don't necessarily make sense in these, we just need certain params:
                "G73Z7R91Q1 G74Z11R12 G80 G81Z9R10 G82Z10R11P12 G83Z11R12Q2 G84Z12R13 G85Z1R2 G88Z30R31 G89Z3R4 "
                "M0 M1 M3S1 M5 M6T2 M7 M8 M9 "
                "(comment)"
                ).split(" ")
        ]

        self.profile_op.Path = Path.Path( handled_gcode )
        self.post.export2()
        self.assertTrue(True, "No Crash")

        # Did we cover all the opensbp_post supported gcodes?
        # remove the redundant x0n from known, we only test xn above
        all_supported = (
            self.post.GCodeSupported - self.post.GCodeUnsupported
        )
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
        )
        untried = set([p for p in all_possible if not re.search(r"0\d$", p)]) - set(
            [p.Name for p in handled_gcode]
        )
        self.assertEqual(
            set(),
            untried,
            f"Untried but CAM/PostProcessing allowed, add to list: {sorted(untried)}",
        )

    @unittest.expectedFailure
    def test_full_export_defaults(self):
        """
        Check the unchangeable defaults:
        Always line-numbers
        """
        self.profile_op.Path = Path.Path(
            [
                # gcode, line-numbered
                Path.Command("G0", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                # shopbot, no line-number
                Path.Command("G2", {"X": 10.0, "Y": 0.0, "I": 5.0, "J": 0.0, "Z": -5.0}),
            ]
        )
        lines = "".join((g for _, g in self.post.export2()))
        lines = lines.split("\n")

        g0 = next((g for g in lines if "G0 " in g), None)
        g2 = next((g for g in lines if "CG," in g), None)

        self.assertIsNotNone(g0, f"Expected a G0 in:\n{eol.join(lines)}\n---")
        self.assertIsNotNone(g2, f"Expected a CG in:\n{eol.join(lines)}\n---")

        # gcode pass-through's must get numbered
        m = re.match(r"N\d+", g0)
        self.assertTrue(m, f"Expected line number for a G0:\n\t{g0}")

        # shopbot native must NOT get numbered
        m = re.match(r"N\d+", g0)
        self.assertFalse(m, f"Expected NO line number for a helix CG:\n\t{g2}")

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
        self.post._merge_machine_config()

        # basic stuff, + something that is shopbot specific
        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                # identical G01's -> elide one
                Path.Command("G1", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
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

        g1s = [g for g in lines if " G1 " in g]
        self.assertEqual(1, len(g1s), f"Expected 1x G1's in\n{eol.join(lines)}\n---")

        arcs = [g for g in lines if "CG," in g]
        print(f"arcs: {arcs}")
        for part, lines in results:
            print(part)
        print("XXX")
        as_text = "\n".join(g for _, g in results)
        print(f"---FINAL\n" + as_text + "---")
        self.assertEqual(
            2,
            len(arcs),
            f"expected 2 (identical) CG's, because the Z is relative, saw:\n{as_text}\n---",
        )  # doesn't remove duplicate arc

    def test_full_export_contains_no_gcode_moves(self):
        """
        Full export of a simple profile must not contain G0 or G1 move commands.
        """
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 10.0, "Y": 0.0, "Z": -1.0, "F": 500.0}),
                Path.Command("G0", {"Z": 5.0}),
            ]
        )
        results = self.post.export2()
        output = "\n".join(g for _, g in results)
        print(output)
        # No bare G0/G1 move lines (comments mentioning "G0" are OK to skip
        # checking here; we look for actual command lines)

        move_lines = [l for l in output.splitlines() if re.match(r"^\s*G[01]\b", l.strip())]
        self.assertEqual(move_lines, [], f"Unexpected G-code move lines: {move_lines}")

    @unittest.expectedFailure
    def test_todo(self):
        self.assertTrue(False, "helix speed projection")
        self.assertTrue(False, "probe")
        self.assertTrue(False, "diff precision for mm|in")
        self.assertTrue(False, "test on machine G20/G21")
        self.assertTrue(False, "implement all in test_modal_commands_suppressed")
        self.assertTrue(False, "precision conversion should round +1 digit, then precision")
        self.assertTrue(False, "drill conversion in _expand_canned_cycles")
        self.assertTrue(
            False, "drill conversion includes g98.... add back to test_full_export_no_crash"
        )
        self.assertTrue(
            False,
            "test_rapid_z should fail, should have 3 digits of precision? or test_rapid_xy should fail should have .0",
        )
        self.assertTrue(False, "do not like _convert_generic")
        self.assertTrue(False, "test in/sec conversion")
        self.assertTrue(False, "block-delete isn't spb compatible")
