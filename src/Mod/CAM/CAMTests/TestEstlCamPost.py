# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Fae Corrigan
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################


import Path
from CAMTests import PathTestUtils
from CAMTests import PostTestMocks
from Path.Post.Processor import (
    PostProcessorFactory,
)
from Machine.models.machine import Machine, Toolhead, ToolheadType

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())
import Constants


class TestEstlcamPost(PathTestUtils.PathTestBase):
    """Test Estlcam-specific features of the Estlcam_post.py postprocessor.
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
        cls.post = PostProcessorFactory.get_post_processor(cls.job, "estlcam")

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
        # Create a machine configuration for each test
        self.post._machine = Machine.create_3axis_config()
        self.post._machine.name = "Test Estlcam Machine"
        self.post.apply_configuration_bundle()
        # Add a default toolhead (required by export2)
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            min_rpm=0,
            max_rpm=24000,
            max_power_kw=1.0,
        )
        self.post._machine.toolheads = [toolhead]

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def _gcode_and_preamble(self):
        """convenience to get the preamble stuff"""
        gcode = self.post.export2()[0][1]
        lines = gcode.splitlines()
        # Preamble stuff, up to next piece, which should be unit-command (`_collect_unit_command`)
        idx = lines.index("G21")  # throws IndexError if unexpectedly missing
        preamble = "\n".join(lines[:idx])
        return gcode, preamble

    def test_convert_mist_commands(self):
        """
        Test M07 is properly converted to M11,

        Expected behavior:
            BEFORE:
            M7
            M9
            G1 X10.0 Y10.0 F1000
            M8
            M9

            AFTER:
            M11
            M10
            G1 X10.0 Y10.0 F1000
            M8
            M9
        """
        # Setup
        commands = [
            Path.Command("M7"),
            Path.Command("M9"),
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("M8"),
            Path.Command("M9"),
        ]
        self.profile_op.Path = Path.Path(commands)
        postables = ("section", [self.profile_op])
        self.post.convert_command_to_gcode(postables)
        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        self.assertEqual(cmd_names[0], "M10", "M7 not converted to M10")
        self.assertEqual(cmd_names[1], "M11", "M9 not converted to M11 after M10")
        self.assertEqual(cmd_names[2], "G1", "mist command modifying commands outside scope")
        self.assertIn(cmd_names[3], Constants.MCODE_COOLANT_FLOOD)
        self.assertIn(cmd_names[4], Constants.MCODE_COOLANT_OFF)

    def test_use_toolchange_altcmd(self):
        """
        Test that TOOL_CHANGE_USE_ALTCMD properly converts M6 to M0 when true
        Expected behavior:
            BEFORE:
            M6 T8
            M06 T8

            AFTER:
            M0 T8
            M0 T8
        """
        # Create a simple path with M3 command
        commands = [
            Path.Command("M6", {"T": 8.0}),
            Path.Command("M06", {"T": 8.0}),
        ]
        self.profile_op.Path = Path.Path(commands)

        # Set pierce delay to 2000ms (should become 2.0 seconds in G4)
        self._set_postprocessor_properties(TOOL_CHANGE_USE_ALTCMD=True)

        postables = [("section", [self.profile_op])]
        self.post.convert_command_to_gcode(postables)

        # Verify the modified path
        result_cmds = self.profile_op.Path.Commands
        cmd_names = [cmd.Name for cmd in result_cmds]

        self.assertEqual(cmd_names[0], "M0", "M6 not converted to M0")
        self.assertAlmostEqual(
            result_cmds[0].Parameters["T"], 8, msg="Tool number modified in error"
        )
        self.assertEqual(cmd_names[1], "M0", "M06 not converted to M0")
        self.assertAlmostEqual(
            result_cmds[1].Parameters["T"], 8, msg="Tool number modified in error"
        )
