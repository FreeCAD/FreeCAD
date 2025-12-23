# SPDX-License-Identifier: LGPL-2.1-or-later

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
import CAMTests.PostTestMocks as PostTestMocks
from Path.Post.Processor import PostProcessorFactory


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestLinuxCNCPost(PathTestUtils.PathTestBase):
    """Test LinuxCNC-specific features of the inuxcnc_post.py postprocessor.

    This test suite focuses on LinuxCNC-specific functionality such as path blending modes.
    Generic postprocessor functionality is tested in TestGenericPost.
    """

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

        # Create mock job with default operation and tool controller
        cls.job, cls.profile_op, cls.tool_controller = (
            PostTestMocks.create_default_job_with_operation()
        )

        # Create postprocessor using the mock job
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "linuxcnc")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does not
        have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # No cleanup needed for mock objects
        pass

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

    def test_blend_mode_exact_path(self):
        """Test EXACT_PATH blend mode outputs G61."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode EXACT_PATH --no-show-editor"
        )
        gcode = self.post.export()[0][1]

        # G61 should be in the preamble
        self.assertIn("G61", gcode)
        # Should not have G64
        self.assertNotIn("G64", gcode)
        # Should not have G61.1
        self.assertNotIn("G61.1", gcode)

    def test_blend_mode_exact_stop(self):
        """Test EXACT_STOP blend mode outputs G61.1."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode EXACT_STOP --no-show-editor"
        )
        gcode = self.post.export()[0][1]

        # G61.1 should be in the preamble
        self.assertIn("G61.1", gcode)
        # Should not have G64
        self.assertNotIn("G64", gcode)

    def test_blend_mode_blend_default(self):
        """Test BLEND mode with default tolerance (0) outputs G64."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = "--no-header --no-comments --blend-mode BLEND --no-show-editor"
        gcode = self.post.export()[0][1]

        # G64 should be in the preamble (without P parameter)
        lines = gcode.splitlines()
        has_g64 = any("G64" in line and "P" not in line for line in lines)
        self.assertTrue(has_g64, "Expected G64 without P parameter")

    def test_blend_mode_blend_with_tolerance(self):
        """Test BLEND mode with tolerance outputs G64 P<tolerance>."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode BLEND --blend-tolerance 0.05 --no-show-editor"
        )
        gcode = self.post.export()[0][1]

        # G64 P0.05 should be in the preamble
        self.assertIn("G64 P0.0500", gcode)

    def test_blend_mode_blend_with_custom_tolerance(self):
        """Test BLEND mode with custom tolerance value."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode BLEND --blend-tolerance 0.02 --no-show-editor"
        )
        gcode = self.post.export()[0][1]

        # G64 P0.02 should be in the preamble
        self.assertIn("G64 P0.0200", gcode)

    def test_blend_mode_in_preamble_position(self):
        """Test that blend mode command appears in correct position in preamble."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode BLEND --blend-tolerance 0.1 --no-show-editor"
        )
        gcode = self.post.export()[0][1]
        lines = gcode.splitlines()

        # Find G64 P line
        g64_line_idx = None
        for i, line in enumerate(lines):
            if "G64 P" in line:
                g64_line_idx = i
                break

        self.assertIsNotNone(g64_line_idx, "G64 P command not found")
        # Should be early in output (within first few lines of preamble)
        self.assertLess(g64_line_idx, 5, "G64 command should be in preamble")

    def test_blend_tolerance_zero_equals_no_tolerance(self):
        """Test that blend tolerance of 0 outputs G64 without P parameter."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            "--no-header --no-comments --blend-mode BLEND --blend-tolerance 0 --no-show-editor"
        )
        gcode = self.post.export()[0][1]

        # Should have G64 without P
        lines = gcode.splitlines()
        has_g64_without_p = any("G64" in line and "P" not in line for line in lines)
        self.assertTrue(has_g64_without_p, "Expected G64 without P parameter when tolerance is 0")

    def test_blend_interaction_with_preamble_argument(self):
        """Test interaction with a --preamble command line argument."""
        self.profile_op.Path = Path.Path([])
        self.job.PostProcessorArgs = (
            '--no-header --no-comments --blend-mode BLEND --preamble="G80 G90" --no-show-editor'
        )
        gcode = self.post.export()[0][1]
        lines = gcode.splitlines()
        self.assertEqual(lines[0], "G80 G90")
        self.assertEqual(lines[1], "G64")
