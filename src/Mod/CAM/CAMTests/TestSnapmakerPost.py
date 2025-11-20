#!/usr/bin/env python3
# SPDX-License-Identifier: EUPL-1.2

# ********************************************************************************
# *  Copyright (c) 2025 Clair-Loup Sergent <clsergent@free.fr>                   *
# *                                                                              *
# *  Licensed under the EUPL-1.2 with the specific provision                     *
# *  (EUPL articles 14 & 15) that the applicable law is the French law           *
# *  and the Jurisdiction Paris.                                                 *
# *  Any redistribution must include the specific provision above.               *
# *                                                                              *
# *  You may obtain a copy of the Licence at:                                    *
# *  https://interoperable-europe.ec.europa.eu/collection/eupl/eupl-text-eupl-12 *
# *                                                                              *
# *  Unless required by applicable law or agreed to in writing, software         *
# *  distributed under the Licence is distributed on an "AS IS" basis,           *
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or             *
# *  implied. See the Licence for the specific language governing                *
# *  permissions and limitations under the Licence.                              *
# ********************************************************************************
import argparse
import re
from typing import List

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

    def get_gcode(self, ops: List[str], arguments: str) -> str:
        """Get postprocessed gcode from a list of operations and postprocessor arguments"""
        self.profile_op.Path = Path.Path(ops)
        self.job.PostProcessorArgs = "--no-show-editor --no-gui --no-thumbnail " + arguments
        return self.post.export()[0][1]

    def test_general(self):
        """Test Output Generation"""

        expected_header = """\
;Header Start
;header_type: cnc
;machine: Snapmaker 2 A350 50W CNC module
;Post Processor: snapmaker_post
;CAM File: boxtest.fcstd
;Output Time: \\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{0,6}
;thumbnail: deactivated.""".replace('\n', self.post.values["END_OF_LINE_CHARACTERS"])

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
""".replace('\n', self.post.values["END_OF_LINE_CHARACTERS"])

        # test header and body with comments
        gcode = self.get_gcode([], "--machine=A350 --toolhead=50W_CNC")

        g_lines = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])
        e_lines = (
            expected_header.split(self.post.values["END_OF_LINE_CHARACTERS"])
            + expected_body.split(self.post.values["END_OF_LINE_CHARACTERS"])
        )

        self.assertTrue(len(g_lines), len(e_lines))
        for (nbr, exp), line in zip(enumerate(e_lines), g_lines):
            if exp.startswith(";Output Time:"):
                self.assertTrue(re.match(exp, line) is not None)
            else:
                self.assertEqual(exp, line)

        # test body without header
        gcode = self.get_gcode([], "--machine=A350 --toolhead=50W_CNC --no-header")
        self.assertEqual(gcode, expected_body)

        # test body without comments
        gcode = self.get_gcode([], "--machine=A350 --toolhead=50W_CNC --no-header --no-comments")
        expected = self.post.values["END_OF_LINE_CHARACTERS"].join(
            [line for line in expected_body.split(self.post.values["END_OF_LINE_CHARACTERS"]) if not line.startswith(";")]
        )
        self.assertEqual(gcode, expected)

    def test_command(self):
        """Test command Generation"""
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.000 Y20.000 Z30.000"

        gcode = self.get_gcode([command], "--machine=A350 --toolhead=50W_CNC --no-header")
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)

    def test_precision(self):
        """Test Precision"""
        # test G0 command with precision 2 digits precision
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.00 Y20.00 Z30.00"

        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W_CNC --no-header --precision=2"
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)

    def test_lines(self):
        """Test Line Numbers"""
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "N46 G0 X10.000 Y20.000 Z30.000"

        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W_CNC --no-header --line-numbers --line-number=10 --line-increment=2",
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)

    def test_preamble(self):
        """Test Pre-amble"""
        gcode = self.get_gcode(
            [],
            "--machine=A350 --toolhead=50W_CNC --no-header --preamble='G18 G55' --no-comments",
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[0]
        self.assertEqual(result, "G18 G55")

    def test_postamble(self):
        """Test Post-amble"""
        gcode = self.get_gcode(
            [],
            "--machine=A350 --toolhead=50W_CNC --no-header --postamble='G0 Z50\nM2' --no-comments",
        )
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[-3], "G0 Z50")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[-2], "M2")

    def test_inches(self):
        """Test inches conversion"""

        command = Path.Command("G0 X10 Y20 Z30")

        # test inches conversion
        expected = "G0 X0.3937 Y0.7874 Z1.1811"
        gcode = self.get_gcode([command], "--machine=A350 --toolhead=50W_CNC --no-header --inches")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[3], "G20")
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)

        # test inches conversion with 2 digits precision
        expected = "G0 X0.39 Y0.79 Z1.18"
        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W_CNC --no-header --inches --precision=2",
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)

    def test_axis_modal(self):
        """Test axis modal - Suppress the axis coordinate if the same as previous"""

        c0 = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        expected = "G0 Y30.000"

        gcode = self.get_gcode(
            [c0, c1], "--machine=A350 --toolhead=50W_CNC --no-header --axis-modal"
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[19]
        self.assertEqual(result, expected)

    def test_tool_change(self):
        """Test tool change"""

        c0 = Path.Command("M6 T2")
        c1 = Path.Command("M3 S3000")

        gcode = self.get_gcode([c0, c1], "--machine=A350 --toolhead=50W_CNC --no-header")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[19:22], ["M5", "M76", "M6 T2"])
        self.assertEqual(
            gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[22], "M3 P25"
        )  # no TLO on Snapmaker (G43 inserted after tool change)

    def test_machines(self):
        """Test the various machines, and also test machines that don't exist cause an error."""
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.000 Y20.000 Z30.000"

        with self.assertRaises(SystemExit):  # missing --machine fails
            self.get_gcode(
                [command],
                "--no-header",
            )

        with self.assertRaises(SystemExit):  # invalid --machine fails
            gcode = self.get_gcode(
                [command],
                "--machine=robot --no-header",
            )

        for machine in ("Original", "Artisan"):
            gcode = self.get_gcode(  # valid machine passes
                [command],
                f"--machine={machine} --no-header",
            )
            result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
            self.assertEqual(result, expected)

        for machine in ("A150", "A250", "A250T", "A350", "A350T"):
            gcode = self.get_gcode(  # valid toolhead passes with toolhead
                [command],
                f"--machine={machine} --toolhead=50W_CNC --no-header",
            )
            result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
            self.assertEqual(result, expected)

    def test_mod_kits(self):
        """Test the modkits against various machines and check calculated boundaries"""

        # Reference for boundaries with the bracing kit and quick swap kit combinations
        # [1] https://support.snapmaker.com/hc/en-us/articles/20786910972311-FAQ-for-Bracing-Kit-for-Snapmaker-2-0-Linear-Modules#h_01HN4Z7S9WJE5BRT492WR0CKH1
        # Reference for quick swap kit
        # [2] https://support.snapmaker.com/hc/en-us/articles/15320624494103-Pre-sale-FAQ-for-Quick-Swap-Kit

        def process_modkits(machine, modkits=None, toolhead=None) -> tuple[bool, argparse.Namespace]:
            """process arguments for the given combination of machine, toolhead and modkits"""
            self.job.PostProcessorArgs = (
                f"--no-show-editor --no-gui --no-thumbnail --machine={machine} "
                + (f"--toolhead={toolhead} " if toolhead else "")
                + (f"--modkits {modkits} " if modkits else " ")
            )
            return self.post.snapmaker_process_arguments()

        # Original
        result, args = process_modkits("Original")
        self.assertTrue(result)
        self.assertEqual(len(self.post.values["MODKITS"]), 0)
        # https://forum.snapmaker.com/t/cnc-work-area-size/5178
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=125, Y=125, Z=50))

        result, args = process_modkits("Original", "z_extension")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"z_extension"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=125, Y=125, Z=146))

        result, args = process_modkits("Original",  "BK")
        self.assertFalse(result)

        # Artisan
        result, args = process_modkits("Artisan")
        self.assertTrue(result)
        self.assertEqual(len(self.post.values["MODKITS"]), 0)
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=400, Y=400, Z=400))

        result, args = process_modkits("Artisan", "quick_swap")
        self.assertFalse(result)

        result, args = process_modkits("Artisan", "bracing_kit")
        self.assertFalse(result)

        # A150
        # This test case is covered in reference [1]
        result, args = process_modkits("A150", toolhead="50W_CNC")
        self.assertTrue(result)
        self.assertEqual(len(self.post.values["MODKITS"]), 0)
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=145, Y=160, Z=90))

        # This test case is covered in reference [1]
        result, args = process_modkits("A150", "BK", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=145, Y=148, Z=90))

        # This test case is not covered in references, but allowed in Luban 4.15
        result, args = process_modkits("A150", toolhead="200W_CNC")
        self.assertTrue(result)
        self.assertEqual(len(self.post.values["MODKITS"]), 0)
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=145, Y=147, Z=90))

        result, args = process_modkits("A150", "BK", "200W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=145, Y=135, Z=90))

        # This is incompatible according to [2]
        result, args = process_modkits("A150",  "QS")
        self.assertFalse(result)

        # A250
        # This test case is covered in reference [1]
        result, args = process_modkits("A250", "QS", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=230, Y=235, Z=180))

        # This test case is covered in reference [1]
        result, args = process_modkits("A250", "BK", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=230, Y=238, Z=180))

        # This test case is covered in reference [1]
        result, args = process_modkits("A250", "BK QS", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap", "bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=230, Y=223, Z=180))

        # A250T + 200W_CNC
        # This test case is covered in reference [1]
        result, args = process_modkits("A250T", "BK", "200W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=230, Y=225, Z=180))

        # This test case is covered in reference [1]
        result, args = process_modkits("A250T", "BK QS", "200W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap", "bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=230, Y=210, Z=180))

        # A350 + 50W_CNC
        # This test case is covered in reference [1]
        result, args = process_modkits("A350", "BK", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=320, Y=338, Z=275))

        # This test case is covered in reference [1]
        result, args = process_modkits("A350", "QS", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=320, Y=335, Z=275))

        # This test case is covered in reference [1]
        result, args = process_modkits("A350", "BK QS", "50W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap", "bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=320, Y=323, Z=275))

        # A350T + 200W_CNC
        # This test case is covered in reference [1]
        result, args = process_modkits("A350T", "BK", "200W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=320, Y=325, Z=275))

        # This test case is covered in reference [1]
        result, args = process_modkits("A350T", "BK QS", "200W_CNC")
        self.assertTrue(result)
        self.assertEqual(set(self.post.values["MODKITS"].keys()), {"quick_swap", "bracing_kit"})
        self.assertEqual(self.post.values["BOUNDARIES"], dict(X=320, Y=310, Z=275))

    def test_toolhead_selection(self):
        """Test automatic selection of toolhead where appropriate"""

        # check succeeds
        command = Path.Command("G0 X10 Y20 Z30")
        expected = "G0 X10.000 Y20.000 Z30.000"

        gcode = self.get_gcode(
            [command],
            "--machine=Original --no-header",
        )

        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)
        self.assertEqual(
            self.post.values["TOOLHEAD"]["name"],
            self.post.values["TOOLHEADS_LIST"]["Original_CNC"]["name"]
        )

        # check succeed with artisan (which base is bigger)
        gcode = self.get_gcode(
            [command],
            "--machine=Artisan --no-header --boundaries-check",
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        self.assertEqual(result, expected)
        self.assertEqual(
            self.post.values["TOOLHEAD"]["name"],
            self.post.values["TOOLHEADS_LIST"]["200W_CNC"]["name"]
        )

        for machine in ("A250", "A250T", "A350", "A350T"):
            gcode = self.get_gcode(  # missing --toolhead fails when multiple toolheads available
                [command],
                f"--machine={machine} --no-header",
            )
            self.assertFalse(isinstance(gcode, str))

    def test_spindle_percent_rpm_auto_select(self):
        """Test automatic selection of spindle speed rpm vs percent"""

        command = Path.Command("M3 S2100")

        # test original toolhead
        gcode = self.get_gcode([command], "--machine=Original --no-header")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 P30")

        command = Path.Command("M3 S3600")

        # test 50W toolhead
        gcode = self.get_gcode([command], "--machine=A350 --toolhead=50W_CNC --no-header")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 P30")

        # test 200W toolhead
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=200W_CNC --no-header"
        )
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 S3600")

        # test 200W toolhead
        gcode = self.get_gcode([command], "--machine=Artisan --no-header")
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 S3600")

    def test_spindle_percent(self):
        """Test spindle speed conversion from RPM to percents"""

        command = Path.Command("M3 S3600")

        # test 50W toolhead
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W_CNC --spindle-percent --no-header"
        )
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 P30")

        # test 200W toolhead
        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=200W_CNC --spindle-percent --no-header",
        )
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 P20")

        # test custom spindle speed extrema
        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=200W_CNC --spindle-percent --no-header --spindle-speeds=3000,4000",
        )
        self.assertEqual(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18], "M3 P90")

    def test_comment(self):
        """Test comment"""

        command = Path.Command("(comment)")
        gcode = self.get_gcode(
            [command], "--machine=A350 --toolhead=50W_CNC --spindle-percent --no-header"
        )
        result = gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])[18]
        expected = ";comment"
        self.assertEqual(result, expected)

    def test_boundaries(self):
        """Test boundaries check"""

        # check succeeds
        command = Path.Command("G0 X100 Y-100.5 Z-1")

        gcode = self.get_gcode(
            [command],
            "--machine=A350 --toolhead=50W_CNC --no-header --boundaries-check",
        )
        self.assertTrue(self.post.check_boundaries(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])))

        # check fails with A350
        c0 = Path.Command("G01 X100 Y-100.5 Z-1")
        c1 = Path.Command("G02 Y260")
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=A350 --toolhead=50W_CNC --no-header --boundaries-check",
        )
        self.assertFalse(self.post.check_boundaries(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])))

        # check succeed with artisan (which base is bigger)
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=Artisan --no-header --boundaries-check",
        )
        self.assertTrue(self.post.check_boundaries(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])))

        # check fails with custom boundaries
        gcode = self.get_gcode(
            [c0, c1],
            "--machine=A350 --toolhead=50W_CNC --no-header --boundaries-check --boundaries='50,400,10'",
        )
        self.assertFalse(self.post.check_boundaries(gcode.split(self.post.values["END_OF_LINE_CHARACTERS"])))
