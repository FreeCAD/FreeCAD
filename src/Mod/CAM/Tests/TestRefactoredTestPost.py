# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022-2023 Larry Woestman <LarryWoestman2@gmail.com>     *
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
import Tests.PathTestUtils as PathTestUtils
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
        postprocessor.init_values(postprocessor.global_values)

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
        gcode = postprocessor.export(postables, "-", args)
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
        gcode = postprocessor.export(postables, "-", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[2], expected)

    #############################################################################
    #
    # The tests are organized into groups:
    #
    #   00000 - 00099  tests that don't fit any other category
    #   00100 - 00999  tests for all of the various arguments/options
    #   01000 - 01999  tests for the various G codes at 1000 + 10 * g_code_value
    #   02000 - 02999  tests for the various M codes at 2000 + 10 * m_code_value
    #
    #############################################################################

    def test00100(self):
        """Test axis modal.

        Suppress the axis coordinate if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]

        args = "--axis-modal"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 Y30.000")

        args = "--no-axis-modal"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 X10.000 Y30.000 Z30.000")

    #############################################################################

    def test00110(self):
        """Test axis-precision."""
        self.compare_third_line(
            "G0 X10 Y20 Z30", "G0 X10.00 Y20.00 Z30.00", "--axis-precision=2"
        )

    #############################################################################

    def test00120(self):
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

    #############################################################################

    def test00130(self):
        """Test comments."""
        c = Path.Command("(comment)")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = "--comments"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[4], "(comment)")

    #############################################################################

    def test00140(self):
        """Test feed-precision."""
        #
        c = Path.Command("G1 X10 Y20 Z30 F123.123456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = ""
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.407")

        args = "--feed-precision=2"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        # Note:  The "internal" F speed is in mm/s,
        #        while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.41")

    #############################################################################

    def test00150(self):
        """Test output with an empty path.

        Also tests the interactions between --comments and --header.
        """
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        # Test generating with comments and header.
        # The header contains a time stamp that messes up unit testing.
        # Only test the length of the line that contains the time.
        args = "--comments --header"
        gcode = postprocessor.export(postables, "-", args)
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
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test without comments with header.
        args = "--no-comments --header"
        gcode = postprocessor.export(postables, "-", args)
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
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    #############################################################################

    def test00160(self):
        """Test Line Numbers."""
        self.compare_third_line(
            "G0 X10 Y20 Z30", "N120 G0 X10.000 Y20.000 Z30.000", "--line-numbers"
        )

    #############################################################################

    def test00170(self):
        """Test inches."""
        #
        c = Path.Command("G0 X10 Y20 Z30 A10 B20 C30 U10 V20 W30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = "--inches"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[1], "G20")
        self.assertEqual(
            gcode.splitlines()[2],
            "G0 X0.3937 Y0.7874 Z1.1811 A0.3937 B0.7874 C1.1811 U0.3937 V0.7874 W1.1811",
        )

    #############################################################################

    def test00180(self):
        """Test modal.

        Suppress the command name if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")
        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]
        args = "--modal"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "X10.000 Y30.000 Z30.000")
        args = "--no-modal"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[3], "G0 X10.000 Y30.000 Z30.000")

    #############################################################################

    def test00190(self):
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
            postables, "-", "--output_all_arguments"
        )
        # The argparse help routine turns out to be sensitive to the
        # number of columns in the terminal window that the tests
        # are run from.  This affects the indenting in the output.
        # The next couple of lines remove all of the white space.
        gcode = "".join(gcode.split())
        expected = "".join(expected.split())
        self.assertEqual(gcode, expected)

    #############################################################################

    def test00200(self):
        """Test Outputting visible arguments.

        Empty path.  Outputs visible arguments.
        """
        self.single_compare([], "", "--output_visible_arguments")

    #############################################################################

    def test00210(self):
        """Test Post-amble."""
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--postamble='G0 Z50\nM2'"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[-2], "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    #############################################################################

    def test00220(self):
        """Test Pre-amble."""
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--preamble='G18 G55'"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[0], "G18 G55")

    #############################################################################

    def test00230(self):
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

    #############################################################################

    def test00240(self):
        """Test return-to."""
        self.compare_third_line("", "G0 X12 Y34 Z56", "--return-to='12,34,56'")

    #############################################################################

    def test00250(self):
        """Test tlo."""
        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c, c2])
        postables = [self.docobj]
        args = "--tlo"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "G43 H2")
        self.assertEqual(gcode.splitlines()[4], "M3 S3000")
        # suppress TLO
        args = "--no-tlo"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "M3 S3000")

    #############################################################################

    def test00260(self):
        """Test tool_change."""
        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c, c2])
        postables = [self.docobj]
        args = "--tool_change"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M6 T2")
        self.assertEqual(gcode.splitlines()[3], "M3 S3000")
        args = "--comments --no-tool_change"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[5], "( M6 T2 )")
        self.assertEqual(gcode.splitlines()[6], "M3 S3000")

    #############################################################################

    def test00270(self):
        """Test wait-for-spindle."""
        c = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        args = ""
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M3 S3000")
        args = "--wait-for-spindle=1.23456"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M3 S3000")
        self.assertEqual(gcode.splitlines()[3], "G4 P1.23456")
        c = Path.Command("M4 S3000")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]
        # This also tests that the default for --wait-for-spindle
        # goes back to 0.0 (no wait)
        args = ""
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M4 S3000")
        args = "--wait-for-spindle=1.23456"
        gcode = postprocessor.export(postables, "-", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "M4 S3000")
        self.assertEqual(gcode.splitlines()[3], "G4 P1.23456")
