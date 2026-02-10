# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

from Path.Post.Command import DlgSelectPostProcessor
from Path.Post.Processor import PostProcessor, PostProcessorFactory
from unittest.mock import patch, MagicMock
import FreeCAD
import Path
import Path.Post.Command as PathCommand
import Path.Post.Processor as PathPost
import Path.Post.Utils as PostUtils
import Path.Post.UtilsExport as PostUtilsExport
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import difflib
import os
import unittest

from .FilePathTestUtils import assertFilePathsEqual

PathCommand.LOG_MODULE = Path.Log.thisModule()
Path.Log.setLevel(Path.Log.Level.INFO, PathCommand.LOG_MODULE)


class TestFileNameGenerator(unittest.TestCase):
    r"""
    String substitution allows the following:
    %D ... directory of the active document
    %d ... name of the active document (with extension)
    %M ... user macro directory
    %j ... name of the active Job object


    The Following can be used if output is being split. If Output is not split
    these will be ignored.

    %S ... Sequence Number (default)

    Either:
    %T ... Tool Number
    %t ... Tool Controller label

    %W ... Work Coordinate System
    %O ... Operation Label

    |split on| use | Ignore |
    |-----------|-------|--------|
    |fixture | %W | %O %T %t |
    |Operation| %O | %T %t %W |
    |Tool| **Either %T or %t** | %O %W |

    The confusing bit is that for split on tool,  it will use EITHER the tool number or the tool label.
    If you include both, the second one overrides the first.
    And for split on operation, where including the tool should be possible, it ignores it altogether.

        self.job.Fixtures = ["G54"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Fixture"

    Assume:
    active document: self.assertTrue(filename, f"{home}/testdoc.fcstd
    user macro: ~/.local/share/FreeCAD/Macro
    Job:  MainJob
    Operations:
        OutsideProfile
        DrillAllHoles
    TC: 7/16" two flute  (5)
    TC: Drill (2)
    Fixtures: (G54, G55)

    Strings should be sanitized like this to ensure valid filenames
    # import re
    # filename="TC: 7/16" two flute"
    # >>> re.sub(r"[^\w\d-]","_",filename)
    # "TC__7_16__two_flute"

    """

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("TestFileNaming")
        cls.testfilename = cls.doc.Name
        cls.testfilepath = os.getcwd()
        cls.macro = FreeCAD.getUserMacroDir()

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessor = "linuxcnc"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]

        # Create a tool controller for testing tool-related substitutions
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

        tc = PathToolController.Create("TC_Test_Tool", tool, 5)
        tc.Label = "TC: 6mm Endmill"
        cls.job.addObject(tc)

        # Create a simple mock operation for testing operation-related substitutions
        profile_op = cls.doc.addObject("Path::FeaturePython", "TestProfile")
        profile_op.Label = "OutsideProfile"
        # Path::FeaturePython objects already have a Path property
        profile_op.Path = Path.Path()
        cls.job.Operations.addObject(profile_op)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def test000(self):
        # Test basic name generation with empty string
        FreeCAD.setActiveDocument(self.doc.Label)
        teststring = ""
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)
        Path.Log.debug(filename)
        assertFilePathsEqual(
            self, filename, os.path.join(self.testfilepath, f"{self.testfilename}.nc")
        )

    def test010(self):
        # Substitute current file path
        teststring = "%D/testfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        print(os.path.normpath(filename))
        assertFilePathsEqual(self, filename, f"{self.testfilepath}/testfile.nc")

    def test015(self):
        # Test basic string substitution without splitting
        teststring = "~/Desktop/%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, "~/Desktop/MainJob.nc")

    def test020(self):
        teststring = "%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        expected = os.path.join(self.testfilepath, f"{self.testfilename}.nc")

        assertFilePathsEqual(self, filename, expected)

    def test030(self):
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.macro}outfile.nc")

    def test040(self):
        # unused substitution strings should be ignored
        teststring = "%d%T%t%W%O/testdoc.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.testfilename}/testdoc.nc")

    def test045(self):
        """Testing the sequence number substitution"""
        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        expected_filenames = [f"TestFileNaming{os.sep}testdoc.nc"] + [
            f"TestFileNaming{os.sep}testdoc-{i}.nc" for i in range(1, 5)
        ]
        for expected_filename in expected_filenames:
            filename = next(filename_generator)
            assertFilePathsEqual(self, filename, expected_filename)

    def test046(self):
        """Testing the sequence number substitution"""
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        expected_filenames = [
            os.path.join(self.testfilepath, f"{i}-TestFileNaming.nc") for i in range(5)
        ]
        for expected_filename in expected_filenames:
            filename = next(filename_generator)
            assertFilePathsEqual(self, filename, expected_filename)

    def test050(self):
        # explicitly using the sequence number should include it where indicated.
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "0-TestFileNaming.nc"))

    def test060(self):
        """Test subpart naming"""
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(teststring, "Append Unique ID on conflict")

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("Tool")
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, f"{self.macro}outfile-Tool.nc")

    def test070(self):
        """Test %T substitution (tool number) with actual tool controller"""
        teststring = "%T.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("5")  # Tool number from our test tool controller
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "5.nc"))

    def test071(self):
        """Test %t substitution (tool description) with actual tool controller"""
        teststring = "%t.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("TC__6mm_Endmill")  # Sanitized tool label
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "TC__6mm_Endmill.nc"))

    def test072(self):
        """Test %W substitution (work coordinate system/fixture)"""
        teststring = "%W.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("G54")  # First fixture from our job setup
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "G54.nc"))

    def test073(self):
        """Test %O substitution (operation label)"""
        teststring = "%O.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        generator.set_subpartname("OutsideProfile")  # Operation label from our test setup
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "OutsideProfile.nc"))

    def test075(self):
        """Test path and filename substitutions together"""
        teststring = "%D/%j_%S.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        # %D should resolve to document directory (empty since doc has no filename)
        # %j should resolve to job name "MainJob"
        # %S should resolve to sequence number "0"
        assertFilePathsEqual(self, filename, os.path.join(".", "MainJob_0.nc"))

    def test076(self):
        """Test invalid substitution characters are ignored"""
        teststring = "%X%Y%Z/invalid_%Q.nc"
        self.job.PostProcessorOutputFile = teststring

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        # Invalid substitutions should be removed, leaving "invalid_.nc"
        assertFilePathsEqual(self, filename, os.path.join(self.testfilepath, "invalid_.nc"))


