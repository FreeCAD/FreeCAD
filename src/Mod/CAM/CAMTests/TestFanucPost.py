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
Tests for the machine-based Fanuc post processor (fanuc_post.py).

The first class drives the post through export2() on a mock job and pins
the Fanuc-specific output: tape marks, program number, rigid tapping,
canned cycles, dwell units, tool length offset forms and the empty-spindle
park. The second builds a real Job with a work plane and a rotary machine
and pins the tilted-work-plane program under both rotation strategies.
"""

import re
import unittest

import FreeCAD
import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory, VALID_SCOPES, property_scope
from Path.Post.CAMErrors import CAMValueError
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits

try:
    from Path.Post.TiltedWorkPlane import PlaneCommand
    from Machine.models.machine import RotationStrategy
except ImportError:
    PlaneCommand = RotationStrategy = None

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


class TestFanucPost(PathTestUtils.PathTestBase):
    """Fanuc output on a three-axis mock job."""

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = "fanuc"
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "fanuc")

    def setUp(self):
        self.maxDiff = None

        machine = Machine.create_3axis_config()
        machine.name = "Test Fanuc"
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

    def test_fanuc_defaults(self):
        schema = {p["name"]: p for p in self.post.get_common_property_schema()}
        self.assertEqual("", schema["drill_cycles_to_translate"]["default"])
        self.assertTrue(schema["supports_tool_radius_compensation"]["default"])
        self.assertEqual(0, schema["spindle_decimals"]["default"])
        self.assertNotIn("K", schema["parameter_order"]["default"])
        self.assertEqual("G17 G54 G40 G49 G80 G90 G94", schema["preamble"]["default"])
        self.assertTrue(schema["postamble"]["default"].endswith("M30"))

    @NEEDS_WORK_PLANES
    def test_plane_command_and_strategies(self):
        """Fanuc writes the G68.2 family and can serve both DWO and TWP machines."""
        self.assertEqual(PlaneCommand.G68_2, self.post.PLANE_COMMAND)
        self.assertEqual(("dwo", "twp"), self.post.ROTATION_STRATEGIES)

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

    def test_preamble_units_and_postamble(self):
        lines = self.lines([])
        preamble = lines.index("G17 G54 G40 G49 G80 G90 G94")
        self.assertEqual("G21", lines[preamble + 1])
        self.assertEqual(["M05", "G17 G54 G90 G80 G40", "M30", "%"], lines[-4:])

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
    # motion
    # ------------------------------------------------------------------

    def test_moves_pass_through(self):
        lines = self.lines(["G0 X1 Y2 Z3", "G1 X10 F10", "G2 X20 Y10 I10 J0 K0"])
        self.assert_line("G0 X1.000 Y2.000 Z3.000", lines)
        self.assert_line("G1 X10.000 F600.000", lines)
        self.assert_line("G2 X20.000 Y10.000 I10.000 J0.000", lines)

    def test_arcs_carry_no_k(self):
        """A G17 arc has no K on a Fanuc; a helix carries its pitch in Z."""
        lines = self.lines(["G0 X0 Y0 Z0", "G2 X10 Y10 Z-1 I10 J0 K-0.5 F10"])
        arc = next(l for l in lines if l.startswith("G2 "))
        self.assertNotIn("K", arc)
        self.assertIn("Z-1.000", arc)

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
    # fixtures, coolant, program control
    # ------------------------------------------------------------------

    def test_fixtures_and_m_codes(self):
        lines = self.lines(["G55", "M8", "M9", "M0", "M1"])
        for line in ("G55", "M8", "M9", "M0", "M1"):
            self.assert_line(line, lines)

    @NEEDS_WORK_PLANES
    def test_extended_work_offset_from_a_plane_is_accepted(self):
        """A work plane may name G54.1 P3; the fixture word survives the post."""
        plane = type("Plane", (), {"Label": "Side", "Fixture": "G54.1 P3", "Placement": None})()
        self.profile_op.Workplane = plane
        try:
            lines = self.lines(["G0 X1"])
        finally:
            del self.profile_op.Workplane
        self.assert_line("G54.1 P3", lines)

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

    # ------------------------------------------------------------------
    # whole pipeline
    # ------------------------------------------------------------------

    def test_full_export_no_crash(self):
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.output_header = True
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.processing.tool_change = True
        self.post._machine.processing.filter_inefficient_moves = True
        self.post._machine.postprocessor_properties["program_number"] = 7
        self.post._machine.postprocessor_properties["uppercase_output"] = True
        self.post._machine.postprocessor_properties["end_spindle_empty"] = True

        commands = [
            Path.Command(c)
            for c in (
                "G17 G21 G90 G94 G54 "
                "G0X0Y0Z10 G1X10Y0Z-1F10 G2X20Y10I0J10 G3X10Y20I-10J0 G2X0Y10I0J-10Z-2 G4P1 "
                "G41D1 G1X5 G40 "
                "G98 G81X1Y1Z-5R2F5 G82X2Y2Z-5R2F5P1 G83X3Y3Z-5R2F5Q1 G73X4Y4Z-5R2F5Q1 "
                "G99 G85X6Y6Z-5R2F5 G80 "
                "M0 M1 M3S1000 M4S1500 M5 M7 M8 M9 M6T2 G43H2 T3 M2 M30 (comment)"
            ).split(" ")
        ] + [_tap(), _tap("G74")]
        gcode = self.export(commands)
        lines = gcode.split(EOL)
        self.assertEqual("%", lines[0])
        self.assertEqual("%", lines[-1])
        self.assertTrue(any("M29 S500" in l for l in lines), gcode)


def _machine_ac(strategy):
    """A table-table AC machine, C carrying A, as TestPathTiltedWorkPlane builds it."""
    from Machine.models.machine import RotaryAxis, AxisRole

    machine = Machine.create_3axis_config()
    machine.name = "Test Fanuc AC"
    machine.rotary_axes["C"] = RotaryAxis(
        name="C",
        rotation_vector=FreeCAD.Vector(0, 0, 1),
        role=AxisRole.TABLE_ROTARY,
        sequence=0,
    )
    machine.rotary_axes["A"] = RotaryAxis(
        name="A",
        rotation_vector=FreeCAD.Vector(1, 0, 0),
        min_limit=-120,
        max_limit=120,
        role=AxisRole.TABLE_ROTARY,
        parent="C",
        sequence=1,
    )
    machine.kinematics.rotation_strategy = strategy
    machine.output.output_header = False
    machine.output.comments.enabled = False
    machine.output.precision.axis = 3
    machine.output.precision.feed = 3
    machine.output.precision.spindle = 0
    machine.processing.tool_change = False
    machine.postprocessor_properties["uppercase_output"] = False
    return machine


PLANE_PATH = [
    Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}),
    Path.Command("G1", {"X": 10, "Y": 0, "Z": -2, "F": 10}),
    Path.Command("G2", {"X": 20, "Y": 10, "Z": -2, "I": 10, "J": 0}),
]

DECLARE = "G68.2 X30.000 Y10.000 Z5.000 I0.000 J45.000 K0.000"


@NEEDS_WORK_PLANES
class TestFanucTiltedWorkPlane(PathTestUtils.PathTestBase):
    """The Fanuc post on a Job with a tilted work plane and a rotary machine."""

    def setUp(self):
        import Path.Main.Job as PathJob

        self.maxDiff = None
        self.doc = FreeCAD.newDocument("TestFanucTiltedWorkPlane")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 100, 50
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _plane(self, degrees=45, origin=FreeCAD.Vector(30, 10, 5)):
        import Path.Main.Workplane as PathWorkplane

        axis = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), degrees).multVec(FreeCAD.Vector(0, 0, 1))
        return PathWorkplane.createWorkplaneFromToolAxis(self.job, axis, origin=origin)

    def _op(self, label, plane=None, path=None):
        import Path.Op.Custom as PathCustom

        op = PathCustom.Create(label, parentJob=self.job)
        op.Workplane = plane
        self.doc.recompute()
        op.Path = Path.Path(list(path or PLANE_PATH))
        return op

    def _export(self, machine, **properties):
        machine.postprocessor_properties.update(properties)
        self.job.Proxy.getMachine = lambda: machine
        post = PostProcessorFactory.get_post_processor(self.job, "fanuc")
        post.reinitialize()
        post._machine = machine
        post.apply_configuration_bundle()
        gcode = "\n".join(g for _, g in post.export2())
        return post, [line.strip() for line in gcode.splitlines()]

    def test_twp_declares_the_plane_and_leaves_the_path_in_plane_coordinates(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_ac(RotationStrategy.TWP), pre_rotary_move="G53 G0 Z0")
        declare = lines.index(DECLARE)
        self.assertEqual(["G53 G0 Z0", DECLARE, "G53.1"], lines[declare - 1 : declare + 2])
        self.assertIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines)
        self.assertIn("G2 X20.000 Y10.000 Z-2.000 I10.000 J0.000", lines)
        self.assertLess(declare, lines.index("G69"))
        self.assertFalse(any(re.match(r"^G0 .*[AC]-?\d", l) for l in lines), lines)

    def test_twp_cancels_the_plane_before_the_postamble_and_tape_end(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_ac(RotationStrategy.TWP))
        self.assertLess(lines.index("G69"), lines.index("M30"))
        self.assertEqual("%", lines[-1])

    def test_twp_program_positions_the_rotaries_when_the_control_does_not(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(
            _machine_ac(RotationStrategy.TWP),
            twp_control_positions_rotaries=False,
            post_rotary_move="G54.2 P0",
        )
        rotary = next(l for l in lines if re.match(r"^G0 .*A", l))
        self.assertLess(lines.index(rotary), lines.index(DECLARE))
        self.assertNotIn("G53.1", lines)
        self.assertIn("G54.2 P0", lines)

    def test_twp_cancels_before_a_tool_change_and_redeclares_after(self):
        """Two tools on one tilted face: G69 before the second M6, G68.2 again after it."""
        import Path.Tool.Controller as PathToolController

        machine = _machine_ac(RotationStrategy.TWP)
        machine.processing.tool_change = True
        plane = self._plane()
        first = self._op("First", plane)
        second = self._op("Second", plane)
        tc2 = PathToolController.Create("TC2", tool=first.ToolController.Tool, toolNumber=2)
        self.job.Proxy.addToolController(tc2)
        second.ToolController = tc2
        self.doc.recompute()
        _, lines = self._export(machine)
        m6 = lines.index("M6 T2")
        declares = [i for i, l in enumerate(lines) if l == DECLARE]
        cancels = [i for i, l in enumerate(lines) if l == "G69"]
        self.assertEqual(2, len(declares), lines)
        self.assertEqual(2, len(cancels), lines)
        self.assertLess(declares[0], cancels[0])
        self.assertLess(cancels[0], m6)
        self.assertLess(m6, declares[1])
        self.assertLess(declares[1], cancels[1])

    def test_twp_rigid_tap_inside_the_plane(self):
        """A tapped hole on the tilted face: M29 and G84 in plane coordinates after G68.2."""
        self._op(
            "Tapped", self._plane(), path=[Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}), _tap()]
        )
        _, lines = self._export(_machine_ac(RotationStrategy.TWP))
        m29 = lines.index("M29 S500")
        self.assertLess(lines.index(DECLARE), m29)
        self.assertEqual("G84 X5.000 Y5.000 Z-10.000 F625.000 R2.000", lines[m29 + 1])
        self.assertLess(m29, lines.index("G69"))

    def test_dwo_commands_the_rotaries_and_rotates_the_path(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(
            _machine_ac(RotationStrategy.DWO),
            pre_rotary_move="G53 G0 Z0",
            post_rotary_move="G54.2 P1",
        )
        rotary = next(l for l in lines if re.match(r"^G0 .*A", l))
        index = lines.index(rotary)
        self.assertEqual("G53 G0 Z0", lines[index - 1])
        self.assertEqual("G54.2 P1", lines[index + 1])
        self.assertNotIn(DECLARE, lines)
        self.assertNotIn("G69", lines)
        self.assertNotIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines, "the path is rotated")

    def test_a_machine_with_no_strategy_refuses(self):
        self._op("Tilted", self._plane())
        with self.assertRaisesRegex(CAMValueError, "Rotation strategy"):
            self._export(_machine_ac(RotationStrategy.NONE))

    def test_a_three_axis_machine_refuses_a_tilted_plane(self):
        self._op("Tilted", self._plane())
        machine = Machine.create_3axis_config()
        machine.output.output_header = False
        machine.output.comments.enabled = False
        machine.processing.tool_change = False
        with self.assertRaisesRegex(CAMValueError, "rotary axes"):
            self._export(machine)

    def test_a_plain_operation_on_a_rotary_machine_starts_at_the_home_pose(self):
        self._op("Plain")
        _, lines = self._export(_machine_ac(RotationStrategy.TWP), pre_rotary_move="G53 G0 Z0")
        rotary = next(l for l in lines if re.match(r"^G0 .*A", l))
        self.assertIn("A0.000", rotary)
        self.assertNotIn(DECLARE, lines)
        self.assertIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines)

    def test_sanity_warns_when_the_pre_rotary_block_is_empty(self):
        """The rotaries move between a plain operation and a tilted one."""
        self._op("Plain")
        self._op("Tilted", self._plane())
        post, _ = self._export(_machine_ac(RotationStrategy.TWP))
        squawks = post.get_sanity_checks(self.job)
        self.assertTrue(any("Pre-Rotary Move" in s["Note"] for s in squawks), squawks)

    def test_sanity_is_quiet_with_a_pre_rotary_block(self):
        self._op("Plain")
        self._op("Tilted", self._plane())
        post, _ = self._export(_machine_ac(RotationStrategy.TWP), pre_rotary_move="G53 G0 Z0")
        squawks = post.get_sanity_checks(self.job)
        self.assertFalse(any("Pre-Rotary Move" in s["Note"] for s in squawks), squawks)
