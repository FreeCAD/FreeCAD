# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
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

import FreeCAD

import Path
import PathTests.PathTestUtils as PathTestUtils
from Path.Post.scripts import refactored_test_post as postprocessor


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestRefactoredTestPost(PathTestUtils.PathTestBase):
    @classmethod
    def setUpClass(cls):
        """setUpClass()...

        This method is called upon instantiation of this test class.  Add code
        and objects here that are needed for the duration of the test() methods
        in this class.  In other words, set up the 'global' test environment
        here; use the `setUp()` method to set up a 'local' test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        # Open existing FreeCAD document with test geometry
        FreeCAD.newDocument("Unnamed")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """
        # Close geometry document without saving
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        self.maxDiff = None
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console
        self.docobj = FreeCAD.ActiveDocument.addObject("Path::Feature", "testpath")
        #
        # Re-initialize all of the values before doing a test.
        #
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.values)

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        FreeCAD.ActiveDocument.removeObject("testpath")

    def single_compare(self, path, expected, args, debug=False):
        """Perform a test with a single comparison."""
        nl = "\n"
        self.docobj.Path = Path.Path(path)
        postables = [self.docobj]
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    def compare_third_line(self, path_string, expected, args, debug=False):
        """Perform a test with a single comparison to the third line of the output."""
        nl = "\n"
        if path_string:
            self.docobj.Path = Path.Path([Path.Command(path_string)])
        else:
            self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[2], expected)

    #
    # The tests are organized into groups:
    #
    #   00000 - 00099  tests that don't fit any other category
    #   00100 - 00999  tests for all of the various arguments/options
    #   01000 - 01999  tests for the various G codes at 1000 + 10 * g_code_value
    #   02000 - 02999  tests for the various M codes at 2000 + 10 * m_code_value
    #

    def test00000(self):
        """Test Output Generation.

        Empty path.  Produces only the preamble and postable.
        Also tests the interactions between --comments and --header.
        """
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        # Test generating with comments and header.
        # The header contains a time stamp that messes up unit testing.
        # Only test the length of the line that contains the time.
        args = "--comments --header"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[0], "(Exported by FreeCAD)")
        self.assertEqual(
            gcode.splitlines()[1],
            "(Post Processor: Path.Post.scripts.refactored_test_post)",
        )
        self.assertEqual(gcode.splitlines()[2], "(Cam File: )")
        self.assertIn("(Output Time: ", gcode.splitlines()[3])
        self.assertTrue(len(gcode.splitlines()[3]) == 41)
        self.assertEqual(gcode.splitlines()[4], "(Begin preamble)")
        self.assertEqual(gcode.splitlines()[5], "G90")
        self.assertEqual(gcode.splitlines()[6], "G21")
        self.assertEqual(gcode.splitlines()[7], "(Begin operation)")
        self.assertEqual(gcode.splitlines()[8], "(Finish operation: testpath)")
        self.assertEqual(gcode.splitlines()[9], "(Begin postamble)")

        # Test with comments without header.
        expected = """(Begin preamble)
G90
G21
(Begin operation)
(Finish operation: testpath)
(Begin postamble)
"""
        args = "--comments --no-header"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test without comments with header.
        args = "--no-comments --header"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[0], "(Exported by FreeCAD)")
        self.assertEqual(
            gcode.splitlines()[1],
            "(Post Processor: Path.Post.scripts.refactored_test_post)",
        )
        self.assertEqual(gcode.splitlines()[2], "(Cam File: )")
        self.assertIn("(Output Time: ", gcode.splitlines()[3])
        self.assertTrue(len(gcode.splitlines()[3]) == 41)
        self.assertEqual(gcode.splitlines()[4], "G90")
        self.assertEqual(gcode.splitlines()[5], "G21")

        # Test without comments or header.
        expected = """G90
G21
"""
        args = "--no-comments --no-header"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00010(self):
        """Test Outputting all arguments.

        Empty path.  Outputs all arguments.
        """
        expected = """Arguments that are commonly used:
  --metric              Convert output for Metric mode (G21) (default)
  --inches              Convert output for US imperial mode (G20)
  --axis-modal          Don't output axis values if they are the same as the
                        previous line
  --no-axis-modal       Output axis values even if they are the same as the
                        previous line (default)
  --axis-precision AXIS_PRECISION
                        Number of digits of precision for axis moves, default
                        is 3
  --bcnc                Add Job operations as bCNC block headers. Consider
                        suppressing comments by adding --no-comments
  --no-bcnc             Suppress bCNC block header output (default)
  --comments            Output comments (default)
  --no-comments         Suppress comment output
  --feed-precision FEED_PRECISION
                        Number of digits of precision for feed rate, default
                        is 3
  --header              Output headers (default)
  --no-header           Suppress header output
  --line-numbers        Prefix with line numbers
  --no-line-numbers     Don't prefix with line numbers (default)
  --modal               Don't output the G-command name if it is the same as
                        the previous line
  --no-modal            Output the G-command name even if it is the same as
                        the previous line (default)
  --output_all_arguments
                        Output all of the available arguments
  --no-output_all_arguments
                        Don't output all of the available arguments (default)
  --output_visible_arguments
                        Output all of the visible arguments
  --no-output_visible_arguments
                        Don't output the visible arguments (default)
  --postamble POSTAMBLE
                        Set commands to be issued after the last command,
                        default is ""
  --preamble PREAMBLE   Set commands to be issued before the first command,
                        default is ""
  --precision PRECISION
                        Number of digits of precision for both feed rate and
                        axis moves, default is 3 for metric or 4 for inches
  --return-to RETURN_TO
                        Move to the specified x,y,z coordinates at the end,
                        e.g. --return-to=0,0,0 (default is do not move)
  --show-editor         Pop up editor before writing output (default)
  --no-show-editor      Don't pop up editor before writing output
  --tlo                 Output tool length offset (G43) following tool changes
                        (default)
  --no-tlo              Suppress tool length offset (G43) following tool
                        changes
  --tool_change         Insert M6 and any other tool change G-code for all
                        tool changes (default)
  --no-tool_change      Convert M6 to a comment for all tool changes
  --translate_drill     Translate drill cycles G73, G81, G82 & G83 into G0/G1
                        movements
  --no-translate_drill  Don't translate drill cycles G73, G81, G82 & G83 into
                        G0/G1 movements (default)
  --wait-for-spindle WAIT_FOR_SPINDLE
                        Time to wait (in seconds) after M3, M4 (default = 0.0)
"""
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        gcode: str = postprocessor.export(
            postables, "gcode.tmp", "--output_all_arguments"
        )
        # The argparse help routine turns out to be sensitive to the
        # number of columns in the terminal window that the tests
        # are run from.  This affects the indenting in the output.
        # The next couple of lines remove all of the white space.
        gcode = "".join(gcode.split())
        expected = "".join(expected.split())
        self.assertEqual(gcode, expected)

    def test00020(self):
        """Test Outputting visible arguments.

        Empty path.  Outputs visible arguments.
        """
        self.single_compare([], "", "--output_visible_arguments")

    def test00100(self):
        """Test bcnc."""
        self.single_compare(
            [],
            """G90
G21
(Block-name: testpath)
(Block-expand: 0)
(Block-enable: 1)
(Block-name: post_amble)
(Block-expand: 0)
(Block-enable: 1)
""",
            "--bcnc",
        )
        self.single_compare(
            [],
            """G90
G21
""",
            "--no-bcnc",
        )

    def test00110(self):
        """Test axis modal.

        Suppress the axis coordinate if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]

        args = "--axis-modal"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 Y30.000")

        args = "--no-axis-modal"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 X10.000 Y30.000 Z30.000")

    def test00120(self):
        """Test axis-precision."""
        self.compare_third_line(
            "G0 X10 Y20 Z30", "G0 X10.00 Y20.00 Z30.00", "--axis-precision=2"
        )

    def test00130(self):
        """Test comments."""
        c = Path.Command("(comment)")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = "--comments"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[4], "(comment)")

    def test00140(self):
        """Test feed-precision."""
        #
        c = Path.Command("G1 X10 Y20 Z30 F123.123456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.407")

        args = "--feed-precision=2"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.41")

    def test00150(self):
        """Test Line Numbers."""
        self.compare_third_line(
            "G0 X10 Y20 Z30", "N120 G0 X10.000 Y20.000 Z30.000", "--line-numbers"
        )

    def test00160(self):
        """Test inches."""
        #
        c = Path.Command("G0 X10 Y20 Z30 A10 B20 C30 U10 V20 W30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = "--inches"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[1], "G20")
        self.assertEqual(
            gcode.splitlines()[2],
            "G0 X0.3937 Y0.7874 Z1.1811 A0.3937 B0.7874 C1.1811 U0.3937 V0.7874 W1.1811",
        )

    def test00170(self):
        """Test modal.

        Suppress the command name if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]
        args = "--modal"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "X10.000 Y30.000 Z30.000")
        args = "--no-modal"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 X10.000 Y30.000 Z30.000")

    def test00180(self):
        """Test Post-amble."""
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--postamble='G0 Z50\nM2'"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[-2], "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    def test00190(self):
        """Test Pre-amble."""
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--preamble='G18 G55'"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[0], "G18 G55")

    def test00200(self):
        """Test precision."""
        self.compare_third_line(
            "G1 X10 Y20 Z30 F100",
            "G1 X10.00 Y20.00 Z30.00 F6000.00",
            "--precision=2",
        )
        self.compare_third_line(
            "G1 X10 Y20 Z30 F100",
            "G1 X0.39 Y0.79 Z1.18 F236.22",
            "--inches --precision=2",
        )

    def test00210(self):
        """Test return-to."""
        self.compare_third_line("", "G0 X12 Y34 Z56", "--return-to='12,34,56'")

    def test00220(self):
        """Test tlo."""
        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c, c2])
        postables = [self.docobj]
        args = "--tlo"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "G43 H2")
        self.assertEqual(gcode.splitlines()[4], "M3 S3000")
        # suppress TLO
        args = "--no-tlo"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "M3 S3000")

    def test00230(self):
        """Test tool_change."""
        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c, c2])
        postables = [self.docobj]
        args = "--tool_change"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "M3 S3000")
        args = "--comments --no-tool_change"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[5], "( M6 T2 )")
        self.assertEqual(gcode.splitlines()[6], "M3 S3000")

    def test00300(self):
        """Test wait-for-spindle."""
        c = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M3 S3000")
        args = "--wait-for-spindle=1.23456"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M3 S3000")
        self.assertEqual(gcode.splitlines()[3], "G4 P1.23456")
        c = Path.Command("M4 S3000")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        # This also tests that the default for --wait-for-spindle
        # goes back to 0.0 (no wait)
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M4 S3000")
        args = "--wait-for-spindle=1.23456"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M4 S3000")
        self.assertEqual(gcode.splitlines()[3], "G4 P1.23456")

    def test01000(self):
        """Test G0 command Generation."""
        self.compare_third_line(
            "G0 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            "G0 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000",
            "",
        )
        self.compare_third_line(
            "G00 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            "G00 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000",
            "",
        )

    def test01010(self):
        """Test G1 command Generation."""
        self.compare_third_line(
            "G1 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            "G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F74.074",
            "",
        )
        self.compare_third_line(
            "G01 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            "G01 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F74.074",
            "",
        )
        # Test argument order
        self.compare_third_line(
            "G1 F1.23456 Z30 V80 C60 W90 X10 B50 U70 Y20 A40",
            "G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F74.074",
            "",
        )
        self.compare_third_line(
            "G1 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            "G1 X0.3937 Y0.7874 Z1.1811 A1.5748 B1.9685 C2.3622 U2.7559 V3.1496 W3.5433 F2.9163",
            "--inches",
        )

    def test01020(self):
        """Test G2 command Generation."""
        #
        self.compare_third_line(
            "G2 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G2 X10.000 Y20.000 Z30.000 I40.000 J50.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G02 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G02 X10.000 Y20.000 Z30.000 I40.000 J50.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G2 X10 Y20 Z30 R40 P60 F1.23456",
            "G2 X10.000 Y20.000 Z30.000 R40.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G2 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G2 X0.3937 Y0.7874 Z1.1811 I1.5748 J1.9685 P60 F2.9163",
            "--inches",
        )
        self.compare_third_line(
            "G2 X10 Y20 Z30 R40 P60 F1.23456",
            "G2 X0.3937 Y0.7874 Z1.1811 R1.5748 P60 F2.9163",
            "--inches",
        )

    def test01030(self):
        """Test G3 command Generation."""
        self.compare_third_line(
            "G3 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G3 X10.000 Y20.000 Z30.000 I40.000 J50.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G03 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G03 X10.000 Y20.000 Z30.000 I40.000 J50.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G3 X10 Y20 Z30 R40 P60 F1.23456",
            "G3 X10.000 Y20.000 Z30.000 R40.000 P60 F74.074",
            "",
        )
        self.compare_third_line(
            "G3 X10 Y20 Z30 I40 J50 P60 F1.23456",
            "G3 X0.3937 Y0.7874 Z1.1811 I1.5748 J1.9685 P60 F2.9163",
            "--inches",
        )
        self.compare_third_line(
            "G3 X10 Y20 Z30 R40 P60 F1.23456",
            "G3 X0.3937 Y0.7874 Z1.1811 R1.5748 P60 F2.9163",
            "--inches",
        )

    def test01040(self):
        """Test G4 command Generation."""
        # Should some sort of "precision" be applied to the P parameter?
        # The code as currently written does not do so intentionally.
        # The P parameter indicates "time to wait" where a 0.001 would
        # be a millisecond wait, so more than 3 or 4 digits of precision
        # might be useful.
        self.compare_third_line("G4 P1.23456", "G4 P1.23456", "")
        self.compare_third_line("G04 P1.23456", "G04 P1.23456", "")
        self.compare_third_line("G4 P1.23456", "G4 P1.23456", "--inches")

    def test01070(self):
        """Test G7 command Generation."""
        self.compare_third_line("G7", "G7", "")

    def test01080(self):
        """Test G8 command Generation."""
        self.compare_third_line("G8", "G8", "")

    def test01100(self):
        """Test G10 command Generation."""
        self.compare_third_line("G10 L1 P2 Z1.23456", "G10 L1 Z1.235 P2", "")
        self.compare_third_line(
            "G10 L1 P2 R1.23456 I2.34567 J3.456789 Q3",
            "G10 L1 I2.346 J3.457 R1.235 P2 Q3",
            "",
        )
        self.compare_third_line(
            "G10 L2 P3 X1.23456 Y2.34567 Z3.456789",
            "G10 L2 X1.235 Y2.346 Z3.457 P3",
            "",
        )
        self.compare_third_line(
            "G10 L2 P0 X0 Y0 Z0", "G10 L2 X0.000 Y0.000 Z0.000 P0", ""
        )
        self.compare_third_line(
            "G10 L10 P1 X1.23456 Y2.34567 Z3.456789",
            "G10 L10 X1.235 Y2.346 Z3.457 P1",
            "",
        )
        self.compare_third_line(
            "G10 L10 P2 R1.23456 I2.34567 J3.456789 Q3",
            "G10 L10 I2.346 J3.457 R1.235 P2 Q3",
            "",
        )
        self.compare_third_line(
            "G10 L11 P1 X1.23456 Y2.34567 Z3.456789",
            "G10 L11 X1.235 Y2.346 Z3.457 P1",
            "",
        )
        self.compare_third_line(
            "G10 L11 P2 R1.23456 I2.34567 J3.456789 Q3",
            "G10 L11 I2.346 J3.457 R1.235 P2 Q3",
            "",
        )
        self.compare_third_line(
            "G10 L20 P9 X1.23456 Y2.34567 Z3.456789",
            "G10 L20 X1.235 Y2.346 Z3.457 P9",
            "",
        )

    def test01170(self):
        """Test G17 command Generation."""
        self.compare_third_line("G17", "G17", "")

    def test01171(self):
        """Test G17.1 command Generation."""
        self.compare_third_line("G17.1", "G17.1", "")

    def test01180(self):
        """Test G18 command Generation."""
        self.compare_third_line("G18", "G18", "")

    def test01181(self):
        """Test G18.1 command Generation."""
        self.compare_third_line("G18.1", "G18.1", "")

    def test01190(self):
        """Test G19 command Generation."""
        self.compare_third_line("G19", "G19", "")

    def test01191(self):
        """Test G19.1 command Generation."""
        self.compare_third_line("G19.1", "G19.1", "")

    def test01200(self):
        """Test G20 command Generation."""
        self.compare_third_line("G20", "G20", "")

    def test01210(self):
        """Test G21 command Generation."""
        self.compare_third_line("G21", "G21", "")

    def test01280(self):
        """Test G28 command Generation."""
        self.compare_third_line("G28", "G28", "")
        self.compare_third_line(
            "G28 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            "G28 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000",
            "",
        )

    def test01281(self):
        """Test G28.1 command Generation."""
        self.compare_third_line("G28.1", "G28.1", "")

    def test01300(self):
        """Test G30 command Generation."""
        self.compare_third_line("G30", "G30", "")
        self.compare_third_line(
            "G30 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            "G30 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000",
            "",
        )

    def test01382(self):
        """Test G38.2 command Generation."""
        self.compare_third_line(
            "G38.2 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            "G38.2 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F7380.000",
            "",
        )

    def test01383(self):
        """Test G38.3 command Generation."""
        self.compare_third_line(
            "G38.3 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            "G38.3 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F7380.000",
            "",
        )

    def test01384(self):
        """Test G38.4 command Generation."""
        self.compare_third_line(
            "G38.4 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            "G38.4 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F7380.000",
            "",
        )

    def test01385(self):
        """Test G38.5 command Generation."""
        self.compare_third_line(
            "G38.5 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            "G38.5 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F7380.000",
            "",
        )

    def test01301(self):
        """Test G30.1 command Generation."""
        self.compare_third_line("G30.1", "G30.1", "")

    def test01400(self):
        """Test G40 command Generation."""
        self.compare_third_line("G40", "G40", "")
        self.compare_third_line("G40", "G40", "--inches")

    def test01410(self):
        """Test G41 command Generation."""
        self.compare_third_line("G41 D1.23456", "G41 D1", "")
        self.compare_third_line("G41 D0", "G41 D0", "")
        self.compare_third_line("G41 D1.23456", "G41 D1", "--inches")

    def test01411(self):
        """Test G41.1 command Generation."""
        self.compare_third_line("G41.1 D1.23456 L3", "G41.1 D1.235 L3", "")
        self.compare_third_line("G41.1 D1.23456 L3", "G41.1 D0.0486 L3", "--inches")

    def test01420(self):
        """Test G42 command Generation."""
        self.compare_third_line("G42 D1.23456", "G42 D1", "")
        self.compare_third_line("G42 D0", "G42 D0", "")
        self.compare_third_line("G42 D1.23456", "G42 D1", "--inches")

    def test01421(self):
        """Test G42.1 command Generation."""
        self.compare_third_line("G42.1 D1.23456 L3", "G42.1 D1.235 L3", "")
        self.compare_third_line("G42.1 D1.23456 L3", "G42.1 D0.0486 L3", "--inches")

    def test01430(self):
        """Test G43 command Generation."""
        self.compare_third_line("G43", "G43", "")
        self.compare_third_line("G43 H1.23456", "G43 H1", "")
        self.compare_third_line("G43 H0", "G43 H0", "")
        self.compare_third_line("G43 H1.23456", "G43 H1", "--inches")

    def test01431(self):
        """Test G43.1 command Generation."""
        self.compare_third_line(
            "G43.1 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 U7.891234 V8.912345 W9.123456",
            "G43.1 X1.235 Y2.346 Z3.457 A4.568 B5.679 C6.789 U7.891 V8.912 W9.123",
            "",
        )
        self.compare_third_line(
            "G43.1 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 U7.891234 V8.912345 W9.123456",
            "G43.1 X0.0486 Y0.0923 Z0.1361 A0.1798 B0.2236 C0.2673 U0.3107 V0.3509 W0.3592",
            "--inches",
        )

    def test01432(self):
        """Test G43.2 command Generation."""
        self.compare_third_line("G43.2 H1.23456", "G43.2 H1", "")

    def test01490(self):
        """Test G49 command Generation."""
        self.compare_third_line("G49", "G49", "")

    def test01520(self):
        """Test G52 command Generation."""
        self.single_compare(
            [
                Path.Command(
                    "G52 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 U7.891234 V8.912345 W9.123456"
                ),
                Path.Command(
                    "G52 X0 Y0.0 Z0.00 A0.000 B0.0000 C0.00000 U0.000000 V0 W0"
                ),
            ],
            """G90
G21
G52 X1.235 Y2.346 Z3.457 A4.568 B5.679 C6.789 U7.891 V8.912 W9.123
G52 X0.000 Y0.000 Z0.000 A0.000 B0.000 C0.000 U0.000 V0.000 W0.000
""",
            "",
        )
        self.single_compare(
            [
                Path.Command(
                    "G52 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 U7.891234 V8.912345 W9.123456"
                ),
                Path.Command(
                    "G52 X0 Y0.0 Z0.00 A0.000 B0.0000 C0.00000 U0.000000 V0 W0"
                ),
            ],
            """G90
G20
G52 X0.0486 Y0.0923 Z0.1361 A0.1798 B0.2236 C0.2673 U0.3107 V0.3509 W0.3592
G52 X0.0000 Y0.0000 Z0.0000 A0.0000 B0.0000 C0.0000 U0.0000 V0.0000 W0.0000
""",
            "--inches",
        )

    #     def test01530(self):
    #         """Test G53 command Generation."""
    #         #
    #         # G53 is handled differently in different gcode interpreters.
    #         # It always means "absolute machine coordinates", but it is
    #         # used like G0 in Centroid and Mach4, and used in front of
    #         # G0 or G1 on the same line in Fanuc, Grbl, LinuxCNC, and Tormach.
    #         # It is not modal in any gcode interpreter I currently know about.
    #         # The current FreeCAD code treats G53 as modal (like G54-G59.9).
    #         # The current refactored postprocessor code does not
    #         # handle having two G-commands on the same line.
    #         #
    #         c = Path.Command("G53 G0 X10 Y20 Z30 A40 B50 C60 U70 V80 W90")

    #         self.docobj.Path = Path.Path([c])
    #         postables = [self.docobj]

    #         expected = """G90
    # G21
    # G53 G0 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000
    # """
    #         args = ""
    #         gcode = postprocessor.export(postables, "gcode.tmp", args)
    #         print("--------\n" + gcode + "--------\n")
    #         self.assertEqual(gcode, expected)

    def test01540(self):
        """Test G54 command Generation."""
        self.compare_third_line("G54", "G54", "")

    def test01541(self):
        """Test G54.1 command Generation."""
        #
        # Some gcode interpreters use G54.1 P- to select additional
        # work coordinate systems.
        #
        self.compare_third_line("G54.1 P2.34567", "G54.1 P2", "")

    def test01550(self):
        """Test G55 command Generation."""
        self.compare_third_line("G55", "G55", "")

    def test01560(self):
        """Test G56 command Generation."""
        self.compare_third_line("G56", "G56", "")

    def test01570(self):
        """Test G57 command Generation."""
        self.compare_third_line("G57", "G57", "")

    def test01580(self):
        """Test G58 command Generation."""
        self.compare_third_line("G58", "G58", "")

    def test01590(self):
        """Test G59 command Generation."""
        self.compare_third_line("G59", "G59", "")
        #
        # Some gcode interpreters use G59 P- to select additional
        # work coordinate systems.  This is considered somewhat
        # obsolete and is being replaced by G54.1 P- instead.
        #
        self.compare_third_line("G59 P2.34567", "G59 P2", "")

    def test01591(self):
        """Test G59.1 command Generation."""
        self.compare_third_line("G59.1", "G59.1", "")

    def test01592(self):
        """Test G59.2 command Generation."""
        self.compare_third_line("G59.2", "G59.2", "")

    def test01593(self):
        """Test G59.3 command Generation."""
        self.compare_third_line("G59.3", "G59.3", "")

    def test01594(self):
        """Test G59.4 command Generation."""
        self.compare_third_line("G59.4", "G59.4", "")

    def test01595(self):
        """Test G59.5 command Generation."""
        self.compare_third_line("G59.5", "G59.5", "")

    def test01596(self):
        """Test G59.6 command Generation."""
        self.compare_third_line("G59.6", "G59.6", "")

    def test01597(self):
        """Test G59.7 command Generation."""
        self.compare_third_line("G59.7", "G59.7", "")

    def test01598(self):
        """Test G59.8 command Generation."""
        self.compare_third_line("G59.8", "G59.8", "")

    def test01599(self):
        """Test G59.9 command Generation."""
        self.compare_third_line("G59.9", "G59.9", "")

    def test01610(self):
        """Test G61 command Generation."""
        self.compare_third_line("G61", "G61", "")

    def test01611(self):
        """Test G61.1 command Generation."""
        self.compare_third_line("G61.1", "G61.1", "")

    def test01640(self):
        """Test G64 command Generation."""
        self.compare_third_line("G64", "G64", "")
        self.compare_third_line("G64 P3.456789", "G64 P3.457", "")
        self.compare_third_line("G64 P3.456789 Q4.567891", "G64 P3.457 Q4.568", "")
        self.compare_third_line(
            "G64 P3.456789 Q4.567891", "G64 P0.1361 Q0.1798", "--inches"
        )

    def test01730(self):
        """Test G73 command Generation."""
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G73 X1 Y2 Z0 F123 Q1.5 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G73 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000
G80
G90
""",
            "",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z3.750
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z2.250
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z0.750
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G73 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z3.750
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z2.250
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z0.750
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )
        #
        # Re-initialize all of the values before doing more tests.
        #
        postprocessor.init_values(postprocessor.values)
        #
        # Test translate_drill with G83 and G91.
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G91"),
            Path.Command("G99"),
            Path.Command("G73 X1 Y2 Z0 F123 Q1.5 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G73 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000
G80
G90
""",
            "--no-comments --no-translate_drill",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z11.500 F7380.000
