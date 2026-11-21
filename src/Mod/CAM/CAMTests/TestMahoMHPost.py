# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Klaus Naumann <denkbrettl@gmail.com>               *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,             *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
Tests for the machine-based Maho MH post processor (maho_mh_post.py).

Drives the post through export2() on a mock job and pins the Maho MH /
Philips 432 dialect: the %PM prefix and program number, G70/G71 unit codes,
absolute arc centers in I/J/K, Philips 432 canned cycles (G81-G85 definition
followed by G79 execution block, B retract height, G73 chip break, G84 tapping
ramp parameter), line numbering, and the end-of-transmission marker.
"""

import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import (
    VALID_SCOPES,
    property_scope,
)
from Path.Post.scripts.maho_mh_post import Maho_Mh
from Path.Post.CAMErrors import CAMAttributeError
from Machine.models.machine import Machine, Toolhead, ToolheadType, OutputUnits

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

EOL = "\n"


class TestMahoMHPost(PathTestUtils.PathTestBase):
    """Maho MH output on a three-axis mock job."""

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.job.Machine = "maho_mh"
        cls.post = Maho_Mh(cls.job)

    def setUp(self):
        self.maxDiff = None

        machine = Machine.create_3axis_config()
        machine.name = "Test Maho MH"
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
                max_rpm=5000,
                max_power_kw=5.0,
            )
        ]
        self.profile_op.Path = Path.Path([])
        self.profile_op.ClearanceHeight = type("obj", (object,), {"Value": 18.0})()
        machine.postprocessor_properties["emit_graphical_view_setup"] = False
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
        """The lines produced between the fixture and postamble."""
        lines = self.lines(commands)
        start = lines.index("G54") + 1
        end = len(lines) - lines[::-1].index("M5") - 1
        return lines[start:end]

    def assert_line(self, expected, lines):
        msg = f"expected line {expected!r} in\n{EOL.join(lines)}"
        self.assertIn(expected, lines, msg)

    def assert_no_line(self, unexpected, lines):
        msg = f"unexpected line {unexpected!r} in\n{EOL.join(lines)}"
        self.assertNotIn(unexpected, lines, msg)

    # ------------------------------------------------------------------
    # schema
    # ------------------------------------------------------------------

    def test_default_file_extension(self):
        self.assertEqual("nc", self.post.get_file_extension())

    def test_property_schema_is_well_formed(self):
        for prop in self.post.get_full_property_schema():
            self.assertIn(property_scope(prop), VALID_SCOPES, prop["name"])
        names = {prop["name"] for prop in self.post.get_property_schema()}
        self.assertEqual(
            names,
            {
                "program_number",
                "absolute_arc_centers",
                "chip_break_retract",
                "tool_changer_size",
                "emit_graphical_view_setup",
                "graphical_view_window_size",
            },
        )

    def test_common_property_schema_defaults(self):
        common_props = {p["name"]: p.get("default") for p in self.post.get_common_property_schema()}
        self.assertTrue(common_props["supports_tool_radius_compensation"])
        self.assertEqual(common_props["preamble"], "G17\nG52")
        self.assertEqual(common_props["postamble"], "M5\nM9\nG17\nG40\nM30")
        self.assertEqual(common_props["safetyblock"], "")
        supported = common_props["supported_commands"].split("\n")
        for cmd in ("G70", "G71", "G79", "G81", "G82", "G83", "G84", "G85"):
            self.assertIn(cmd, supported)

    def test_init_values(self):
        vals = {}
        self.post.init_values(vals)
        self.assertEqual("Maho MH", vals["MACHINE_NAME"])
        file_name = vals["POSTPROCESSOR_FILE_NAME"]
        self.assertTrue(file_name.endswith("maho_mh_post"))

    # ------------------------------------------------------------------
    # program frame
    # ------------------------------------------------------------------

    def test_empty_program(self):
        """%PM, program number, preamble, G71, postamble, and EOT."""
        self.assertEqual(
            self.lines([]),
            [
                "%PM",
                "N90001",
                "G17",
                "G52",
                "G71",
                "M6 T1",
                "G43 H1",
                "M3 S1000",
                "G54",
                "M5",
                "M9",
                "G17",
                "G40",
                "M30",
                "\x04",
            ],
        )

    def test_program_number_property(self):
        props = self.post._machine.postprocessor_properties
        props["program_number"] = "N12345"
        lines = self.lines([])
        self.assertEqual("%PM", lines[0])
        self.assertEqual("N12345", lines[1])

    def test_program_number_default_fallback(self):
        props = self.post._machine.postprocessor_properties
        props["program_number"] = ""
        lines = self.lines([])
        self.assertEqual("N90001", lines[1])

    def test_end_of_transmission_with_comments(self):
        self.post._machine.output.comments.enabled = True
        lines = self.lines([])
        self.assertEqual("\x04", lines[-1])
        self.assertEqual(
            "(The next line is the EOT character, which is important for "
            "Philips 432 controllers. Do not remove.)",
            lines[-2],
        )

    # ------------------------------------------------------------------
    # units
    # ------------------------------------------------------------------

    def test_metric_units(self):
        lines = self.lines(["G0 X25.4 Y50.8 Z12.7"])
        self.assert_line("G71", lines)
        self.assert_no_line("G70", lines)
        self.assert_no_line("G21", lines)
        self.assert_line("G0 X25.400 Y50.800 Z12.700", lines)

    def test_inch_units(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.lines(["G0 X25.4 Y50.8 Z12.7"])
        self.assert_line("G70", lines)
        self.assert_no_line("G71", lines)
        self.assert_no_line("G20", lines)
        self.assert_line("G0 X1.000 Y2.000 Z0.500", lines)

    def test_invalid_units_raises(self):
        self.post.values["OUTPUT_UNITS"] = "InvalidUnit"
        with self.assertRaises(CAMAttributeError):
            self.post._collect_unit_command()

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
                "G0 X10.000 Y0.000 Z18.000",
                "G2 X0.000 Y10.000 Z18.000 F6000.000 I0.000 J0.000",
                "G3 X10.000 Y20.000 Z18.000 F6000.000 I10.000 J10.000",
            ],
        )

    def test_arc_centers_incremental_when_asked(self):
        props = self.post._machine.postprocessor_properties
        props["absolute_arc_centers"] = False
        lines = self.op_lines(["G0 X10 Y0 Z18", "G2 X0 Y10 Z18 I-10 J0 F100"])
        self.assertEqual(
            "G2 X0.000 Y10.000 Z18.000 F6000.000 I-10.000 J0.000",
            lines[1],
        )

    def test_arc_centers_absolute_in_inches(self):
        self.post._machine.output.units = OutputUnits.IMPERIAL
        lines = self.op_lines(["G0 X25.4 Y0 Z25.4", "G2 X0 Y25.4 Z25.4 I-25.4 J0 F100"])
        self.assertEqual(
            "G2 X0.000 Y1.000 Z1.000 F236.220 I0.000 J0.000",
            lines[1],
        )

    # ------------------------------------------------------------------
    # canned cycles
    # ------------------------------------------------------------------

    def test_drill_cycle_g81(self):
        """G81 definition with Y (retract) and Z (depth), executed via G79."""
        lines = self.op_lines(
            [
                "G0 Z10",
                "G0 X30 Y8",
                "G0 Z4",
                "G99",
                "G81 X30 Y8 Z-15 F200 R4",
                "G80",
                "G0 Z10",
            ]
        )
        self.assert_no_line("G99", lines)
        self.assert_no_line("G80", lines)
        self.assertEqual(
            lines,
            [
                "G0 Z10.000",
                "G0 X30.000 Y8.000",
                "G0 Z4.000",
                "G81 Y4.000 Z-15.000 F12000.000",
                "G79 X30.000 Y8.000 Z0.000",
                "G0 Z10.000",
            ],
        )

    def test_drill_cycle_g82_dwell(self):
        """G82 uses G81 definition with dwell parameter P mapped to X."""
        cmd = Path.Command("G82 X5 Y10 Z-12 R2 P1.5 F100")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G81 X1.500 Y2.000 Z-12.000 F6000.000", lines)
        self.assert_line("G79 X5.000 Y10.000 Z0.000", lines)

    def test_drill_cycle_g83_peck(self):
        """G83 peck depth Q mapped to K, reduction parameter I set to 0."""
        cmd = Path.Command("G83 X10 Y20 Z-25 R2 Q5 F150")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G83 Y2.000 Z-25.000 F9000.000 I0.000 K5.000", lines)
        self.assert_line("G79 X10.000 Y20.000 Z0.000", lines)

    def test_drill_cycle_g73_chip_break(self):
        """G73 chip break cycle converted to G83 with retract parameter J."""
        cmd = Path.Command("G73 X10 Y20 Z-25 R2 Q5 F150")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G83 Y2.000 Z-25.000 F9000.000 I0.000 J0.500 K5.000", lines)
        self.assert_line("G79 X10.000 Y20.000 Z0.000", lines)

    def test_drill_cycle_g73_custom_chip_break_retract(self):
        props = self.post._machine.postprocessor_properties
        props["chip_break_retract"] = 1.25
        cmd = Path.Command("G73 X10 Y20 Z-25 R2 Q5 F150")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G83 Y2.000 Z-25.000 F9000.000 I0.000 J1.250 K5.000", lines)

    def test_drill_cycle_g84_tapping(self):
        """G84 tapping cycle definition with ramp parameter I."""
        cmd = Path.Command("G84 X10 Y20 Z-15 R2 S500 F250")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G84 Y2.000 Z-15.000 F15000.000 I5.000", lines)
        self.assert_line("G79 X10.000 Y20.000 Z0.000", lines)

    def test_drill_cycle_g85_boring(self):
        cmd = Path.Command("G85 X10 Y20 Z-15 R2 F200")
        lines = self.op_lines(["G0 Z10", "G99", cmd, "G80"])
        self.assert_line("G85 Y2.000 Z-15.000 F12000.000", lines)
        self.assert_line("G79 X10.000 Y20.000 Z0.000", lines)

    def test_drill_cycle_g98_retract_height(self):
        """When G98 is active, B parameter provides retract height above R."""
        cmd = Path.Command("G81 X10 Y20 Z-15 R5 F200")
        lines = self.op_lines(["G0 Z20", "G98", cmd, "G80"])
        self.assert_line("G81 Y5.000 Z-15.000 B15.000 F12000.000", lines)

    def test_drill_cycle_g98_first_operation_uses_clearance_height(self):
        """When a drill cycle with G98 has no prior Z move, it uses ClearanceHeight for B."""
        cmd = Path.Command("G81 X10 Y20 Z-15 R5 F200")
        lines = self.op_lines(["G98", cmd, "G80"])
        # ClearanceHeight is 18.0, R is 5.0 -> B = 18.0 - 5.0 = 13.0
        self.assert_line("G81 Y5.000 Z-15.000 B13.000 F12000.000", lines)

    def test_drill_cycle_g98_no_b_when_previous_z_not_greater_than_r(self):
        cmd = Path.Command("G81 X10 Y20 Z-15 R5 F200")
        lines = self.op_lines(["G0 Z5", "G98", cmd, "G80"])
        self.assert_line("G81 Y5.000 Z-15.000 F12000.000", lines)
        for line in lines:
            if line.startswith("G81"):
                self.assertNotIn("B", line)

    def test_drill_cycle_missing_feed_raises(self):
        with self.assertRaises(Exception) as ctx:
            self.lines([Path.Command("G81 X10 Y20 Z-15 R5")])
        self.assertIn("missing 'F' parameter", str(ctx.exception))

    def test_drill_cycle_unsupported_cycle_raises(self):
        with self.assertRaises(Exception) as ctx:
            self.lines([Path.Command("G88 X10 Y20 Z-15 R5 F200")])
        self.assertIn("Drill cycle G88 not supported", str(ctx.exception))

    # ------------------------------------------------------------------
    # line numbering
    # ------------------------------------------------------------------

    def test_line_numbers_enabled(self):
        """Line numbers on all blocks except %PM and program number."""
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 1
        self.post._machine.output.formatting.line_increment = 1
        self.post._machine.output.formatting.line_number_prefix = "N"
        self.post._machine.output.comments.enabled = True
        lines = self.lines([Path.Command("(Profile move)"), "G0 X10 Y20 Z30"])
        self.assertEqual("%PM", lines[0])
        self.assertEqual("N90001", lines[1])
        self.assertEqual("N1 G17", lines[2])
        self.assertEqual("N2 G52", lines[3])
        self.assertEqual("N3 G71", lines[4])
        self.assertEqual("\x04", lines[-1])

        comment_lines = [line for line in lines if "(Profile move)" in line]
        self.assertEqual(1, len(comment_lines))
        self.assertTrue(comment_lines[0].startswith("N"), comment_lines[0])

        g0_lines = [line for line in lines if "G0 X10.000" in line]
        self.assertEqual(1, len(g0_lines))
        self.assertTrue(g0_lines[0].startswith("N"), g0_lines[0])

    def test_line_numbers_with_comments_disabled(self):
        """When comments are disabled, suppressed comments produce no lines."""
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 1
        self.post._machine.output.formatting.line_increment = 1
        self.post._machine.output.formatting.line_number_prefix = "N"
        self.post._machine.output.comments.enabled = False
        cmd = Path.Command("(Suppressed comment)")
        lines = self.lines([cmd, "G0 X10 Y20 Z30"])
        self.assertEqual("%PM", lines[0])
        self.assertEqual("N90001", lines[1])
        self.assertEqual("\x04", lines[-1])
        for line in lines:
            self.assertNotIn("None", line)
            self.assertNotIn("(Suppressed comment)", line)

    def test_line_numbers_custom_start_and_increment(self):
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.output.formatting.line_number_start = 10
        self.post._machine.output.formatting.line_increment = 5
        self.post._machine.output.formatting.line_number_prefix = "N"
        self.post._machine.output.comments.enabled = True
        lines = self.lines([])
        self.assertEqual("N10 G17", lines[2])
        self.assertEqual("N15 G52", lines[3])
        self.assertEqual("N20 G71", lines[4])

    # ------------------------------------------------------------------
    # graphical view setup
    # ------------------------------------------------------------------

    def test_graphical_view_setup_enabled(self):
        props = self.post._machine.postprocessor_properties
        props["emit_graphical_view_setup"] = True
        lines = self.lines([])
        self.assert_line("G98 X-5.000 Y-5.000 Z-0.500 I105.000 J105.000 K10.500", lines)
        self.assert_line("G99 X0.000 Y0.000 Z0.000 I100.000 J100.000 K10.000", lines)

    def test_graphical_view_setup_custom_window_size(self):
        props = self.post._machine.postprocessor_properties
        props["emit_graphical_view_setup"] = True
        props["graphical_view_window_size"] = 120.0
        lines = self.lines([])
        self.assert_line("G98 X-10.000 Y-10.000 Z-1.000 I110.000 J110.000 K11.000", lines)
        self.assert_line("G99 X0.000 Y0.000 Z0.000 I100.000 J100.000 K10.000", lines)

    def test_graphical_view_setup_missing_stock(self):
        props = self.post._machine.postprocessor_properties
        props["emit_graphical_view_setup"] = True
        original_stock = self.job.Stock
        try:
            self.job.Stock = None
            lines = self.lines([])
            self.assertFalse(any(line.startswith("G98") for line in lines), lines)
            self.assertFalse(any(line.startswith("G99") for line in lines), lines)
        finally:
            self.job.Stock = original_stock

    # ------------------------------------------------------------------
    # tool changer
    # ------------------------------------------------------------------

    def test_tool_changer_m6_within_limit(self):
        self.post._machine.processing.tool_change = True
        props = self.post._machine.postprocessor_properties
        props["tool_changer_size"] = 12
        cmd = Path.Command("M6", {"T": 5})
        lines = self.op_lines([cmd])
        self.assert_line("M6 T5", lines)

    def test_tool_changer_m66_above_limit(self):
        self.post._machine.processing.tool_change = True
        props = self.post._machine.postprocessor_properties
        props["tool_changer_size"] = 12
        cmd = Path.Command("M6", {"T": 15})
        lines = self.op_lines([cmd])
        self.assert_line("M66 T15", lines)

    def test_tool_changer_disabled_uses_m6(self):
        self.post._machine.processing.tool_change = True
        props = self.post._machine.postprocessor_properties
        props["tool_changer_size"] = 0
        cmd = Path.Command("M6", {"T": 25})
        lines = self.op_lines([cmd])
        self.assert_line("M6 T25", lines)
