# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022 Larry Woestman <LarryWoestman2@gmail.com>          *
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


class TestOpenSBPPost(PathTestUtils.PathTestBase):
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
        self.post = PostProcessorFactory.get_post_processor(self.job, "opensbp")

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
        Empty path.  Only produces dxf file, with '-' as the file name.
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-show-editor"

        result = self.post.export()[0][1]
        lines = result.splitlines()
        # Only count lines as output contain timestamp
        self.assertEqual(8, len(lines))

        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        sbp = self.post.export()[0][1]

        expected = """&ToolName=1
&Tool=1
TR,1000.0
C6
PAUSE 2
"""
        self.assertEqual(sbp, expected)

        self.job.PostProcessorArgs = "--comments --no-header --no-show-editor"
        sbp = self.post.export()[0][1]
        expected = """'(begin preamble)
'(begin operation: TC: Default Tool)
'(Path: TC: Default Tool)
'a tool change happens now
&ToolName=1
&Tool=1
TR,1000.0
C6
PAUSE 2
'(finish operation: TC: Default Tool)
'(begin operation: Fixture)
'(Path: Fixture)
'(finish operation: Fixture)
'(begin operation: Profile)
'(Path: Profile)
'(finish operation: Profile)
'(begin postamble)
"""
        self.assertEqual(sbp, expected)

    def test_metric(self):
        """
        Test metric (mm) move
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --no-show-editor"
        sbp = self.post.export()[0][1]
        self.assertEqual(sbp.splitlines()[2], "TR,1000.0")
        self.assertEqual(sbp.splitlines()[3], "C6")

        result = sbp.splitlines()[5]
        expected = "J3,10.0000,20.0000,30.0000"
        self.assertEqual(result, expected)

    def test_inches(self):
        """
        Test inches move
        """

        c = Path.Command("G0 X10 Y20 Z30")
        self.profile_op.Path = Path.Path([c])
        self.job.PostProcessorArgs = "--no-header --inches --no-show-editor"
        sbp = self.post.export()[0][1]
        self.assertEqual(sbp.splitlines()[2], "TR,1000.0")
        self.assertEqual(sbp.splitlines()[3], "C6")

        result = sbp.splitlines()[5]
        expected = "J3,0.3937,0.7874,1.1811"
        self.assertEqual(result, expected)
