# SPDX-License-Identifier: LGPL-2.1-or-later

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

from Path.Post.Processor import PostProcessorFactory
from unittest.mock import patch
import FreeCAD
import Path
import Path.Post.Command as PathCommand
import Path.Post.Processor as PathPost
import Path.Post.Utils as PostUtils
import Path.Main.Job as PathJob
import Path.Tool.Controller as PathToolController
import os
import unittest
from Path.Post.Processor import _HeaderBuilder

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
        """ Test Termination of Canned Cycles"""
        # Test basic cycle termination when parameters change
        test_path = Path.Path([
            Path.Command('G0', {'Z': 1.0}),
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -1.0, 'R': 0.2, 'F': 10.0}),  # Different Z depth
            Path.Command('G1', {'X': 3.0, 'Y': 3.0})
        ])

        expected_path = Path.Path([
            Path.Command('G0', {'Z': 1.0}),
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G80'),  # Terminate due to parameter change
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -1.0, 'R': 0.2, 'F': 10.0}),
            Path.Command('G80'),  # Final termination
            Path.Command('G1', {'X': 3.0, 'Y': 3.0})
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
    
    def test030_canned_cycle_termination_with_non_cycle_commands(self):
        """Test cycle termination when non-cycle commands are encountered"""
        test_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G0', {'X': 2.0, 'Y': 2.0}),  # Non-cycle command
            Path.Command('G82', {'X': 3.0, 'Y': 3.0, 'Z': -1.0, 'R': 0.2, 'P': 1.0, 'F': 10.0}),
        ])

        expected_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G80'),  # Terminate before non-cycle command
            Path.Command('G0', {'X': 2.0, 'Y': 2.0}),
            Path.Command('G82', {'X': 3.0, 'Y': 3.0, 'Z': -1.0, 'R': 0.2, 'P': 1.0, 'F': 10.0}),
            Path.Command('G80'),  # Final termination
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
    
    def test040_canned_cycle_modal_same_parameters(self):
        """Test modal cycles with same parameters don't get terminated"""
        test_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),  # Modal - same parameters
            Path.Command('G81', {'X': 3.0, 'Y': 3.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),  # Modal - same parameters
        ])

        expected_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),  # No termination - same params
            Path.Command('G81', {'X': 3.0, 'Y': 3.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),  # No termination - same params
            Path.Command('G80'),  # Final termination
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
    
    def test050_canned_cycle_feed_rate_change(self):
        """Test cycle termination when feed rate changes"""
        test_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'F': 20.0}),  # Different feed rate
        ])

        expected_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G80'),  # Terminate due to feed rate change
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'F': 20.0}),
            Path.Command('G80'),  # Final termination
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
    
    def test060_canned_cycle_retract_plane_change(self):
        """Test cycle termination when retract plane changes"""
        test_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.2, 'F': 10.0}),  # Different R plane
        ])

        expected_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G80'),  # Terminate due to R plane change
            Path.Command('G81', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.2, 'F': 10.0}),
            Path.Command('G80'),  # Final termination
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
    
    def test070_canned_cycle_mixed_cycle_types(self):
        """Test termination between different cycle types"""
        test_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G82', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'P': 1.0, 'F': 10.0}),  # Different cycle type
        ])

        expected_path = Path.Path([
            Path.Command('G81', {'X': 1.0, 'Y': 1.0, 'Z': -0.5, 'R': 0.1, 'F': 10.0}),
            Path.Command('G80'),  # Terminate due to different cycle type (different parameters)
            Path.Command('G82', {'X': 2.0, 'Y': 2.0, 'Z': -0.5, 'R': 0.1, 'P': 1.0, 'F': 10.0}),
            Path.Command('G80'),  # Final termination
        ])

        result = PostUtils.cannedCycleTerminator(test_path)
        self.assertEqual(len(result.Commands), len(expected_path.Commands))
        for i, (res, exp) in enumerate(zip(result.Commands, expected_path.Commands)):
            self.assertEqual(res.Name, exp.Name, f"Command {i}: name mismatch")
            self.assertEqual(res.Parameters, exp.Parameters, f"Command {i}: parameters mismatch")
        
        
        

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
        print(f"DEBUG test030: postlist length={len(firstoplist)}, expected=14")
        print(f"DEBUG test030: firstoplist={[str(item) for item in firstoplist]}")
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
        print(f"DEBUG test040: firstoutputitem[0]={firstoutputitem[0]}, expected='5'")
        print(f"DEBUG test040: tool numbers={[tc.ToolNumber for tc in self.job.Tools.Group]}")
        self.assertTrue(firstoutputitem[0] == str(5))

        # check length of output
        firstoplist = firstoutputitem[1]
        print(f"DEBUG test040: postlist length={len(firstoplist)}, expected=5")
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

