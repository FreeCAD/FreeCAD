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
Tests for the machine-based Haas post processor (haas_post.py).

The first class drives the post through export2() on a mock job and pins the
Haas-specific output: the five-digit O number, the G53 retracts, rigid tapping
without a mode word, G154 extended work offsets and the G268 plane command.
What the Haas post shares with the other industrial posts is tested in
TestGenericMultiAxisIndustrial.py. The second builds a real Job with a work
plane and a UMC-style B/C machine and pins the indexed program under both
rotation strategies: G254 / G255 for DWO, G268 / G253 / G269 for TWP.
"""

import re
import unittest

import FreeCAD
import Path
from CAMTests import PathTestUtils
from CAMTests.TestGenericMultiAxisIndustrial import EOL, IndustrialPostTestBase, _tap
from Path.Post.Processor import PostProcessorFactory, WrapperPost
from Path.Post.CAMErrors import CAMError, CAMValueError
from Machine.models.machine import Machine

try:
    from Path.Post.TiltedWorkPlane import PlaneCommand
    from Machine.models.machine import RotationStrategy
except ImportError:
    PlaneCommand = RotationStrategy = None

NEEDS_WORK_PLANES = unittest.skipIf(
    PlaneCommand is None, "tilted work planes need the post-processor's plane command"
)

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestHaasPost(IndustrialPostTestBase):
    """Haas output on a three-axis mock job."""

    POST_NAME = "haas"
    MACHINE_NAME = "Test Haas"

    # ------------------------------------------------------------------
    # schema
    # ------------------------------------------------------------------

    def test_is_a_machine_based_post(self):
        self.assertEqual("machine", Path.Preferences.classifyPostProcessor("haas"))
        self.assertNotIsInstance(self.post, WrapperPost)
        self.assertEqual("HaasPost", type(self.post).__name__)

    def test_haas_defaults(self):
        schema = {p["name"]: p for p in self.post.get_common_property_schema()}
        self.assertEqual("G17 G40 G49 G80 G90 G94", schema["preamble"]["default"])
        self.assertEqual("M05\nG53 G0 Z0\nM30", schema["postamble"]["default"])
        self.assertEqual("", schema["safetyblock"]["default"])
        self.assertEqual("M05\nG53 G0 Z0", schema["pre_tool_change"]["default"])
        self.assertEqual("G53 G0 Z0", schema["pre_rotary_move"]["default"])
        own = {p["name"]: p for p in self.post.get_property_schema()}
        self.assertEqual(99999, own["program_number"]["max"])
        self.assertEqual("G53 G0 Z0", own["end_spindle_empty_retract"]["default"])
        if PlaneCommand is not None:
            self.assertEqual(PlaneCommand.G268, self.post.PLANE_COMMAND)
        self.assertEqual(("dwo", "twp"), self.post.ROTATION_STRATEGIES)

    def test_no_g28_in_any_default(self):
        """A Haas ignores a dynamic work offset for G28; the defaults retract with G53."""
        for prop in self.post.get_full_property_schema():
            if isinstance(prop.get("default"), str) and prop["name"] != "supported_commands":
                self.assertNotIn("G28", prop["default"], prop["name"])

    # ------------------------------------------------------------------
    # program frame
    # ------------------------------------------------------------------

    def test_program_number_has_five_digits(self):
        self.post._machine.postprocessor_properties["program_number"] = 12
        lines = self.lines([])
        self.assertEqual("%", lines[0])
        self.assertEqual("O00012", lines[1])

    def test_preamble_units_and_postamble(self):
        lines = self.lines([])
        preamble = lines.index("G17 G40 G49 G80 G90 G94")
        self.assertEqual("G21", lines[preamble + 1])
        self.assertEqual(["M05", "G53 G0 Z0", "M30", "%"], lines[-4:])

    def test_tool_change_sequence(self):
        self.post._machine.processing.tool_change = True
        lines = self.lines([])
        start = lines.index("M05")
        self.assertEqual(
            ["M05", "G53 G0 Z0", "M6 T1", "G43 H1", "M3 S1000"], lines[start : start + 5]
        )

    def test_end_spindle_empty_retracts_with_g53(self):
        self.post._machine.postprocessor_properties["end_spindle_empty"] = True
        lines = self.lines(["G0 X1"])
        index = lines.index("M6 T0")
        self.assertEqual(["M05", "G53 G0 Z0", "M6 T0"], lines[index - 2 : index + 1])

    # ------------------------------------------------------------------
    # dwell
    # ------------------------------------------------------------------

    def test_dwell_is_an_integer_number_of_milliseconds(self):
        """No decimal point: a Haas reads P1500 as milliseconds, P1.5 as seconds."""
        lines = self.lines(["G4 P1.5", "G0 X0 Y0 Z10", "G82 X5 Y5 Z-10 R2 P0.5 F5"])
        self.assert_line("G4 P1500", lines)
        cycle = next(l for l in lines if l.startswith("G82"))
        self.assertRegex(cycle, r"P500(\s|$)")

    # ------------------------------------------------------------------
    # tapping
    # ------------------------------------------------------------------

    def test_rigid_tap_writes_s_alone_and_never_m29(self):
        """M29 is an output relay on a Haas; the S block goes ahead of the tap."""
        lines = self.lines(["G0 X0 Y0 Z10", "G98", _tap()])
        index = lines.index("S500")
        self.assertEqual("G84 X5.000 Y5.000 Z-10.000 F625.000 R2.000", lines[index + 1])
        self.assert_line("G80", lines)
        self.assertFalse(any("M29" in l for l in lines), lines)

    def test_left_hand_tap(self):
        lines = self.lines(["G0 X0 Y0 Z10", _tap("G74")])
        index = lines.index("S500")
        self.assertTrue(lines[index + 1].startswith("G74 "), lines[index + 1])

    def test_floating_tap_keeps_the_speed_on_the_cycle(self):
        self.post._machine.postprocessor_properties["rigid_tapping"] = False
        lines = self.lines(["G0 X0 Y0 Z10", _tap()])
        self.assert_no_line("S500", lines)
        self.assert_line("G84 X5.000 Y5.000 Z-10.000 F625.000 S500 R2.000", lines)

    # ------------------------------------------------------------------
    # work offsets
    # ------------------------------------------------------------------

    def test_standard_work_offsets(self):
        lines = self.lines(["G55", "G59"])
        self.assert_line("G55", lines)
        self.assert_line("G59", lines)

    def test_extended_work_offset_is_g154(self):
        lines = self.lines([Path.Command("G54.1", {"P": 3}), "G0 X1"])
        self.assert_line("G154 P3", lines)
        self.assertFalse(any("G54.1" in l for l in lines), lines)

    def test_g154_passes_through(self):
        lines = self.lines([Path.Command("G154", {"P": 22}), "G0 X1"])
        self.assert_line("G154 P22", lines)

    def test_extended_work_offset_out_of_range_is_refused(self):
        with self.assertRaisesRegex(CAMValueError, "P from 1 to 99"):
            self.export([Path.Command("G54.1", {"P": 100})])

    def test_linuxcnc_work_offsets_are_refused(self):
        """G59.1 to G59.9 are not Haas codes."""
        with self.assertRaises(CAMError):
            self.export(["G59.1", "G0 X1"])

    @NEEDS_WORK_PLANES
    def test_extended_work_offset_from_a_plane_is_g154(self):
        """A work plane names G54.1 P3; the Haas reads it as G154 P3."""
        plane = type("Plane", (), {"Label": "Side", "Fixture": "G54.1 P3", "Placement": None})()
        self.profile_op.Workplane = plane
        try:
            lines = self.lines(["G0 X1"])
        finally:
            del self.profile_op.Workplane
        self.assert_line("G154 P3", lines)

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
        self.assertTrue(any(l.startswith("O00007") for l in lines), gcode)
        self.assertFalse(any("M29" in l or "G28" in l for l in lines), gcode)


def _machine_bc(strategy):
    """A UMC-style table-table machine: B tilts about Y and carries C."""
    from Machine.models.machine import RotaryAxis, AxisRole

    machine = Machine.create_3axis_config()
    machine.name = "Test Haas UMC"
    machine.rotary_axes["C"] = RotaryAxis(
        name="C",
        rotation_vector=FreeCAD.Vector(0, 0, 1),
        role=AxisRole.TABLE_ROTARY,
        parent="B",
        sequence=0,
    )
    machine.rotary_axes["B"] = RotaryAxis(
        name="B",
        rotation_vector=FreeCAD.Vector(0, 1, 0),
        min_limit=-35,
        max_limit=120,
        role=AxisRole.TABLE_ROTARY,
        parent=None,
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

# The plane is the table turned 45 degrees about X. G268 takes the rotations
# about the work coordinate system's X, Y, Z in that order, so the 45 is on I.
DECLARE = "G268 X30.000 Y10.000 Z5.000 I45.000 J0.000 K0.000 Q123"
ROTARY = re.compile(r"^G0 [BC]-?\d")


@NEEDS_WORK_PLANES
class TestHaasTiltedWorkPlane(PathTestUtils.PathTestBase):
    """The Haas post on a Job with a tilted work plane and a B/C machine."""

    def setUp(self):
        import Path.Main.Job as PathJob

        self.maxDiff = None
        self.doc = FreeCAD.newDocument("TestHaasTiltedWorkPlane")
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
        post = PostProcessorFactory.get_post_processor(self.job, "haas")
        post.reinitialize()
        post._machine = machine
        post.apply_configuration_bundle()
        gcode = "\n".join(g for _, g in post.export2())
        return post, [line.strip() for line in gcode.splitlines()]

    # ------------------------------------------------------------------
    # TWP: G268 / G253 / G269
    # ------------------------------------------------------------------

    def test_twp_declares_the_plane_and_leaves_the_path_in_plane_coordinates(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_bc(RotationStrategy.TWP))
        declare = lines.index(DECLARE)
        self.assertEqual(["G53 G0 Z0", DECLARE, "G253"], lines[declare - 1 : declare + 2])
        self.assertIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines)
        self.assertIn("G2 X20.000 Y10.000 Z-2.000 I10.000 J0.000", lines)
        self.assertLess(declare, lines.index("G269"))
        self.assertFalse(any(ROTARY.match(l) for l in lines), lines)
        for word in ("G68.2", "G53.1", "G69", "G254", "G255"):
            self.assertFalse(any(l.split(" ")[0] == word for l in lines), word)

    def test_twp_cancels_the_plane_before_the_postamble(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_bc(RotationStrategy.TWP))
        self.assertLess(lines.index("G269"), lines.index("M30"))
        self.assertEqual("%", lines[-1])

    def test_twp_declares_after_the_tool_length_offset(self):
        """G268 alarms unless G43 is active."""
        machine = _machine_bc(RotationStrategy.TWP)
        machine.processing.tool_change = True
        self._op("Tilted", self._plane())
        _, lines = self._export(machine)
        self.assertLess(lines.index("G43 H1"), lines.index(DECLARE))

    def test_twp_program_positions_the_rotaries_when_the_control_does_not(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(
            _machine_bc(RotationStrategy.TWP), twp_control_positions_rotaries=False
        )
        rotary = next(i for i, l in enumerate(lines) if ROTARY.match(l))
        self.assertLess(rotary, lines.index(DECLARE))
        self.assertNotIn("G253", lines)

    def test_twp_rigid_tap_inside_the_plane(self):
        self._op(
            "Tapped", self._plane(), path=[Path.Command("G0", {"X": 0, "Y": 0, "Z": 5}), _tap()]
        )
        _, lines = self._export(_machine_bc(RotationStrategy.TWP))
        speed = lines.index("S500")
        self.assertLess(lines.index(DECLARE), speed)
        self.assertEqual("G84 X5.000 Y5.000 Z-10.000 F625.000 R2.000", lines[speed + 1])
        self.assertLess(speed, lines.index("G269"))

    def test_sanity_warns_when_twp_has_no_tool_length_offset(self):
        self._op("Tilted", self._plane())
        post, _ = self._export(_machine_bc(RotationStrategy.TWP))
        squawks = post.get_sanity_checks(self.job)
        self.assertTrue(any("G43 active before G268" in s["Note"] for s in squawks), squawks)

    def test_sanity_is_quiet_when_twp_has_a_tool_length_offset(self):
        machine = _machine_bc(RotationStrategy.TWP)
        machine.processing.tool_change = True
        self._op("Tilted", self._plane())
        post, _ = self._export(machine)
        squawks = post.get_sanity_checks(self.job)
        self.assertFalse(any("G268" in s["Note"] for s in squawks), squawks)

    # ------------------------------------------------------------------
    # DWO: G254 / G255
    # ------------------------------------------------------------------

    def test_dwo_wraps_the_rotary_move_in_g255_and_g254(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_bc(RotationStrategy.DWO))
        moves = [i for i, l in enumerate(lines) if ROTARY.match(l)]
        self.assertTrue(moves, lines)
        first, last = moves[0], moves[-1]
        self.assertEqual(["G53 G0 Z0", "G255"], lines[first - 2 : first])
        self.assertEqual("G254", lines[last + 1])
        self.assertNotIn(DECLARE, lines)
        self.assertNotIn("G269", lines)
        self.assertNotIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines, "the path is rotated")

    def test_dwo_is_cancelled_before_the_postamble(self):
        self._op("Tilted", self._plane())
        _, lines = self._export(_machine_bc(RotationStrategy.DWO))
        on = lines.index("G254")
        off = [i for i, l in enumerate(lines) if l == "G255"]
        self.assertGreater(off[-1], on)
        self.assertLess(off[-1], lines.index("M30"))

    def test_dwo_is_cancelled_before_the_rotaries_move_again(self):
        """Tilted, then plain: G255, the move home, and no G254 at the home pose."""
        self._op("Tilted", self._plane())
        self._op("Plain")
        _, lines = self._export(_machine_bc(RotationStrategy.DWO))
        on = [i for i, l in enumerate(lines) if l == "G254"]
        off = [i for i, l in enumerate(lines) if l == "G255"]
        moves = [i for i, l in enumerate(lines) if ROTARY.match(l)]
        self.assertEqual(1, len(on), lines)
        home = [i for i in moves if i > on[0]]
        self.assertTrue(home, lines)
        self.assertTrue(any(on[0] < i < home[0] for i in off), lines)
        self.assertEqual(lines.count("G255"), len(off))
        self.assertGreater(lines.index("M30"), home[-1])

    def test_dwo_selects_the_work_offset_before_g254(self):
        """An active work offset must be applied before G254 is commanded."""
        plane = self._plane()
        plane.Fixture = "G54.1 P2"
        self._op("Tilted", plane)
        _, lines = self._export(_machine_bc(RotationStrategy.DWO))
        self.assertLess(lines.index("G154 P2"), lines.index("G254"))

    def test_a_plain_operation_writes_no_g254(self):
        self._op("Plain")
        _, lines = self._export(_machine_bc(RotationStrategy.DWO))
        self.assertNotIn("G254", lines)
        self.assertIn("G1 X10.000 Y0.000 Z-2.000 F600.000", lines)

    # ------------------------------------------------------------------
    # refusals
    # ------------------------------------------------------------------

    def test_a_machine_with_no_strategy_refuses(self):
        self._op("Tilted", self._plane())
        with self.assertRaisesRegex(CAMValueError, "Rotation strategy"):
            self._export(_machine_bc(RotationStrategy.NONE))
