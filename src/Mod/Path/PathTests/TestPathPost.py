# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts
import PathScripts.post
import PathScripts.PathProfileContour
import PathScripts.PathJob
import PathScripts.PathPost as PathPost
import PathScripts.PathToolController
import PathScripts.PathUtil
import PathScripts.PostUtils as PostUtils
import difflib
import unittest
import Path
import os
import PathScripts.PathPost as PathPost

WriteDebugOutput = False


class PathPostTestCases(unittest.TestCase):
    def setUp(self):
        testfile = FreeCAD.getHomePath() + "Mod/Path/PathTests/boxtest.fcstd"
        self.doc = FreeCAD.open(testfile)
        self.job = FreeCAD.ActiveDocument.getObject("Job")
        self.postlist = []
        currTool = None
        for obj in self.job.Group:
            if not isinstance(obj.Proxy, PathScripts.PathToolController.ToolController):
                tc = PathScripts.PathUtil.toolControllerForOp(obj)
                if tc is not None:
                    if tc.ToolNumber != currTool:
                        self.postlist.append(tc)
                self.postlist.append(obj)

    def tearDown(self):
        FreeCAD.closeDocument("boxtest")

    def testLinuxCNC(self):
        from PathScripts.post import linuxcnc_post as postprocessor

        args = (
            "--no-header --no-line-numbers --no-comments --no-show-editor --precision=2"
        )
        gcode = postprocessor.export(self.postlist, "gcode.tmp", args)

        referenceFile = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_linuxcnc_00.ngc"
        )
        with open(referenceFile, "r") as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open("testLinuxCNC.tmp", "w") as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = "".join(
                difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True))
            )
            self.fail("linuxcnc output doesn't match: " + msg)

    def testLinuxCNCImperial(self):
        from PathScripts.post import linuxcnc_post as postprocessor

        args = "--no-header --no-line-numbers --no-comments --no-show-editor --precision=2 --inches"
        gcode = postprocessor.export(self.postlist, "gcode.tmp", args)

        referenceFile = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_linuxcnc_10.ngc"
        )
        with open(referenceFile, "r") as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open("testLinuxCNCImplerial.tmp", "w") as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = "".join(
                difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True))
            )
            self.fail("linuxcnc output doesn't match: " + msg)

    def testCentroid(self):
        from PathScripts.post import centroid_post as postprocessor

        args = "--no-header --no-line-numbers --no-comments --no-show-editor --axis-precision=2 --feed-precision=2"
        gcode = postprocessor.export(self.postlist, "gcode.tmp", args)

        referenceFile = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_centroid_00.ngc"
        )
        with open(referenceFile, "r") as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open("testCentroid.tmp", "w") as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = "".join(
                difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True))
            )
            self.fail("linuxcnc output doesn't match: " + msg)


class TestPathPostUtils(unittest.TestCase):
    def test010(self):

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

    testfile = FreeCAD.getHomePath() + "Mod/Path/PathTests/test_filenaming.fcstd"
    doc = FreeCAD.open(testfile)
    job = doc.getObjectsByLabel("MainJob")[0]

    def setUp(self):
        pass

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

    testfile = FreeCAD.getHomePath() + "Mod/Path/PathTests/test_filenaming.fcstd"
    testfilepath, testfilename = os.path.split(testfile)
    testfilename, ext = os.path.splitext(testfilename)

    doc = FreeCAD.open(testfile)
    job = doc.getObjectsByLabel("MainJob")[0]
    macro = FreeCAD.getUserMacroDir()


    def test000(self):
        # Test basic name generation with empty string
        FreeCAD.setActiveDocument(self.doc.Label)
        teststring = ""
        self.job.PostProcessorOutputFile = teststring
        self.job.SplitOutput = False
        outlist = PathPost.buildPostList(self.job)

        self.assertTrue(len(outlist) == 1)
        subpart, objs = outlist[0]

        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilename}.nc")

    def test015(self):
        # Test basic string substitution without splitting
        teststring = "~/Desktop/%j.nc"
        self.job.PostProcessorOutputFile = teststring
        self.job.SplitOutput = False
        outlist = PathPost.buildPostList(self.job)

        self.assertTrue(len(outlist) == 1)
        subpart, objs = outlist[0]

        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "~/Desktop/MainJob.nc")

    def test010(self):
        # Substitute current file path
        teststring = "%D/testfile.nc"
        self.job.PostProcessorOutputFile = teststring
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilepath}/testfile.nc")

    def test020(self):
        teststring = "%d.nc"
        self.job.PostProcessorOutputFile = teststring
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilename}.nc")

    def test030(self):
        teststring = "%M/outfile.nc"
        self.job.PostProcessorOutputFile = teststring
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.macro}outfile.nc")

    def test040(self):
        # unused substitution strings should be ignored
        teststring = "%d%T%t%W%O/testdoc.nc"
        self.job.PostProcessorOutputFile = teststring
        outlist = PathPost.buildPostList(self.job)
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, f"{self.testfilename}/testdoc.nc")

    def test050(self):
        # explicitly using the sequence number should include it where indicated.
        teststring = "%S-%d.nc"
        self.job.PostProcessorOutputFile = teststring
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
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        # Use Toolnumbers and default sequence numbers
        teststring = "%T.nc"
        self.job.PostProcessorOutputFile = teststring
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
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        teststring = "%W-%j.nc"
        self.job.PostProcessorOutputFile = teststring
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
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "MainJob-1.nc")

        teststring = "%O-%j.nc"
        self.job.PostProcessorOutputFile = teststring
        subpart, objs = outlist[0]
        filename = PathPost.resolveFileName(self.job, subpart, 0)
        self.assertEqual(filename, "OutsideProfile-MainJob-0.nc")
        subpart, objs = outlist[1]
        filename = PathPost.resolveFileName(self.job, subpart, 1)
        self.assertEqual(filename, "DrillAllHoles-MainJob-1.nc")
