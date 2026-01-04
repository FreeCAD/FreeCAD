# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
# *   Copyright (c) 2025 Petter Reinholdtsen <pere@hungry.com>              *
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

import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import PostProcessorFactory


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestFanucPost(PathTestUtils.PathTestBase):
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

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """

        # Create mock job with default operation and tool controller
        self.job, self.profile_op, self.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )

        # Create postprocessor using the mock job
        self.post = PostProcessorFactory.get_post_processor(self.job, "fanuc")

        # allow a full length "diff" if an error occurs
        self.maxDiff = None
        # reinitialize the postprocessor data structures between tests
        self.post.reinitialize()

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """

    def test_empty_path(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-show-editor"

        # Test generating with header
        # Header contains a time stamp that messes up unit testing.
        # Only test length of result.
        gcode = self.post.export()[0][1]
        self.assertEqual(28, len(gcode.splitlines()))
        # Test without header
        expected = """%
(BEGIN PREAMBLE)
G17 G54 G40 G49 G80 G90
G21
(BEGIN OPERATION: TC: DEFAULT TOOL)
(MACHINE UNITS: MM/MIN)
M05
G28 G91 Z0
M6 T1
G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120
G90
M3 S1000
(FINISH OPERATION: TC: DEFAULT TOOL)
(BEGIN OPERATION: FIXTURE)
(MACHINE UNITS: MM/MIN)
G54
(FINISH OPERATION: FIXTURE)
(BEGIN OPERATION: PROFILE)
(MACHINE UNITS: MM/MIN)
(FINISH OPERATION: PROFILE)
(BEGIN POSTAMBLE)
M05
G17 G54 G90 G80 G40
M30
%
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-show-editor"
            # "--no-header --no-comments --no-show-editor --precision=2"
        )

        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """%
G17 G54 G40 G49 G80 G90
G21
M05
G28 G91 Z0
M6 T1
G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120
G90
M3 S1000
G54
M05
G17 G54 G90 G80 G40
M30
%
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --no-show-editor"
            # "--no-header --no-comments --no-show-editor --precision=2"
        )
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

    def test_empty_path_spindle_empty(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postable.
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-show-editor --end-spindle-empty"

        # Test generating with header
        # Header contains a time stamp that messes up unit testing.
        # Only test length of result.
        gcode = self.post.export()[0][1]
        self.assertEqual(32, len(gcode.splitlines()))
        # Test without header
        expected = """%
(BEGIN PREAMBLE)
G17 G54 G40 G49 G80 G90
G21
(BEGIN OPERATION: TC: DEFAULT TOOL)
(MACHINE UNITS: MM/MIN)
M05
G28 G91 Z0
M6 T1
G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120
G90
M3 S1000
(FINISH OPERATION: TC: DEFAULT TOOL)
(BEGIN OPERATION: FIXTURE)
(MACHINE UNITS: MM/MIN)
G54
(FINISH OPERATION: FIXTURE)
(BEGIN OPERATION: PROFILE)
(MACHINE UNITS: MM/MIN)
(FINISH OPERATION: PROFILE)
(BEGIN MAKING SPINDLE EMPTY)
M05
G28 G91 Z0
M6 T0
(BEGIN POSTAMBLE)
M05
G17 G54 G90 G80 G40
M30
%
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-show-editor --end-spindle-empty"
            # "--no-header --no-comments --no-show-editor --precision=2"
        )

        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """%
G17 G54 G40 G49 G80 G90
G21
M05
G28 G91 Z0
M6 T1
G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120
G90
M3 S1000
G54
M05
G28 G91 Z0
M6 T0
M05
G17 G54 G90 G80 G40
M30
%
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --no-show-editor --end-spindle-empty"
            # "--no-header --no-comments --no-show-editor --precision=2"
        )
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

    def test_precision(self):
        """Test command Generation.
        Test Precision
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "G0 X10.000 Y20.000 Z30.000"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "G0 X10.00 Y20.00 Z30.00"
        self.assertEqual(result, expected)

    def test_line_numbers(self):
        """
        Test Line Numbers
        """
        c = Path.Command("G0 X10 Y20 Z30")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --line-numbers --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "N290  G0 X10.000 Y20.000 Z30.000"
        self.assertEqual(result, expected)

    def test_pre_amble(self):
        """
        Test Pre-amble
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --preamble='G18 G55' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[1]
        self.assertEqual(result, "G18 G55")

    def test_post_amble(self):
        """
        Test Post-amble
        """
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --postamble='G0 Z50\nM30' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[-3], "G0 Z50")
        self.assertEqual(gcode.splitlines()[-2], "M30")
        self.assertEqual(gcode.splitlines()[-1], "%")

    def test_inches(self):
        """
        Test inches
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --inches --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[3], "G20")

        result = gcode.splitlines()[19]
        expected = "G0 X0.3937 Y0.7874 Z1.1811"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --inches --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "G0 X0.39 Y0.79 Z1.18"
        self.assertEqual(result, expected)

    def test_tool_change(self):
        """
        Test tool change
        """
        c = Path.Command("M6 T1")
        c2 = Path.Command("M3 S3000")
        self.profile_op.Path = Path.Path([c, c2])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[19], "M05")
        self.assertEqual(gcode.splitlines()[20], "G28 G91 Z0")
        self.assertEqual(gcode.splitlines()[21], "M6 T1")
        self.assertEqual(gcode.splitlines()[22], "G91 G0 G43 G54 Z-[#[2000+#4120]] H#4120")
        self.assertEqual(gcode.splitlines()[23], "G90")
        self.assertEqual(gcode.splitlines()[24], "M3 S3000")

        # suppress TLO
        self.job.PostProcessorArgs = "--no-header --no-tlo --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[20], "M3 S3000")

    def test_thread_tap(self):
        """
        Test threading using drill cycle converted to tapping
        """

        self.tool_controller.Tool.ShapeName = "tap"
        c = Path.Command("G0 X10 Y10")
        c2 = Path.Command("G81 X10 Y10 Z-10 R20 F1 P1 Q1")
        self.profile_op.Path = Path.Path([c, c2])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[18], "G0 X10.000 Y10.000")
        self.assertEqual(gcode.splitlines()[19], "G95")
        self.assertEqual(gcode.splitlines()[20], "M29 S1000")
        self.assertEqual(gcode.splitlines()[21], "G84 Z-10.000 R20.000 F1.000 P1.000 Q1.000")
        self.assertEqual(gcode.splitlines()[22], "G80")
        self.assertEqual(gcode.splitlines()[23], "G94")

    def test_comment(self):
        """
        Test comment
        """

        c = Path.Command("(comment)")

        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "(COMMENT)"
        self.assertEqual(result, expected)
