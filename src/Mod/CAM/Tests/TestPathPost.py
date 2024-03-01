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

import difflib
import os
import unittest

import FreeCAD
import Path

import Path.Post.Command as PathPost
import Path.Post.Utils as PostUtils

from Path.Post.Processor import PostProcessor

# If KEEP_DEBUG_OUTPUT is False, remove the gcode file after the test succeeds.
# If KEEP_DEBUG_OUTPUT is True or the test fails leave the gcode file behind
# so it can be looked at easily.
KEEP_DEBUG_OUTPUT = False

PathPost.LOG_MODULE = Path.Log.thisModule()
Path.Log.setLevel(Path.Log.Level.INFO, PathPost.LOG_MODULE)


class TestPathPost(unittest.TestCase):
    """Test some of the output of the postprocessors.

    So far there are three tests each for the linuxcnc
    and centroid postprocessors.
    """

    def setUp(self):
        """Set up the postprocessor tests."""
        pass

    def tearDown(self):
        """Tear down after the postprocessor tests."""
        pass

    #
    # You can run just this test using:
    # ./FreeCAD -c -t Tests.TestPathPost.TestPathPost.test_postprocessors
    #
    def test_postprocessors(self):
        """Test the postprocessors."""
        #
        # The tests are performed in the order they are listed:
        # one test performed on all of the postprocessors
        # then the next test on all of the postprocessors, etc.
        # You can comment out the tuples for tests that you don't want
        # to use.
        #
        tests_to_perform = (
            # (output_file_id, freecad_document, job_name, postprocessor_arguments,
            #  postprocessor_list)
            #
            # test with all of the defaults (metric mode, etc.)
            ("default", "boxtest1", "Job", "--no-show-editor", ()),
            # test in Imperial mode
            ("imperial", "boxtest1", "Job", "--no-show-editor --inches", ()),
            # test in metric, G55, M4, the other way around the part
            ("other_way", "boxtest1", "Job001", "--no-show-editor", ()),
            # test in metric, split by fixtures, G54, G55, G56
            ("split", "boxtest1", "Job002", "--no-show-editor", ()),
            # test in metric mode without the header
            ("no_header", "boxtest1", "Job", "--no-header --no-show-editor", ()),
            # test translating G81, G82, and G83 to G00 and G01 commands
            (
                "drill_translate",
                "drill_test1",
                "Job",
                "--no-show-editor --translate_drill",
                ("grbl", "refactored_grbl"),
            ),
        )
        #
        # The postprocessors to test.
        # You can comment out any postprocessors that you don't want
        # to test.
        #
        postprocessors_to_test = (
            "centroid",
            # "fanuc",
            "grbl",
            "linuxcnc",
            "mach3_mach4",
            "refactored_centroid",
            # "refactored_fanuc",
            "refactored_grbl",
            "refactored_linuxcnc",
            "refactored_mach3_mach4",
            "refactored_test",
        )
        #
        # Enough of the path to where the tests are stored so that
        # they can be found by the python interpreter.
        #
        PATHTESTS_LOCATION = "Mod/CAM/Tests"
        #
        # The following code tries to re-use an open FreeCAD document
        # as much as possible.  It compares the current document with
        # the document for the next test.  If the names are different
        # then the current document is closed and the new document is
        # opened.  The final document is closed at the end of the code.
        #
        current_document = ""
        for (
            output_file_id,
            freecad_document,
            job_name,
            postprocessor_arguments,
            postprocessor_list,
        ) in tests_to_perform:
            if current_document != freecad_document:
                if current_document != "":
                    FreeCAD.closeDocument(current_document)
                current_document = freecad_document
                current_document_path = (
                    FreeCAD.getHomePath()
                    + PATHTESTS_LOCATION
                    + os.path.sep
                    + current_document
                    + ".fcstd"
                )
                FreeCAD.open(current_document_path)
            job = FreeCAD.ActiveDocument.getObject(job_name)
            # Create the objects to be written by the postprocessor.
            postlist = PathPost.buildPostList(job)
            for postprocessor_id in postprocessors_to_test:
                if postprocessor_list == () or postprocessor_id in postprocessor_list:
                    print(
                        "\nRunning %s test on %s postprocessor:\n"
                        % (output_file_id, postprocessor_id)
                    )
                    processor = PostProcessor.load(postprocessor_id)
                    output_file_path = FreeCAD.getHomePath() + PATHTESTS_LOCATION
                    output_file_pattern = "test_%s_%s" % (
                        postprocessor_id,
                        output_file_id,
                    )
                    output_file_extension = ".ngc"
                    for idx, section in enumerate(postlist):
                        partname = section[0]
                        sublist = section[1]
                        output_filename = PathPost.processFileNameSubstitutions(
                            job,
                            partname,
                            idx,
                            output_file_path,
                            output_file_pattern,
                            output_file_extension,
                        )
                        # print("output file: " + output_filename)
                        file_path, extension = os.path.splitext(output_filename)
                        reference_file_name = "%s%s%s" % (file_path, "_ref", extension)
                        # print("reference file: " + reference_file_name)
                        gcode = processor.export(
                            sublist, output_filename, postprocessor_arguments
                        )
                        if not gcode:
                            print("no gcode")
                        with open(reference_file_name, "r") as fp:
                            reference_gcode = fp.read()
                        if not reference_gcode:
                            print("no reference gcode")
                        # Remove the "Output Time:" line in the header from the
                        # comparison if it is present because it changes with
                        # every test.
                        gcode_lines = [
                            i for i in gcode.splitlines(True) if "Output Time:" not in i
                        ]
                        reference_gcode_lines = [
                            i
                            for i in reference_gcode.splitlines(True)
                            if "Output Time:" not in i
                        ]
                        if gcode_lines != reference_gcode_lines:
                            msg = "".join(
                                difflib.ndiff(gcode_lines, reference_gcode_lines)
                            )
                            self.fail(
                                os.path.basename(output_filename)
                                + " output doesn't match:\n"
                                + msg
                            )
                        if not KEEP_DEBUG_OUTPUT:
                            os.remove(output_filename)
        if current_document != "":
            FreeCAD.closeDocument(current_document)


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
        self.assertTrue(
            len([c for c in testpath.Commands if c.Name in ["G2", "G3"]]) == 4
        )

        results = PostUtils.splitArcs(testpath)
        # self.assertTrue(len(results.Commands) == 117)
        self.assertTrue(
            len([c for c in results.Commands if c.Name in ["G2", "G3"]]) == 0
        )


