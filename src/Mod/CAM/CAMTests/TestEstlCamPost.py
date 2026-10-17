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
import re


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
        Test M09 is properly converted to M10 and its M9 is converted to M11

        Expected behavior:
            BEFORE:
            M7
            M9
            G1 X10.0 Y10.0 F1000
            M8
            M9

            AFTER:
            M10
            M11
            G1 X10.0 Y10.0 F1000
            M8
            M9
        """
        # Setup
        self.post._machine.output.output_tool_length_offset = False
        commands = [
            Path.Command("M7"),
            Path.Command("M9"),
            Path.Command("G1", {"X": 10.0, "Y": 10.0, "F": 1000}),
            Path.Command("M8"),
            Path.Command("M9"),
        ]
        self.profile_op.Path = Path.Path(commands)
        gcode = self.post.export2()[0][1]
        lines = gcode.splitlines()
        self.assertIn("M10", lines)
        self.assertLess(lines.index("M10"), lines.index("M11"))
        self.assertNotIn("M7", lines)
        self.assertIn("M8", lines)
        self.assertLess(lines.index("M8"), lines.index("M9"))

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

        self.post._machine.postprocessor_properties["TOOL_CHANGE_USE_ALTCMD"] = True
        self.post._machine.output.output_tool_length_offset = False

        self.profile_op.Path = Path.Path(commands)
        lines = self.post.export2()[0][1].splitlines()
        self.assertTrue(
            any(l.startswith("M0") and "T8" in l for l in lines), "Tool number modified in error"
        )
        self.assertFalse(
            any(l.startswith("M6") or l.startswith("M06") for l in lines), "M6 not converted to M0"
        )

    def test_full_export_no_crash(self):
        """export2() doesn't crash with every command Estlcam supports"""
        self.post._machine.output.comments.enabled = True
        self.post._machine.output.output_header = True
        self.post._machine.output.duplicates.commands = False
        self.post._machine.output.duplicates.parameters = False
        self.post._machine.processing.filter_inefficient_moves = True
        self.post._machine.output.formatting.line_numbers = True
        self.post._machine.postprocessor_properties["automatic_spindle"] = True
        self.post._machine.postprocessor_properties["automatic_tool_changer"] = True
        self.post.apply_configuration_bundle()

        handled_gcode = [
            Path.Command(g)
            for g in (
                "G0X1Y2Z3F110 G1X4Y5Z6F50 "
                "G2X7Y8I9J10 G3X11Y12I13J14 G2X7Y8I9J10Z11 G3X11Y12I13J14Z12 "
                # ignored by Estlcam (stripped)
                "G17 G20 G21 G54 G98 G99 G80 "
                # drill cycles are translated to G0/G1
                "G73X1Y2Z7F100R91Q1 G81X1Y2Z9F100R10 G82X1Y2Z10F100R11P12 G83X1Y2Z11F100R12Q2 "
                "M0 M1 M3S1 M4S1 M5 M6T2 M7 M8 M9 M10 M11 "
                "(comment)"
            ).split(" ")
        ]

        self.profile_op.Path = Path.Path(handled_gcode)
        self.post.export2()

        supported = self.post._command_names(self.post.values["SUPPORTED_COMMANDS"])
        ignored = self.post._command_names(self.post.values["IGNORED_COMMANDS"])
        tried = {c.Name for c in handled_gcode}
        untried = {p for p in supported - ignored if not re.search(r"0\d$", p)} - tried
        self.assertEqual(set(), untried, f"Supported by Estlcam but untried: {sorted(untried)}")
