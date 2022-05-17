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

from importlib import reload

import FreeCAD

# import Part
import Path
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils
from PathScripts.post import refactored_test_post as postprocessor


PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())


class TestRefactoredTestPost(PathTestUtils.PathTestBase):
    @classmethod
    def setUpClass(cls):
        """setUpClass()...

        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        # Open existing FreeCAD document with test geometry
        FreeCAD.newDocument("Unnamed")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # Close geometry document without saving
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
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
            gcode.splitlines()[1], "(Post Processor: PathScripts.post.refactored_test_post)"
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
            gcode.splitlines()[1], "(Post Processor: PathScripts.post.refactored_test_post)"
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

    def test00100(self):
        """Test bcnc."""
        #
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        expected = """G90
G21
(Block-name: testpath)
(Block-expand: 0)
(Block-enable: 1)
(Block-name: post_amble)
(Block-expand: 0)
(Block-enable: 1)
"""
        args = "--bcnc"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
G21
"""
        args = "--no-bcnc"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

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
        #
        c = Path.Command("G0 X10 Y20 Z30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--axis-precision=2"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "G0 X10.00 Y20.00 Z30.00")

    def test00130(self):
        """Test comments."""
        #
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
        # Note:  The "internal" F speed is in mm/s, while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.407")

        args = "--feed-precision=2"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        # Note:  The "internal" F speed is in mm/s, while the output F speed is in mm/min.
        self.assertEqual(gcode.splitlines()[2], "G1 X10.000 Y20.000 Z30.000 F7387.41")

    def test00150(self):
        """Test Line Numbers."""
        #
        c = Path.Command("G0 X10 Y20 Z30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--line-numbers"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "N120 G0 X10.000 Y20.000 Z30.000")

    def test00160(self):
        """Test inches."""
        #
        c = Path.Command("G0 X10 Y20 Z30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--inches"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[1], "G20")
        self.assertEqual(gcode.splitlines()[2], "G0 X0.3937 Y0.7874 Z1.1811")

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
        #
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--postamble='G0 Z50\nM2'"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[-2], "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    def test00190(self):
        """Test Pre-amble."""
        #
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--preamble='G18 G55'"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[0], "G18 G55")

    def test00200(self):
        """Test precision."""
        #
        c = Path.Command("G1 X10 Y20 Z30 F100")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--precision=2"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "G1 X10.00 Y20.00 Z30.00 F6000.00")

        args = "--inches --precision=2"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode.splitlines()[2], "G1 X0.39 Y0.79 Z1.18 F236.22")

    def test00210(self):
        """Test return-to."""
        #
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        expected = """G90
G21
G0 X12 Y34 Z56
"""
        args = "--return-to='12,34,56'"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00220(self):
        """Test tlo."""
        #
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
        #
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

    def test00240(self):
        """Test translate_drill with G81."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G90")
        c3 = Path.Command("G99")
        c4 = Path.Command("G81 X1 Y2 Z0 F123 R5")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G81 X1.000 Y2.000 Z0.000 F7380.000 R5.000
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G0 Z5.000
G90
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G81 X1.000 Y2.000 Z0.000 F7380.000 R5.000 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00250(self):
        """Test translate_drill with G82."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G90")
        c3 = Path.Command("G99")
        c4 = Path.Command("G82 X1 Y2 Z0 F123 R5 P1.23456")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G82 X1.000 Y2.000 Z0.000 F7380.000 R5.000 P1.23456
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
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
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G82 X1.000 Y2.000 Z0.000 F7380.000 R5.000 P1.23456 )
G0 X1.000 Y2.000
G1 Z5.000 F7380.000
G1 Z0.000 F7380.000
G4 P1.23456
G0 Z5.000
( G80 )
G90
(Finish operation: testpath)
(Begin postamble)
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00260(self):
        """Test translate_drill with G83."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G90")
        c3 = Path.Command("G99")
        c4 = Path.Command("G83 X1 Y2 Z0 F123 Q1.5 R5")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G90
G99
G83 X1.000 Y2.000 Z0.000 F7380.000 Q1.500 R5.000
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
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
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G90
( G99 )
( G83 X1.000 Y2.000 Z0.000 F7380.000 Q1.500 R5.000 )
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
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00270(self):
        """Test translate_drill with G81 and G91."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G91")
        c3 = Path.Command("G99")
        c4 = Path.Command("G81 X1 Y2 Z0 F123 R5")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G81 X1.000 Y2.000 Z0.000 F7380.000 R5.000
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
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
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G81 X1.000 Y2.000 Z0.000 F7380.000 R5.000 )
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
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00280(self):
        """Test translate_drill with G82 and G91."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G91")
        c3 = Path.Command("G99")
        c4 = Path.Command("G82 X1 Y2 Z0 F123 R5 P1.23456")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G82 X1.000 Y2.000 Z0.000 F7380.000 R5.000 P1.23456
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
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
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G82 X1.000 Y2.000 Z0.000 F7380.000 R5.000 P1.23456 )
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
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00290(self):
        """Test translate_drill with G83 and G91."""
        #
        c = Path.Command("G0 X1 Y2")
        c1 = Path.Command("G0 Z8")
        c2 = Path.Command("G91")
        c3 = Path.Command("G99")
        c4 = Path.Command("G83 X1 Y2 Z0 F123 Q1.5 R5")
        c5 = Path.Command("G80")
        c6 = Path.Command("G90")

        self.docobj.Path = Path.Path([c, c1, c2, c3, c4, c5, c6])
        postables = [self.docobj]

        expected = """G90