G0 Z11.750
G0 Z11.575
G1 Z10.000 F7380.000
G0 Z10.250
G0 Z10.075
G1 Z8.500 F7380.000
G0 Z8.750
G0 Z8.575
G1 Z8.000 F7380.000
G0 Z13.000
G91
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G73 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000 )
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z11.500 F7380.000
G0 Z11.750
G0 Z11.575
G1 Z10.000 F7380.000
G0 Z10.250
G0 Z10.075
G1 Z8.500 F7380.000
G0 Z8.750
G0 Z8.575
G1 Z8.000 F7380.000
G0 Z13.000
G91
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )

    def test01810(self):
        """Test G81 command Generation."""
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G81 X1 Y2 Z0 F123 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G81 X1.000 Y2.000 Z0.000 R5.000 F7380.000
G80
G90
""",
            "",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G0 Z5.000
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G81 X1.000 Y2.000 Z0.000 R5.000 F7380.000 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )
        #
        # Re-initialize all of the values before doing more tests.
        #
        postprocessor.init_values(postprocessor.values)
        #
        # Test translate_drill with G81 and G91.
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G91"),
            Path.Command("G99"),
            Path.Command("G81 X1 Y2 Z0 F123 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G81 X1.000 Y2.000 Z0.000 R5.000 F7380.000
G80
G90
""",
            "--no-comments --no-translate_drill",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z8.000 F7380.000
