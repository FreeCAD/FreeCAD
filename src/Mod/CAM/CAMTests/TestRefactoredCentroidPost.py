# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 - 2025 Larry Woestman <LarryWoestman2@gmail.com>   *
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
from Path.Post.Processor import PostProcessorFactory


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestRefactoredCentroidPost(PathTestUtils.PathTestBase):
    """Test the refactored_centroid_post.py postprocessor."""

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

        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "refactored_centroid")
        # locate the operation named "Profile"
        for op in cls.job.Operations.Group:
            if op.Label == "Profile":
                # remember the "Profile" operation
                cls.profile_op = op
                return

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does not
        have access to the class `self` reference.  This method is able to
        call static methods within this same class.
        """
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        # reinitialize the postprocessor data structures between tests
        self.post.reinitialize()

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def single_compare(self, path, expected, args, debug=False):
        """Perform a test with a single line of gcode comparison."""
        nl = "\n"
        self.job.PostProcessorArgs = args
        # replace the original path (that came with the job and operation) with our path
        self.profile_op.Path = Path.Path(path)
        # the gcode is in the first section for this particular job and operation
        gcode = self.post.export()[0][1]
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        # there are 4 lines of "other stuff" before the line we are interested in
        self.assertEqual(gcode.splitlines()[4], expected)

    def multi_compare(self, path, expected, args, debug=False):
        """Perform a test with multiple lines of gcode comparison."""
        nl = "\n"
        self.job.PostProcessorArgs = args
        # replace the original path (that came with the job and operation) with our path
        self.profile_op.Path = Path.Path(path)
        # the gcode is in the first section for this particular job and operation
        gcode = self.post.export()[0][1]
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    def test000(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """
        nl = "\n"

        self.profile_op.Path = Path.Path([])

        # Test generating with header
        # Header contains a time stamp that messes up unit testing.
        # Only test length of result.
        self.job.PostProcessorArgs = "--no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertTrue(len(gcode.splitlines()) == 25)

        # Test without header
        expected = """G90 G80 G40 G49
;T1=TC__Default_Tool
;Begin preamble
G53 G00 G17
G21
;Begin operation
G54
;End operation: Fixture
;Begin operation
;TC: Default Tool
;Begin toolchange
M6 T1
;End operation: TC: Default Tool
;Begin operation
;End operation: Profile
;Begin postamble
M5
M25
G49 H0
G90 G80 G40 G49
M99
"""

        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """G90 G80 G40 G49
G53 G00 G17
G21
G54
M6 T1
M5
M25
G49 H0
G90 G80 G40 G49
M99
"""

        # args = ("--no-header --no-comments --no-show-editor --precision=2")
        self.job.PostProcessorArgs = "--no-header --no-comments --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    def test010(self):
        """Test command Generation.
        Test Precision
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[14]
        expected = "G0 X10.0000 Y20.0000 Z30.0000"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --axis-precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[14]
        expected = "G0 X10.00 Y20.00 Z30.00"
        self.assertEqual(result, expected)

    def test020(self):
        """
        Test Line Numbers
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--no-header --line-numbers --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[14]
        expected = "N240 G0 X10.0000 Y20.0000 Z30.0000"
        self.assertEqual(result, expected)

    def test030(self):
        """
        Test Pre-amble
        """
        nl = "\n"

        self.profile_op.Path = Path.Path([])

        self.job.PostProcessorArgs = (
            "--no-header --no-comments --preamble='G18 G55' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[1]
        self.assertEqual(result, "G18 G55")

    def test040(self):
        """
        Test Post-amble
        """
        nl = "\n"

        self.profile_op.Path = Path.Path([])

        self.job.PostProcessorArgs = (
            "--no-header --no-comments --postamble='G0 Z50\nM2' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[-2]
        self.assertEqual(result, "G0 Z50")
        self.assertEqual(gcode.splitlines()[-1], "M2")

    def test050(self):
        """
        Test inches
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--no-header --inches --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[4], "G20")

        result = gcode.splitlines()[14]
        expected = "G0 X0.3937 Y0.7874 Z1.1811"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --inches --axis-precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[14]
        expected = "G0 X0.39 Y0.79 Z1.18"
        self.assertEqual(result, expected)

    def test060(self):
        """
        Test test modal
        Suppress the command name if the same as previous
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.profile_op.Path = Path.Path([c, c1])

        self.job.PostProcessorArgs = "--no-header --modal --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[15]
        expected = "X10.0000 Y30.0000 Z30.0000"
        self.assertEqual(result, expected)

    def test070(self):
        """
        Test axis modal
        Suppress the axis coordinate if the same as previous
        """
        nl = "\n"

        c = Path.Command("G0 X10 Y20 Z30")
        c1 = Path.Command("G0 X10 Y30 Z30")

        self.profile_op.Path = Path.Path([c, c1])

        self.job.PostProcessorArgs = "--no-header --axis-modal --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[15]
        expected = "G0 Y30.0000"
        self.assertEqual(result, expected)

    def test080(self):
        """
        Test tool change
        """
        nl = "\n"

        c = Path.Command("M6 T2")
        c2 = Path.Command("M3 S3000")

        self.profile_op.Path = Path.Path([c, c2])

        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[15], "M6 T2")
        self.assertEqual(gcode.splitlines()[16], "M3 S3000")

        # suppress TLO
        self.job.PostProcessorArgs = "--no-header --no-tlo --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[16], "M3 S3000")

    def test090(self):
        """
        Test comment
        """
        nl = "\n"

        c = Path.Command("(comment)")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        # print(f"--------{nl}{gcode}--------{nl}")
        result = gcode.splitlines()[14]
        expected = ";comment"
        self.assertEqual(result, expected)
