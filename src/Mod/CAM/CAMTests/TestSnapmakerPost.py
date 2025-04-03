#!/usr/bin/env python3
# ***************************************************************************
# *  Copyright (c) 2025 Clair-Loup Sergent <clsergent@free.fr>              *
# *                                                                         *
# *  Licensed under the EUPL-1.2 with the specific provision                *
# *  (EUPL articles 14 & 15) that the applicable law is the French law.     *
# *  and the Jurisdiction Paris.                                            *
# *  Any redistribution must include the specific provision above.          *
# *                                                                         *
# *  You may obtain a copy of the Licence at:                               *
# *  https://joinup.ec.europa.eu/software/page/eupl5                        *
# *                                                                         *
# *  Unless required by applicable law or agreed to in writing, software    *
# *  distributed under the Licence is distributed on an "AS IS" basis,      *
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        *
# *  implied. See the Licence for the specific language governing           *
# *  permissions and limitations under the Licence.                         *
# ***************************************************************************
import re

import FreeCAD

import Path
import CAMTests.PathTestUtils as PathTestUtils
from Path.Post.Processor import PostProcessorFactory

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestSnapmakerPost(PathTestUtils.PathTestBase):
    """Test the Snapmaker postprocessor."""

    @classmethod
    def setUpClass(cls):
        """Set up the test environment"""

        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "snapmaker")
        # locate the operation named "Profile"
        for op in cls.job.Operations.Group:
            if op.Label == "Profile":
                # remember the "Profile" operation
                cls.profile_op = op
                return

    @classmethod
    def tearDownClass(cls):
        """Tear down the test environment"""
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        """Unit test init"""
        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        # reinitialize the postprocessor data structures between tests
        self.post.initialize()

    def tearDown(self):
        """Unit test tear down"""
        pass

    def get_gcode(self, ops: [str], arguments: str) -> str:
        """Get postprocessed gcode from a list of operations and postprocessor arguments"""
        self.profile_op.Path = Path.Path(ops)
        self.job.PostProcessorArgs = "--no-show-editor --no-gui --no-thumbnail " + arguments
        return self.post.export()[0][1]

    def test_general(self):
        """Test Output Generation"""

        expected_header = """\
;Header Start
;header_type: cnc
;machine: Snapmaker 2 A350(T)
;Post Processor: Snapmaker_post
;Cam File: boxtest.fcstd
;Output Time: \d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{0,6}
;thumbnail: deactivated."""

        expected_body = """\
;Begin preamble
G90
G17
G21
;Begin operation: Fixture
;Path: Fixture
G54
;End operation: Fixture
;Begin operation: TC: Default Tool
;Path: TC: Default Tool
;TC: Default Tool
;Begin toolchange
M5
M76
M6 T1
;End operation: TC: Default Tool
;Begin operation: Profile
;Path: Profile
;End operation: Profile
;Begin postamble
M400
M5
"""

        # test header and body with comments
        gcode = self.get_gcode([], "--machine=A350 --toolhead=50W --spindle-percent")

        g_lines = gcode.splitlines()
        e_lines = expected_header.splitlines() + expected_body.splitlines()

        self.assertTrue(len(g_lines), len(e_lines))
        for (nbr, exp), line in zip(enumerate(e_lines), g_lines):
            if exp.startswith(";Output Time:"):
                self.assertTrue(re.match(exp, line) is not None)
            else:
                self.assertTrue(line, exp)

        # test body without header
        gcode = self.get_gcode([], "--machine=A350 --toolhead=50W --spindle-percent --no-header")
        self.assertEqual(gcode, expected_body)

        # test body without comments
        gcode = self.get_gcode(
            [], "--machine=A350 --toolhead=50W --spindle-percent --no-header --no-comments"
        )
        expected = "".join(
            [line for line in expected_body.splitlines(keepends=True) if not line.startswith(";")]
        )
        self.assertEqual(gcode, expected)

    def test_command(self):
        """Test command Generation"""
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.000 Y20.000 Z30.000"

        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W --spindle-percent --no-header"
        )
        result = gcode.splitlines()[18]
        self.assertEqual(result, expected)

    def test_precision(self):
        """Test Precision"""
        # test G0 command with precision 2 digits precision
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.00 Y20.00 Z30.00"

        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W --spindle-percent --no-header --precision=2"
        )
        result = gcode.splitlines()[18]
        self.assertEqual(result, expected)

    def test_lines(self):
        """Test Line Numbers"""
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "N46 G0 X10.000 Y20.000 Z30.000"

        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --line-numbers --line-number=10 --line-increment=2",
        )
        result = gcode.splitlines()[18]
        self.assertEqual(result, expected)

    def test_preamble(self):
        """Test Pre-amble"""
        gcode = self.get_gcode(
            [],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --preamble='G18 G55' --no-comments",
        )
        result = gcode.splitlines()[0]
        self.assertEqual(result, "G18 G55")

    def test_postamble(self):
        """Test Post-amble"""
        gcode = self.get_gcode(
            [],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --postamble='G0 Z50\nM2' --no-comments",
        )
        result = gcode.splitlines()[-2]
        self.assertEqual(result, "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    def test_inches(self):
        """Test inches conversion"""

        command = Path.Command("G0 X10 Y20 Z30")

        # test inches conversion
        expected = "G0 X0.3937 Y0.7874 Z1.1811"
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W --spindle-percent --no-header --inches"
        )
        self.assertEqual(gcode.splitlines()[3], "G20")
        result = gcode.splitlines()[18]
        self.assertEqual(result, expected)

        # test inches conversion with 2 digits precision
        expected = "G0 X0.39 Y0.79 Z1.18"
        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --inches --precision=2",
        )
        result = gcode.splitlines()[18]
        self.assertEqual(result, expected)

    def test_axis_modal(self):
        """Test axis modal - Suppress the axis coordinate if the same as previous"""

        c0 = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        expected = "G0 Y30.000"

        gcode = self.get_gcode(
            [c0, c1], "--machine=A350 --toolhead=50W --spindle-percent --no-header --axis-modal"
        )
        result = gcode.splitlines()[19]
        self.assertEqual(result, expected)

    def test_tool_change(self):
        """Test tool change"""

        c0 = Path.Command("M6 T2")
        c1 = Path.Command("M3 S3000")

        gcode = self.get_gcode(
            [c0, c1], "--machine=A350 --toolhead=50W --spindle-percent --no-header"
        )
        self.assertEqual(gcode.splitlines()[19:22], ["M5", "M76", "M6 T2"])
        self.assertEqual(
            gcode.splitlines()[22], "M3 P25"
        )  # no TLO on Snapmaker (G43 inserted after tool change)

    def test_spindle(self):
        """Test spindle speed conversion from RPM to percents"""

        command = Path.Command("M3 S3600")

        # test 50W toolhead
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W --spindle-percent --no-header"
        )
        self.assertEqual(gcode.splitlines()[18], "M3 P30")

        # test 200W toolhead
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=200W --spindle-percent --no-header"
        )
        self.assertEqual(gcode.splitlines()[18], "M3 P20")

        # test custom spindle speed extrema
        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=200W --spindle-percent --no-header --spindle-speeds=3000,4000",
        )
        self.assertEqual(gcode.splitlines()[18], "M3 P90")

    def test_comment(self):
        """Test comment"""

        command = Path.Command("(comment)")
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W --spindle-percent --no-header"
        )
        result = gcode.splitlines()[18]
        expected = ";comment"
        self.assertEqual(result, expected)

    def test_boundaries(self):
        """Test boundaries check"""

        # check succeeds
        command = Path.Command("G0 X100 Y-100.5 Z-1")

        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --boundaries-check",
        )
        self.assertTrue(self.post.check_boundaries(gcode.splitlines()))

        # check fails with A350
        c0 = Path.Command("G01 X100 Y-100.5 Z-1")
        c1 = Path.Command("G02 Y260")
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --boundaries-check",
        )
        self.assertFalse(self.post.check_boundaries(gcode.splitlines()))

        # check succeed with artisan (which base is bigger)
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=artisan --toolhead=50W --spindle-percent --no-header --boundaries-check",
        )
        self.assertTrue(self.post.check_boundaries(gcode.splitlines()))

        # check fails with custom boundaries
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=A350 --toolhead=50W --spindle-percent --no-header --boundaries-check --boundaries='50,400,10'",
        )
        self.assertFalse(self.post.check_boundaries(gcode.splitlines()))