def dumpgroup(group):
    print("====Dump Group======")
    for i in group:
        print(i[0])
        for j in i[1]:
            print(f"--->{j.Name}")
    print("====================")


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

    def setUp(self):
        self.testfile = FreeCAD.getHomePath() + "Mod/CAM/Tests/test_filenaming.fcstd"
        self.doc = FreeCAD.open(self.testfile)
        self.job = self.doc.getObjectsByLabel("MainJob")[0]

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test000(self):

        # check that the test file is structured correctly
        self.assertTrue(len(self.job.Tools.Group) == 2)
        self.assertTrue(len(self.job.Fixtures) == 2)
        self.assertTrue(len(self.job.Operations.Group) == 3)

        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"

    def test010(self):
        postlist = PathPost.buildPostList(self.job)

        self.assertTrue(type(postlist) is list)

        firstoutputitem = postlist[0]
        self.assertTrue(type(firstoutputitem) is tuple)
        self.assertTrue(type(firstoutputitem[0]) is str)
        self.assertTrue(type(firstoutputitem[1]) is list)

    def test020(self):
        # Without splitting, result should be list of one item
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = PathPost.buildPostList(self.job)
        self.assertTrue(len(postlist) == 1)

    def test030(self):
        # No splitting should include all ops, tools, and fixtures
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"
        postlist = PathPost.buildPostList(self.job)
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
        postlist = PathPost.buildPostList(self.job)

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
        postlist = PathPost.buildPostList(self.job)

        firstoutputitem = postlist[0]
        self.assertTrue(firstoutputitem[0] == "TC__7_16__two_flute")

    def test060(self):
        # Ordering by fixture and splitting
        teststring = "%W.nc"
        self.job.SplitOutput = True
        self.job.PostProcessorOutputFile = teststring
        self.job.OrderOutputBy = "Fixture"

        postlist = PathPost.buildPostList(self.job)
        firstoutputitem = postlist[0]
        firstoplist = firstoutputitem[1]
        self.assertTrue(len(firstoplist) == 6)
        self.assertTrue(firstoutputitem[0] == "G54")


class TestOutputNameSubstitution(unittest.TestCase):

    """
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

    def setUp(self):
        self.testfile = FreeCAD.getHomePath() + "Mod/CAM/Tests/test_filenaming.fcstd"
        self.testfilepath, self.testfilename = os.path.split(self.testfile)
        self.testfilename, self.ext = os.path.splitext(self.testfilename)

        self.doc = FreeCAD.open(self.testfile)
        self.job = self.doc.getObjectsByLabel("MainJob")[0]
        self.macro = FreeCAD.getUserMacroDir()
        self.job.SplitOutput = False

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test000(self):
        # Test basic name generation with empty string
        FreeCAD.setActiveDocument(self.doc.Label)
        teststring = ""
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)

        self.assertTrue(len(outlist) == 1)
        subpart, objs = outlist[0]

        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilename}.nc")

    def test015(self):
        # Test basic string substitution without splitting
        teststring = "~/Desktop/%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)

        self.assertTrue(len(outlist) == 1)
        subpart, objs = outlist[0]

        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(
            os.path.normpath(filename), os.path.normpath("~/Desktop/MainJob.nc")
        )

    def test010(self):
        # Substitute current file path
        teststring = "%D/testfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(
            os.path.normpath(filename),
            os.path.normpath(f"{self.testfilepath}/testfile.nc"),
        )

    def test020(self):
        teststring = "%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilename}.nc")

    def test030(self):
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(
            os.path.normpath(filename),
            os.path.normpath(f"{self.macro}outfile.nc")
        )

    def test040(self):
        # unused substitution strings should be ignored
        teststring = "%d%T%t%W%O/testdoc.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(
            os.path.normpath(filename),
            os.path.normpath(f"{self.testfilename}/testdoc.nc"),
        )

    def test050(self):
        # explicitly using the sequence number should include it where indicated.
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "0-test_filenaming.nc")

    def test060(self):
        # # Split by Tool
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Tool"
        outlist = PathPost.buildPostList(self.job)

        # substitute jobname and use default sequence numbers
        teststring = "%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        # Use Toolnumbers and default sequence numbers
        teststring = "%T.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "5-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "2-1.nc")

        # Use Tooldescriptions and default sequence numbers
        teststring = "%t.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "TC__7_16__two_flute-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "TC__Drill-1.nc")

    def test070(self):
        # Split by WCS
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Fixture"
        outlist = PathPost.buildPostList(self.job)

        teststring = "%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        teststring = "%W-%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "G54-MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "G55-MainJob-1.nc")

    def test080(self):
        # Split by Operation
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Operation"
        outlist = PathPost.buildPostList(self.job)

        teststring = "%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        teststring = "%O-%j.nc"
        self.job.PostProcessorOutputFile = teststring
        Path.Preferences.setOutputFileDefaults(
            teststring, "Append Unique ID on conflict"
        )
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "OutsideProfile-MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "DrillAllHoles-MainJob-1.nc")
