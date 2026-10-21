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
Tests for the output shared by the industrial machine posts
(generic_multi_axis_industrial_post.py).

The generic post is not a standalone post, so these tests drive it through a
concrete child, the Fanuc post. They pin what the children inherit: tape marks,
program number, upper-case output, dwell units, native canned cycles, rigid
tapping, the parametric tool length offset, the empty-spindle park and the
rigid-tapping sanity check. Where a line belongs to the child (the M29 mode
word, the postamble) it is the Fanuc one.

IndustrialPostTestBase carries the mock job and the helpers and has no tests of
its own, so a child post's test module can build on it without collecting these
tests a second time.
"""

import re
import unittest

import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory, VALID_SCOPES, property_scope
from Path.Post.CAMErrors import CAMValueError
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits

try:
    from Path.Post.TiltedWorkPlane import PlaneCommand
except ImportError:
    PlaneCommand = None

NEEDS_WORK_PLANES = unittest.skipIf(
    PlaneCommand is None, "tilted work planes need the post-processor's plane command"
)

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

EOL = "\n"


def _tap(name="G84", **overrides):
    """A tapping cycle as the tapping generator writes it: F is the pitch, S the speed."""
    params = {"X": 5.0, "Y": 5.0, "Z": -10.0, "R": 2.0, "F": 1.25, "S": 500.0}
    params.update(overrides)
    command = Path.Command(name, params)
    command.addAnnotations({"operation": "tapping", "rigid": "False"})
    return command


class IndustrialPostTestBase(PathTestUtils.PathTestBase):
    """A mock job and the export helpers for an industrial post.

    A subclass names the post to drive and the machine it builds. This class has
    no tests of its own.
    """

    POST_NAME = None
    MACHINE_NAME = None

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = cls.POST_NAME
        cls.post = PostProcessorFactory.get_post_processor(cls.job, cls.POST_NAME)
        if not hasattr(cls.post, "apply_configuration_bundle"):
            # The factory returns a CAMError, not a post, when it cannot load the file.
            raise AssertionError(f"The '{cls.POST_NAME}' post did not load: {cls.post}")

    def setUp(self):
        self.maxDiff = None

        machine = Machine.create_3axis_config()
        machine.name = self.MACHINE_NAME
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
                max_rpm=12000,
                max_power_kw=10.0,
            )
        ]
        # Upper-casing is tested on its own; everything else reads easier as written.
        machine.postprocessor_properties["uppercase_output"] = False
        self.profile_op.Path = Path.Path([])
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

    def assert_line(self, expected, lines):
        self.assertIn(expected, lines, f"expected line {expected!r} in\n{EOL.join(lines)}")

    def assert_no_line(self, unexpected, lines):
        self.assertNotIn(unexpected, lines, f"unexpected line {unexpected!r} in\n{EOL.join(lines)}")


class TestGenericMultiAxisIndustrial(IndustrialPostTestBase):
    """Output shared by the industrial posts, on a three-axis mock job."""

    POST_NAME = "fanuc"
    MACHINE_NAME = "Test Fanuc"

    # ------------------------------------------------------------------
    # schema
    # ------------------------------------------------------------------

    def test_default_file_extension(self):
        self.assertEqual("nc", self.post.get_file_extension())

    def test_property_schema_is_well_formed(self):
        """Every property declares a valid scope and reaches the values dict."""
        for prop in self.post.get_full_property_schema():
            self.assertIn(property_scope(prop), VALID_SCOPES, prop["name"])
            if prop["type"] == "choice":
                self.assertIn(prop["default"], prop["choices"], prop["name"])
            self.assertIn(prop["name"].upper(), self.post.values, prop["name"])

    def test_common_defaults(self):
        schema = {p["name"]: p for p in self.post.get_common_property_schema()}
        self.assertEqual("", schema["drill_cycles_to_translate"]["default"])
        self.assertTrue(schema["supports_tool_radius_compensation"]["default"])
        self.assertEqual(0, schema["spindle_decimals"]["default"])

    # ------------------------------------------------------------------
    # program frame
    # ------------------------------------------------------------------

    def test_tape_marks_wrap_the_program(self):
        lines = self.lines(["G0 X1 Y2 Z3"])
        self.assertEqual("%", lines[0])
        self.assertEqual("%", lines[-1])
        self.assertEqual(2, lines.count("%"))

    def test_tape_marks_precede_the_header(self):
        self.post._machine.output.output_header = True
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.header.include_date = False
        lines = self.lines([])
        self.assertEqual("%", lines[0])
        self.assertTrue(lines[1].startswith("(Machine: Test Fanuc"), lines[1])

    def test_tape_marks_disabled(self):
        self.post._machine.postprocessor_properties["wrap_in_percent"] = False
        lines = self.lines(["G0 X1 Y2 Z3"])
        self.assert_no_line("%", lines)

    def test_program_number(self):
        self.post._machine.postprocessor_properties["program_number"] = 12
        lines = self.lines([])
        self.assertEqual("%", lines[0])
        self.assertEqual("O0012", lines[1])

    def test_program_number_carries_the_job_label_as_a_comment(self):
        self.post._machine.output.comments.enabled = True
        self.post._machine.postprocessor_properties["program_number"] = 1234
        lines = self.lines([])
        self.assertEqual("O1234 (MockJob)", lines[1])

    def test_program_number_zero_writes_no_o_line(self):
        lines = self.lines([])
        self.assertFalse(any(re.match(r"^O\d", l) for l in lines), lines)

    def test_upper_case_output(self):
        self.post._machine.postprocessor_properties["uppercase_output"] = True
        self.post._machine.output.comments.enabled = True
        lines = self.lines(["(a comment)", "G0 X1"])
        self.assert_line("(A COMMENT)", lines)
        self.assertFalse(any(re.search(r"[a-z]", l) for l in lines), lines)

    def test_line_numbers(self):
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 10
        self.post._machine.output.formatting.line_increment = 10
        lines = self.lines(["G0 X1 Y2 Z3"])
        self.assertEqual("%", lines[0], "the tape mark is never numbered")
        self.assertTrue(any(re.match(r"^N\d+ G0 X1\.000", l) for l in lines), lines)

    # ------------------------------------------------------------------
    # formatting
    # ------------------------------------------------------------------

    def test_spindle_speed_is_an_integer(self):
        self.post._machine.processing.tool_change = True
        lines = self.lines([])
        self.assert_line("M3 S1000", lines)

    def test_comment_symbol_is_parenthesis(self):
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.comments.symbol = ";"
        lines = self.lines(["(hello)"])
        self.assert_line("(hello)", lines)

    def test_imperial_output(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.lines(["G0 X25.4 Y50.8 Z2.54", "G1 X0 F25.4"])
        self.assert_line("G20", lines)
        self.assert_line("G0 X1.000 Y2.000 Z0.100", lines)
        self.assert_line("G1 X0.000 F60.000", lines)

    # ------------------------------------------------------------------
    # fixtures, coolant, program control
    # ------------------------------------------------------------------

    def test_fixtures_and_m_codes(self):
        lines = self.lines(["G55", "M8", "M9", "M0", "M1"])
        for line in ("G55", "M8", "M9", "M0", "M1"):
            self.assert_line(line, lines)

    # ------------------------------------------------------------------
    # dwell
    # ------------------------------------------------------------------

    def test_dwell_in_milliseconds(self):
        lines = self.lines(["G4 P1.5"])
        self.assert_line("G4 P1500", lines)

    def test_dwell_in_seconds(self):
        self.post._machine.postprocessor_properties["dwell_in_milliseconds"] = False
        lines = self.lines(["G4 P1.5"])
        self.assert_line("G4 P1.500", lines)

    def test_spindle_wait_dwell_is_in_milliseconds(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.toolheads[0].spindle_wait = 2.0
        lines = self.lines([])
        index = lines.index("M3 S1000")
        self.assertEqual("G4 P2000", lines[index + 1])

    # ------------------------------------------------------------------
    # canned cycles
    # ------------------------------------------------------------------

    def test_canned_cycles_are_native(self):
        lines = self.lines(["G0 X0 Y0 Z10", "G98", "G81 X5 Y5 Z-10 R2 F5"])
        self.assert_line("G98", lines)
        self.assert_line("G81 X5.000 Y5.000 Z-10.000 F300.000 R2.000", lines)
        self.assert_line("G80", lines)

    def test_every_hole_carries_the_full_cycle(self):
        """Modal output on, yet every block keeps its cycle word, Z, R, Q and F."""
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        lines = self.lines(
            [
                "G0 X0 Y0 Z10",
                "G98",
                "G83 X5 Y5 Z-10 R2 Q3 F5",
                "G83 X15 Y5 Z-10 R2 Q3 F5",
                "G83 X25 Y5 Z-10 R2 Q3 F5",
            ]
        )
        holes = [l for l in lines if l.startswith("G83")]
        self.assertEqual(3, len(holes), lines)
        for hole in holes:
            for word in ("Z-10.000", "R2.000", "Q3.000", "F300.000"):
                self.assertIn(word, hole, hole)
        self.assertEqual(1, lines.count("G80"), lines)

    def test_dwell_cycle_p_is_in_milliseconds(self):
        lines = self.lines(["G0 X0 Y0 Z10", "G82 X5 Y5 Z-10 R2 P0.5 F5"])
        cycle = next(l for l in lines if l.startswith("G82"))
        self.assertIn("P500", cycle)
        self.assertNotIn("P0.5", cycle)

    def test_peck_depth_is_a_length_not_a_dwell(self):
        lines = self.lines(["G0 X0 Y0 Z10", "G73 X5 Y5 Z-10 R2 Q1.5 F5"])
        cycle = next(l for l in lines if l.startswith("G73"))
        self.assertIn("Q1.500", cycle)

    def test_translated_cycles_when_the_machine_asks(self):
        self.post._machine.processing.translate_drill_cycles = True
        lines = self.lines(["G0 X0 Y0 Z10 F20", "G81 X5 Y5 Z-10 R2 F5"])
        self.assertFalse(any(l.startswith("G81") for l in lines), lines)
        self.assert_line("G1 X5.000 Y5.000 Z-10.000 F300.000", lines)

    # ------------------------------------------------------------------
    # tapping
    # ------------------------------------------------------------------

    def test_rigid_tap_writes_m29_and_pitch_times_speed(self):
        """M29 S<rpm> immediately before the G84, whose F is pitch times rpm."""
        lines = self.lines(["G0 X0 Y0 Z10", "G98", _tap()])
        index = lines.index("M29 S500")
        self.assertEqual("G84 X5.000 Y5.000 Z-10.000 F625.000 R2.000", lines[index + 1])
        self.assert_line("G80", lines)

    def test_left_hand_tap(self):
        lines = self.lines(["G0 X0 Y0 Z10", _tap("G74")])
        index = lines.index("M29 S500")
        self.assertTrue(lines[index + 1].startswith("G74 "), lines[index + 1])

    def test_rigid_tap_is_line_numbered(self):
        self.post._machine.output.formatting.line_numbers = True
        lines = self.lines(["G0 X0 Y0 Z10", _tap()])
        m29 = next(l for l in lines if "M29" in l)
        self.assertRegex(m29, r"^N\d+ M29 S500$")
        g84 = lines[lines.index(m29) + 1]
        self.assertRegex(g84, r"^N\d+ G84 ")

    def test_rigid_tap_repeats_s_after_the_spindle_command(self):
        """The S on M29 is never dropped as a duplicate of the M3's."""
        self.post._machine.output.duplicates.parameters = False
        lines = self.lines(["M3 S500", "G0 X0 Y0 Z10", _tap()])
        self.assert_line("M3 S500", lines)
        self.assert_line("M29 S500", lines)

    def test_rigid_tap_speed_from_the_last_spindle_command(self):
        """A raw G84 from a Custom op has no S; the last M3 supplies it."""
        lines = self.lines(["M3 S800", "G0 X0 Y0 Z10", "G84 X5 Y5 Z-10 R2 F10"])
        self.assert_line("M29 S800", lines)

    def test_rigid_tap_without_a_spindle_speed_is_refused(self):
        """No S on the cycle and none commanded before it: nothing to put on M29."""
        tc_path = self.tool_controller.Path
        self.tool_controller.Path = Path.Path([Path.Command("M6 T1")])
        try:
            with self.assertRaisesRegex(CAMValueError, "needs a spindle speed"):
                self.export(["G0 X0 Y0 Z10", "G84 X5 Y5 Z-10 R2 F10"])
        finally:
            self.tool_controller.Path = tc_path

    def test_floating_tap(self):
        """Rigid tapping off: no M29, the feed is still pitch times speed."""
        self.post._machine.postprocessor_properties["rigid_tapping"] = False
        lines = self.lines(["G0 X0 Y0 Z10", _tap()])
        self.assertFalse(any("M29" in l for l in lines), lines)
        self.assert_line("G84 X5.000 Y5.000 Z-10.000 F625.000 S500 R2.000", lines)

    def test_tap_dwell_is_in_milliseconds(self):
        lines = self.lines(["G0 X0 Y0 Z10", _tap(P=0.25)])
        cycle = next(l for l in lines if l.startswith("G84"))
        self.assertIn("P250", cycle)

    def test_tap_in_imperial(self):
        """Pitch and feed convert to inches; the speed does not."""
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.lines(["G0 X0 Y0 Z254", _tap(X=25.4, Y=25.4, Z=-25.4, R=25.4, F=2.54)])
        self.assert_line("M29 S500", lines)
        self.assert_line("G84 X1.000 Y1.000 Z-1.000 F50.000 R1.000", lines)

    # ------------------------------------------------------------------
    # tool change and tool length offset
    # ------------------------------------------------------------------

    def test_tool_change_sequence(self):
        self.post._machine.processing.tool_change = True
        lines = self.lines([])
        start = lines.index("M05")
        self.assertEqual(
            ["M05", "G28 G91 Z0", "G90", "M6 T1", "G43 H1", "M3 S1000"],
            lines[start : start + 6],
        )

    def test_tool_length_offset_macro_form(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.postprocessor_properties["tool_length_offset_macro"] = True
        lines = self.lines([])
        index = lines.index("M6 T1")
        self.assertEqual("G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120", lines[index + 1])
        self.assertEqual("G90", lines[index + 2])
        self.assert_no_line("G43 H1", lines)

    def test_tool_length_offset_suppressed(self):
        self.post._machine.processing.tool_change = True
        self.post._machine.output.output_tool_length_offset = False
        lines = self.lines([])
        self.assert_no_line("G43 H1", lines)

    def test_end_spindle_empty(self):
        self.post._machine.postprocessor_properties["end_spindle_empty"] = True
        lines = self.lines(["G0 X1"])
        self.assertEqual(
            ["M05", "G28 G91 Z0", "G90", "M6 T0", "M05", "G17 G54 G90 G80 G40", "M30", "%"],
            lines[-8:],
        )

    def test_end_spindle_empty_custom_retract(self):
        self.post._machine.postprocessor_properties["end_spindle_empty"] = True
        self.post._machine.postprocessor_properties["end_spindle_empty_retract"] = "G53 G0 Z0"
        lines = self.lines([])
        index = lines.index("M6 T0")
        self.assertEqual(["M05", "G53 G0 Z0", "M6 T0"], lines[index - 2 : index + 1])

    def test_end_spindle_empty_off_by_default(self):
        lines = self.lines([])
        self.assert_no_line("M6 T0", lines)

    # ------------------------------------------------------------------
    # sanity
    # ------------------------------------------------------------------

    def test_sanity_warns_on_a_tap_without_speed(self):
        self.profile_op.Path = Path.Path(
            [Path.Command("G0 X0 Y0 Z10"), Path.Command("G84 X5 Y5 Z-10 R2 F10")]
        )
        self.tool_controller.SpindleSpeed = 0
        try:
            squawks = self.post.get_sanity_checks(self.job)
        finally:
            self.tool_controller.SpindleSpeed = 1000
        notes = [s["Note"] for s in squawks if s["squawkType"] == "WARNING"]
        self.assertTrue(any("rigid tapping" in n for n in notes), squawks)

    def test_sanity_is_quiet_when_the_tap_has_a_speed(self):
        self.profile_op.Path = Path.Path([Path.Command("G0 X0 Y0 Z10"), _tap()])
        squawks = self.post.get_sanity_checks(self.job)
        self.assertFalse(any("rigid tapping" in s["Note"] for s in squawks), squawks)
