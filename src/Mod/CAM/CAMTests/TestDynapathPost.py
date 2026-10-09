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
Tests for the machine-based Dynapath post processor (dynapath_post.py).

Drives the post through export2() on a mock job and pins the Dynapath
dialect: the program name line, (T)...$ text events, G70/G71, E fixture
codes, absolute arc centers, quill cycles in K/L/O words, tapping feed,
the XYZ move after a tool change, dwell as L seconds, four-digit sequence
numbers and the E end-of-file marker.
"""

import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory, VALID_SCOPES, property_scope
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

EOL = "\n"


def _tap(name="G84", **overrides):
    """A tapping cycle as the tapping generator writes it: F is the pitch, S the speed."""
    params = {"X": 5.0, "Y": 15.0, "Z": 2.0, "R": 10.0, "F": 1.25, "S": 500.0}
    params.update(overrides)
    command = Path.Command(name, params)
    command.addAnnotations({"operation": "tapping", "rigid": "False"})
    return command


class TestDynapathPost(PathTestUtils.PathTestBase):
    """Dynapath output on a three-axis mock job."""

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = "dynapath"
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "dynapath")

    def setUp(self):
        self.maxDiff = None

        machine = Machine.create_3axis_config()
        machine.name = "Test Dynapath"
        machine.output.units = OutputUnits.METRIC
        machine.output.output_header = False
        machine.output.comments.enabled = False
        machine.output.precision.axis = 3
        machine.output.precision.feed = 3
        machine.output.precision.spindle = 0
        machine.output.duplicates.commands = True
        machine.output.duplicates.parameters = True
        machine.processing.tool_change = False
        machine.toolheads = [
            Toolhead(
                name="Spindle",
                toolhead_type=ToolheadType.ROTARY,
                min_rpm=0,
                max_rpm=6000,
                max_power_kw=5.0,
            )
        ]
        # The program name is tested on its own.
        machine.postprocessor_properties["program_name"] = "TEST"
        self.profile_op.Path = Path.Path([])
        self.profile_op.ClearanceHeight = type("obj", (object,), {"Value": 18.0})()
        self.post._machine = machine
        self.post.apply_configuration_bundle()

    # ------------------------------------------------------------------
    # helpers
    # ------------------------------------------------------------------

    def export(self, commands=None):
        if commands is not None:
            self.profile_op.Path = Path.Path(
                [Path.Command(c) if isinstance(c, str) else c for c in commands]
            )
        self.post.apply_configuration_bundle()
        return self.post.export2()[0][1]

    def lines(self, commands=None):
        return self.export(commands).split(EOL)

    def op_lines(self, commands):
        """The lines the operation produced, between the fixture and the postamble."""
        lines = self.lines(commands)
        start = lines.index("E01") + 1
        end = len(lines) - lines[::-1].index("M05") - 1
        return lines[start:end]

    def assert_line(self, expected, lines):
        self.assertIn(expected, lines, f"expected line {expected!r} in\n{EOL.join(lines)}")

    def assert_no_line(self, unexpected, lines):
        self.assertNotIn(unexpected, lines, f"unexpected line {unexpected!r} in\n{EOL.join(lines)}")

    # ------------------------------------------------------------------
    # schema
    # ------------------------------------------------------------------

    def test_default_file_extension(self):
        self.assertEqual("ncc", self.post.get_file_extension())

    def test_property_schema_is_well_formed(self):
        for prop in self.post.get_full_property_schema():
            self.assertIn(property_scope(prop), VALID_SCOPES, prop["name"])
        names = {prop["name"] for prop in self.post.get_property_schema()}
        self.assertEqual(
            names,
            {
                "program_name",
                "uppercase_comments",
                "absolute_arc_centers",
                "second_reference_plane",
                "xyz_move_after_tool_change",
                "end_of_file_marker",
            },
        )

    # ------------------------------------------------------------------
    # program frame
    # ------------------------------------------------------------------

    def test_empty_program(self):
        """Name line, preamble one code per block, G71, E01, postamble, E."""
        self.assertEqual(
            self.lines([]),
            [
                "(TEST)",
                "G17",
                "G90",
                "G80",
                "G40",
                "G71",
                "M3S1000",
                "E01",
                "M05",
                "G80",
                "G40",
                "G17",
                "G90",
                "M30",
                "E",
            ],
        )

    def test_program_name_from_document(self):
        """An empty property names the program after the document, trimmed to
        eight letters and digits, in upper case."""
        self.post._machine.postprocessor_properties["program_name"] = ""
        self.job.Document = type("obj", (object,), {"Label": "Bracket v2 (left)"})()
        try:
            self.assertEqual("(BRACKETV)", self.lines([])[0])
            self.post._machine.postprocessor_properties["uppercase_comments"] = False
            self.assertEqual("(Bracketv)", self.lines([])[0])
        finally:
            del self.job.Document

    def test_program_name_property(self):
        self.post._machine.postprocessor_properties["program_name"] = "part-12345"
        self.assertEqual("(PART1234)", self.lines([])[0])

    def test_end_of_file_marker_off(self):
        self.post._machine.postprocessor_properties["end_of_file_marker"] = False
        lines = self.lines([])
        self.assertEqual("M30", lines[-1])
        self.assert_no_line("E", lines)

    def test_inch_units(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.lines(["G0 X25.4 Y50.8 Z12.7"])
        self.assert_line("G70", lines)
        self.assert_no_line("G71", lines)
        self.assert_line("G0X1.000Y2.000Z0.500", lines)

    def test_no_space_between_words(self):
        """The machine file's command space does not apply: a Delta block has none."""
        self.post._machine.output.formatting.command_space = " "
        self.assert_line("G1X10.000Y20.000Z5.000F600.000", self.lines(["G1 X10 Y20 Z5 F10"]))

    # ------------------------------------------------------------------
    # comments and header
    # ------------------------------------------------------------------

    def test_comments_are_text_events(self):
        """A comment is a text event closed by $. Parentheses and $ inside the
        text are removed: the control has no escape for them, and a $ in the text
        would end the event early and leave the rest of the line as a new event."""
        self.post._machine.output.comments.enabled = True
        lines = self.lines([Path.Command("(Begin profile (pass 2) $5)")])
        self.assert_line("(T)BEGIN PROFILE PASS 2 5$", lines)

    def test_comments_keep_case_when_asked(self):
        self.post._machine.output.comments.enabled = True
        self.post._machine.postprocessor_properties["uppercase_comments"] = False
        self.assert_line("(T)Begin profile$", self.lines([Path.Command("(Begin profile)")]))

    def test_header_is_text_events(self):
        self.post._machine.output.output_header = True
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.header.include_date = False
        lines = self.lines([])
        self.assertEqual("(TEST)", lines[0])
        self.assert_line("(T)MACHINE: TEST DYNAPATH$", lines)
        self.assert_line("(T)FIXTURE: G54$", lines)
        for line in lines[1:]:
            if line.startswith("("):
                self.assertTrue(line.startswith("(T)") and line.endswith("$"), line)

    # ------------------------------------------------------------------
    # fixtures
    # ------------------------------------------------------------------

    def test_fixtures_are_e_codes(self):
        self.job.Fixtures = ["G54", "G59", "G59.3"]
        try:
            lines = self.lines([])
        finally:
            self.job.Fixtures = ["G54"]
        self.assert_line("E01", lines)
        self.assert_line("E06", lines)
        self.assert_line("E09", lines)
        for code in ("G54", "G59", "G59.3"):
            self.assert_no_line(code, lines)

    # ------------------------------------------------------------------
    # arcs
    # ------------------------------------------------------------------

    def test_arc_centers_are_absolute(self):
        lines = self.op_lines(
            [
                "G0 X10 Y0 Z18",
                "G2 X0 Y10 Z18 I-10 J0 F100",
                "G3 X10 Y20 Z18 I10 J0 F100",
            ]
        )
        self.assertEqual(
            lines,
            [
                "G0X10.000Y0.000Z18.000",
                "G2X0.000Y10.000Z18.000I0.000J0.000F6000.000",
                "G3X10.000Y20.000Z18.000I10.000J10.000F6000.000",
            ],
        )

    def test_arc_centers_absolute_in_inches(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.op_lines(["G0 X25.4 Y0 Z25.4", "G2 X0 Y25.4 Z25.4 I-25.4 J0 F100"])
        self.assertEqual("G2X0.000Y1.000Z1.000I0.000J0.000F236.220", lines[1])

    def test_arc_centers_incremental_when_asked(self):
        self.post._machine.postprocessor_properties["absolute_arc_centers"] = False
        lines = self.op_lines(["G0 X10 Y0 Z18", "G2 X0 Y10 Z18 I-10 J0 F100"])
        self.assertEqual("G2X0.000Y10.000Z18.000I-10.000J0.000F6000.000", lines[1])

    def test_arc_has_no_k(self):
        lines = self.op_lines(["G0 X10 Y0 Z18", "G2 X0 Y10 Z15 I-10 J0 K-3 F100"])
        self.assertEqual("G2X0.000Y10.000Z15.000I0.000J0.000F6000.000", lines[1])

    # ------------------------------------------------------------------
    # quill cycles
    # ------------------------------------------------------------------

    def test_drill_cycle_words(self):
        """Every hole carries its full parameter set, with O from the operation's
        clearance height, and the group ends in G80."""
        holes = [
            Path.Command("G81 X2 Y2 Z0 R16 F0.8"),
            Path.Command("G81 X18 Y18 Z0 R16 F0.8"),
        ]
        lines = self.op_lines(holes + ["G0 Z18"])
        self.assertEqual(
            lines,
            [
                "G81X2.000Y2.000Z0.000F48.000O18.000R16.000",
                "G81X18.000Y18.000Z0.000F48.000O18.000R16.000",
                "G80",
                "G0Z18.000",
            ],
        )

    def test_drill_cycle_full_words_with_modal_output(self):
        self.post._machine.output.duplicates.parameters = False
        holes = [
            Path.Command("G81 X2 Y2 Z0 R16 F0.8"),
            Path.Command("G81 X2 Y9 Z0 R16 F0.8"),
        ]
        lines = self.op_lines(holes)
        self.assertEqual("G81X2.000Y9.000Z0.000F48.000O18.000R16.000", lines[1])

    def test_peck_and_dwell_words(self):
        """Q is K, P is L in seconds."""
        lines = self.op_lines(
            [
                Path.Command("G83 X2 Y2 Z0 R16 Q3 F0.8"),
                Path.Command("G82 X2 Y9 Z0 R16 P0.5 F0.8"),
            ]
        )
        # The cycle changes between the holes, so the base terminator closes the first.
        self.assertEqual(
            lines[:3],
            [
                "G83X2.000Y2.000Z0.000K3.000F48.000O18.000R16.000",
                "G80",
                "G82X2.000Y9.000Z0.000F48.000O18.000R16.000L0.5",
            ],
        )

    def test_second_reference_plane_off(self):
        self.post._machine.postprocessor_properties["second_reference_plane"] = False
        lines = self.op_lines([Path.Command("G81 X2 Y2 Z0 R16 F0.8")])
        self.assertEqual("G81X2.000Y2.000Z0.000F48.000R16.000", lines[0])

    def test_second_reference_plane_in_inches(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.op_lines([Path.Command("G81 X25.4 Y25.4 Z0 R12.7 F0.8")])
        self.assertEqual("G81X1.000Y1.000Z0.000F1.890O0.709R0.500", lines[0])

    def test_no_g98_g99(self):
        lines = self.op_lines(["G98", Path.Command("G81 X2 Y2 Z0 R16 F0.8"), "G99"])
        self.assert_no_line("G98", lines)
        self.assert_no_line("G99", lines)
        self.assertEqual("G81X2.000Y2.000Z0.000F48.000O18.000R16.000", lines[0])

    def test_tapping_feed_is_pitch_times_speed(self):
        """F is pitch times spindle speed in units per minute; S is not repeated;
        the run of taps ends in G80."""
        lines = self.op_lines([_tap(), _tap("G74", X=15.0, Y=5.0), "G0 Z18"])
        self.assertEqual(
            lines,
            [
                "G84X5.000Y15.000Z2.000F625.000O18.000R10.000",
                "G74X15.000Y5.000Z2.000F625.000O18.000R10.000",
                "G80",
                "G0Z18.000",
            ],
        )

    def test_tapping_feed_in_inches(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.op_lines([_tap()])
        self.assertEqual("G84X0.197Y0.591Z0.079F24.606O0.709R0.394", lines[0])

    def test_tapping_repeat_count_is_dropped(self):
        lines = self.op_lines([_tap(L=2)])
        self.assertNotIn("L", lines[0])

    # ------------------------------------------------------------------
    # tool change and dwell
    # ------------------------------------------------------------------

    def test_tool_change_block(self):
        """M05 before M6, M6 and T in one block, an XYZ move on the first rapid after."""
        self.post._machine.processing.tool_change = True
        lines = self.lines(["G0 Z18", "G0 X5 Y5", "G1 Z-1 F100", "G0 Z18"])
        change = lines.index("M6T1")
        self.assertEqual("M05", lines[change - 1])
        self.assertEqual("M3S1000", lines[change + 1])
        self.assert_line("G0X0.000Y0.000Z18.000", lines)
        self.assertEqual(lines[-2:], ["M30", "E"])
        # Only the first rapid is completed; the retract at the end is left alone.
        self.assertEqual(1, lines.count("G0X0.000Y0.000Z18.000"))
        self.assert_line("G0Z18.000", lines)

    def test_tool_change_first_rapid_with_xy_is_left_alone(self):
        self.post._machine.processing.tool_change = True
        lines = self.lines(["G0 X5 Y5 Z18", "G0 Z18"])
        self.assert_line("G0X5.000Y5.000Z18.000", lines)
        self.assert_no_line("G0X0.000Y0.000Z18.000", lines)

    def test_xyz_move_after_tool_change_off(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.postprocessor_properties["xyz_move_after_tool_change"] = False
        lines = self.lines(["G0 Z18"])
        self.assert_line("G0Z18.000", lines)
        self.assert_no_line("G0X0.000Y0.000Z18.000", lines)

    def test_no_tool_length_offset_by_default(self):
        self.post._machine.processing.tool_change = True
        lines = self.lines(["G0 Z18"])
        self.assertFalse(any(line.startswith("G43") for line in lines), lines)

    def test_dwell_is_l_seconds(self):
        lines = self.op_lines([Path.Command("G4 P1.5"), Path.Command("G4 P2")])
        self.assertEqual(["L1.5", "L2"], lines)

    def test_spindle_wait_is_l_seconds(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.toolheads[0].toolhead_wait = 0.5
        lines = self.lines(["G0 Z18"])
        self.assertEqual("L0.5", lines[lines.index("M3S1000") + 1])

    # ------------------------------------------------------------------
    # sequence numbers
    # ------------------------------------------------------------------

    def test_sequence_numbers_on_every_event(self):
        """Every line but the program name and the E marker is numbered, text
        events and the preamble included, N zero-padded to four digits."""
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 1
        self.post._machine.output.formatting.line_increment = 1
        self.post._machine.output.formatting.line_number_prefix = "N"
        self.post._machine.output.comments.enabled = True
        lines = self.lines([Path.Command("(Profile)"), "G0 X10 Y20 Z30", Path.Command("G4 P1")])
        self.assertEqual("(TEST)", lines[0])
        self.assertEqual("E", lines[-1])
        self.assertEqual("N0001G17", lines[1])
        body = lines[1:-1]
        for index, line in enumerate(body, start=1):
            self.assertTrue(line.startswith(f"N{index:04d}"), line)
        tails = [line[5:] for line in body]
        self.assertIn("E01", tails)
        self.assertIn("(T)PROFILE$", tails)
        self.assertIn("G0X10.000Y20.000Z30.000", tails)
        self.assertIn("L1", tails)
        self.assertEqual("M30", tails[-1])

        self.post._machine.output.formatting.line_number_start = 10
        self.post._machine.output.formatting.line_increment = 10
        lines = self.lines([])
        self.assertEqual(["N0010G17", "N0020G90"], lines[1:3])

    def test_sequence_numbers_on_block_lines(self):
        """A text block is numbered line by line, a block-delete slash stays in
        front of the N, and a text event in a block is numbered like any event."""
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 1
        self.post._machine.output.formatting.line_increment = 1
        self.post._machine.processing.tool_change = True
        self.post._machine.postprocessor_properties["pre_tool_change"] = (
            "M05\n/G0Z50\n(T)CHANGE TOOL$"
        )
        lines = self.lines(["G0 Z18"])
        self.assertEqual("E", lines[-1])
        for number, line in enumerate(lines[1:-1], start=1):
            self.assertTrue(line.lstrip("/").startswith(f"N{number:04d}"), line)
        deleted = next(i for i, line in enumerate(lines) if line.endswith("G0Z50"))
        self.assertEqual(
            [
                f"N{deleted - 1:04d}M05",
                f"/N{deleted:04d}G0Z50",
                f"N{deleted + 1:04d}(T)CHANGE TOOL$",
            ],
            lines[deleted - 1 : deleted + 2],
        )

    def test_sequence_number_sanity(self):
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 1
        self.post._machine.output.formatting.line_increment = 10
        self.profile_op.Path = Path.Path([Path.Command("G1 X%d" % i) for i in range(1001)])
        self.post.apply_configuration_bundle()
        squawks = self.post.get_sanity_checks(self.job)
        self.assertTrue(any("N9999" in s["Note"] for s in squawks), squawks)

        self.post._machine.output.formatting.line_increment = 1
        self.post.apply_configuration_bundle()
        squawks = self.post.get_sanity_checks(self.job)
        self.assertFalse(any("N9999" in s["Note"] for s in squawks), squawks)