class TestResolvingPostProcessorName(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("boxtest")

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "")

    def tearDown(self):
        pass

    def test010(self):
        # Test if post is defined in job
        self.job.PostProcessor = "linuxcnc"
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "linuxcnc")

    def test020(self):
        # Test if post is invalid
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=False):
            with self.assertRaises(ValueError):
                PathCommand._resolve_post_processor_name(self.job)

    def test030(self):
        # Test if post is defined in prefs
        self.job.PostProcessor = ""
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "grbl")

        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "grbl")

    def test040(self):
        # Test if user interaction is correctly handled
        if FreeCAD.GuiUp:
            with patch("Path.Post.Command.DlgSelectPostProcessor") as mock_dlg, patch(
                "Path.Post.Processor.PostProcessor.exists", return_value=True
            ):
                mock_dlg.return_value.exec_.return_value = "generic"
                postname = PathCommand._resolve_post_processor_name(self.job)
                self.assertEqual(postname, "generic")
        else:
            with patch.object(self.job, "PostProcessor", ""):
                with self.assertRaises(ValueError):
                    PathCommand._resolve_post_processor_name(self.job)


class TestPostProcessorFactory(unittest.TestCase):
    """Test creation of postprocessor objects."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        # Create a new document instead of opening external file
        cls.doc = FreeCAD.newDocument("boxtest")

        # Create a simple geometry object for the job
        import Part

        box = cls.doc.addObject("Part::Box", "TestBox")
        box.Length = 100
        box.Width = 100
        box.Height = 20

        # Create CAM job programmatically
        cls.job = PathJob.Create("MainJob", [box], None)
        cls.job.PostProcessor = "linuxcnc"
        cls.job.PostProcessorOutputFile = ""
        cls.job.SplitOutput = False
        cls.job.OrderOutputBy = "Operation"
        cls.job.Fixtures = ["G54", "G55"]

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test020(self):
        # test creation of postprocessor object
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        self.assertTrue(post is not None)
        self.assertTrue(hasattr(post, "export"))
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test030(self):
        # test wrapping of old school postprocessor scripts
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        self.assertTrue(post is not None)
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test040(self):
        """Test that the __name__ of the postprocessor is correct."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc_legacy")
        self.assertEqual(post.script_module.__name__, "linuxcnc_legacy_post")


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
                obj_label = getattr(obj, "Label", str(type(obj).__name__))
                output.append(f"    [{obj_idx}] {obj_label}")

                # Determine object type/role
                obj_type = type(obj).__name__
                if obj_type == "_FixtureSetupObject":
                    output.append(f"        Type: Fixture Setup")
                    if hasattr(obj, "Path") and obj.Path and len(obj.Path.Commands) > 0:
                        fixture_cmd = obj.Path.Commands[0]
                        output.append(f"        Fixture: {fixture_cmd.Name}")
                elif obj_type == "_CommandObject":
                    output.append(f"        Type: Command Object")
                    if hasattr(obj, "Path") and obj.Path and len(obj.Path.Commands) > 0:
                        cmd = obj.Path.Commands[0]
                        params = " ".join(
                            f"{k}:{v}"
                            for k, v in zip(
                                cmd.Parameters.keys() if hasattr(cmd.Parameters, "keys") else [],
                                (
                                    cmd.Parameters.values()
                                    if hasattr(cmd.Parameters, "values")
                                    else cmd.Parameters
                                ),
                            )
                        )
                        output.append(f"        Command: {cmd.Name} {params}")
                elif hasattr(obj, "TypeId"):
                    # Check if it's a tool controller
                    if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "__class__"):
                        proxy_name = obj.Proxy.__class__.__name__
                        if "ToolController" in proxy_name:
                            output.append(f"        Type: Tool Controller")
                            if hasattr(obj, "ToolNumber"):
                                output.append(f"        Tool Number: {obj.ToolNumber}")
                            if hasattr(obj, "Path") and obj.Path and obj.Path.Commands:
                                for cmd in obj.Path.Commands:
                                    if cmd.Name == "M6":
                                        params = " ".join(
                                            f"{k}:{v}"
                                            for k, v in zip(
                                                (
                                                    cmd.Parameters.keys()
                                                    if hasattr(cmd.Parameters, "keys")
                                                    else []
                                                ),
                                                (
                                                    cmd.Parameters.values()
                                                    if hasattr(cmd.Parameters, "values")
                                                    else cmd.Parameters
                                                ),
                                            )
                                        )
                                        output.append(f"        M6 Command: {cmd.Name} {params}")
                        else:
                            output.append(f"        Type: Operation")
                            if hasattr(obj, "ToolController") and obj.ToolController:
                                tc = obj.ToolController
                                output.append(
                                    f"        ToolController: {tc.Label} (T{tc.ToolNumber})"
                                )
                    else:
                        output.append(f"        Type: {obj.TypeId}")
                else:
                    output.append(f"        Type: {obj_type}")

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
        cls.job.PostProcessor = "generic"
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

    def test070(self):
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = "%T.nc"
        self.job.OrderOutputBy = "Tool"
        postables = self.pp._buildPostList(early_tool_prep=True)
        _, sublist = postables[0]

        if self.debug:
            print(self._format_postables(postables, "test070: Early tool prep, split by tool"))

        # Extract all commands from the postables
        commands = []
        if self.debug:
            print("\n=== Extracting commands from postables ===")
        for item in sublist:
            if self.debug:
                item_type = type(item).__name__
                has_path = hasattr(item, "Path")
                path_exists = item.Path if has_path else None
                has_commands = path_exists and item.Path.Commands if path_exists else False
                print(
                    f"Item: {getattr(item, 'Label', item_type)}, Type: {item_type}, HasPath: {has_path}, PathExists: {path_exists is not None}, HasCommands: {bool(has_commands)}"
                )
                if has_commands:
                    print(f"  Commands: {[cmd.Name for cmd in item.Path.Commands]}")
            if hasattr(item, "Path") and item.Path and item.Path.Commands:
                commands.extend(item.Path.Commands)

        if self.debug:
            print(f"\nTotal commands extracted: {len(commands)}")
            print("=" * 40)

        # Should have M6 command with tool parameter
        m6_commands = [cmd for cmd in commands if cmd.Name == "M6"]
        self.assertTrue(len(m6_commands) > 0, "Should have M6 command")

        # First M6 should have T parameter for tool 5
        first_m6 = m6_commands[0]
        self.assertTrue("T" in first_m6.Parameters, "First M6 should have T parameter")
        self.assertEqual(first_m6.Parameters["T"], 5.0, "First M6 should be for tool 5")

        # Should have T2 prep command (early prep for next tool)
        t2_commands = [cmd for cmd in commands if cmd.Name == "T2"]
        self.assertTrue(len(t2_commands) > 0, "Should have T2 early prep command")

        # T2 prep should come after first M6
        first_m6_index = next((i for i, cmd in enumerate(commands) if cmd.Name == "M6"), None)
        t2_index = next((i for i, cmd in enumerate(commands) if cmd.Name == "T2"), None)
        self.assertIsNotNone(first_m6_index, "M6 should exist")
        self.assertIsNotNone(t2_index, "T2 should exist")
        self.assertLess(first_m6_index, t2_index, "M6 should come before T2 prep")

    def test080(self):
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Tool"

        postables = self.pp._buildPostList(early_tool_prep=True)
        _, sublist = postables[0]

        if self.debug:
            print(self._format_postables(postables, "test080: Early tool prep, combined output"))

        # Extract all commands from the postables
        commands = []
        if self.debug:
            print("\n=== Extracting commands from postables ===")
        for item in sublist:
            if self.debug:
                item_type = type(item).__name__
                has_path = hasattr(item, "Path")
                path_exists = item.Path if has_path else None
                has_commands = path_exists and item.Path.Commands if path_exists else False
                print(
                    f"Item: {getattr(item, 'Label', item_type)}, Type: {item_type}, HasPath: {has_path}, PathExists: {path_exists is not None}, HasCommands: {bool(has_commands)}"
                )
                if has_commands:
                    print(f"  Commands: {[cmd.Name for cmd in item.Path.Commands]}")
            if hasattr(item, "Path") and item.Path and item.Path.Commands:
                commands.extend(item.Path.Commands)

        if self.debug:
            print(f"\nTotal commands extracted: {len(commands)}")

        # Expected command sequence with early_tool_prep=True:
        # M6 T5     <- change to tool 5 (standard format)
        # T2        <- prep next tool immediately (early prep)
        # (ops with T5...)
        # M6 T2     <- change to tool 2 (was prepped early)
        # (ops with T2...)

        if self.debug:
            print("\n=== Command Sequence ===")
            for i, cmd in enumerate(commands):
                params = " ".join(
                    f"{k}:{v}"
                    for k, v in zip(
                        cmd.Parameters.keys() if hasattr(cmd.Parameters, "keys") else [],
                        (
                            cmd.Parameters.values()
                            if hasattr(cmd.Parameters, "values")
                            else cmd.Parameters
                        ),
                    )
                )
                print(f"{i:3d}: {cmd.Name} {params}")
            print("=" * 40)

        # Find M6 and T2 commands
        m6_commands = [(i, cmd) for i, cmd in enumerate(commands) if cmd.Name == "M6"]
        t2_commands = [(i, cmd) for i, cmd in enumerate(commands) if cmd.Name == "T2"]

        self.assertTrue(len(m6_commands) >= 2, "Should have at least 2 M6 commands")
        self.assertTrue(len(t2_commands) >= 1, "Should have at least 1 T2 early prep command")

        first_m6_idx, first_m6_cmd = m6_commands[0]
        second_m6_idx, second_m6_cmd = m6_commands[1] if len(m6_commands) >= 2 else (None, None)
        first_t2_idx = t2_commands[0][0]

        # First M6 should have T parameter for tool 5
        self.assertTrue("T" in first_m6_cmd.Parameters, "First M6 should have T parameter")
        self.assertEqual(first_m6_cmd.Parameters["T"], 5.0, "First M6 should be for tool 5")

        # Second M6 should have T parameter for tool 2
        if second_m6_cmd is not None:
            self.assertTrue("T" in second_m6_cmd.Parameters, "Second M6 should have T parameter")
            self.assertEqual(second_m6_cmd.Parameters["T"], 2.0, "Second M6 should be for tool 2")

        # T2 (early prep) should come shortly after first M6 (within a few commands)
        self.assertLess(first_m6_idx, first_t2_idx, "T2 prep should come after first M6")
        self.assertLess(
            first_t2_idx - first_m6_idx, 5, "T2 prep should be within a few commands of first M6"
        )

        # T2 early prep should come before second M6
        if second_m6_idx is not None:
            self.assertLess(
                first_t2_idx, second_m6_idx, "T2 early prep should come before second M6"
            )
