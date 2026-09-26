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
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Tests for the Heidenhain Klartext post processor (heidenhain_post.py).

Klartext is a conversational language, not G-code, so every command is
translated.  The tests run the full export2() pipeline on a mock job and
inspect the numbered blocks that come out.
"""

import re

import Path
import Constants
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory, VALID_SCOPES, property_scope
from Path.Post.CAMErrors import CAMValueError
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

EOL = "\n"


def strip_numbers(lines):
    """Drop the leading block number from every numbered line."""
    return [re.sub(r"^\d+ ", "", line) for line in lines]


class TestHeidenhainPost(PathTestUtils.PathTestBase):
    """Klartext conversion, block numbering and cycle output of heidenhain_post.py."""

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = "heidenhain"
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "heidenhain")

    def setUp(self):
        self.maxDiff = None

        machine = Machine.create_3axis_config()
        machine.name = "Test Heidenhain"
        machine.output.units = OutputUnits.METRIC
        machine.output.output_header = False
        machine.output.comments.enabled = False
        machine.output.precision.axis = 3
        machine.output.precision.feed = 0
        machine.output.precision.spindle = 0
        machine.output.duplicates.commands = True
        machine.output.duplicates.parameters = True
        machine.processing.tool_change = False
        machine.toolheads = [
            Toolhead(
                name="Spindle",
                toolhead_type=ToolheadType.ROTARY,
                min_rpm=0,
                max_rpm=12000,
                max_power_kw=10.0,
            )
        ]
        # Tests set the paths they need; start every export from an empty operation.
        self.profile_op.Path = Path.Path([])
        self.post._machine = machine
        self.post.apply_configuration_bundle()

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def export(self, commands=None):
        """Run export2() on the given commands and return the output text."""
        if commands is not None:
            self.profile_op.Path = Path.Path(
                [Path.Command(c) if isinstance(c, str) else c for c in commands]
            )
        self.post.apply_configuration_bundle()
        return self.post.export2()[0][1]

    def blocks(self, commands=None):
        """Export and return the lines without block numbers."""
        return strip_numbers(self.export(commands).split(EOL))

    def assert_block(self, expected, lines):
        self.assertIn(expected, lines, f"expected block {expected!r} in\n{EOL.join(lines)}")

    def assert_no_block(self, unexpected, lines):
        self.assertNotIn(
            unexpected, lines, f"unexpected block {unexpected!r} in\n{EOL.join(lines)}"
        )

    def assert_no_gcode(self, lines):
        leaks = [l for l in lines if re.match(r"^(N\d+ )?G\d", l)]
        self.assertEqual([], leaks, f"G-code leaked into Klartext output:\n{EOL.join(lines)}")

    # ------------------------------------------------------------------
    # schema
    # ------------------------------------------------------------------

    def test_default_file_extension(self):
        """Klartext programs are .h files."""
        self.assertEqual("h", self.post.get_file_extension())

    def test_property_schema_is_well_formed(self):
        """Every property declares a valid scope and reaches the values dict."""
        for prop in self.post.get_full_property_schema():
            self.assertIn(property_scope(prop), VALID_SCOPES, prop["name"])
            if prop["type"] == "choice":
                self.assertIn(prop["default"], prop["choices"], prop["name"])
            self.assertIn(prop["name"].upper(), self.post.values, prop["name"])

    def test_drill_cycles_not_translated_by_default(self):
        """Every drill cycle CAM emits has a native cycle, so none are expanded."""
        schema = {p["name"]: p for p in self.post.get_common_property_schema()}
        self.assertEqual("", schema["drill_cycles_to_translate"]["default"])
        self.assertFalse(schema["output_tool_length_offset"]["default"])
        self.assertTrue(schema["supports_tool_radius_compensation"]["default"])

    # ------------------------------------------------------------------
    # program frame and numbering
    # ------------------------------------------------------------------

    def test_program_frame(self):
        """BEGIN PGM is block 0 and END PGM is the last block."""
        gcode = self.export([])
        lines = gcode.split(EOL)
        self.assertEqual("0 BEGIN PGM MOCKJOB MM", lines[0])
        self.assertTrue(lines[-1].endswith(" END PGM MOCKJOB MM"), lines[-1])

    def test_block_numbers_are_sequential_physical_lines(self):
        """Blocks count from 0 by 1; lines continued with '~' are not numbered."""
        gcode = self.export(
            [
                "G0 X0 Y0 Z10",
                "G81 X5 Y5 Z-10 R2 F5",
                "G1 X20 F5",
            ]
        )
        lines = gcode.split(EOL)
        expected = 0
        continuation = False
        for line in lines:
            if continuation:
                self.assertFalse(re.match(r"^\d+ ", line), f"continuation numbered: {line}")
            else:
                self.assertTrue(
                    line.startswith(f"{expected} "), f"expected block {expected}: {line}"
                )
                expected += 1
            continuation = line.rstrip().endswith("~")
        self.assertGreater(expected, 5)

    def test_block_numbers_ignore_machine_line_number_settings(self):
        """Klartext always numbers from 0; the N word settings do not apply."""
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 100
        self.post._machine.output.formatting.line_increment = 10
        gcode = self.export(["G0 X1 Y2 Z3"])
        lines = gcode.split(EOL)
        self.assertTrue(lines[0].startswith("0 "), lines[0])
        self.assertTrue(lines[1].startswith("1 "), lines[1])
        self.assertFalse(any(re.search(r"\bN\d+", l) for l in lines), gcode)

    def test_program_name_from_property(self):
        """program_name overrides the job label and is sanitized."""
        self.post._machine.postprocessor_properties["program_name"] = "My Part #1"
        lines = self.export([]).split(EOL)
        self.assertEqual("0 BEGIN PGM MY_PART_1 MM", lines[0])
        self.assertTrue(lines[-1].endswith(" END PGM MY_PART_1 MM"))

    def test_program_name_falls_back_to_job_label(self):
        """An empty program_name uses the job label."""
        self.post._machine.postprocessor_properties["program_name"] = ""
        self.job.Label = "bracket v2"
        try:
            lines = self.export([]).split(EOL)
            self.assertEqual("0 BEGIN PGM BRACKET_V2 MM", lines[0])
        finally:
            self.job.Label = "MockJob"

    def test_blk_form_from_stock(self):
        """BLK FORM 0.1/0.2 come from the stock bounding box, after BEGIN PGM."""
        lines = self.blocks([])
        self.assertEqual("BLK FORM 0.1 Z X+0.000 Y+0.000 Z+0.000", lines[1])
        self.assertEqual("BLK FORM 0.2 X+100.000 Y+100.000 Z+10.000", lines[2])

    def test_blk_form_disabled(self):
        self.post._machine.postprocessor_properties["output_blk_form"] = False
        lines = self.blocks([])
        self.assertFalse(any(l.startswith("BLK FORM") for l in lines), lines)

    def test_blk_form_uses_tool_axis(self):
        self.post._machine.postprocessor_properties["tool_axis"] = "Y"
        lines = self.blocks([])
        self.assertEqual("BLK FORM 0.1 Y X+0.000 Y+0.000 Z+0.000", lines[1])

    def test_no_unit_command(self):
        """Units live in BEGIN PGM; G20/G21 are never written."""
        lines = self.blocks(["G21", "G20"])
        self.assert_no_block("G21", lines)
        self.assert_no_block("G20", lines)
        self.assert_no_gcode(lines)

    # ------------------------------------------------------------------
    # linear motion
    # ------------------------------------------------------------------

    def test_rapid_move(self):
        """G0 becomes an L block with R0 and FMAX."""
        lines = self.blocks(["G0 X10 Y20 Z5"])
        self.assert_block("L X+10.000 Y+20.000 Z+5.000 R0 FMAX", lines)

    def test_rapid_move_with_feed_rate_instead_of_fmax(self):
        """Controls without FMAX get a feed value on rapid blocks."""
        self.post._machine.postprocessor_properties["rapid_feed_rate"] = 8000
        lines = self.blocks(["G0 X10 Y20 Z5"])
        self.assert_block("L X+10.000 Y+20.000 Z+5.000 R0 F8000", lines)
        self.assertFalse(any("FMAX" in l for l in lines))

    def test_rapid_move_ignores_f(self):
        """The F of a G0 is not a cutting feed and does not appear."""
        lines = self.blocks(["G0 X10 F100"])
        self.assert_block("L X+10.000 R0 FMAX", lines)

    def test_linear_move_feed_is_per_minute(self):
        """F is mm/s in the Path and mm/min in Klartext."""
        lines = self.blocks(["G1 X10 Y-5 Z-1 F10"])
        self.assert_block("L X+10.000 Y-5.000 Z-1.000 R0 F600", lines)

    def test_linear_move_reuses_last_feed(self):
        """A G1 without F carries the last programmed feed."""
        lines = self.blocks(["G1 X10 F10", "G1 X20"])
        self.assert_block("L X+10.000 R0 F600", lines)
        self.assert_block("L X+20.000 R0 F600", lines)

    def test_signed_coordinates_and_negative_zero(self):
        """Every coordinate carries a sign; -0 is written +0."""
        lines = self.blocks(["G1 X-0.0000001 Y0 Z-0 F10"])
        self.assert_block("L X+0.000 Y+0.000 Z+0.000 R0 F600", lines)

    def test_rotary_axes_in_l_block(self):
        """A/B/C are written as degrees without unit conversion."""
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.blocks(["G1 X25.4 A90 C-45 F25.4"])
        self.assert_block("L X+1.000 A+90.000 C-45.000 R0 F60", lines)

    def test_duplicate_parameter_suppression(self):
        """Unchanged coordinates may be dropped; the feed is still known."""
        self.post._machine.output.duplicates.parameters = False
        lines = self.blocks(["G1 X10 Y10 F10", "G1 X20 Y10"])
        self.assert_block("L X+10.000 Y+10.000 R0 F600", lines)
        self.assert_block("L X+20.000 R0 F600", lines)

    def test_duplicate_command_suppression_cannot_strip_block_word(self):
        """Even with modal commands enabled, every block keeps its L word."""
        self.post._machine.output.duplicates.commands = False
        lines = self.blocks(["G1 X10 F10", "G1 X20", "G1 X30"])
        self.assert_block("L X+20.000 R0 F600", lines)
        self.assert_block("L X+30.000 R0 F600", lines)

    def test_cutter_compensation(self):
        """G41/G42/G40 set RL/RR/R0 on following blocks and are not written."""
        lines = self.blocks(
            ["G0 X0 Y0 Z1", "G41 D1", "G1 X10 F10", "G42", "G1 X20", "G40", "G1 X30"]
        )
        self.assert_block("L X+10.000 RL F600", lines)
        self.assert_block("L X+20.000 RR F600", lines)
        self.assert_block("L X+30.000 R0 F600", lines)
        self.assert_no_gcode(lines)

    def test_block_delete(self):
        """A blockdelete annotation puts the '/' skip marker on the block."""
        cmd = Path.Command("G0", {"X": 1.0}, {"blockdelete": "True"})
        lines = self.blocks([cmd])
        self.assert_block("/ L X+1.000 R0 FMAX", lines)

    # ------------------------------------------------------------------
    # circular motion
    # ------------------------------------------------------------------

    def test_arc_cw(self):
        """G2 becomes CC (absolute center) and C ... DR-."""
        lines = self.blocks(["G0 X0 Y0 Z0", "G2 X10 Y10 I10 J0 F10"])
        self.assert_block("CC X+10.000 Y+0.000", lines)
        self.assert_block("C X+10.000 Y+10.000 DR- R0 F600", lines)

    def test_arc_ccw(self):
        """G3 becomes C ... DR+."""
        lines = self.blocks(["G0 X0 Y0 Z0", "G3 X10 Y-10 I10 J0 F10"])
        self.assert_block("CC X+10.000 Y+0.000", lines)
        self.assert_block("C X+10.000 Y-10.000 DR+ R0 F600", lines)

    def test_arc_center_is_modal(self):
        """CC is only written when the center changes."""
        lines = self.blocks(
            [
                "G0 X0 Y0 Z0",
                "G2 X10 Y10 I10 J0 F10",
                "G2 X20 Y0 I0 J-10",
                "G2 X10 Y-10 I-10 J0",
            ]
        )
        cc_blocks = [l for l in lines if l.startswith("CC ")]
        self.assertEqual(["CC X+10.000 Y+0.000"], cc_blocks, lines)
        c_blocks = [l for l in lines if l.startswith("C ")]
        self.assertEqual(3, len(c_blocks), lines)

    def test_arc_radius_format(self):
        """G2 with R becomes CR; the sign of R selects the minor or major arc."""
        lines = self.blocks(["G0 X0 Y0 Z0", "G2 X10 Y10 R10 F10", "G3 X0 Y0 R-10"])
        self.assert_block("CR X+10.000 Y+10.000 R+10.000 DR- R0 F600", lines)
        self.assert_block("CR X+0.000 Y+0.000 R-10.000 DR+ R0 F600", lines)
        self.assertFalse(any(l.startswith("CC ") for l in lines), lines)

    def test_full_circle(self):
        """An arc ending where it starts is a full circle."""
        lines = self.blocks(["G0 X10 Y0 Z0", "G3 X10 Y0 I-10 J0 F10"])
        self.assert_block("CC X+0.000 Y+0.000", lines)
        self.assert_block("C X+10.000 Y+0.000 DR+ R0 F600", lines)

    def test_helix_full_turn(self):
        """An arc with a Z change becomes CP with incremental polar angle and IZ."""
        lines = self.blocks(["G0 X10 Y0 Z0", "G3 X10 Y0 I-10 J0 Z-2 F10"])
        self.assert_block("CC X+0.000 Y+0.000", lines)
        self.assert_block("CP IPA+360.000 IZ-2.000 DR+ R0 F600", lines)

    def test_helix_half_turn_cw(self):
        """The swept angle follows the arc direction."""
        lines = self.blocks(["G0 X10 Y0 Z0", "G2 X-10 Y0 I-10 J0 Z-1 F10"])
        self.assert_block("CP IPA+180.000 IZ-1.000 DR- R0 F600", lines)

    def test_helix_quarter_turn_ccw(self):
        lines = self.blocks(["G0 X10 Y0 Z0", "G3 X0 Y10 I-10 J0 Z-0.5 F10"])
        self.assert_block("CP IPA+90.000 IZ-0.500 DR+ R0 F600", lines)

    def test_helix_three_quarter_turn_cw(self):
        """Going clockwise from +X to +Y sweeps 270 degrees."""
        lines = self.blocks(["G0 X10 Y0 Z0", "G2 X0 Y10 I-10 J0 Z-1 F10"])
        self.assert_block("CP IPA+270.000 IZ-1.000 DR- R0 F600", lines)

    def test_arc_requires_known_start(self):
        """An arc before any positioning move cannot be converted."""
        with self.assertRaisesRegex(CAMValueError, "known start position"):
            self.export(["G2 X10 Y10 I10 J0 F10"])

    def test_arc_requires_center_or_radius(self):
        with self.assertRaisesRegex(CAMValueError, "I and J"):
            self.export(["G0 X0 Y0 Z0", "G2 X10 Y10 F10"])

    # ------------------------------------------------------------------
    # drill cycles, Q-parameter format
    # ------------------------------------------------------------------

    def cycle_block(self, lines, title):
        """Return the CYCL DEF block (definition line + Q lines) with the given title."""
        for index, line in enumerate(lines):
            if line == f"CYCL DEF {title} ~":
                block = [line]
                for continued in lines[index + 1 :]:
                    block.append(continued)
                    if not continued.rstrip().endswith("~"):
                        break
                return block
        raise self.failureException(f"no CYCL DEF {title} in\n{EOL.join(lines)}")

    def test_g81_cycle_200(self):
        """G81 is CYCL DEF 200; feed starts at R because Q203 = R - Q200."""
        lines = self.blocks(["G0 X0 Y0 Z10", "G81 X5 Y5 Z-10 R2 F5"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertEqual(
            [
                "CYCL DEF 200 DRILLING ~",
                "    Q200=+2.000 ;SET-UP CLEARANCE ~",
                "    Q201=-10.000 ;DEPTH ~",
                "    Q206=300 ;FEED RATE FOR PLNGNG ~",
                "    Q202=+10.000 ;PLUNGING DEPTH ~",
                "    Q210=+0.000 ;DWELL TIME AT TOP ~",
                "    Q203=+0.000 ;SURFACE COORDINATE ~",
                "    Q204=+8.000 ;2ND SET-UP CLEARANCE ~",
                "    Q211=+0.000 ;DWELL TIME AT DEPTH",
            ],
            block,
        )
        self.assert_block("L X+5.000 Y+5.000 R0 FMAX M99", lines)
        self.assert_no_gcode(lines)

    def test_cycle_definition_is_modal(self):
        """Holes with the same parameters share one CYCL DEF."""
        lines = self.blocks(
            [
                "G0 X0 Y0 Z10",
                "G81 X5 Y5 Z-10 R2 F5",
                "G81 X15 Y5 Z-10 R2 F5",
                "G81 X25 Y5 Z-12 R2 F5",
            ]
        )
        definitions = [l for l in lines if l.startswith("CYCL DEF 200")]
        self.assertEqual(2, len(definitions), lines)
        calls = [l for l in lines if l.endswith(" M99")]
        self.assertEqual(
            [
                "L X+5.000 Y+5.000 R0 FMAX M99",
                "L X+15.000 Y+5.000 R0 FMAX M99",
                "L X+25.000 Y+5.000 R0 FMAX M99",
            ],
            calls,
        )

    def test_g99_retracts_to_r(self):
        """With G99 the second set-up clearance is zero."""
        lines = self.blocks(["G0 X0 Y0 Z10", "G99", "G81 X5 Y5 Z-10 R2 F5"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertIn("    Q204=+0.000 ;2ND SET-UP CLEARANCE ~", block)
        self.assert_no_gcode(lines)

    def test_g82_dwell(self):
        lines = self.blocks(["G0 X0 Y0 Z10", "G82 X5 Y5 Z-10 R2 F5 P1.5"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertIn("    Q211=+1.500 ;DWELL TIME AT DEPTH", block)

    def test_g83_peck(self):
        lines = self.blocks(["G0 X0 Y0 Z10", "G83 X5 Y5 Z-10 R2 F5 Q3"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertIn("    Q202=+3.000 ;PLUNGING DEPTH ~", block)

    def test_g73_chip_breaking_cycle_203(self):
        self.post._machine.postprocessor_properties["chip_break_retract"] = 0.25
        lines = self.blocks(["G0 X0 Y0 Z10", "G73 X5 Y5 Z-10 R2 F5 Q3"])
        block = self.cycle_block(lines, "203 UNIVERSAL DRILLING")
        self.assertIn("    Q202=+3.000 ;PLUNGING DEPTH ~", block)
        self.assertIn("    Q213=+4 ;NR OF CHIP BREAKS ~", block)
        self.assertIn("    Q256=+0.250 ;DIST FOR CHIP BRKNG ~", block)
        self.assertTrue(block[-1].endswith(";DEPTH REFERENCE"), block[-1])
        self.assert_block("L X+5.000 Y+5.000 R0 FMAX M99", lines)

    def test_g84_rigid_tapping_from_generator(self):
        """The tapping generator writes the pitch into F and annotates the command."""
        tap = Path.Command("G84", {"X": 5.0, "Y": 5.0, "Z": -10.0, "R": 2.0, "F": 1.25, "S": 500.0})
        tap.addAnnotations({"operation": "tapping"})
        lines = self.blocks(["G0 X0 Y0 Z10", tap])
        block = self.cycle_block(lines, "207 RIGID TAPPING NEW")
        self.assertEqual(
            [
                "CYCL DEF 207 RIGID TAPPING NEW ~",
                "    Q200=+2.000 ;SET-UP CLEARANCE ~",
                "    Q201=-10.000 ;DEPTH OF THREAD ~",
                "    Q239=+1.250 ;PITCH OF THREAD ~",
                "    Q203=+0.000 ;SURFACE COORDINATE ~",
                "    Q204=+8.000 ;2ND SET-UP CLEARANCE",
            ],
            block,
        )

    def test_g74_left_hand_tapping(self):
        tap = Path.Command("G74", {"X": 5.0, "Y": 5.0, "Z": -10.0, "R": 2.0, "F": 1.25, "S": 500.0})
        tap.addAnnotations({"operation": "tapping"})
        lines = self.blocks(["G0 X0 Y0 Z10", tap])
        block = self.cycle_block(lines, "207 RIGID TAPPING NEW")
        self.assertIn("    Q239=-1.250 ;PITCH OF THREAD ~", block)

    def test_g84_pitch_derived_from_feed_and_speed(self):
        """An unannotated G84 carries a feed (mm/s); pitch = feed / rpm."""
        # 1.25 mm/rev at 500 rpm = 625 mm/min = 10.4166 mm/s
        lines = self.blocks(["G0 X0 Y0 Z10", f"G84 X5 Y5 Z-10 R2 F{625 / 60} S500"])
        block = self.cycle_block(lines, "207 RIGID TAPPING NEW")
        self.assertIn("    Q239=+1.250 ;PITCH OF THREAD ~", block)

    def test_g85_boring_cycle_202(self):
        lines = self.blocks(["G0 X0 Y0 Z10", "G85 X5 Y5 Z-10 R2 F5"])
        block = self.cycle_block(lines, "202 BORING")
        self.assertIn("    Q206=300 ;FEED RATE FOR PLNGNG ~", block)
        self.assertIn("    Q208=300 ;RETRACTION FEED RATE ~", block)
        self.assertIn("    Q214=+0 ;DISENGAGING DIRECTN ~", block)

    def test_drill_setup_clearance_property(self):
        """A larger set-up clearance lowers the surface coordinate by the same amount."""
        self.post._machine.postprocessor_properties["drill_setup_clearance"] = 5.0
        lines = self.blocks(["G0 X0 Y0 Z10", "G81 X5 Y5 Z-10 R2 F5"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertIn("    Q200=+5.000 ;SET-UP CLEARANCE ~", block)
        self.assertIn("    Q203=-3.000 ;SURFACE COORDINATE ~", block)
        self.assertIn("    Q201=-7.000 ;DEPTH ~", block)

    def test_drill_above_retract_is_an_error(self):
        with self.assertRaisesRegex(CAMValueError, "not below the retract height"):
            self.export(["G0 X0 Y0 Z10", "G81 X5 Y5 Z5 R2 F5"])

    def test_drill_requires_r_and_z(self):
        with self.assertRaisesRegex(CAMValueError, "requires R and Z"):
            self.export(["G0 X0 Y0 Z10", Path.Command("G81", {"X": 5.0, "Y": 5.0, "Z": -1.0})])

    def test_g80_and_return_modes_are_silent(self):
        lines = self.blocks(["G0 X0 Y0 Z10", "G98", "G81 X5 Y5 Z-10 R2 F5", "G80", "G99"])
        self.assert_no_gcode(lines)

    def test_translated_drill_cycles(self):
        """With machine translation on, drills become L blocks and no cycle is written."""
        self.post._machine.processing.translate_drill_cycles = True
        lines = self.blocks(["G0 X0 Y0 Z10 F20", "G81 X5 Y5 Z-10 R2 F5"])
        self.assertFalse(any(l.startswith("CYCL DEF 200") for l in lines), lines)
        self.assertFalse(any(" M99" in l for l in lines), lines)
        self.assert_block("L X+5.000 Y+5.000 Z-10.000 R0 F300", lines)
        self.assert_no_gcode(lines)

    # ------------------------------------------------------------------
    # drill cycles, legacy format
    # ------------------------------------------------------------------

    def test_legacy_pecking_cycle(self):
        """The legacy format positions the tool itself and uses CYCL DEF 1.x."""
        self.post._machine.postprocessor_properties["cycle_format"] = "legacy"
        self.post._machine.postprocessor_properties["fixture_output"] = "datum"
        lines = self.blocks(["G0 X0 Y0 Z10", "G83 X5 Y5 Z-10 R2 F5 Q3 P0.5"])
        start = lines.index("CYCL DEF 1.0 PECKING")
        self.assertEqual(
            [
                "CYCL DEF 1.0 PECKING",
                "CYCL DEF 1.1 SET UP 2.000",
                "CYCL DEF 1.2 DEPTH -10.000",
                "CYCL DEF 1.3 PECKG -3.000",
                "CYCL DEF 1.4 DWELL 0.500",
                "CYCL DEF 1.5 F300",
                "L X+5.000 Y+5.000 R0 FMAX",
                "L Z+2.000 R0 FMAX",
                "CYCL CALL",
                "L Z+10.000 R0 FMAX",
            ],
            lines[start : start + 10],
        )
        self.assertFalse(any("~" in l for l in lines), lines)

    def test_legacy_cycle_second_hole_only_moves(self):
        self.post._machine.postprocessor_properties["cycle_format"] = "legacy"
        lines = self.blocks(
            ["G0 X0 Y0 Z10", "G99", "G81 X5 Y5 Z-10 R2 F5", "G81 X15 Y5 Z-10 R2 F5"]
        )
        self.assertEqual(1, len([l for l in lines if l == "CYCL DEF 1.0 PECKING"]), lines)
        self.assertEqual(2, len([l for l in lines if l == "CYCL CALL"]), lines)
        second = lines.index("CYCL CALL", lines.index("CYCL CALL") + 1)
        # G99: already at R after the first hole, so only the XY move precedes the call
        self.assertEqual("L X+15.000 Y+5.000 R0 FMAX", lines[second - 1])
        self.assertEqual("CYCL CALL", lines[second - 2])

    def test_legacy_rigid_tapping_cycle_17(self):
        self.post._machine.postprocessor_properties["cycle_format"] = "legacy"
        tap = Path.Command("G84", {"X": 5.0, "Y": 5.0, "Z": -10.0, "R": 2.0, "F": 1.25, "S": 500.0})
        tap.addAnnotations({"operation": "tapping"})
        lines = self.blocks(["G0 X0 Y0 Z10", tap])
        self.assert_block("CYCL DEF 17.0 RIGID TAPPING", lines)
        self.assert_block("CYCL DEF 17.1 SET UP 2.000", lines)
        self.assert_block("CYCL DEF 17.2 DEPTH -10.000", lines)
        self.assert_block("CYCL DEF 17.3 PITCH +1.250", lines)

    def test_legacy_format_has_no_boring_cycle(self):
        self.post._machine.postprocessor_properties["cycle_format"] = "legacy"
        with self.assertRaisesRegex(CAMValueError, "no legacy Klartext cycle"):
            self.export(["G0 X0 Y0 Z10", "G85 X5 Y5 Z-10 R2 F5"])

    # ------------------------------------------------------------------
    # dwell, tool change, spindle, coolant, program control
    # ------------------------------------------------------------------

    def test_dwell_cycle_9(self):
        lines = self.blocks(["G4 P2.5"])
        index = lines.index("CYCL DEF 9.0 DWELL TIME")
        self.assertEqual("CYCL DEF 9.1 DWELL 2.500", lines[index + 1])

    def test_tool_change_carries_spindle_speed(self):
        """M6 T1 followed by M3 S1000 becomes TOOL CALL 1 Z S1000 then M3."""
        self.post._machine.processing.tool_change = True
        lines = self.blocks([])
        index = lines.index("TOOL CALL 1 Z S1000")
        self.assertEqual("M3", lines[index + 1])
        self.assertFalse(any(l.startswith("TOOL CALL S") for l in lines), lines)
        self.assert_no_block("M6 T1", lines)
        self.assert_no_block("M3 S1000", lines)

    def test_tool_change_uses_tool_axis(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.postprocessor_properties["tool_axis"] = "Y"
        lines = self.blocks([])
        self.assert_block("TOOL CALL 1 Y S1000", lines)

    def test_tool_change_without_spindle_speed(self):
        """A bare M6 gives a TOOL CALL without S."""
        self.post._machine.processing.tool_change = True
        lines = self.blocks(["M6 T3"])
        self.assert_block("TOOL CALL 3 Z", lines)

    def test_tool_change_suppressed(self):
        """Without tool changes the speed still reaches the spindle via TOOL CALL S."""
        self.post._machine.processing.tool_change = False
        self.post._machine.output.comments.enabled = True
        lines = self.blocks([])
        self.assertFalse(any(re.match(r"TOOL CALL \d", l) for l in lines), lines)
        index = lines.index("TOOL CALL S1000")
        self.assertEqual("M3", lines[index + 1])
        self.assert_block("; Tool change suppressed: M6 T1", lines)
        self.assertFalse(any("M6 S" in l for l in lines), lines)

    def test_early_tool_prep(self):
        """A bare Tn pre-selects the next tool with TOOL DEF."""
        lines = self.blocks(["T2"])
        self.assert_block("TOOL DEF 2", lines)

    def test_spindle_speed_change_without_tool_change(self):
        """A new speed on M3 is a TOOL CALL S block before the M3."""
        self.post._machine.processing.tool_change = True
        lines = self.blocks(["M3 S2000", "M5", "M4 S2000"])
        index = lines.index("TOOL CALL S2000")
        self.assertEqual("M3", lines[index + 1])
        self.assertEqual(1, len([l for l in lines if l == "TOOL CALL S2000"]), lines)
        self.assert_block("M5", lines)
        self.assert_block("M4", lines)

    def test_spindle_same_speed_no_tool_call(self):
        self.post._machine.processing.tool_change = True
        lines = self.blocks(["M3 S1000"])
        self.assertFalse(any(l.startswith("TOOL CALL S") for l in lines), lines)
        self.assertEqual(2, len([l for l in lines if l == "M3"]), lines)

    def test_coolant_and_program_control(self):
        lines = self.blocks(["M8", "M9", "M7", "M0", "M1", "M2", "M30"])
        for block in ["M8", "M9", "M7", "M0", "M1", "M2", "M30"]:
            self.assert_block(block, lines)

    def test_coolant_leading_zero_normalized(self):
        lines = self.blocks(["M08", "M09"])
        self.assert_block("M8", lines)
        self.assert_block("M9", lines)

    # ------------------------------------------------------------------
    # fixtures
    # ------------------------------------------------------------------

    def test_fixture_preset(self):
        lines = self.blocks(["G54", "G55", "G59.1"])
        presets = [lines[i + 1] for i, l in enumerate(lines) if l == "CYCL DEF 247 PRESETTING ~"]
        self.assertEqual(
            [
                "    Q339=+1 ;PRESET NUMBER",
                "    Q339=+1 ;PRESET NUMBER",
                "    Q339=+2 ;PRESET NUMBER",
                "    Q339=+7 ;PRESET NUMBER",
            ],
            presets,
            lines,
        )
        self.assert_no_gcode(lines)

    def test_fixture_first_number(self):
        self.post._machine.postprocessor_properties["fixture_first_number"] = 10
        lines = self.blocks(["G55"])
        self.assert_block("    Q339=+11 ;PRESET NUMBER", lines)

    def test_fixture_datum_table(self):
        self.post._machine.postprocessor_properties["fixture_output"] = "datum"
        lines = self.blocks(["G55"])
        index = lines.index("CYCL DEF 7.0 DATUM SHIFT")
        self.assertEqual("CYCL DEF 7.1 #1", lines[index + 1])
        self.assert_block("CYCL DEF 7.1 #2", lines)

    def test_fixture_none(self):
        self.post._machine.postprocessor_properties["fixture_output"] = "none"
        lines = self.blocks(["G55"])
        self.assertFalse(any("PRESET" in l or "DATUM" in l for l in lines), lines)
        self.assert_no_gcode(lines)

    # ------------------------------------------------------------------
    # modal commands, comments, errors
    # ------------------------------------------------------------------

    def test_silent_modal_commands(self):
        """Modal codes with no Klartext meaning are accepted and dropped."""
        lines = self.blocks(["G17", "G21", "G80", "G90", "G94", "G97", "G98", "G99", "G43 H1"])
        self.assert_no_gcode(lines)
        self.assertFalse(any("TLO" in l for l in lines), lines)

    def test_incremental_mode_is_an_error(self):
        """Path commands are absolute; G91 is refused rather than misread."""
        with self.assertRaisesRegex(CAMValueError, "Unsupported command: G91"):
            self.export(["G91"])

    def test_unsupported_command_is_an_error(self):
        with self.assertRaisesRegex(CAMValueError, "Unsupported command"):
            self.export(["G92 X0"])

    def test_comment_block(self):
        """Comments are ';' blocks whatever the machine's comment symbol says."""
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.comments.symbol = "("
        lines = self.blocks(["(hello there)"])
        self.assert_block("; hello there", lines)
        self.assertFalse(any(l.startswith("(") for l in lines), lines)

    def test_comments_disabled(self):
        lines = self.blocks(["(hello there)"])
        self.assertFalse(any("hello there" in l for l in lines), lines)

    def test_header_after_begin_pgm(self):
        """Header comments follow BEGIN PGM; BEGIN PGM stays block 0."""
        self.post._machine.output.output_header = True
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.header.include_date = False
        gcode = self.export([])
        lines = gcode.split(EOL)
        self.assertEqual("0 BEGIN PGM MOCKJOB MM", lines[0])
        self.assertTrue(any("; Machine: Test Heidenhain" in l for l in lines), gcode)
        self.assertTrue(any("; T1=TC: Default Tool" in l for l in lines), gcode)

    def test_preamble_and_postamble_inside_program(self):
        self.post._machine.postprocessor_properties["preamble"] = "M129"
        self.post._machine.postprocessor_properties["postamble"] = "L Z+100 R0 FMAX M2"
        lines = self.blocks(["G0 X1 Y1 Z1"])
        self.assertLess(lines.index("M129"), lines.index("L X+1.000 Y+1.000 Z+1.000 R0 FMAX"))
        self.assertEqual("L Z+100 R0 FMAX M2", lines[-2])
        self.assertEqual("END PGM MOCKJOB MM", lines[-1])

    # ------------------------------------------------------------------
    # units
    # ------------------------------------------------------------------

    def test_imperial_output(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.blocks(["G0 X25.4 Y50.8 Z2.54", "G1 X0 F25.4"])
        self.assertEqual("BEGIN PGM MOCKJOB INCH", lines[0])
        self.assert_block("L X+1.000 Y+2.000 Z+0.100 R0 FMAX", lines)
        self.assert_block("L X+0.000 R0 F60", lines)
        self.assertEqual("END PGM MOCKJOB INCH", lines[-1])

    def test_imperial_drill_cycle(self):
        """Cycle lengths are converted; the set-up clearance property is in mm."""
        self.post._machine.output.units = OutputUnits.IMPERIAL
        self.post._machine.postprocessor_properties["drill_setup_clearance"] = 25.4
        lines = self.blocks(["G0 X0 Y0 Z50.8", "G81 X25.4 Y25.4 Z-25.4 R25.4 F25.4"])
        block = self.cycle_block(lines, "200 DRILLING")
        self.assertIn("    Q200=+1.000 ;SET-UP CLEARANCE ~", block)
        self.assertIn("    Q201=-1.000 ;DEPTH ~", block)
        self.assertIn("    Q206=60 ;FEED RATE FOR PLNGNG ~", block)
        self.assertIn("    Q203=+0.000 ;SURFACE COORDINATE ~", block)
        self.assertIn("    Q204=+1.000 ;2ND SET-UP CLEARANCE ~", block)
        self.assert_block("L X+1.000 Y+1.000 R0 FMAX M99", lines)

    # ------------------------------------------------------------------
    # whole pipeline
    # ------------------------------------------------------------------

    def test_full_export_covers_every_supported_command(self):
        """Every supported command converts without error and nothing leaks as G-code."""
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.output_header = True
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.processing.tool_change = True
        self.post._machine.processing.xy_before_z_after_tool_change = True

        tap = Path.Command(
            "G84", {"X": 5.0, "Y": 5.0, "Z": -10.0, "R": 2.0, "F": 1.25, "S": 1000.0}
        )
        tap.addAnnotations({"operation": "tapping"})
        commands = [
            Path.Command(c)
            for c in (
                "G17 G20 G21 G90 G94 G97 G54 "
                "G0X0Y0Z10 G1X10Y0Z-1F10 G2X20Y10I0J10 G3X10Y20I-10J0 G2X0Y10I0J-10Z-2 "
                "G2X10Y0R10 G4P1 G41 G1X5 G42 G1X6 G40 "
                "G98 G81X1Y1Z-5R2F5 G82X2Y2Z-5R2F5P1 G83X3Y3Z-5R2F5Q1 G73X4Y4Z-5R2F5Q1 "
                "G99 G85X6Y6Z-5R2F5 G80 "
                "M0 M1 M3S1000 M4S1500 M5 M7 M8 M9 M6T2 G43H2 G0X0Y0Z10 T3 M2 M30 (comment)"
            ).split(" ")
        ] + [tap]
        gcode = self.export(commands)
        lines = strip_numbers(gcode.split(EOL))
        self.assert_no_gcode(lines)

        tried = {c.Name for c in commands}
        supported = {
            name
            for name in self.post.GCodeSupported
            if not re.search(r"0\d$", name) and not name.startswith("G59")
        }
        supported -= {"G55", "G56", "G57", "G58", "G59", "G74"}  # same code path as G54 / G84
        self.assertEqual(set(), supported - tried, "add these to the coverage list")

    def test_cam_allowed_commands_are_classified(self):
        """Every command the base post can hand over is either supported or deliberately not."""
        rejected = {
            "G38.2",  # probing: no TCH PROBE support
            "G91",  # incremental mode: Path commands are absolute
            "G92",  # coordinate offset
            "G93",
            "G95",
            "G96",
            "G88",
            "G89",
        }
        allowed = set(
            Constants.GCODE_SUPPORTED + Constants.MCODE_SUPPORTED + Constants.GCODE_NON_CONFORMING
        )
        unclassified = allowed - set(self.post.GCodeSupported) - rejected
        self.assertEqual(set(), unclassified)
        self.assertEqual(set(), rejected & set(self.post.GCodeSupported))
