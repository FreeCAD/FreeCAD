# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2023 Larry Woestman <LarryWoestman2@gmail.com>          *
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
import CAMTests.PathTestUtils as PathTestUtils
from importlib import reload
from Path.Post.scripts import linuxcnc_legacy_post as postprocessor

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestLinuxCNCLegacyPost(PathTestUtils.PathTestBase):
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
        reload(
            postprocessor
        )  # technical debt.  This shouldn't be necessary but here to bypass a bug

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        FreeCAD.ActiveDocument.removeObject("testpath")

    def compare_sixth_line(self, path_string, expected, args, debug=False):
        """Perform a test with a single comparison to the sixth line of the output."""
        nl = "\n"
        if path_string:
            self.docobj.Path = Path.Path([Path.Command(path_string)])
        else:
            self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        gcode = postprocessor.export(postables, "-", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[5], expected)

    def test000(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        # Test generating with header
        # Header contains a time stamp that messes up unit testing.
        # Only test length of result.
        args = "--no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        self.assertTrue(len(gcode.splitlines()) == 13)

        # Test without header
        expected = """(begin preamble)
G17 G54 G40 G49 G80 G90
G21
(begin operation: testpath)
(machine units: mm/min)
(finish operation: testpath)
(begin postamble)
M05
G17 G54 G90 G80 G40
M2
"""

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--no-header --no-show-editor"
        # args = ("--no-header --no-comments --no-show-editor --precision=2")
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """G17 G54 G40 G49 G80 G90
G21
M05
G17 G54 G90 G80 G40
M2
"""

        args = "--no-header --no-comments --no-show-editor"
        # args = ("--no-header --no-comments --no-show-editor --precision=2")
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode, expected)

    def test010(self):
        """Test command Generation.
        Test Precision
        Test imperial / inches
        """
        self.compare_sixth_line(
            "G0 X10 Y20 Z30", "G0 X10.000 Y20.000 Z30.000 ", "--no-header --no-show-editor"
        )
        self.compare_sixth_line(
            "G0 X10 Y20 Z30",
            "G0 X10.00 Y20.00 Z30.00 ",
            "--no-header --precision=2 --no-show-editor",
        )

    def test020(self):
        """
        Test Line Numbers
        """
        self.compare_sixth_line(
            "G0 X10 Y20 Z30",
            "N160  G0 X10.000 Y20.000 Z30.000 ",
            "--no-header --line-numbers --no-show-editor",
        )

    def test030(self):
        """
        Test Pre-amble
        """

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--no-header --no-comments --preamble='G18 G55' --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        result = gcode.splitlines()[0]
        self.assertEqual(result, "G18 G55")

    def test040(self):
        """
        Test Post-amble
        """
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--no-header --no-comments --postamble='G0 Z50\nM2' --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        result = gcode.splitlines()[-2]
        self.assertEqual(result, "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    def test050(self):
        """
        Test inches
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--no-header --inches --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode.splitlines()[2], "G20")

        result = gcode.splitlines()[5]
        expected = "G0 X0.3937 Y0.7874 Z1.1811 "
        self.assertEqual(result, expected)

        # Technical debt.   The following test fails.  Precision not working
        # with imperial units.

        # args = ("--no-header --inches --precision=2")
        # gcode = postprocessor.export(postables, "-", args)
        # result = gcode.splitlines()[5]
        # expected = "G0 X0.39 Y0.78 Z1.18 "
        # self.assertEqual(result, expected)

    def test060(self):
        """
        Test test modal
        Suppress the command name if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]

        args = "--no-header --modal --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        result = gcode.splitlines()[6]
        expected = "X10.000 Y30.000 Z30.000 "
        self.assertEqual(result, expected)

    def test070(self):
        """
        Test axis modal
        Suppress the axis coordinate if the same as previous
        """
        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.docobj.Path = Path.Path([c, c1])
        postables = [self.docobj]

        args = "--no-header --axis-modal --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        result = gcode.splitlines()[6]
        expected = "G0 Y30.000 "
        self.assertEqual(result, expected)

    def test080(self):
        """
        Test tool change
        """
        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")
        self.docobj.Path = Path.Path([c, c2])
        postables = [self.docobj]

        args = "--no-header --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode.splitlines()[5], "M5")
        self.assertEqual(gcode.splitlines()[6], "M6 T2 ")
        self.assertEqual(gcode.splitlines()[7], "G43 H2 ")
        self.assertEqual(gcode.splitlines()[8], "M3 S3000 ")

        # suppress TLO
        args = "--no-header --no-tlo --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode.splitlines()[7], "M3 S3000 ")

    def test090(self):
        """
        Test comment
        """
        self.compare_sixth_line("(comment)", "(comment) ", "--no-header --no-show-editor")

    def test100(self):
        """Test A, B, & C axis output for values between 0 and 90 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A40 B50 C60",
            "G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A40 B50 C60",
            "G1 X0.3937 Y0.7874 Z1.1811 A40.0000 B50.0000 C60.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test110(self):
        """Test A, B, & C axis output for 89 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A89 B89 C89",
            "G1 X10.000 Y20.000 Z30.000 A89.000 B89.000 C89.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A89 B89 C89",
            "G1 X0.3937 Y0.7874 Z1.1811 A89.0000 B89.0000 C89.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test120(self):
        """Test A, B, & C axis output for 90 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A90 B90 C90",
            "G1 X10.000 Y20.000 Z30.000 A90.000 B90.000 C90.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A90 B90 C90",
            "G1 X0.3937 Y0.7874 Z1.1811 A90.0000 B90.0000 C90.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test130(self):
        """Test A, B, & C axis output for 91 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A91 B91 C91",
            "G1 X10.000 Y20.000 Z30.000 A91.000 B91.000 C91.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A91 B91 C91",
            "G1 X0.3937 Y0.7874 Z1.1811 A91.0000 B91.0000 C91.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test140(self):
        """Test A, B, & C axis output for values between 90 and 180 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A100 B110 C120",
            "G1 X10.000 Y20.000 Z30.000 A100.000 B110.000 C120.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A100 B110 C120",
            "G1 X0.3937 Y0.7874 Z1.1811 A100.0000 B110.0000 C120.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test150(self):
        """Test A, B, & C axis output for values between 180 and 360 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A240 B250 C260",
            "G1 X10.000 Y20.000 Z30.000 A240.000 B250.000 C260.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A240 B250 C260",
            "G1 X0.3937 Y0.7874 Z1.1811 A240.0000 B250.0000 C260.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test160(self):
        """Test A, B, & C axis output for values greater than 360 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A440 B450 C460",
            "G1 X10.000 Y20.000 Z30.000 A440.000 B450.000 C460.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A440 B450 C460",
            "G1 X0.3937 Y0.7874 Z1.1811 A440.0000 B450.0000 C460.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test170(self):
        """Test A, B, & C axis output for values between 0 and -90 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-40 B-50 C-60",
            "G1 X10.000 Y20.000 Z30.000 A-40.000 B-50.000 C-60.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-40 B-50 C-60",
            "G1 X0.3937 Y0.7874 Z1.1811 A-40.0000 B-50.0000 C-60.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test180(self):
        """Test A, B, & C axis output for values between -90 and -180 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-100 B-110 C-120",
            "G1 X10.000 Y20.000 Z30.000 A-100.000 B-110.000 C-120.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-100 B-110 C-120",
            "G1 X0.3937 Y0.7874 Z1.1811 A-100.0000 B-110.0000 C-120.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test190(self):
        """Test A, B, & C axis output for values between -180 and -360 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-240 B-250 C-260",
            "G1 X10.000 Y20.000 Z30.000 A-240.000 B-250.000 C-260.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-240 B-250 C-260",
            "G1 X0.3937 Y0.7874 Z1.1811 A-240.0000 B-250.0000 C-260.0000 ",
            "--no-header --inches --no-show-editor",
        )

    def test200(self):
        """Test A, B, & C axis output for values below -360 degrees"""
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-440 B-450 C-460",
            "G1 X10.000 Y20.000 Z30.000 A-440.000 B-450.000 C-460.000 ",
            "--no-header --no-show-editor",
        )
        self.compare_sixth_line(
            "G1 X10 Y20 Z30 A-440 B-450 C-460",
            "G1 X0.3937 Y0.7874 Z1.1811 A-440.0000 B-450.0000 C-460.0000 ",
            "--no-header --inches --no-show-editor",
        )
