# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2016 shopinthewoods@gmail.com
# SPDX-FileCopyrightText: 2022 LarryWoestman2@gmail.com

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

import re
import FreeCAD
import Constants
import Path
import Path.Post.Command as PathCommand
import Path.Post.PostList as PostList
import Path.Post.Processor as PathPost
import Path.Post.Utils as PostUtils
from Path.Post.CAMErrors import CAMValueError
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import unittest

PathCommand.LOG_MODULE = Path.Log.thisModule()
Path.Log.setLevel(Path.Log.Level.INFO, PathCommand.LOG_MODULE)


class TestPathPostUtils(unittest.TestCase):
    def test010(self):
        """Test the utility functions in the PostUtils.py file."""
        commands = [
            Path.Command("G1 X-7.5 Y5.0 Z0.0"),
            Path.Command("G2 I2.5 J0.0 K0.0 X-5.0 Y7.5 Z0.0"),
            Path.Command("G1 X5.0 Y7.5 Z0.0"),
            Path.Command("G2 I0.0 J-2.5 K0.0 X7.5 Y5.0 Z0.0"),
            Path.Command("G1 X7.5 Y-5.0 Z0.0"),
            Path.Command("G2 I-2.5 J0.0 K0.0 X5.0 Y-7.5 Z0.0"),
            Path.Command("G1 X-5.0 Y-7.5 Z0.0"),
            Path.Command("G2 I0.0 J2.5 K0.0 X-7.5 Y-5.0 Z0.0"),
            Path.Command("G1 X-7.5 Y0.0 Z0.0"),
        ]

        testpath = Path.Path(commands)
        self.assertTrue(len(testpath.Commands) == 9)
        self.assertTrue(len([c for c in testpath.Commands if c.Name in ["G2", "G3"]]) == 4)

        results = PostUtils.splitArcs(testpath)
        # self.assertTrue(len(results.Commands) == 117)
        self.assertTrue(len([c for c in results.Commands if c.Name in ["G2", "G3"]]) == 0)

    def test020(self):
        """Test Termination of Canned Cycles"""
        # Test basic cycle termination when parameters change
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -1.0, "R": 0.2, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                Path.Command("G0", {"Z": 1.0}),
                cmd1,
                cmd2,  # Different Z depth
                Path.Command("G1", {"X": 3.0, "Y": 3.0}),
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G0", {"Z": 1.0}),
                Path.Command("G98"),  # Retract mode for first cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command("G80"),  # Terminate due to parameter change
                Path.Command("G98"),  # Retract mode for second cycle
                Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -1.0, "R": 0.2, "F": 10.0}),
                Path.Command("G80"),  # Final termination
                Path.Command("G1", {"X": 3.0, "Y": 3.0}),
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)

        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test030_canned_cycle_termination_with_non_cycle_commands(self):
        """Test cycle termination when non-cycle commands are encountered"""
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G82", {"X": 3.0, "Y": 3.0, "Z": -1.0, "R": 0.2, "P": 1.0, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                cmd1,
                Path.Command("G0", {"X": 2.0, "Y": 2.0}),  # Non-cycle command
                cmd2,
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G98"),  # Retract mode for first cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command("G80"),  # Terminate before non-cycle command
                Path.Command("G0", {"X": 2.0, "Y": 2.0}),
                Path.Command("G98"),  # Retract mode for second cycle
                Path.Command("G82", {"X": 3.0, "Y": 3.0, "Z": -1.0, "R": 0.2, "P": 1.0, "F": 10.0}),
                Path.Command("G80"),  # Final termination
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test040_canned_cycle_modal_same_parameters(self):
        """Test modal cycles with same parameters don't get terminated"""
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}
        cmd3 = Path.Command("G81", {"X": 3.0, "Y": 3.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd3.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                cmd1,
                cmd2,  # Modal - same parameters
                cmd3,  # Modal - same parameters
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G98"),  # Retract mode at start of cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command(
                    "G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "F": 10.0}
                ),  # No termination - same params
                Path.Command(
                    "G81", {"X": 3.0, "Y": 3.0, "Z": -0.5, "R": 0.1, "F": 10.0}
                ),  # No termination - same params
                Path.Command("G80"),  # Final termination
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test050_canned_cycle_feed_rate_change(self):
        """Test cycle termination when feed rate changes"""
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "F": 20.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                cmd1,
                cmd2,  # Different feed rate
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G98"),  # Retract mode for first cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command("G80"),  # Terminate due to feed rate change
                Path.Command("G98"),  # Retract mode for second cycle
                Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "F": 20.0}),
                Path.Command("G80"),  # Final termination
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test060_canned_cycle_retract_plane_change(self):
        """Test cycle termination when retract plane changes"""
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.2, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                cmd1,
                cmd2,  # Different R plane
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G98"),  # Retract mode for first cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command("G80"),  # Terminate due to R plane change
                Path.Command("G98"),  # Retract mode for second cycle
                Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.2, "F": 10.0}),
                Path.Command("G80"),  # Final termination
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test070_canned_cycle_mixed_cycle_types(self):
        """Test termination between different cycle types"""
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}
        cmd2 = Path.Command("G82", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "P": 1.0, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        test_path = Path.Path(
            [
                cmd1,
                cmd2,  # Different cycle type
            ]
        )

        expected_path = Path.Path(
            [
                Path.Command("G98"),  # Retract mode for first cycle
                Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0}),
                Path.Command("G80"),  # Terminate due to different cycle type (different parameters)
                Path.Command("G98"),  # Retract mode for second cycle
                Path.Command("G82", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "P": 1.0, "F": 10.0}),
                Path.Command("G80"),  # Final termination
            ]
        )

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")

    def test080_canned_cycle_retract_mode_change(self):
        """Test cycle termination and retract mode insertion when RetractMode annotation changes"""
        # Create commands with RetractMode annotations
        cmd1 = Path.Command("G81", {"X": 1.0, "Y": 1.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd1.Annotations = {"RetractMode": "G98"}

        cmd2 = Path.Command("G81", {"X": 2.0, "Y": 2.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd2.Annotations = {"RetractMode": "G98"}

        cmd3 = Path.Command("G81", {"X": 3.0, "Y": 3.0, "Z": -0.5, "R": 0.1, "F": 10.0})
        cmd3.Annotations = {"RetractMode": "G99"}  # Mode change

        test_path = Path.Path([cmd1, cmd2, cmd3])

        result = PostUtils.cannedCycleTerminator(test_path)

        # Expected: G98, G81, G81 (modal), G80 (terminate), G99, G81, G80 (final)
        self.assertEqual(result.Commands[0].Name, "G98")
        self.assertEqual(result.Commands[1].Name, "G81")
        self.assertEqual(result.Commands[2].Name, "G81")
        self.assertEqual(result.Commands[3].Name, "G80")  # Terminate due to mode change
        self.assertEqual(result.Commands[4].Name, "G99")  # New retract mode
        self.assertEqual(result.Commands[5].Name, "G81")
        self.assertEqual(result.Commands[6].Name, "G80")  # Final termination
        self.assertEqual(len(result.Commands), 7)

    def test090_canned_cycle_inserted_commands_carry_annotations(self):
        """Inserted G98/G99/G80 carry the cycle's annotations (linuxcnc rigid tapping keys on them)"""
        annotations = {"RetractMode": "G99", "operation": "tapping", "rigid": "True"}
        cmd1 = Path.Command("G84", {"X": 0.0, "Y": 0.0, "Z": -5.0, "R": 1.0, "F": 1.25})
        cmd1.Annotations = annotations
        cmd2 = Path.Command("G84", {"X": 5.0, "Y": 0.0, "Z": -5.0, "R": 1.0, "F": 1.25})
        cmd2.Annotations = annotations

        result = PostUtils.cannedCycleTerminator(Path.Path([cmd1, cmd2, Path.Command("G0 Z10")]))

        self.assertEqual([c.Name for c in result.Commands], ["G99", "G84", "G84", "G80", "G0"])
        for i in (0, 3):
            self.assertEqual(result.Commands[i].Annotations.get("operation"), "tapping")
            self.assertEqual(result.Commands[i].Annotations.get("rigid"), "True")

    def test100_canned_cycle_unannotated_keeps_path_retract_mode(self):
        """Cycles without a RetractMode annotation use the last literal G98/G99, not G98"""
        # Shape of the deprecated Tapping op: one G99, then a G0 before every hole.
        commands = [Path.Command("G99")]
        for x in (0.0, 5.0):
            commands.append(Path.Command("G0", {"X": x, "Y": 0.0, "Z": 10.0}))
            commands.append(Path.Command("G84", {"X": x, "Y": 0.0, "Z": -5.0, "R": 1.0}))
        commands.append(Path.Command("G80"))

        result = PostUtils.cannedCycleTerminator(Path.Path(commands))

        names = [c.Name for c in result.Commands]
        self.assertNotIn("G98", names)
        for i, name in enumerate(names):
            if name == "G84":
                self.assertEqual(names[i - 1], "G99", f"G84 at {i} not preceded by G99: {names}")

        # A G80 in the path ends the explicit mode; later bare cycles default to G98 again.
        later = Path.Command("G81", {"X": 0.0, "Y": 0.0, "Z": -1.0, "R": 1.0})
        result = PostUtils.cannedCycleTerminator(Path.Path(commands + [later]))
        self.assertEqual(result.Commands[-3].Name, "G98")


class TestBuildPostList(unittest.TestCase):
    """
    The postlist is the list of postprocessable elements from the job.
    The list varies depending on
        -The operations
        -The tool controllers
        -The work coordinate systems (WCS) or 'fixtures'
        -How the job is ordering the output (WCS, tool, operation)
        -Whether or not the output is being split to multiple files
    This test case ensures that the correct sequence of postable objects is
    created.

    The list will be comprised of a list of tuples. Each tuple consists of
    (subobject string, [list of objects])
    The subobject string can be used in output name generation if splitting output
    the list of objects is all postable elements to be written to that file

    """

    # Set to True to enable verbose debug output for test validation
    debug = False

    @classmethod
    def _format_postables(cls, postables, title="Postables"):
        """Format postables for readable debug output, following dumper_post.py pattern."""
        output = []
        output.append("=" * 80)
        output.append(title)
        output.append("=" * 80)
        output.append("")

        for idx, postable in enumerate(postables, 1):
            group_key = postable[0]
            objects = postable[1]

            # Format the group key display
            if group_key == "":
                display_key = "(empty string)"
            elif group_key == "allitems":
                display_key = '"allitems" (combined output)'
            else:
                display_key = f'"{group_key}"'

            output.append(f"[{idx}] Group: {display_key}")
            output.append(f"    Objects: {len(objects)}")
            output.append("")

            for obj_idx, obj in enumerate(objects, 1):
                if isinstance(obj, PostList.Postable):
                    output.append(f"    [{obj_idx}] {obj.label}")
                    output.append(f"        item_type: {obj.item_type}")
                    if obj.path and obj.path.Commands:
                        output.append(f"        Commands: {[c.Name for c in obj.path.Commands]}")
                    if obj.data:
                        output.append(f"        data: {obj.data}")
                else:
                    obj_label = getattr(obj, "Label", str(type(obj).__name__))
                    output.append(f"    [{obj_idx}] {obj_label} (raw {type(obj).__name__})")

            output.append("")

        output.append("=" * 80)
        output.append(f"Total Groups: {len(postables)}")
        total_objects = sum(len(p[1]) for p in postables)
        output.append(f"Total Objects: {total_objects}")
        output.append("=" * 80)

        return "\n".join(output)

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("test_filenaming")

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessor = "linuxcnc_legacy"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]  # 2 fixtures as expected by tests

        # Create additional tool controllers to match original file structure
        # Original had 2 tool controllers both with "TC: 7/16\" two flute" label

        # Modify the first tool controller to have the expected values
        cls.job.Tools.Group[0].ToolNumber = 5
        cls.job.Tools.Group[0].Label = (
            'TC: 7/16" two flute'  # test050 expects this sanitized to "TC__7_16__two_flute"
        )

        # Add second tool controller with same label but different number
        tc2 = PathToolController.Create()
        tc2.ToolNumber = 2
        tc2.Label = 'TC: 7/16" two flute'  # Same label as first tool controller
        cls.job.Proxy.addToolController(tc2)

        # Recompute tool controllers to populate their Path.Commands with M6 commands
        cls.job.Tools.Group[0].recompute()
        cls.job.Tools.Group[1].recompute()

        # Create mock operations to match original file structure
        # Original had 3 operations: outsideprofile, DrillAllHoles, Comment
        # The Comment operation has no tool controller
        operation_names = ["outsideprofile", "DrillAllHoles", "Comment"]

        for i, name in enumerate(operation_names):
            # Create a simple document object that mimics an operation
            op = cls.doc.addObject("Path::FeaturePython", name)
            op.Label = name
            # Path::FeaturePython objects already have a Path property
            op.Path = Path.Path()

            # Only add ToolController property for operations that need it
            if name != "Comment":
                # Add ToolController property to the operation
                op.addProperty(
                    "App::PropertyLink",
                    "ToolController",
                    "Base",
                    "Tool controller for this operation",
                )
                # Assign operations to tool controllers
                if i == 0:  # outsideprofile uses first tool controller (tool 5)
                    op.ToolController = cls.job.Tools.Group[0]
                elif i == 1:  # DrillAllHoles uses second tool controller (tool 2)
                    op.ToolController = cls.job.Tools.Group[1]
            # Comment operation has no tool controller (None)

            # Add to job operations
            cls.job.Operations.addObject(op)

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        self.pp = PathPost.PostProcessor(self.job, "generic", "", "")

    def tearDown(self):
        pass

    def test000(self):

        # check that the test file is structured correctly
        self.assertEqual(len(self.job.Tools.Group), 2)
        self.assertEqual(len(self.job.Fixtures), 2)
        self.assertEqual(
            len(self.job.Operations.Group), 3
        )  # Updated back to 3 operations, Comment has no tool controller

        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"

    def test010(self):
        postlist = self.pp._buildPostList()

        self.assertTrue(type(postlist) is list)

        firstoutputitem = postlist[0]
        self.assertTrue(type(firstoutputitem) is tuple)
        self.assertTrue(type(firstoutputitem[0]) is str)
        self.assertTrue(type(firstoutputitem[1]) is list)

    def test020(self):
        # Without splitting, result should be list of one item
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        self.assertEqual(len(postlist), 1)

    def test030(self):
        # No splitting should include all ops, tools, and fixtures
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        firstoutputitem = postlist[0]
        firstoplist = firstoutputitem[1]
        if self.debug:
            print(self._format_postables(postlist, "test030: No splitting, order by Operation"))
        self.assertEqual(len(firstoplist), 14)

    def test040(self):
        # Test splitting by tool
        # ordering by tool with toolnumber for string
        teststring = "%T.nc"
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = teststring
        self.job.OrderOutputBy = "Tool"
        postlist = self.pp._buildPostList()

        firstoutputitem = postlist[0]
        if self.debug:
            print(self._format_postables(postlist, "test040: Split by tool, order by Tool"))
        self.assertTrue(firstoutputitem[0] == str(5))

        # check length of output
        firstoplist = firstoutputitem[1]
        self.assertEqual(len(firstoplist), 5)

    def test050(self):
        # ordering by tool with tool description for string
        teststring = "%t.nc"
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = teststring
        self.job.OrderOutputBy = "Tool"
        postlist = self.pp._buildPostList()

        firstoutputitem = postlist[0]
        self.assertTrue(firstoutputitem[0] == "TC__7_16__two_flute")

    def test060(self):
        # Ordering by fixture and splitting
        teststring = "%W.nc"
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = teststring
        self.job.OrderOutputBy = "Fixture"
        postlist = self.pp._buildPostList()

        firstoutputitem = postlist[0]
        firstoplist = firstoutputitem[1]
        self.assertEqual(len(firstoplist), 6)
        self.assertTrue(firstoutputitem[0] == "G54")

    def test090_buildpostlist_returns_postable_instances(self):
        """
        Test that all items returned by _buildPostList are Postable instances.

        Given: A job with 3 operations, 2 tool controllers, and 2 fixtures
        When: _buildPostList() is called with no-split, order-by-operation
        Then: Every item in the flat sublist is an instance of PostList.Postable

        Example:
            postlist = pp._buildPostList()
            for _, sublist in postlist:
                for item in sublist:
                    assert isinstance(item, PostList.Postable)
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        for item in sublist:
            self.assertIsInstance(
                item,
                PostList.Postable,
                f"Expected Postable, got {type(item).__name__}",
            )

    def test091_postable_standard_fields(self):
        """
        Test that every Postable exposes the required standard interface.

        Given: A job producing fixture, tool_controller, and operation items
        When: _buildPostList() is called
        Then: Each Postable has:
            - item_type: a non-empty string
            - label: a string
            - path: a Path.Path instance
            - source: the original object or None
            - data: a dict

        Example:
            item.item_type == "operation"
            item.label == "outsideprofile"
            isinstance(item.path, Path.Path)
            isinstance(item.data, dict)
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        for item in sublist:
            self.assertIsInstance(item.item_type, str)
            self.assertTrue(len(item.item_type) > 0)
            self.assertIsInstance(item.label, str)
            self.assertIsInstance(item.path, Path.Path)
            self.assertIsInstance(item.data, dict)

    def test092_postable_item_types_correct(self):
        """
        Test that item_type is set correctly for each kind of postable.

        Given: A job ordered by fixture with 2 fixtures, producing fixture, TC, and op items
        When: _buildPostList() is called with split=True, order-by-fixture
        Then: The first item in the first group is item_type="fixture",
              and the set of item_types across all items includes "tool_controller"
              and "operation".

        Example:
            sublist[0].item_type == "fixture"
            types = {item.item_type for item in sublist}
            assert "tool_controller" in types
            assert "operation" in types
        """
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = "%W.nc"
        self.job.OrderOutputBy = "Fixture"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        self.assertEqual(sublist[0].item_type, "fixture")
        types = {item.item_type for item in sublist}
        self.assertIn("tool_controller", types)
        self.assertIn("operation", types)

    def test093_postable_path_is_copy(self):
        """
        Test that Postable wraps a copy of the source path, not the original object.

        Given: An operation with a non-empty path in the job
        When: _buildPostList() creates a Postable for that operation
        Then: postable.path is a different Python object from source.Path
              so mutations to postable.path do not touch the document object.

        Example:
            op_item.source.Path is not op_item.path  # different object
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        for item in sublist:
            if item.item_type == "operation" and item.source is not None:
                self.assertIsNot(
                    item.path,
                    item.source.Path,
                    "Postable.path must be a copy, not the original source.Path",
                )

    def test094_tool_controller_postable_has_tool_number(self):
        """
        Test that tool_controller Postables carry the tool number in data["tool_number"].

        Given: A job with tool controllers whose ToolNumber values are 5 and 2
        When: _buildPostList() is called
        Then: Each tool_controller Postable has data["tool_number"] matching ToolNumber.

        Example:
            tc_item.item_type == "tool_controller"
            tc_item.data["tool_number"] == 5
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        tc_items = [item for item in sublist if item.item_type == "tool_controller"]
        self.assertTrue(len(tc_items) > 0, "Expected at least one tool_controller item")
        for tc_item in tc_items:
            self.assertIn("tool_number", tc_item.data)
            self.assertIsInstance(tc_item.data["tool_number"], (int, float))

    def test095_operation_postable_tool_controller_is_postable(self):
        """
        Test that accessing ToolController on an operation Postable returns a Postable,
        not the raw FreeCAD document object.

        When legacy post scripts or other code accesses item.ToolController on an
        operation Postable, they must receive a Postable wrapper—not a live reference
        to the real TC document object.  Returning the live object allows callers to
        mutate it, which marks the operation dirty even though no re-machining occurred.

        Given: A job with operations linked to tool controllers
        When: _buildPostList() is called and we access .ToolController on an operation item
        Then: The returned value is a PostList.Postable instance (not a FreeCAD document object)
              and its item_type is "tool_controller"

        Example:
            op_item.item_type == "operation"
            isinstance(op_item.ToolController, PostList.Postable)  # True
            op_item.ToolController.item_type == "tool_controller"  # True
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        op_items = [
            item
            for item in sublist
            if item.item_type == "operation"
            and item.source is not None
            and hasattr(item.source, "ToolController")
            and item.source.ToolController is not None
        ]
        self.assertTrue(len(op_items) > 0, "Expected at least one operation with a ToolController")
        for op_item in op_items:
            tc = op_item.ToolController
            self.assertIsInstance(
                tc,
                PostList.Postable,
                f"op_item.ToolController must be a Postable, got {type(tc).__name__}",
            )
            self.assertEqual(tc.item_type, "tool_controller")

    def test096_operation_postable_tool_controller_is_isolated(self):
        """
        Test that the Postable ToolController returned from an operation Postable is
        a copy, so mutations do not propagate back to the real FreeCAD TC document object.

        Given: An operation Postable whose ToolController attribute returns a Postable
        When: We replace the Postable TC's path with a new empty path
        Then: The original FreeCAD TC document object's Path is unchanged

        Example:
            original_tc_path = op_item.source.ToolController.Path
            op_item.ToolController.path = Path.Path([])    # mutate the copy
            op_item.source.ToolController.Path == original_tc_path  # True — unaffected
        """
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        _, sublist = postlist[0]
        op_items = [
            item
            for item in sublist
            if item.item_type == "operation"
            and item.source is not None
            and hasattr(item.source, "ToolController")
            and item.source.ToolController is not None
        ]
        self.assertTrue(len(op_items) > 0, "Expected at least one operation with a ToolController")
        op_item = op_items[0]
        real_tc = op_item.source.ToolController
        original_size = real_tc.Path.Size if real_tc.Path else 0

        # Mutate the postable TC's path
        op_item.ToolController.path = Path.Path([])

        # The real TC document object must be unaffected
        new_size = real_tc.Path.Size if real_tc.Path else 0
        self.assertEqual(
            original_size,
            new_size,
            "Mutating the Postable TC path must not affect the real TC document object",
        )

    def test097_buildpostlist_does_not_touch_operations(self):
        """
        Test that _buildPostList() does not mark operations as Touched (needing recompute).

        Post-processing is a read-only activity: it must produce G-code from the current
        operation state without side-effecting the FreeCAD document.  When operations are
        marked Touched after postprocessing, the user sees spurious "recompute required"
        prompts every time they post-process.

        Given:  A job whose operations are up-to-date (not Touched) after document recompute
        When:   _buildPostList() is called
        Then:   No operation that was clean before the call is Touched afterwards

        Example:
            doc.recompute()
            # op.State does not contain "Touched"
            pp._buildPostList()
            # op.State still does not contain "Touched"
        """
        # Ensure the document starts in a fully-computed state
        self.job.Document.recompute()

        ops = list(self.job.Operations.Group)

        # Record which operations are clean before postprocessing.
        # (Some mock ops without execute() may already be Touched; skip those.)
        clean_before = {op.Name for op in ops if "Touched" not in op.State}

        # At least some operations should be clean to make the test meaningful
        self.assertGreater(
            len(clean_before),
            0,
            "No operations were clean after recompute — test cannot validate the invariant",
        )

        self.job.OrderOutputBy = "Operation"
        self.job.SplitOutput = False
        self.pp._buildPostList()

        # No previously-clean operation should now be Touched
        for op in ops:
            if op.Name in clean_before:
                self.assertNotIn(
                    "Touched",
                    op.State,
                    f"Operation {op.Label!r} was Touched by _buildPostList() — "
                    "post-processing must not mark operations dirty.",
                )


class TestJobPropertyOverrides(unittest.TestCase):
    """Test job-level postprocessor property overrides."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.newDocument("job_override_test")

        # Create test geometry
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create job
        cls.job = PathJob.Create("OverrideTestJob", [box], None)
        cls.job.PostProcessor = "linuxcnc_legacy"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54"]
        cls.job.Machine = "TestMachine"

        # Create tool
        from Path.Tool.toolbit import ToolBit

        tool_attrs = {
            "name": "TestTool",
            "shape": "endmill.fcstd",
            "parameter": {"Diameter": 6.0},
            "attribute": {},
        }
        toolbit = ToolBit.from_dict(tool_attrs)
        tool = toolbit.attach_to_doc(doc=cls.doc)
        tool.Label = "6mm_Endmill"

        tc = PathToolController.Create("TC_Test_Tool", tool, 1)
        tc.Label = "TC: 6mm Endmill"
        cls.job.Proxy.addToolController(tc)

        # Create operation
        profile_op = cls.doc.addObject("Path::FeaturePython", "TestProfile")
        profile_op.Label = "TestProfile"
        profile_op.Path = Path.Path(
            [
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                Path.Command("G1", {"X": 100.0, "Y": 0.0, "Z": -5.0, "F": 100.0}),
                Path.Command("G1", {"X": 100.0, "Y": 100.0, "Z": -5.0}),
                Path.Command("G1", {"X": 0.0, "Y": 100.0, "Z": -5.0}),
                Path.Command("G1", {"X": 0.0, "Y": 0.0, "Z": -5.0}),
                Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            ]
        )
        cls.job.Operations.addObject(profile_op)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def _create_test_machine(self, **properties):
        """Create a test machine with specified postprocessor properties."""
        from Machine.models.machine import Machine, Toolhead, ToolheadType

        machine = Machine.create_3axis_config()
        machine.name = "TestMachine"
        machine.postprocessor_file_name = "generic"
        machine.postprocessor_properties = {
            "pierce_delay": 1000,
            "cooling_delay": 500,
            "force_rapid_feeds": False,
            "show_dialog": False,  # Disable dialogs for automated tests
            **properties,
        }

        # Add toolhead
        toolhead = Toolhead(
            name="Default Toolhead",
            toolhead_type=ToolheadType.ROTARY,
            id="toolhead1",
            max_power_kw=2.2,
            max_rpm=24000,
            min_rpm=6000,
            tool_change="manual",
        )
        machine.toolheads = [toolhead]
        return machine

    def test_job_property_overrides_basic(self):
        """
        Test that job-level postprocessor property overrides work correctly.

        Expected:
            - Job overrides take precedence over machine defaults
            - Only specified keys are overridden
            - Invalid JSON is handled gracefully
        """
        from Path.Post.Processor import PostProcessor
        from Machine.models.machine import MachineFactory

        # Reset job overrides to clean state
        self.job.PostProcessorPropertyOverrides = "{}"

        # Create test machine
        machine = self._create_test_machine()

        # Mock MachineFactory to return our test machine
        original_get_machine = MachineFactory.get_machine
        MachineFactory.get_machine = lambda name: machine

        try:
            # Test 1: Basic override functionality
            self.job.PostProcessorPropertyOverrides = '{"pierce_delay": 1800, "cooling_delay": 700}'

            processor = PostProcessor(self.job, "", "", "mm")
            # Call export2 to trigger the override mechanism
            processor.export2()

            # Verify overrides were applied
            self.assertEqual(processor._machine.postprocessor_properties["pierce_delay"], 1800)
            self.assertEqual(processor._machine.postprocessor_properties["cooling_delay"], 700)
            # Verify non-overridden property stays at machine default
            self.assertEqual(
                processor._machine.postprocessor_properties["force_rapid_feeds"], False
            )

            # Test 2: Empty overrides do nothing
            machine2 = self._create_test_machine()  # Fresh machine instance
            MachineFactory.get_machine = lambda name: machine2
            self.job.PostProcessorPropertyOverrides = "{}"
            processor = PostProcessor(self.job, "", "", "mm")
            processor.export2()
            self.assertEqual(processor._machine.postprocessor_properties["pierce_delay"], 1000)
            self.assertEqual(processor._machine.postprocessor_properties["cooling_delay"], 500)

            # Test 3: Invalid JSON is an error
            machine3 = self._create_test_machine()  # Fresh machine instance
            MachineFactory.get_machine = lambda name: machine3
            self.job.PostProcessorPropertyOverrides = (
                '{"pierce_delay": 1800,'  # Missing closing brace
            )
            processor = PostProcessor(self.job, "", "", "mm")
            with self.assertRaisesRegex(
                CAMValueError, "Invalid PostProcessorPropertyOverrides JSON"
            ) as e:
                processor.export2()

            # Test 4: Unknown keys are ignored
            machine4 = self._create_test_machine()  # Fresh machine instance
            MachineFactory.get_machine = lambda name: machine4
            self.job.PostProcessorPropertyOverrides = (
                '{"unknown_property": 1234, "pierce_delay": 1500}'
            )
            processor = PostProcessor(self.job, "", "", "mm")
            processor.export2()
            # Known property should be overridden
            self.assertEqual(processor._machine.postprocessor_properties["pierce_delay"], 1500)
            # Unknown property should not be added
            self.assertNotIn("unknown_property", processor._machine.postprocessor_properties)

        finally:
            # Restore original MachineFactory
            MachineFactory.get_machine = original_get_machine

    def test_job_property_overrides_with_plasma(self):
        """
        Test that job-level overrides affect G-code output with plasma postprocessor.

        Expected:
            - Override values are reflected in the final G-code output
        """
        from Path.Post.scripts.generic_sheet_cutting_post import GenericSheetCutting
        from Machine.models.machine import MachineFactory

        # Reset job overrides to clean state
        self.job.PostProcessorPropertyOverrides = "{}"

        # Create machine with plasma postprocessor
        machine = self._create_test_machine(pierce_delay=1000)
        machine.postprocessor_file_name = "generic_sheet_cutting"

        # Add M3/M4 commands to trigger plasma behavior
        plasma_commands = [
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
            Path.Command("M3", {}),  # Torch on - should trigger pierce delay
            Path.Command("G1", {"X": 100.0, "Y": 0.0, "Z": -5.0, "F": 100.0}),
            Path.Command("M5", {}),  # Torch off
            Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
        ]

        # Update operation path
        profile_op = self.doc.getObject("TestProfile")
        original_path = profile_op.Path
        profile_op.Path = Path.Path(plasma_commands)

        try:
            # Mock MachineFactory to return our test machine with dialog disabled
            original_get_machine = MachineFactory.get_machine
            MachineFactory.get_machine = lambda name: machine

            # Test with no overrides (machine defaults)
            self.job.PostProcessorPropertyOverrides = "{}"
            processor = GenericSheetCutting(self.job, "", "", "mm")
            # Ensure the processor uses our test machine with dialog disabled
            processor._machine = machine
            results = processor.export2()
            gcode_no_override = ""
            for section_name, gcode in results:
                gcode_no_override += gcode

            # Test with pierce_delay override
            self.job.PostProcessorPropertyOverrides = '{"pierce_delay": 2500}'  # 2.5 seconds
            processor = GenericSheetCutting(self.job, "", "", "mm")
            # Ensure the processor uses our test machine with dialog disabled
            processor._machine = machine
            results = processor.export2()
            gcode_with_override = ""
            for section_name, gcode in results:
                gcode_with_override += gcode

            # The override should result in different G-code
            self.assertNotEqual(gcode_no_override, gcode_with_override)

            # Verify the specific G4 dwell command reflects the override
            # With 2500ms override, we should see G4 P2.5
            self.assertIn("G4 P2.5", gcode_with_override)
            # With 1000ms default, we should see G4 P1.0
            self.assertIn("G4 P1.0", gcode_no_override)

        finally:
            # Restore original path and MachineFactory
            profile_op.Path = original_path
            MachineFactory.get_machine = original_get_machine

    def test_job_property_overrides_template_round_trip(self):
        """
        Test that job property overrides survive template save/restore cycle.

        Expected:
            - Overrides are saved to template
            - Overrides are restored from template
            - Empty overrides are not saved to template
        """
        import json
        import tempfile
        import os

        # Set some overrides and machine
        self.job.PostProcessorPropertyOverrides = '{"pierce_delay": 1800, "cooling_delay": 700}'
        self.job.Machine = "TestMachine"

        # Save to template
        template_attrs = self.job.Proxy.templateAttrs(self.job)

        # Verify overrides are in template
        self.assertIn("PostPropertyOverrides", template_attrs)
        self.assertEqual(
            template_attrs["PostPropertyOverrides"], {"pierce_delay": 1800, "cooling_delay": 700}
        )

        # Verify machine is in template
        self.assertIn("Machine", template_attrs)
        self.assertEqual(template_attrs["Machine"], "TestMachine")

        # Test empty overrides are not saved
        self.job.PostProcessorPropertyOverrides = "{}"
        template_attrs = self.job.Proxy.templateAttrs(self.job)
        self.assertNotIn("PostPropertyOverrides", template_attrs)

        # Test round-trip: save to file and restore
        self.job.PostProcessorPropertyOverrides = '{"pierce_delay": 1500}'
        self.job.Machine = ""  # Use empty machine (no machine) for test
        template_attrs = self.job.Proxy.templateAttrs(self.job)

        with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
            json.dump(template_attrs, f)
            template_path = f.name

        try:
            # Create a new job and restore from template
            new_job = PathJob.Create("TemplateTestJob", [self.job.Stock], None)
            new_job.Proxy.setFromTemplateFile(new_job, template_path)

            # Verify overrides were restored
            self.assertEqual(new_job.PostProcessorPropertyOverrides, '{"pierce_delay": 1500}')

            # Verify machine was restored
            self.assertEqual(new_job.Machine, "")

        finally:
            os.unlink(template_path)

    def test_job_property_overrides_beat_merge_machine_config(self):
        """
        Regression test: job-level overrides must win even for properties
        that _merge_machine_config() also sets directly from the live
        machine model (e.g. f_for_rapid_moves, tool_change).
        """
        from Path.Post.Processor import PostProcessor
        from Machine.models.machine import MachineFactory

        self.job.PostProcessorPropertyOverrides = "{}"

        machine = self._create_test_machine()
        # Machine-level config: opposite of what we will override to.
        machine.processing.f_for_rapid_moves = True
        machine.processing.tool_change = True

        original_get_machine = MachineFactory.get_machine
        MachineFactory.get_machine = lambda name: machine

        try:
            self.job.PostProcessorPropertyOverrides = (
                '{"f_for_rapid_moves": false, "tool_change": false}'
            )
            processor = PostProcessor(self.job, "", "", "mm")
            processor._machine = machine
            processor.export2()

            self.assertFalse(
                processor.values["F_FOR_RAPID_MOVES"],
                "job override of f_for_rapid_moves must win over machine.processing",
            )
            self.assertFalse(
                processor.values["TOOL_CHANGE"],
                "job override of tool_change must win over machine.processing",
            )

        finally:
            MachineFactory.get_machine = original_get_machine


class TestToolChangeFormatOutput(unittest.TestCase):
    """
    The G-code written for each tool change format.

    PostProcessor._buildPostList() applies the format to the M6 as annotations and
    the G-code is written by convert_command_to_gcode(). The line number leads the
    line, block delete leads the line number, and the words are separated by the
    machine's command_space.

    The job has two tool controllers and an operation for each, T5 then T2, so the
    next tool, early tool prep and output groups can be tested.
    """

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.newDocument("tool_change_format_test")

        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        cls.job = PathJob.Create("ToolChangeFormatJob", [box], None)
        cls.job.PostProcessor = "linuxcnc_legacy"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54"]
        cls.job.Machine = "linuxcnc"

        # Two tool controllers, T5 then T2. Recompute to populate their M6
        cls.job.Tools.Group[0].ToolNumber = 5
        tc2 = PathToolController.Create()
        tc2.ToolNumber = 2
        cls.job.Proxy.addToolController(tc2)
        cls.job.Tools.Group[0].recompute()
        cls.job.Tools.Group[1].recompute()

        # A tool controller is only output if an operation uses it
        for index, name in enumerate(["TestProfile", "TestDrill"]):
            op = cls.doc.addObject("Path::FeaturePython", name)
            op.Label = name
            op.Path = Path.Path(
                [
                    Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                    Path.Command("G1", {"X": 100.0, "Y": 0.0, "Z": -5.0, "F": 100.0}),
                    Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 5.0}),
                ]
            )
            op.addProperty(
                "App::PropertyLink", "ToolController", "Base", "Tool controller for this operation"
            )
            op.ToolController = cls.job.Tools.Group[index]
            cls.job.Operations.addObject(op)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def _machine(
        self,
        fmt,
        early=False,
        assert_prep=False,
        line_numbers=False,
        command_space=" ",
        comments=False,
    ):
        """A machine with the tool change settings under test."""
        from Machine.models.machine import Machine, Toolhead, ToolheadType

        machine = Machine.create_3axis_config()
        machine.name = "TestMachine"
        machine.toolheads = [
            Toolhead(
                name="Default Toolhead",
                toolhead_type=ToolheadType.ROTARY,
                id="toolhead1",
                max_power_kw=2.2,
                max_rpm=24000,
                min_rpm=6000,
                tool_change="manual",
                toolhead_wait=1.0,
            )
        ]
        machine.processing.tool_change_format = fmt
        machine.processing.early_tool_prep = early
        machine.processing.assert_tool_prep = assert_prep
        machine.output.formatting.line_numbers = line_numbers
        machine.output.formatting.line_number_start = 100
        machine.output.formatting.line_increment = 10
        machine.output.formatting.command_space = command_space
        machine.output.comments.enabled = comments
        return machine

    def _post(self, machine):
        # one file, in operation order; the postlist tests change this on the job
        self.job.SplitOutput = False
        self.job.PostProcessorOutputFile = ""
        self.job.OrderOutputBy = "Operation"
        post = PathPost.PostProcessor(self.job, "", "", "mm")
        post._machine = machine
        post.apply_configuration_bundle()
        return post

    def _gcode(self, post, command):
        """convert_command_to_gcode() needs the machine state that the export loop provides."""
        with post.use_machine_state():
            return post.convert_command_to_gcode(command)

    def _m6(self, fmt, line_number=None, block_delete=False, tool=3, next_tool="5"):
        """An M6 as _buildPostList() writes it, with the N that _add_line_numbers adds."""
        params = {"T": tool}
        if line_number is not None:
            params = {"N": line_number, **params}
        annotations = {
            Constants.ANNOT_TOOL_CHANGE_FORMAT: fmt.value,
            Constants.ANNOT_NEXT_TOOL: next_tool,
        }
        if block_delete:
            annotations["blockdelete"] = "True"  # not set by the post itself
        return Path.Command("M6", params, annotations)

    @staticmethod
    def _tool_lines(gcode):
        """The M6 / T lines of the G-code (line numbers kept, comments skipped)."""
        lines = []
        for line in gcode.split("\n"):
            line = line.strip()
            if line.startswith("(") or line.startswith(";"):
                continue
            if re.search(r"\b(M6|T\d+)\b", line):
                lines.append(line)
        return lines

    # ------------------------------------------------------------------
    # the postlist: what _buildPostList() makes of the tool changes (T5 then T2)
    # ------------------------------------------------------------------

    @staticmethod
    def _tool_words(sublist):
        """Names of the M6 and bare T commands in the postables, in order."""
        words = []
        for item in sublist:
            for cmd in item.path.Commands if item.path else []:
                if cmd.Name in Constants.MCODE_TOOL_CHANGE or cmd.Name.startswith(
                    tuple(Constants.GCODE_BARE_T)
                ):
                    words.append(cmd.Name)
        return words

    def _tool_change_postlist(self, fmt, early=False, assert_prep=False, split=False):
        """_buildPostList() with the tool change settings, the tools ordered T5 then T2."""
        post = self._post(self._machine(fmt, early=early, assert_prep=assert_prep))
        self.job.SplitOutput = split
        self.job.PostProcessorOutputFile = "%T.nc" if split else ""
        self.job.OrderOutputBy = "Tool"
        return post._buildPostList()

    def test070_tool_change_annotations(self):
        """
        Test that the M6 keeps T<current tool> and records the format and next tool.

        Everything downstream that reads Parameters["T"] (G43 TLO, tool tracking)
        relies on the M6 keeping T<current tool>, whatever the format. The format
        itself is applied when the G-code is written.

        Expected for T5 then T2, every format:
            M6 T5  ANNOT_TOOL_CHANGE_FORMAT=<format>, ANNOT_NEXT_TOOL="2"
            M6 T2  ANNOT_TOOL_CHANGE_FORMAT=<format>, no ANNOT_NEXT_TOOL
        """
        for fmt in Constants.ToolChangeFormat:
            with self.subTest(fmt=fmt):
                _, sublist = self._tool_change_postlist(fmt)[0]
                m6s = [
                    cmd
                    for item in sublist
                    if item.item_type == "tool_controller"
                    for cmd in item.path.Commands
                    if cmd.Name in Constants.MCODE_TOOL_CHANGE
                ]
                self.assertEqual(2, len(m6s))
                self.assertEqual([5, 2], [int(cmd.Parameters["T"]) for cmd in m6s])
                for cmd in m6s:
                    self.assertEqual(fmt.value, cmd.Annotations[Constants.ANNOT_TOOL_CHANGE_FORMAT])
                self.assertEqual("2", m6s[0].Annotations[Constants.ANNOT_NEXT_TOOL])
                self.assertNotIn(Constants.ANNOT_NEXT_TOOL, m6s[1].Annotations)

    def test071_no_tool_prep_without_early_tool_prep(self):
        """
        Test that no standalone T is written unless early_tool_prep is on.
        assert_tool_prep does nothing by itself.

        The exception is M6 T<next>: its M6 line names the NEXT tool, so the first
        tool must be prepped with a T whether or not early_tool_prep is on.

        Expected, with and without assert_tool_prep:
            m6_t_early_prep   T5, M6, M6
            every other       M6, M6
        """
        for fmt in Constants.ToolChangeFormat:
            for assert_prep in (False, True):
                with self.subTest(fmt=fmt, assert_prep=assert_prep):
                    _, sublist = self._tool_change_postlist(fmt, assert_prep=assert_prep)[0]
                    expected = ["M6", "M6"]
                    if fmt == Constants.ToolChangeFormat.M6_T_EARLY_PREP:
                        expected = ["T5", "M6", "M6"]
                    self.assertEqual(expected, self._tool_words(sublist))

    def test072_early_tool_prep_m6_only(self):
        """
        Test early tool prep with M6 only: each tool is prepped before its M6,
        the next tool right after each M6.

        Expected:
            T5
            M6
            T2      <- prep next tool
            (ops with T5)
            M6      <- changes to the prepped T2
        """
        fmt = Constants.ToolChangeFormat.M6_ONLY
        _, sublist = self._tool_change_postlist(fmt, early=True)[0]
        self.assertEqual(["T5", "M6", "T2", "M6"], self._tool_words(sublist))

    def test073_assert_tool_prep(self):
        """
        Test assert_tool_prep writes the T again right before every M6, so the
        program can be started partway through. A T that directly follows the
        same T is not repeated.

        Expected:
            T5      <- initial prep and assert are one T
            M6
            T2      <- prep next tool
            (ops with T5)
            T2      <- assert
            M6
        """
        fmt = Constants.ToolChangeFormat.M6_ONLY
        _, sublist = self._tool_change_postlist(fmt, early=True, assert_prep=True)[0]
        self.assertEqual(["T5", "M6", "T2", "T2", "M6"], self._tool_words(sublist))

    def test074_early_tool_prep_format_m6_t(self):
        """
        Test the M6 T<next> format: the T on the M6 line is the NEXT tool, so the
        first tool is prepped with a standalone T before its M6 and nothing else
        is added.

        Expected:
            T5
            M6 T2   <- changes to T5, preps T2   (an M6 with next_tool=2)
            (ops with T5)
            M6      <- changes to T2, no next tool
        """
        fmt = Constants.ToolChangeFormat.M6_T_EARLY_PREP
        _, sublist = self._tool_change_postlist(fmt, early=True)[0]
        self.assertEqual(["T5", "M6", "M6"], self._tool_words(sublist))

    def test075_early_tool_prep_with_m6_t(self):
        """
        Test early tool prep with the plain M6 T<current> format: the next tool is
        prepped after each M6.

        Expected:
            M6 T5
            T2      <- prep next tool
            (ops with T5)
            M6 T2
        """
        fmt = Constants.ToolChangeFormat.M6_T
        _, sublist = self._tool_change_postlist(fmt, early=True)[0]
        self.assertEqual(["M6", "T2", "M6"], self._tool_words(sublist))

    def test076_no_tool_change_writes_no_tool_prep(self):
        """
        Test that no T is added when tool changes are suppressed, even with
        early_tool_prep and assert_tool_prep on.
        """
        fmt = Constants.ToolChangeFormat.NO_TOOL_CHANGE
        _, sublist = self._tool_change_postlist(fmt, early=True, assert_prep=True)[0]
        self.assertEqual(["M6", "M6"], self._tool_words(sublist))

    def test077_tool_prep_crosses_output_groups(self):
        """
        Test that with output split by tool the next tool crosses the group
        boundary, and the first tool of each group is prepped even though the
        previous group already prepped it, since each group is a separate file.

        Expected:
            group 1:  T5, M6, T2
            group 2:  T2, M6
        """
        fmt = Constants.ToolChangeFormat.M6_ONLY
        postlist = self._tool_change_postlist(fmt, early=True, split=True)
        self.assertEqual(2, len(postlist))
        self.assertEqual(["T5", "M6", "T2"], self._tool_words(postlist[0][1]))
        self.assertEqual(["T2", "M6"], self._tool_words(postlist[1][1]))

    def test078_tool_change_format_setting(self):
        """
        Test how the machine's tool_change_format is read.

        A missing or unrecognised format is M6 T<tool>. A plain value is accepted
        as well as the enum.
        """
        post = self._post(self._machine(Constants.ToolChangeFormat.M6_T))
        for setting, expected in [
            (None, Constants.ToolChangeFormat.M6_T),
            ("not a format", Constants.ToolChangeFormat.M6_T),
            ("t_m6", Constants.ToolChangeFormat.T_M6),
            (Constants.ToolChangeFormat.M6_ONLY, Constants.ToolChangeFormat.M6_ONLY),
        ]:
            with self.subTest(setting=setting):
                post._machine.processing.tool_change_format = setting
                self.assertEqual(expected, post._tool_change_format())

    def test010_convert_tool_change_formats(self):
        """
        Test how each format is written.

        Expected for M6 T3, with T5 as the next tool:
            m6_t             M6 T3
            t_m6             T3 M6
            m6_t_early_prep  M6 T5
            m6_only          M6
            no_tool_change   (Tool change suppressed: M6 T3)
        """
        expected = {
            Constants.ToolChangeFormat.M6_T: "M6 T3",
            Constants.ToolChangeFormat.T_M6: "T3 M6",
            Constants.ToolChangeFormat.M6_T_EARLY_PREP: "M6 T5",
            Constants.ToolChangeFormat.M6_ONLY: "M6",
        }
        for fmt, text in expected.items():
            with self.subTest(fmt=fmt):
                post = self._post(self._machine(fmt))
                self.assertEqual(text, self._gcode(post, self._m6(fmt)).strip())

        fmt = Constants.ToolChangeFormat.NO_TOOL_CHANGE
        post = self._post(self._machine(fmt, comments=True))
        gcode = self._gcode(post, self._m6(fmt))
        self.assertIn("Tool change suppressed: M6 T3", gcode)

        post = self._post(self._machine(fmt, comments=False))
        self.assertFalse(self._gcode(post, self._m6(fmt)))

    def test011_early_prep_without_next_tool(self):
        """Test that M6 T<next> with no next tool (the last tool change) is a plain M6."""
        fmt = Constants.ToolChangeFormat.M6_T_EARLY_PREP
        post = self._post(self._machine(fmt, early=True))
        cmd = Path.Command("M6", {"T": 3}, {Constants.ANNOT_TOOL_CHANGE_FORMAT: fmt.value})
        self.assertEqual("M6", self._gcode(post, cmd).strip())

    def test020_line_number_comes_first(self):
        """
        Test that the line number leads the tool change, then the T.

        Expected:
            m6_t             N100 M6 T3
            t_m6             N100 T3 M6      <- not: T3 N100 M6
            m6_t_early_prep  N100 M6 T5
            m6_only          N100 M6
        """
        expected = {
            Constants.ToolChangeFormat.M6_T: "{n}100 M6 T3",
            Constants.ToolChangeFormat.T_M6: "{n}100 T3 M6",
            Constants.ToolChangeFormat.M6_T_EARLY_PREP: "{n}100 M6 T5",
            Constants.ToolChangeFormat.M6_ONLY: "{n}100 M6",
        }
        for fmt, text in expected.items():
            with self.subTest(fmt=fmt):
                post = self._post(self._machine(fmt, line_numbers=True))
                prefix = post.values["LINE_NUMBER_PREFIX"]
                gcode = self._gcode(post, self._m6(fmt, line_number=100))
                self.assertEqual(text.format(n=prefix), gcode.strip())

    def test021_block_delete_comes_first(self):
        """
        Test that block delete leads the whole line, before the line number.

        Expected:
            m6_t   /N100 M6 T3
            t_m6   /N100 T3 M6      <- not: T3 /N100 M6
        and without a line number:
            t_m6   /T3 M6
        """
        expected = {
            Constants.ToolChangeFormat.M6_T: "/{n}100 M6 T3",
            Constants.ToolChangeFormat.T_M6: "/{n}100 T3 M6",
        }
        for fmt, text in expected.items():
            with self.subTest(fmt=fmt):
                post = self._post(self._machine(fmt, line_numbers=True))
                prefix = post.values["LINE_NUMBER_PREFIX"]
                cmd = self._m6(fmt, line_number=100, block_delete=True)
                self.assertEqual(text.format(n=prefix), self._gcode(post, cmd).strip())

        fmt = Constants.ToolChangeFormat.T_M6
        post = self._post(self._machine(fmt))
        cmd = self._m6(fmt, block_delete=True)
        self.assertEqual("/T3 M6", self._gcode(post, cmd).strip())

    def test030_tool_change_follows_command_space(self):
        """
        Test that the T and the M6 are separated by command_space like any other
        words, not by a fixed space.

        Expected with command_space="":    N100T3M6
        Expected with command_space="  ":  N100  T3  M6
        """
        fmt = Constants.ToolChangeFormat.T_M6
        for space in ("", "  "):
            with self.subTest(space=space):
                post = self._post(self._machine(fmt, line_numbers=True, command_space=space))
                prefix = post.values["LINE_NUMBER_PREFIX"]
                gcode = self._gcode(post, self._m6(fmt, line_number=100))
                self.assertEqual(space.join([f"{prefix}100", "T3", "M6"]), gcode.strip("\n"))

        fmt = Constants.ToolChangeFormat.M6_T
        post = self._post(self._machine(fmt, command_space=""))
        self.assertEqual("M6T3", self._gcode(post, self._m6(fmt)).strip())

    def test040_standalone_tool_prep_command(self):
        """
        Test the standalone T<n> prep command, with line number and block delete.

        Expected:
            T6
            N100 T6
            /N100 T6
        """
        post = self._post(self._machine(Constants.ToolChangeFormat.M6_ONLY, line_numbers=True))
        prefix = post.values["LINE_NUMBER_PREFIX"]

        self.assertEqual("T6", self._gcode(post, Path.Command("T6")).strip())
        self.assertEqual(
            f"{prefix}100 T6",
            self._gcode(post, Path.Command("T6", {"N": 100})).strip(),
        )
        self.assertEqual(
            f"/{prefix}100 T6",
            self._gcode(post, Path.Command("T6", {"N": 100}, {"blockdelete": "True"})).strip(),
        )

    def test050_export2_tool_change_formats(self):
        """
        Test the formats end to end, T5 then T2.

        Expected:
            m6_t             M6 T5, M6 T2
            t_m6             T5 M6, T2 M6
            m6_t_early_prep  T5, M6 T2, M6   (T5 is prepped, each M6 names the NEXT tool)
            m6_only          M6, M6
            no_tool_change   (no M6 or T line)
        """
        expected = {
            Constants.ToolChangeFormat.M6_T: ["M6 T5", "M6 T2"],
            Constants.ToolChangeFormat.T_M6: ["T5 M6", "T2 M6"],
            Constants.ToolChangeFormat.M6_T_EARLY_PREP: ["T5", "M6 T2", "M6"],
            Constants.ToolChangeFormat.M6_ONLY: ["M6", "M6"],
            Constants.ToolChangeFormat.NO_TOOL_CHANGE: [],
        }
        for fmt, lines in expected.items():
            with self.subTest(fmt=fmt):
                gcode = self._post(self._machine(fmt)).export2()[0][1]
                self.assertEqual(lines, self._tool_lines(gcode), gcode)

    def test051_export2_early_tool_prep(self):
        """
        Test early tool prep end to end, T5 then T2.

        Expected:
            m6_only          T5, M6, T2, M6
            m6_t_early_prep  T5, M6 T2, M6
            m6_t             M6 T5, T2, M6 T2
            m6_only+assert   T5, M6, T2, T2, M6   (the second T2 is the assert)

        t_m6 with early prep is not tested, it is the combination that the sanity
        check warns about.
        """
        cases = [
            (Constants.ToolChangeFormat.M6_ONLY, False, ["T5", "M6", "T2", "M6"]),
            (Constants.ToolChangeFormat.M6_T_EARLY_PREP, False, ["T5", "M6 T2", "M6"]),
            (Constants.ToolChangeFormat.M6_T, False, ["M6 T5", "T2", "M6 T2"]),
            (Constants.ToolChangeFormat.M6_ONLY, True, ["T5", "M6", "T2", "T2", "M6"]),
        ]
        for fmt, assert_prep, lines in cases:
            with self.subTest(fmt=fmt, assert_prep=assert_prep):
                machine = self._machine(fmt, early=True, assert_prep=assert_prep)
                gcode = self._post(machine).export2()[0][1]
                self.assertEqual(lines, self._tool_lines(gcode), gcode)

    def test052_export2_line_numbers_with_tool_change(self):
        """
        Test that every tool change line is numbered, with the number first.

        Expected, line_numbers on:
            m6_t             N.. M6 T5, N.. M6 T2
            t_m6             N.. T5 M6, N.. T2 M6
            m6_only+early    N.. T5, N.. M6, N.. T2, N.. M6
        """
        cases = [
            (Constants.ToolChangeFormat.M6_T, False, [r"{n}\d+ M6 T5", r"{n}\d+ M6 T2"]),
            (Constants.ToolChangeFormat.T_M6, False, [r"{n}\d+ T5 M6", r"{n}\d+ T2 M6"]),
            (
                Constants.ToolChangeFormat.M6_ONLY,
                True,
                [r"{n}\d+ T5", r"{n}\d+ M6", r"{n}\d+ T2", r"{n}\d+ M6"],
            ),
        ]
        for fmt, early, patterns in cases:
            with self.subTest(fmt=fmt, early=early):
                post = self._post(self._machine(fmt, early=early, line_numbers=True))
                gcode = post.export2()[0][1]
                prefix = re.escape(post.values["LINE_NUMBER_PREFIX"])
                lines = self._tool_lines(gcode)
                self.assertEqual(len(patterns), len(lines), gcode)
                for pattern, line in zip(patterns, lines):
                    self.assertRegex(line, "^" + pattern.format(n=prefix) + "$")

    def test060_tool_change_sanity_checks(self):
        """
        Test the warnings for tool change settings that do not work together.

        T# M6 with early tool prep:        warns
        assert_tool_prep, no early prep:   warns
        tool_change off, but a format:     warns, use the tool change format instead
        no_tool_change with tool_change off, or the defaults: no warnings
        """

        def notes(machine):
            return [s["Note"] for s in self._post(machine)._tool_change_sanity_checks(self.job)]

        self.assertEqual([], notes(self._machine(Constants.ToolChangeFormat.M6_T)))

        found = notes(self._machine(Constants.ToolChangeFormat.T_M6, early=True))
        self.assertEqual(1, len(found), found)
        self.assertIn("'T# M6'", found[0])

        found = notes(self._machine(Constants.ToolChangeFormat.M6_ONLY, assert_prep=True))
        self.assertEqual(1, len(found), found)
        self.assertIn("Assert Tool Prep", found[0])

        machine = self._machine(Constants.ToolChangeFormat.M6_T)
        machine.processing.tool_change = False
        found = notes(machine)
        self.assertEqual(1, len(found), found)
        self.assertIn("Tool Change Format", found[0])

        machine = self._machine(Constants.ToolChangeFormat.NO_TOOL_CHANGE)
        machine.processing.tool_change = False
        self.assertEqual([], notes(machine))