G0 Z13.000
G91
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G81 X1.000 Y2.000 Z0.000 R5.000 F7380.000 )
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z8.000 F7380.000
G0 Z13.000
G91
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )

    def test01820(self):
        """Test G82 command Generation."""
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G82 X1 Y2 Z0 F123 R5 P1.23456"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G82 X1.000 Y2.000 Z0.000 R5.000 P1.23456 F7380.000
G80
G90
""",
            "",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G4 P1.23456
G0 Z5.000
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G82 X1.000 Y2.000 Z0.000 R5.000 P1.23456 F7380.000 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G4 P1.23456
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )
        #
        # Re-initialize all of the values before doing more tests.
        #
        postprocessor.init_values(postprocessor.values)
        #
        # Test translate_drill with G82 and G91.
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G91"),
            Path.Command("G99"),
            Path.Command("G82 X1 Y2 Z0 F123 R5 P1.23456"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G82 X1.000 Y2.000 Z0.000 R5.000 P1.23456 F7380.000
G80
G90
""",
            "--no-comments --no-translate_drill",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z8.000 F7380.000
G4 P1.23456
G0 Z13.000
G91
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G82 X1.000 Y2.000 Z0.000 R5.000 P1.23456 F7380.000 )
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z8.000 F7380.000
G4 P1.23456
G0 Z13.000
G91
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )

    def test01830(self):
        """Test G83 command Generation."""
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G90"),
            Path.Command("G99"),
            Path.Command("G83 X1 Y2 Z0 F123 Q1.5 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G83 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000
G80
G90
""",
            "",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z5.000
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z5.000
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z5.000
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G83 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z3.500 F7380.000
G0 Z5.000
G0 Z3.575
G1 Z2.000 F7380.000
G0 Z5.000
G0 Z2.075
G1 Z0.500 F7380.000
G0 Z5.000
G0 Z0.575
G1 Z0.000 F7380.000
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )
        #
        # Re-initialize all of the values before doing more tests.
        #
        postprocessor.init_values(postprocessor.values)
        #
        # Test translate_drill with G83 and G91.
        path = [
            Path.Command("G0 X1 Y2"),
            Path.Command("G0 Z8"),
            Path.Command("G91"),
            Path.Command("G99"),
            Path.Command("G83 X1 Y2 Z0 F123 Q1.5 R5"),
            Path.Command("G80"),
            Path.Command("G90"),
        ]
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G83 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000
G80
G90
""",
            "--no-comments --no-translate_drill",
        )
        self.single_compare(
            path,
            """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z11.500 F7380.000
G0 Z13.000
G0 Z11.575
G1 Z10.000 F7380.000
G0 Z13.000
G0 Z10.075
G1 Z8.500 F7380.000
G0 Z13.000
G0 Z8.575
G1 Z8.000 F7380.000
G0 Z13.000
G91
G90
""",
            "--translate_drill",
        )
        self.single_compare(
            path,
            """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G83 X1.000 Y2.000 Z0.000 R5.000 Q1.500 F7380.000 )
G90
G0 Z13.000
G0 X2.000 Y4.000
G1 Z11.500 F7380.000
G0 Z13.000
G0 Z11.575
G1 Z10.000 F7380.000
G0 Z13.000
G0 Z10.075
G1 Z8.500 F7380.000
G0 Z13.000
G0 Z8.575
G1 Z8.000 F7380.000
G0 Z13.000
G91
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
""",
            "--comments --translate_drill",
        )

    def test01900(self):
        """Test G90 command Generation."""
        self.compare_third_line("G90", "G90", "")

    def test01901(self):
        """Test G90.1 command Generation."""
        self.compare_third_line("G90.1", "G90.1", "")

    def test01910(self):
        """Test G91 command Generation."""
        self.compare_third_line("G91", "G91", "")

    def test01911(self):
        """Test G91.1 command Generation."""
        self.compare_third_line("G91.1", "G91.1", "")

    def test01920(self):
        """Test G92 command Generation."""
        self.compare_third_line(
            "G92 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            "G92 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000",
            "",
        )

    def test01921(self):
        """Test G92.1 command Generation."""
        self.compare_third_line("G92.1", "G92.1", "")

    def test01922(self):
        """Test G92.2 command Generation."""
        self.compare_third_line("G92.2", "G92.2", "")

    def test01923(self):
        """Test G92.3 command Generation."""
        self.compare_third_line("G92.3", "G92.3", "")

    def test01930(self):
        """Test G93 command Generation."""
        self.compare_third_line("G93", "G93", "")

    def test01940(self):
        """Test G94 command Generation."""
        self.compare_third_line("G94", "G94", "")

    def test01950(self):
        """Test G95 command Generation."""
        self.compare_third_line("G95", "G95", "")

    def test01980(self):
        """Test G98 command Generation."""
        self.compare_third_line("G98", "G98", "")

    def test01990(self):
        """Test G99 command Generation."""
        self.compare_third_line("G99", "G99", "")

    def test02000(self):
        """Test M0 command Generation."""
        self.compare_third_line("M0", "M0", "")
        self.compare_third_line("M00", "M00", "")

    def test02010(self):
        """Test M1 command Generation."""
        self.compare_third_line("M1", "M1", "")
        self.compare_third_line("M01", "M01", "")

    def test02020(self):
        """Test M2 command Generation."""
        self.compare_third_line("M2", "M2", "")
        self.compare_third_line("M02", "M02", "")

    def test02030(self):
        """Test M3 command Generation."""
        self.compare_third_line("M3", "M3", "")
        self.compare_third_line("M03", "M03", "")

    def test02040(self):
        """Test M4 command Generation."""
        self.compare_third_line("M4", "M4", "")
        self.compare_third_line("M04", "M04", "")

    def test02050(self):
        """Test M5 command Generation."""
        self.compare_third_line("M5", "M5", "")
        self.compare_third_line("M05", "M05", "")

    def test02060(self):
        """Test M6 command Generation."""
        self.compare_third_line("M6", "M6", "")
        self.compare_third_line("M06", "M06", "")

    def test02070(self):
        """Test M7 command Generation."""
        self.compare_third_line("M7", "M7", "")
        self.compare_third_line("M07", "M07", "")

    def test02080(self):
        """Test M8 command Generation."""
        self.compare_third_line("M8", "M8", "")
        self.compare_third_line("M08", "M08", "")

    def test02090(self):
        """Test M9 command Generation."""
        self.compare_third_line("M9", "M9", "")
        self.compare_third_line("M09", "M09", "")

    def test02300(self):
        """Test M30 command Generation."""
        self.compare_third_line("M30", "M30", "")

    def test02480(self):
        """Test M48 command Generation."""
        self.compare_third_line("M48", "M48", "")

    def test02490(self):
        """Test M49 command Generation."""
        self.compare_third_line("M49", "M49", "")

    def test02600(self):
        """Test M60 command Generation."""
        self.compare_third_line("M60", "M60", "")