G21
G0 X1.000 Y2.000
G0 Z8.000
G91
G99
G83 X1.000 Y2.000 Z0.000 F7380.000 Q1.500 R5.000
G80
G90
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """G90
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
"""
        args = "--translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        expected = """(Begin preamble)
G90
G21
(Begin operation)
G0 X1.000 Y2.000
G0 Z8.000
G91
( G99 )
( G83 X1.000 Y2.000 Z0.000 F7380.000 Q1.500 R5.000 )
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
"""
        args = "--comments --translate_drill"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test00300(self):
        """Test wait-for-spindle."""
        #
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
        #
        c = Path.Command("G0 X10 Y20 Z30 A40 B50 C60 U70 V80 W90")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G0 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01010(self):
        """Test G1 command Generation."""
        #
        c = Path.Command("G1 X10 Y20 Z30 A40 B50 C60 U70 V80 W90 F1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F74.074
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        # Test argument order
        c = Path.Command("G1 F1.23456 Z30 V80 C60 W90 X10 B50 U70 Y20 A40")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G1 X10.000 Y20.000 Z30.000 A40.000 B50.000 C60.000 U70.000 V80.000 W90.000 F74.074
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01020(self):
        """Test G2 command Generation."""
        #
        c = Path.Command("G2 X10 Y20 Z30 I40 J50 P60 F1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G2 X10.000 Y20.000 Z30.000 I40.000 J50.000 F74.074 P60
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)
        #
        c = Path.Command("G2 X10 Y20 Z30 R40 P60 F1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G2 X10.000 Y20.000 Z30.000 F74.074 R40.000 P60
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01030(self):
        """Test G3 command Generation."""
        #
        c = Path.Command("G3 X10 Y20 Z30 I40 J50 P60 F1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G3 X10.000 Y20.000 Z30.000 I40.000 J50.000 F74.074 P60
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)
        #
        c = Path.Command("G3 X10 Y20 Z30 R40 P60 F1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G3 X10.000 Y20.000 Z30.000 F74.074 R40.000 P60
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01040(self):
        """Test G4 command Generation."""
        # Should some sort of "precision" be applied to the P parameter?
        # The code as currently written does not do so intentionally.
        # The P parameter indicates "time to wait" where a 0.001 would
        # be a millisecond wait, so more than 3 or 4 digits of precision
        # might be useful.
        c = Path.Command("G4 P1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G4 P1.23456
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01170(self):
        """Test G17 command Generation."""
        #
        c = Path.Command("G17")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G17
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01171(self):
        """Test G17.1 command Generation."""
        #
        c = Path.Command("G17.1")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G17.1
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01180(self):
        """Test G18 command Generation."""
        #
        c = Path.Command("G18")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G18
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01181(self):
        """Test G18.1 command Generation."""
        #
        c = Path.Command("G18.1")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G18.1
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01190(self):
        """Test G19 command Generation."""
        #
        c = Path.Command("G19")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G19
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01191(self):
        """Test G19.1 command Generation."""
        #
        c = Path.Command("G19.1")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G19.1
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01200(self):
        """Test G20 command Generation."""
        #
        c = Path.Command("G20")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G20
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01210(self):
        """Test G21 command Generation."""
        #
        c = Path.Command("G21")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G21
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01400(self):
        """Test G40 command Generation."""
        #
        c = Path.Command("G40")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G40
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01410(self):
        """Test G41 command Generation."""
        #
        c = Path.Command("G41 D1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G41 D1
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        c = Path.Command("G41 D0")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G41 D0
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01411(self):
        """Test G41.1 command Generation."""
        #
        c = Path.Command("G41.1 D1.23456 L3")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G41.1 L3 D1.23456
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01420(self):
        """Test G42 command Generation."""
        #
        c = Path.Command("G42 D1.23456")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G42 D1
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

        c = Path.Command("G42 D0")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G42 D0
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)

    def test01421(self):
        """Test G42.1 command Generation."""
        #
        c = Path.Command("G42.1 D1.23456 L3")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        expected = """G90
G21
G42.1 L3 D1.23456
"""
        args = ""
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        # print("--------\n" + gcode + "--------\n")
        self.assertEqual(gcode, expected)
