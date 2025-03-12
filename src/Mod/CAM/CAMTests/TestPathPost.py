# -*- coding: utf-8 -*-
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

    %T ... Tool Number
    %t ... Tool Controller label

    %W ... Work Coordinate System
    %O ... Operation Label

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
        # cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        # cls.job = cls.doc.getObject("Job")

        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

        cls.testfile = FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_filenaming.fcstd"
        cls.testfilepath, cls.testfilename = os.path.split(cls.testfile)
        cls.testfilename, cls.ext = os.path.splitext(cls.testfilename)

        cls.doc = FreeCAD.open(cls.testfile)
        cls.job = cls.doc.getObjectsByLabel("MainJob")[0]
        cls.macro = FreeCAD.getUserMacroDir()
        cls.job.SplitOutput = False

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    # def test010(self):
    #     self.job.PostProcessorOutputFile = ""
    #     generator = PostUtils.FilenameGenerator(job=self.job)

    #     filename_generator = generator.generate_filenames()
    #     generated_filename = next(filename_generator)
    #     self.assertEqual(generated_filename, "-Job.nc")

    # def test020(self):
    #     generator = PostUtils.FilenameGenerator(job=self.job)
    #     filename_generator = generator.generate_filenames()
    #     expected_filenames = ["-Job.nc"] + [f"-Job-{i}.nc" for i in range(1, 5)]
    #     print(expected_filenames)
    #     for expected_filename in expected_filenames:
    #         generated_filename = next(filename_generator)
    #         self.assertEqual(generated_filename, expected_filename)

    # def setUp(self):
    #     self.testfile = FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_filenaming.fcstd"
    #     self.testfilepath, self.testfilename = os.path.split(self.testfile)
    #     self.testfilename, self.ext = os.path.splitext(self.testfilename)

    #     self.doc = FreeCAD.open(self.testfile)
    #     self.job = self.doc.getObjectsByLabel("MainJob")[0]
    #     self.macro = FreeCAD.getUserMacroDir()
    #     self.job.SplitOutput = False

    # def tearDown(self):
    #     FreeCAD.closeDocument(self.doc.Name)

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
        # outlist = PathPost.buildPostList(self.job)

        # self.assertTrue(len(outlist) == 1)
        # subpart, objs = outlist[0]

        # filename = PathPost.resolveFileName(self.job, subpart, 0)
        # self.assertEqual(filename, os.path.normpath(f"{self.testfilename}.nc"))
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
        # outlist = PathPost.buildPostList(self.job)

        # self.assertTrue(len(outlist) == 1)
        # subpart, objs = outlist[0]

        generator = PostUtils.FilenameGenerator(job=self.job)
        filename_generator = generator.generate_filenames()
        filename = next(filename_generator)

        # filename = PathPost.resolveFileName(self.job, subpart, 0)
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
        expected_filenames = [f"test_filenaming{os.sep}testdoc.nc"] + [
            f"test_filenaming{os.sep}testdoc-{i}.nc" for i in range(1, 5)
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
            os.path.join(self.testfilepath, f"{i}-test_filenaming.nc") for i in range(5)
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

        assertFilePathsEqual(
            self, filename, os.path.join(self.testfilepath, "0-test_filenaming.nc")
        )

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


class TestResolvingPostProcessorName(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")

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
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        pref.SetString("PostProcessorDefault", "grbl")
        self.job.PostProcessor = ""
        with patch("Path.Post.Processor.PostProcessor.exists", return_value=True):
            postname = PathCommand._resolve_post_processor_name(self.job)
            self.assertEqual(postname, "grbl")

    def test040(self):
        # Test if user interaction is correctly handled
        self.job.PostProcessor = ""
        if FreeCAD.GuiUp:
            with patch("Path.Post.Command.DlgSelectPostProcessor") as mock_dlg, patch(
                "Path.Post.Processor.PostProcessor.exists", return_value=True
            ):
                mock_dlg.return_value.exec_.return_value = "generic"
                postname = PathCommand._resolve_post_processor_name(self.job)
                self.assertEqual(postname, "generic")
        else:
            with self.assertRaises(ValueError):
                PathCommand._resolve_post_processor_name(self.job)


class TestPostProcessorFactory(unittest.TestCase):
    """Test creation of postprocessor objects."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")

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
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        self.assertTrue(post is not None)
        self.assertTrue(hasattr(post, "_buildPostList"))

    def test040(self):
        """Test that the __name__ of the postprocessor is correct."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        self.assertEqual(post.script_module.__name__, "linuxcnc_post")


class TestPostProcessorClass(unittest.TestCase):
    """Test new post structure objects."""

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
        cls.job = cls.doc.getObject("Job")

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test010(self):
        """Test the export function."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        sections = post.export()
        for sec in sections:
            print(sec[0])

    def test020(self):
        """Test the export function with splitting."""
        post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
        sections = post.export()
        for sec in sections:
            print(sec[0])

    def test030(self):
        """Test the export function with splitting."""
        post = PostProcessorFactory.get_post_processor(self.job, "generic")
        sections = post.export()
        for sec in sections:
            print(sec[0])


# class TestPostProcessorScript(unittest.TestCase):
#     """Test old-school posts"""

#     def setUp(self):
#         self.doc = FreeCAD.open(FreeCAD.getHomePath() + "/Mod/CAM/CAMTests/boxtest.fcstd")
#         self.job = self.doc.getObject("Job")
#         post = PostProcessorFactory.get_post_processor(self.job, "linuxcnc")
#         results = post.export()

#     def tearDown(self):
#         FreeCAD.closeDocument("boxtest")

##
## You can run just this test using:
## ./FreeCAD -c -t CAMTests.TestPathPost.TestPathPost.test_postprocessors
##
# def test_postprocessors(self):
#    """Test the postprocessors."""
#    #
#    # The tests are performed in the order they are listed:
#    # one test performed on all of the postprocessors
#    # then the next test on all of the postprocessors, etc.
#    # You can comment out the tuples for tests that you don't want
#    # to use.
#    #
#    tests_to_perform = (
#        # (output_file_id, freecad_document, job_name, postprocessor_arguments,
#        #  postprocessor_list)
#        #
#        # test with all of the defaults (metric mode, etc.)
#        ("default", "boxtest1", "Job", "--no-show-editor", ()),
#        # test in Imperial mode
#        ("imperial", "boxtest1", "Job", "--no-show-editor --inches", ()),
#        # test in metric, G55, M4, the other way around the part
#        ("other_way", "boxtest1", "Job001", "--no-show-editor", ()),
#        # test in metric, split by fixtures, G54, G55, G56
#        ("split", "boxtest1", "Job002", "--no-show-editor", ()),
#        # test in metric mode without the header
#        ("no_header", "boxtest1", "Job", "--no-header --no-show-editor", ()),
#        # test translating G81, G82, and G83 to G00 and G01 commands
#        (
#            "drill_translate",
#            "drill_test1",
#            "Job",
#            "--no-show-editor --translate_drill",
#            ("grbl", "refactored_grbl"),
#        ),
#    )
#    #
#    # The postprocessors to test.
#    # You can comment out any postprocessors that you don't want
#    # to test.
#    #
#    postprocessors_to_test = (
#        "centroid",
#        # "fanuc",
#        "grbl",
#        "linuxcnc",
#        "mach3_mach4",
#        "refactored_centroid",
#        # "refactored_fanuc",
#        "refactored_grbl",
#        "refactored_linuxcnc",
#        "refactored_mach3_mach4",
#        "refactored_test",
#    )
#    #
#    # Enough of the path to where the tests are stored so that
#    # they can be found by the python interpreter.
#    #
#    PATHTESTS_LOCATION = "Mod/CAM/CAMTests"
#    #
#    # The following code tries to reuse an open FreeCAD document
#    # as much as possible.  It compares the current document with
#    # the document for the next test.  If the names are different
#    # then the current document is closed and the new document is
#    # opened.  The final document is closed at the end of the code.
#    #
#    current_document = ""
#    for (
#        output_file_id,
#        freecad_document,
#        job_name,
#        postprocessor_arguments,
#        postprocessor_list,
#    ) in tests_to_perform:
#        if current_document != freecad_document:
#            if current_document != "":
#                FreeCAD.closeDocument(current_document)
#            current_document = freecad_document
#            current_document_path = (
#                FreeCAD.getHomePath()
#                + PATHTESTS_LOCATION
#                + os.path.sep
#                + current_document
#                + ".fcstd"
#            )
#            FreeCAD.open(current_document_path)
#        job = FreeCAD.ActiveDocument.getObject(job_name)
#        # Create the objects to be written by the postprocessor.
#        self.pp._buildPostList(job)
#        for postprocessor_id in postprocessors_to_test:
#            if postprocessor_list == () or postprocessor_id in postprocessor_list:
#                print(
#                    "\nRunning %s test on %s postprocessor:\n"
#                    % (output_file_id, postprocessor_id)
#                )
#                processor = PostProcessor.load(postprocessor_id)
#                output_file_path = FreeCAD.getHomePath() + PATHTESTS_LOCATION
#                output_file_pattern = "test_%s_%s" % (
#                    postprocessor_id,
#                    output_file_id,
#                )
#                output_file_extension = ".ngc"
#                for idx, section in enumerate(postlist):
#                    partname = section[0]
#                    sublist = section[1]
#                    output_filename = PathPost.processFileNameSubstitutions(
#                        job,
#                        partname,
#                        idx,
#                        output_file_path,
#                        output_file_pattern,
#                        output_file_extension,
#                    )
#                    # print("output file: " + output_filename)
#                    file_path, extension = os.path.splitext(output_filename)
#                    reference_file_name = "%s%s%s" % (file_path, "_ref", extension)
#                    # print("reference file: " + reference_file_name)
#                    gcode = processor.export(
#                        sublist, output_filename, postprocessor_arguments
#                    )
#                    if not gcode:
#                        print("no gcode")
#                    with open(reference_file_name, "r") as fp:
#                        reference_gcode = fp.read()
#                    if not reference_gcode:
#                        print("no reference gcode")
#                    # Remove the "Output Time:" line in the header from the
#                    # comparison if it is present because it changes with
#                    # every test.
#                    gcode_lines = [
#                        i for i in gcode.splitlines(True) if "Output Time:" not in i
#                    ]
#                    reference_gcode_lines = [
#                        i
#                        for i in reference_gcode.splitlines(True)
#                        if "Output Time:" not in i
#                    ]
#                    if gcode_lines != reference_gcode_lines:
#                        msg = "".join(
#                            difflib.ndiff(gcode_lines, reference_gcode_lines)
#                        )
#                        self.fail(
#                            os.path.basename(output_filename)
#                            + " output doesn't match:\n"
#                            + msg
#                        )
#                    if not KEEP_DEBUG_OUTPUT:
#                        os.remove(output_filename)
#    if current_document != "":
#        FreeCAD.closeDocument(current_document)


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
        cls.testfile = FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_filenaming.fcstd"
        cls.doc = FreeCAD.open(cls.testfile)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")
        cls.job = cls.doc.getObjectsByLabel("MainJob")[0]

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)

    def setUp(self):
        self.pp = PathPost.PostProcessor(self.job, "generic", "", "")

    def tearDown(self):
        pass

    def test000(self):

        # check that the test file is structured correctly
        self.assertTrue(len(self.job.Tools.Group) == 2)
        self.assertTrue(len(self.job.Fixtures) == 2)
        self.assertTrue(len(self.job.Operations.Group) == 3)

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
        self.assertTrue(len(postlist) == 1)

    def test030(self):
        # No splitting should include all ops, tools, and fixtures
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = self.pp._buildPostList()
        firstoutputitem = postlist[0]
        firstoplist = firstoutputitem[1]
        self.assertTrue(len(firstoplist) == 14)

    def test040(self):
        # Test splitting by tool
        # ordering by tool with toolnumber for string
        teststring = "%T.nc"
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = teststring
        self.job.OrderOutputBy = "Tool"
        postlist = self.pp._buildPostList()

        firstoutputitem = postlist[0]
        self.assertTrue(firstoutputitem[0] == str(5))

        # check length of output
        firstoplist = firstoutputitem[1]
        self.assertTrue(len(firstoplist) == 5)

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
        self.assertTrue(len(firstoplist) == 6)
        self.assertTrue(firstoutputitem[0] == "G54")
