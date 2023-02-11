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

import Path
import PathTests.PathTestUtils as PathTestUtils
from Path.Post.scripts import mach3_mach4_post as postprocessor


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestMach3Mach4Post(PathTestUtils.PathTestBase):
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        self.assertTrue(len(gcode.splitlines()) == 13)

        # Test without header
        expected = """(begin preamble)
G17 G54 G40 G49 G80 G90
G21
(begin operation: testpath)
(machine: mach3_4, mm/min)
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        self.assertEqual(gcode, expected)

    def test010(self):
        """Test command Generation.
        Test Precision
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--no-header --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[5]
        expected = "G0 X10.000 Y20.000 Z30.000"
        self.assertEqual(result, expected)

        args = "--no-header --precision=2 --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[5]
        expected = "G0 X10.00 Y20.00 Z30.00"
        self.assertEqual(result, expected)

    def test020(self):
        """
        Test Line Numbers
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--no-header --line-numbers --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[5]
        expected = "N160  G0 X10.000 Y20.000 Z30.000"
        self.assertEqual(result, expected)

    def test030(self):
        """
        Test Pre-amble
        """

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--no-header --no-comments --preamble='G18 G55' --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[0]
        self.assertEqual(result, "G18 G55")

    def test040(self):
        """
        Test Post-amble
        """
        self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        args = "--no-header --no-comments --postamble='G0 Z50\nM2' --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        self.assertEqual(gcode.splitlines()[2], "G20")

        result = gcode.splitlines()[5]
        expected = "G0 X0.3937 Y0.7874 Z1.1811"
        self.assertEqual(result, expected)

        # Technical debt.   The following test fails.  Precision not working
        # with imperial units.

        # args = ("--no-header --inches --precision=2 --no-show-editor")
        # gcode = postprocessor.export(postables, "gcode.tmp", args)
        # result = gcode.splitlines()[5]
        # expected = "G0 X0.39 Y0.79 Z1.18"
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[6]
        expected = "X10.000 Y30.000 Z30.000"
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[6]
        expected = "G0 Y30.000"
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
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        self.assertEqual(gcode.splitlines()[5], "M5")
        self.assertEqual(gcode.splitlines()[6], "M6 T2 ")
        self.assertEqual(gcode.splitlines()[7], "G43 H2")
        self.assertEqual(gcode.splitlines()[8], "M3 S3000")

        # suppress TLO
        args = "--no-header --no-tlo --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        self.assertEqual(gcode.splitlines()[7], "M3 S3000")

    def test090(self):
        """
        Test comment
        """

        c = Path.Command("(comment)")

        self.docobj.Path = Path.Path([c])
        postables = [self.docobj]

        args = "--no-header --no-show-editor"
        gcode = postprocessor.export(postables, "gcode.tmp", args)
        result = gcode.splitlines()[5]
        expected = "(comment)"
        self.assertEqual(result, expected)
