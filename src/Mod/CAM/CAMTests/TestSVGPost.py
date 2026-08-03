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


class TestSVGPost(PathTestUtils.PathTestBase):
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
        self.post = PostProcessorFactory.get_post_processor(self.job, "svg")

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
        Empty path.  Produces nothing.
        """

        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = ()
        svg = self.post.export()[0][1]
        self.assertEqual(svg, "")

    def test_lines(self):
        """Test layered output"""

        expected = """<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" width="10.0mm" height="10.0mm" viewBox="0 0 10.0 10.0">
  <path d="M 0.0000 10.0000 L 10.0000 10.0000 L 10.0000 0.0000 L 0.0000 0.0000 L 0.0000 10.0000 Z" stroke="black" stroke-width="0.1" fill="none" />
</svg>"""
        lines = [
            Path.Command("G0 X100 Y100"),
            Path.Command("G1 X110 Y100"),
            Path.Command("G1 X110 Y110"),
            Path.Command("G1 X100 Y110"),
            Path.Command("G1 X100 Y100"),
        ]
        self.profile_op.Path = Path.Path(lines)
        self.job.PostProcessorArgs = ()
        svg = self.post.export()[0][1]
        self.assertEqual(svg, expected)

    def test_lines_layers(self):
        """Test layered output"""

        lines = [
            Path.Command("G0 X100 Y100"),
            Path.Command("G1 X110 Y100"),
            Path.Command("G1 X110 Y110"),
            Path.Command("G1 X100 Y110"),
            Path.Command("G1 X100 Y100"),
        ]
        self.profile_op.Path = Path.Path(lines)
        self.job.PostProcessorArgs = "--layers"
        svg = self.post.export()[0][1]
        svglines = svg.splitlines()
        self.assertEqual(
            svglines[1],
            '<g id="layer0_2" inkscape:label="Profile" inkscape:groupmode="layer">',
        )
        self.assertEqual(
            svglines[2],
            '  <path d="M 0.0000 10.0000 L 10.0000 10.0000 L 10.0000 0.0000 L 0.0000 0.0000 L 0.0000 10.0000 Z" stroke="black" stroke-width="0.1" fill="none" />',
        )
        self.assertEqual(svglines[3], "</g>")
