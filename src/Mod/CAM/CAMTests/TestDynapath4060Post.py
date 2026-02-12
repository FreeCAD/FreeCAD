# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2023 Larry Woestman <LarryWoestman2@gmail.com>          *
# *   Copyright (c) 2026 Petter Reinholdtsen <pere@hungry.com>              *
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


class TestDynapath4060Post(PathTestUtils.PathTestBase):
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
        self.post = PostProcessorFactory.get_post_processor(self.job, "dynapath_4060")

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
        self.assertEqual(32, len(gcode.splitlines()))
        # Test without header
        expected = """(T)BEGIN PREAMBLE$
G17
G90
G80
G40
G71
(T)BEGIN OPERATION: TC: DEFAULT TOOL$
(T)MACHINE UNITS: MM/MIN$
(T)BEGIN TOOLCHANGE$
M6T1
M3S1000
L1.0
(T)FINISH OPERATION: TC: DEFAULT TOOL$
(T)BEGIN OPERATION: FIXTURE$
(T)MACHINE UNITS: MM/MIN$
E01
(T)FINISH OPERATION: FIXTURE$
(T)BEGIN OPERATION: PROFILE$
(T)MACHINE UNITS: MM/MIN$
(T)FINISH OPERATION: PROFILE$
(T)BEGIN POSTAMBLE$
M05
G80
G40
G17
G90
M30
E
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-show-editor"
            # "--no-header --no-comments --no-show-editor --precision=2"
        )

        gcode = self.post.export()[0][1]
        self.assertEqual(gcode, expected)

        # test without comments
        expected = """G17
G90
G80
G40
G71
M6T1
M3S1000
L1.0
E01
M05
G80
G40
G17
G90
M30
E
"""

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --no-show-editor"
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
        expected = "G0X10.000Y20.000Z30.000"
        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "G0X10.00Y20.00Z30.00"
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
        expected = "N0020G0X10.000Y20.000Z30.000"
        self.assertEqual(result, expected)

    def test_pre_amble(self):
        """
        Test Pre-amble
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --preamble='G18\nG55' --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        lines = gcode.splitlines()
        self.assertEqual(lines[0], "G18")
        self.assertEqual(lines[1], "G55")

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
        self.assertEqual(gcode.splitlines()[-1], "E")

    def test_inches(self):
        """
        Test inches
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --inches --no-show-editor"
        gcode = self.post.export()[0][1]
        self.assertEqual(gcode.splitlines()[5], "G70")

        result = gcode.splitlines()[19]
        expected = "G0X0.394Y0.787Z1.181"

        self.assertEqual(result, expected)

        self.job.PostProcessorArgs = "--no-header --inches --precision=2 --no-show-editor"
        gcode = self.post.export()[0][1]
        result = gcode.splitlines()[19]
        expected = "G0X0.39Y0.79Z1.18"
        self.assertEqual(result, expected)
