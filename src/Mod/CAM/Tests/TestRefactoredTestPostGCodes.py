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


class TestRefactoredTestPostGCodes(PathTestUtils.PathTestBase):
    """Test the refactored_test_post.py postprocessor G codes."""

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
    #   00100 - 09999  tests for all of the various arguments/options
    #   10000 - 19999  tests for the various G codes at 10000 + 10 * g_code_value
    #   20000 - 29999  tests for the various M codes at 20000 + 10 * m_code_value
    #
    #############################################################################

    def test10000(self):
        """Test G0 command Generation."""
        self.compare_third_line(
            "G0 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            (
                "G0 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000"
            ),
            "",
        )
        self.compare_third_line(
            "G00 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            (
                "G00 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000"
            ),
            "",
        )

    #############################################################################

    def test10010(self):
        """Test G1 command Generation."""
        self.compare_third_line(
            "G1 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            (
                "G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F74.074"
            ),
            "",
        )
        self.compare_third_line(
            "G01 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            (
                "G01 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F74.074"
            ),
            "",
        )
        # Test argument order
        self.compare_third_line(
            "G1 F1.23456 Z30 V80 C60 W90 X10 B50 U70 Y20 A40",
            (
                "G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F74.074"
            ),
            "",
        )
        self.compare_third_line(
            "G1 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456",
            (
                "G1 X0.3937 Y0.7874 Z1.1811 A1.5748 B1.9685 C2.3622 "
                "U2.7559 V3.1496 W3.5433 F2.9163"
            ),
            "--inches",
        )

    #############################################################################

    def test10020(self):
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

    #############################################################################

    def test10030(self):
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

    #############################################################################

    def test10040(self):
        """Test G4 command Generation."""
        # Should some sort of "precision" be applied to the P parameter?
        # The code as currently written does not do so intentionally.
        # The P parameter indicates "time to wait" where a 0.001 would
        # be a millisecond wait, so more than 3 or 4 digits of precision
        # might be useful.
        self.compare_third_line("G4 P1.23456", "G4 P1.23456", "")
        self.compare_third_line("G04 P1.23456", "G04 P1.23456", "")
        self.compare_third_line("G4 P1.23456", "G4 P1.23456", "--inches")

    #############################################################################

    def test10070(self):
        """Test G7 command Generation."""
        self.compare_third_line("G7", "G7", "")

    #############################################################################

    def test10080(self):
        """Test G8 command Generation."""
        self.compare_third_line("G8", "G8", "")

    #############################################################################

    def test10100(self):
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

    #############################################################################

    def test10170(self):
        """Test G17 command Generation."""
        self.compare_third_line("G17", "G17", "")

    #############################################################################

    def test10171(self):
        """Test G17.1 command Generation."""
        self.compare_third_line("G17.1", "G17.1", "")

    #############################################################################

    def test10180(self):
        """Test G18 command Generation."""
        self.compare_third_line("G18", "G18", "")

    #############################################################################

    def test10181(self):
        """Test G18.1 command Generation."""
        self.compare_third_line("G18.1", "G18.1", "")

    #############################################################################

    def test10190(self):
        """Test G19 command Generation."""
        self.compare_third_line("G19", "G19", "")

    #############################################################################

    def test10191(self):
        """Test G19.1 command Generation."""
        self.compare_third_line("G19.1", "G19.1", "")

    #############################################################################

    def test10200(self):
        """Test G20 command Generation."""
        self.compare_third_line("G20", "G20", "")

    #############################################################################

    def test10210(self):
        """Test G21 command Generation."""
        self.compare_third_line("G21", "G21", "")

    #############################################################################

    def test10280(self):
        """Test G28 command Generation."""
        self.compare_third_line("G28", "G28", "")
        self.compare_third_line(
            "G28 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            (
                "G28 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000"
            ),
            "",
        )

    #############################################################################

    def test10281(self):
        """Test G28.1 command Generation."""
        self.compare_third_line("G28.1", "G28.1", "")

    #############################################################################

    def test10300(self):
        """Test G30 command Generation."""
        self.compare_third_line("G30", "G30", "")
        self.compare_third_line(
            "G30 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            (
                "G30 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000"
            ),
            "",
        )

    #############################################################################

    def test10382(self):
        """Test G38.2 command Generation."""
        self.compare_third_line(
            "G38.2 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            (
                "G38.2 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F7380.000"
            ),
            "",
        )

    #############################################################################

    def test10383(self):
        """Test G38.3 command Generation."""
        self.compare_third_line(
            "G38.3 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            (
                "G38.3 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F7380.000"
            ),
            "",
        )

    #############################################################################

    def test10384(self):
        """Test G38.4 command Generation."""
        self.compare_third_line(
            "G38.4 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            (
                "G38.4 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F7380.000"
            ),
            "",
        )

    #############################################################################

    def test10385(self):
        """Test G38.5 command Generation."""
        self.compare_third_line(
            "G38.5 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F123",
            (
                "G38.5 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000 F7380.000"
            ),
            "",
        )

    #############################################################################

    def test10301(self):
        """Test G30.1 command Generation."""
        self.compare_third_line("G30.1", "G30.1", "")

    #############################################################################

    def test10400(self):
        """Test G40 command Generation."""
        self.compare_third_line("G40", "G40", "")
        self.compare_third_line("G40", "G40", "--inches")

    #############################################################################

    def test10410(self):
        """Test G41 command Generation."""
        self.compare_third_line("G41 D1.23456", "G41 D1", "")
        self.compare_third_line("G41 D0", "G41 D0", "")
        self.compare_third_line("G41 D1.23456", "G41 D1", "--inches")

    #############################################################################

    def test10411(self):
        """Test G41.1 command Generation."""
        self.compare_third_line("G41.1 D1.23456 L3", "G41.1 D1.235 L3", "")
        self.compare_third_line("G41.1 D1.23456 L3", "G41.1 D0.0486 L3", "--inches")

    #############################################################################

    def test10420(self):
        """Test G42 command Generation."""
        self.compare_third_line("G42 D1.23456", "G42 D1", "")
        self.compare_third_line("G42 D0", "G42 D0", "")
        self.compare_third_line("G42 D1.23456", "G42 D1", "--inches")

    #############################################################################

    def test10421(self):
        """Test G42.1 command Generation."""
        self.compare_third_line("G42.1 D1.23456 L3", "G42.1 D1.235 L3", "")
        self.compare_third_line("G42.1 D1.23456 L3", "G42.1 D0.0486 L3", "--inches")

    #############################################################################

    def test10430(self):
        """Test G43 command Generation."""
        self.compare_third_line("G43", "G43", "")
        self.compare_third_line("G43 H1.23456", "G43 H1", "")
        self.compare_third_line("G43 H0", "G43 H0", "")
        self.compare_third_line("G43 H1.23456", "G43 H1", "--inches")

    #############################################################################

    def test10431(self):
        """Test G43.1 command Generation."""
        self.compare_third_line(
            (
                "G43.1 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 "
                "U7.891234 V8.912345 W9.123456"
            ),
            "G43.1 X1.235 Y2.346 Z3.457 A4.568 B5.679 C6.789 U7.891 V8.912 W9.123",
            "",
        )
        self.compare_third_line(
            (
                "G43.1 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 C6.789123 "
                "U7.891234 V8.912345 W9.123456"
            ),
            (
                "G43.1 X0.0486 Y0.0923 Z0.1361 A0.1798 B0.2236 C0.2673 "
                "U0.3107 V0.3509 W0.3592"
            ),
            "--inches",
        )

    #############################################################################

    def test10432(self):
        """Test G43.2 command Generation."""
        self.compare_third_line("G43.2 H1.23456", "G43.2 H1", "")

    #############################################################################

    def test10490(self):
        """Test G49 command Generation."""
        self.compare_third_line("G49", "G49", "")

    #############################################################################

    def test10520(self):
        """Test G52 command Generation."""
        self.single_compare(
            [
                Path.Command(
                    (
                        "G52 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 "
                        "C6.789123 U7.891234 V8.912345 W9.123456"
                    )
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
                    (
                        "G52 X1.234567 Y2.345678 Z3.456789 A4.567891 B5.678912 "
                        "C6.789123 U7.891234 V8.912345 W9.123456"
                    )
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

    #############################################################################

    #     def test10530(self):
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
    #         gcode = postprocessor.export(postables, "-", args)
    #         print("--------\n" + gcode + "--------\n")
    #         self.assertEqual(gcode, expected)

    #############################################################################

    def test10540(self):
        """Test G54 command Generation."""
        self.compare_third_line("G54", "G54", "")

    #############################################################################

    def test10541(self):
        """Test G54.1 command Generation."""
        #
        # Some gcode interpreters use G54.1 P- to select additional
        # work coordinate systems.
        #
        self.compare_third_line("G54.1 P2.34567", "G54.1 P2", "")

    #############################################################################

    def test10550(self):
        """Test G55 command Generation."""
        self.compare_third_line("G55", "G55", "")

    #############################################################################

    def test10560(self):
        """Test G56 command Generation."""
        self.compare_third_line("G56", "G56", "")

    #############################################################################

    def test10570(self):
        """Test G57 command Generation."""
        self.compare_third_line("G57", "G57", "")

    #############################################################################

    def test10580(self):
        """Test G58 command Generation."""
        self.compare_third_line("G58", "G58", "")

    #############################################################################

    def test10590(self):
        """Test G59 command Generation."""
        self.compare_third_line("G59", "G59", "")
        #
        # Some gcode interpreters use G59 P- to select additional
        # work coordinate systems.  This is considered somewhat
        # obsolete and is being replaced by G54.1 P- instead.
        #
        self.compare_third_line("G59 P2.34567", "G59 P2", "")

    #############################################################################

    def test10591(self):
        """Test G59.1 command Generation."""
        self.compare_third_line("G59.1", "G59.1", "")

    #############################################################################

    def test10592(self):
        """Test G59.2 command Generation."""
        self.compare_third_line("G59.2", "G59.2", "")

    #############################################################################

    def test10593(self):
        """Test G59.3 command Generation."""
        self.compare_third_line("G59.3", "G59.3", "")

    #############################################################################

    def test10594(self):
        """Test G59.4 command Generation."""
        self.compare_third_line("G59.4", "G59.4", "")

    #############################################################################

    def test10595(self):
        """Test G59.5 command Generation."""
        self.compare_third_line("G59.5", "G59.5", "")

    #############################################################################

    def test10596(self):
        """Test G59.6 command Generation."""
        self.compare_third_line("G59.6", "G59.6", "")

    #############################################################################

    def test10597(self):
        """Test G59.7 command Generation."""
        self.compare_third_line("G59.7", "G59.7", "")

    #############################################################################

    def test10598(self):
        """Test G59.8 command Generation."""
        self.compare_third_line("G59.8", "G59.8", "")

    #############################################################################

    def test10599(self):
        """Test G59.9 command Generation."""
        self.compare_third_line("G59.9", "G59.9", "")

    #############################################################################

    def test10610(self):
        """Test G61 command Generation."""
        self.compare_third_line("G61", "G61", "")

    #############################################################################

    def test10611(self):
        """Test G61.1 command Generation."""
        self.compare_third_line("G61.1", "G61.1", "")

    #############################################################################

    def test10640(self):
        """Test G64 command Generation."""
        self.compare_third_line("G64", "G64", "")
        self.compare_third_line("G64 P3.456789", "G64 P3.457", "")
        self.compare_third_line("G64 P3.456789 Q4.567891", "G64 P3.457 Q4.568", "")
        self.compare_third_line(
            "G64 P3.456789 Q4.567891", "G64 P0.1361 Q0.1798", "--inches"
        )

    #############################################################################

    def test10730(self):
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
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.global_values)
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

    #############################################################################

    def test10810(self):
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
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.global_values)
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

    #############################################################################

    def test10820(self):
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
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.global_values)
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

    #############################################################################

    def test10830(self):
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
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.global_values)
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

    #############################################################################

    def test10900(self):
        """Test G90 command Generation."""
        self.compare_third_line("G90", "G90", "")

    #############################################################################

    def test10901(self):
        """Test G90.1 command Generation."""
        self.compare_third_line("G90.1", "G90.1", "")

    #############################################################################

    def test10910(self):
        """Test G91 command Generation."""
        self.compare_third_line("G91", "G91", "")

    #############################################################################

    def test10911(self):
        """Test G91.1 command Generation."""
        self.compare_third_line("G91.1", "G91.1", "")

    #############################################################################

    def test10920(self):
        """Test G92 command Generation."""
        self.compare_third_line(
            "G92 X10 Y20 Z30 A40 B50 C60 U70 V80 W90",
            (
                "G92 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 "
                "U70.000 V80.000 W90.000"
            ),
            "",
        )

    #############################################################################

    def test10921(self):
        """Test G92.1 command Generation."""
        self.compare_third_line("G92.1", "G92.1", "")

    #############################################################################

    def test10922(self):
        """Test G92.2 command Generation."""
        self.compare_third_line("G92.2", "G92.2", "")

    #############################################################################

    def test10923(self):
        """Test G92.3 command Generation."""
        self.compare_third_line("G92.3", "G92.3", "")

    #############################################################################

    def test10930(self):
        """Test G93 command Generation."""
        self.compare_third_line("G93", "G93", "")

    #############################################################################

    def test10940(self):
        """Test G94 command Generation."""
        self.compare_third_line("G94", "G94", "")

    #############################################################################

    def test10950(self):
        """Test G95 command Generation."""
        self.compare_third_line("G95", "G95", "")

    #############################################################################

    def test10980(self):
        """Test G98 command Generation."""
        self.compare_third_line("G98", "G98", "")

    #############################################################################

    def test10990(self):
        """Test G99 command Generation."""
        self.compare_third_line("G99", "G99", "")