class TestHeaderBuilder(unittest.TestCase):
    """Test the HeaderBuilder class."""
    
    def test010_initialization(self):
        """Test that HeaderBuilder initializes with empty data structures."""
        
        builder = _HeaderBuilder()
        
        # Check initial state
        self.assertIsNone(builder._exporter)
        self.assertIsNone(builder._post_processor)
        self.assertIsNone(builder._cam_file)
        self.assertIsNone(builder._output_time)
        self.assertEqual(builder._tools, [])
        self.assertEqual(builder._fixtures, [])
        self.assertEqual(builder._notes, [])
    
    def test020_add_methods(self):
        """Test adding header elements."""
        
        builder = _HeaderBuilder()
        
        # Add various elements
        builder.add_exporter_info("TestExporter")
        builder.add_machine_info("TestMachine")
        builder.add_post_processor("test_post")
        builder.add_cam_file("test.fcstd")
        builder.add_author("Test Author")
        builder.add_output_time("2024-12-24 10:00:00")
        builder.add_tool(1, "End Mill")
        builder.add_tool(2, "Drill Bit")
        builder.add_fixture("G54")
        builder.add_fixture("G55")
        builder.add_note("This is a test note")
        
        # Verify elements were added
        self.assertEqual(builder._exporter, "TestExporter")
        self.assertEqual(builder._machine, "TestMachine")
        self.assertEqual(builder._post_processor, "test_post")
        self.assertEqual(builder._cam_file, "test.fcstd")
        self.assertEqual(builder._author, "Test Author")
        self.assertEqual(builder._output_time, "2024-12-24 10:00:00")
        self.assertEqual(builder._tools, [(1, "End Mill"), (2, "Drill Bit")])
        self.assertEqual(builder._fixtures, ["G54", "G55"])
        self.assertEqual(builder._notes, ["This is a test note"])
    
    def test030_path_property_empty(self):
        """Test Path property with no data returns empty Path."""
        
        builder = _HeaderBuilder()
        path = builder.Path
        
        self.assertIsInstance(path, Path.Path)
        self.assertEqual(len(path.Commands), 0)
    
    def test040_path_property_complete(self):
        """Test Path property generates correct comment commands."""
        
        builder = _HeaderBuilder()
        
        # Add complete header data
        builder.add_exporter_info("FreeCAD")
        builder.add_machine_info("CNC Router")
        builder.add_post_processor("linuxcnc")
        builder.add_cam_file("project.fcstd")
        builder.add_author("John Doe")
        builder.add_output_time("2024-12-24 10:00:00")
        builder.add_tool(1, "1/4\" End Mill")
        builder.add_fixture("G54")
        builder.add_note("Test operation")
        
        path = builder.Path
        
        # Verify it's a Path object
        self.assertIsInstance(path, Path.Path)
        
        # Check expected number of commands
        expected_commands = [
            "(Exported by FreeCAD)",
            "(Machine: CNC Router)",
            "(Post Processor: linuxcnc)",
            "(Cam File: project.fcstd)",
            "(Author: John Doe)",
            "(Output Time: 2024-12-24 10:00:00)",
            "(T1=1/4\" End Mill)",
            "(Fixture: G54)",
            "(Note: Test operation)"
        ]
        
        self.assertEqual(len(path.Commands), len(expected_commands))
        
        # Verify each command
        for i, expected_comment in enumerate(expected_commands):
            self.assertIsInstance(path.Commands[i], Path.Command)
            self.assertEqual(path.Commands[i].Name, expected_comment)
    
    def test050_path_property_partial(self):
        """Test Path property with partial data."""
        
        builder = _HeaderBuilder()
        
        # Add only some elements
        builder.add_exporter_info()
        builder.add_tool(5, "Drill")
        builder.add_note("Partial test")
        
        path = builder.Path
        
        expected_commands = [
            "(Exported by FreeCAD)",
            "(T5=Drill)",
            "(Note: Partial test)"
        ]
        
        self.assertEqual(len(path.Commands), len(expected_commands))
        for i, expected_comment in enumerate(expected_commands):
            self.assertEqual(path.Commands[i].Name, expected_comment)

        # converted
        expected_gcode = "(Exported by FreeCAD)\n(T5=Drill)\n(Note: Partial test)\n"
        gcode = path.toGCode()
        self.assertEqual(gcode, expected_gcode)

    
    def test060_multiple_tools_fixtures_notes(self):
        """Test adding multiple tools, fixtures, and notes."""
        
        builder = _HeaderBuilder()
        
        # Add multiple items
        builder.add_tool(1, "Tool A")
        builder.add_tool(2, "Tool B")
        builder.add_tool(3, "Tool C")
        
        builder.add_fixture("G54")
        builder.add_fixture("G55")
        builder.add_fixture("G56")
        
        builder.add_note("Note 1")
        builder.add_note("Note 2")
        
        path = builder.Path
        
        # Should have 8 commands (3 tools + 3 fixtures + 2 notes)
        self.assertEqual(len(path.Commands), 8)
        
        # Check tool commands
        self.assertEqual(path.Commands[0].Name, "(T1=Tool A)")
        self.assertEqual(path.Commands[1].Name, "(T2=Tool B)")
        self.assertEqual(path.Commands[2].Name, "(T3=Tool C)")
        
        # Check fixture commands
        self.assertEqual(path.Commands[3].Name, "(Fixture: G54)")
        self.assertEqual(path.Commands[4].Name, "(Fixture: G55)")
        self.assertEqual(path.Commands[5].Name, "(Fixture: G56)")
        
        # Check note commands
        self.assertEqual(path.Commands[6].Name, "(Note: Note 1)")
        self.assertEqual(path.Commands[7].Name, "(Note: Note 2)")
