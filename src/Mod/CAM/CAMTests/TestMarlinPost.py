# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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

import re

import Path
import CAMTests.PathTestUtils as PathTestUtils
import CAMTests.PostTestMocks as PostTestMocks
from Path.Post.Processor import PostProcessorFactory
from Machine.models.machine import Machine, Toolhead, ToolheadType

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestMarlinPost(PathTestUtils.PathTestBase):
    """Test Marlin-specific features of the marlin_post.py postprocessor.

    The only Marlin-specific behaviour is G4 dwell P→S conversion.
    All other functionality is inherited from the base PostProcessor.
    """

    @classmethod
    def setUpClass(cls):
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "marlin")

    @classmethod
    def tearDownClass(cls):
        pass

    def setUp(self):
        self.maxDiff = None
        self.post.reinitialize()
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.name = "Test Marlin Machine"
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            min_rpm=0,
            max_rpm=24000,
            max_power_kw=1.0,
        )
        self.post._machine.toolheads = [toolhead]

    def tearDown(self):
        pass

    # ----- Dwell P→S conversion tests -----

    def test_dwell_p_converted_to_s(self):
        """G4 P<seconds> should be output as G4 S<seconds> for Marlin."""
        cmd = Path.Command("G4", {"P": 3.0})
        result = self.post.convert_command_to_gcode(cmd)
        self.assertIsNotNone(result)
        self.assertIn("S", result)
        self.assertNotIn("P", result)
        self.assertIn("3.000", result)

    def test_dwell_s_passthrough(self):
        """G4 S<seconds> should pass through unchanged."""
        cmd = Path.Command("G4", {"S": 5.0})
        result = self.post.convert_command_to_gcode(cmd)
        self.assertIsNotNone(result)
        self.assertIn("S", result)
        self.assertIn("5.000", result)

    def test_dwell_fractional_seconds(self):
        """G4 with fractional seconds should preserve precision."""
        cmd = Path.Command("G4", {"P": 0.5})
        result = self.post.convert_command_to_gcode(cmd)
        self.assertIsNotNone(result)
        self.assertIn("S0.500", result)

    def test_dwell_in_full_export(self):
        """G4 dwell in full export2 pipeline should use S not P."""
        self.profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0, "Y": 0, "Z": 10}),
                Path.Command("G4", {"P": 2.0}),
                Path.Command("G1", {"X": 10, "Y": 10, "Z": -5, "F": 100}),
            ]
        )
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        results = self.post.export2()
        gcode = results[0][1]

        # Should contain G4 S, not G4 P
        lines = gcode.splitlines()
        dwell_re = re.compile(r"\bG0?4\b")
        dwell_lines = [l for l in lines if dwell_re.search(l)]
        self.assertTrue(len(dwell_lines) > 0, "Expected at least one G4 dwell line")
        for line in dwell_lines:
            self.assertIn("S", line, f"Dwell line should use S: {line}")
            self.assertNotIn(" P", line, f"Dwell line should not use P: {line}")

    def test_spindle_wait_uses_s(self):
        """Spindle wait (G4 after M3) should use S parameter for Marlin."""
        # Enable spindle wait on the toolhead
        self.post._machine.toolheads[0].spindle_wait = 3.0

        self.profile_op.Path = Path.Path(
            [
                Path.Command("M3", {"S": 1000}),
                Path.Command("G1", {"X": 10, "Y": 10, "Z": -5, "F": 100}),
            ]
        )
        self.post._machine.output.comments.enabled = False
        self.post._machine.output.output_header = False
        results = self.post.export2()
        gcode = results[0][1]

        lines = gcode.splitlines()
        dwell_re = re.compile(r"\bG0?4\b")
        dwell_lines = [l for l in lines if dwell_re.search(l)]
        for line in dwell_lines:
            self.assertIn("S", line, f"Spindle wait dwell should use S: {line}")

    # ----- Common property defaults -----

    def test_default_file_extension(self):
        """Marlin default file extension should be 'gcode'."""
        schema = self.post.__class__.get_common_property_schema()
        ext_prop = next(p for p in schema if p["name"] == "file_extension")
        self.assertEqual(ext_prop["default"], "gcode")

    def test_default_preamble(self):
        """Marlin default preamble should include G90, G21, G17."""
        schema = self.post.__class__.get_common_property_schema()
        preamble_prop = next(p for p in schema if p["name"] == "preamble")
        self.assertIn("G90", preamble_prop["default"])
        self.assertIn("G21", preamble_prop["default"])
        self.assertIn("G17", preamble_prop["default"])

    def test_default_postamble(self):
        """Marlin default postamble should be M5."""
        schema = self.post.__class__.get_common_property_schema()
        postamble_prop = next(p for p in schema if p["name"] == "postamble")
        self.assertEqual(postamble_prop["default"], "M5")

    def test_no_custom_properties(self):
        """Marlin should have no custom properties beyond common ones."""
        schema = self.post.__class__.get_property_schema()
        self.assertEqual(len(schema), 0)

    # ----- Init values -----

    def test_tool_change_disabled(self):
        """Marlin should have tool change disabled by default."""
        self.assertEqual(self.post.values.get("OUTPUT_TOOL_CHANGE"), False)

    def test_tlo_disabled(self):
        """Marlin should have tool length offset disabled by default."""
        self.assertEqual(self.post.values.get("USE_TLO"), False)
