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

    def test000(self):

        # check that the test file is structured correctly
        self.assertTrue(len(self.job.Tools.Group) == 2)
        self.assertTrue(len(self.job.Fixtures) == 2)
        self.assertTrue(len(self.job.Operations.Group) == 3)

        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"

    def test010(self):
        # check that function returns correct hash
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

class TestPathPostImport(unittest.TestCase):
    def test001(self):
        """test001()... Verify 'No active document' exception thrown if no document open."""
        from PathScripts.post import gcode_pre as gcode_pre

        self.assertRaises(
            gcode_pre.PathNoActiveDocumentException,
            gcode_pre._isImportEnvironmentReady,
        )

    def test002(self):
        """test002()... Verify 'No job object' exception thrown if Job object available."""
        from PathScripts.post import gcode_pre as gcode_pre

        doc = FreeCAD.newDocument("TestPathPost")

        self.assertRaises(
            gcode_pre.PathNoJobException,
            gcode_pre._isImportEnvironmentReady,
        )
        FreeCAD.closeDocument(doc.Name)

    def test003(self, close=True):
        """test003()... Verify 'No job object' exception thrown if Job object available."""
        from PathScripts.post import gcode_pre as gcode_pre

        doc = FreeCAD.newDocument("TestPathPost")

        # Add temporary receiving Job object
        box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        box.Label = "Temporary Box"
        # Add Job object with view provider support when possible
        if FreeCAD.GuiUp:
            import PathScripts.PathJobGui as PathJobGui

            box.ViewObject.Visibility = False
            job = PathJobGui.Create([box], openTaskPanel=False)
        else:
            import PathScripts.PathJob as PathJob

            job = PathJob.Create("Job", [box])

        importFile = FreeCAD.getHomePath() + "Mod/Path/PathTests/test_centroid_00.ngc"
        gcode_pre.insert(importFile, "test_centroid_00")

        # self.assertTrue(doc.Name == "test_centroid_00")

        opList = doc.Job.Operations.Group
        self.assertTrue(
            len(opList) == 2,
            "Expected 2 Custom operations to be created from source g-code file, test_centroid_00.ngc",
        )
        self.assertTrue(
            opList[0].Name == "Custom", "Expected first operation to be Custom"
        )
        self.assertTrue(
            opList[1].Name == "Custom001", "Expected second operation to be Custom001"
        )

        if close:
            FreeCAD.closeDocument(doc.Name)

    def test004(self):
        """test004()... Verify g-code imported with g-code pre-processor"""

        self.test003(close=False)

        doc = FreeCAD.ActiveDocument
        op1 = doc.Job.Operations.Group[0]
        op2 = doc.Job.Operations.Group[1]

        # Verify g-code sizes
        self.assertTrue(
            op1.Path.Size == 4, "Expected Custom g-code command count to be 4."
        )
        self.assertTrue(
            op2.Path.Size == 60, "Expected Custom g-code command count to be 60."
        )

        # Verify g-code commands
        op1_code = (
            "(Custom_test_centroid_00)\n(Begin Custom)\nG90 G49.000000\n(End Custom)\n"
        )
        op2_code = "(Custom001_test_centroid_00)\n(Begin Custom)\nG0 Z15.000000\nG90\nG0 Z15.000000\nG0 X10.000000 Y10.000000\nG0 Z10.000000\nG1 X10.000000 Y10.000000 Z9.000000\nG1 X10.000000 Y0.000000 Z9.000000\nG1 X0.000000 Y0.000000 Z9.000000\nG1 X0.000000 Y10.000000 Z9.000000\nG1 X10.000000 Y10.000000 Z9.000000\nG1 X10.000000 Y10.000000 Z8.000000\nG1 X10.000000 Y0.000000 Z8.000000\nG1 X0.000000 Y0.000000 Z8.000000\nG1 X0.000000 Y10.000000 Z8.000000\nG1 X10.000000 Y10.000000 Z8.000000\nG1 X10.000000 Y10.000000 Z7.000000\nG1 X10.000000 Y0.000000 Z7.000000\nG1 X0.000000 Y0.000000 Z7.000000\nG1 X0.000000 Y10.000000 Z7.000000\nG1 X10.000000 Y10.000000 Z7.000000\nG1 X10.000000 Y10.000000 Z6.000000\nG1 X10.000000 Y0.000000 Z6.000000\nG1 X0.000000 Y0.000000 Z6.000000\nG1 X0.000000 Y10.000000 Z6.000000\nG1 X10.000000 Y10.000000 Z6.000000\nG1 X10.000000 Y10.000000 Z5.000000\nG1 X10.000000 Y0.000000 Z5.000000\nG1 X0.000000 Y0.000000 Z5.000000\nG1 X0.000000 Y10.000000 Z5.000000\nG1 X10.000000 Y10.000000 Z5.000000\nG1 X10.000000 Y10.000000 Z4.000000\nG1 X10.000000 Y0.000000 Z4.000000\nG1 X0.000000 Y0.000000 Z4.000000\nG1 X0.000000 Y10.000000 Z4.000000\nG1 X10.000000 Y10.000000 Z4.000000\nG1 X10.000000 Y10.000000 Z3.000000\nG1 X10.000000 Y0.000000 Z3.000000\nG1 X0.000000 Y0.000000 Z3.000000\nG1 X0.000000 Y10.000000 Z3.000000\nG1 X10.000000 Y10.000000 Z3.000000\nG1 X10.000000 Y10.000000 Z2.000000\nG1 X10.000000 Y0.000000 Z2.000000\nG1 X0.000000 Y0.000000 Z2.000000\nG1 X0.000000 Y10.000000 Z2.000000\nG1 X10.000000 Y10.000000 Z2.000000\nG1 X10.000000 Y10.000000 Z1.000000\nG1 X10.000000 Y0.000000 Z1.000000\nG1 X0.000000 Y0.000000 Z1.000000\nG1 X0.000000 Y10.000000 Z1.000000\nG1 X10.000000 Y10.000000 Z1.000000\nG1 X10.000000 Y10.000000 Z0.000000\nG1 X10.000000 Y0.000000 Z0.000000\nG1 X0.000000 Y0.000000 Z0.000000\nG1 X0.000000 Y10.000000 Z0.000000\nG1 X10.000000 Y10.000000 Z0.000000\nG0 Z15.000000\nG90 G49.000000\n(End Custom)\n"
        code1 = op1.Path.toGCode()
        self.assertTrue(
            code1 == op1_code,
            f"Gcode is not what is expected:\n~~~\n{code1}\n~~~\n\n\n~~~\n{op1_code}\n~~~",
        )
        code2 = op2.Path.toGCode()
        self.assertTrue(
            code2 == op2_code,
            f"Gcode is not what is expected:\n~~~\n{code2}\n~~~\n\n\n~~~\n{op2_code}\n~~~",
        )
        FreeCAD.closeDocument(doc.Name)

    def test005(self):
        """test005()... verify `_identifygcodeByToolNumberList()` produces correct output"""

        from PathScripts.post import gcode_pre as gcode_pre

        importFile = FreeCAD.getHomePath() + "Mod/Path/PathTests/test_centroid_00.ngc"
        gcodeByToolNumberList = gcode_pre._identifygcodeByToolNumberList(importFile)

        self.assertTrue(gcodeByToolNumberList[0][0] == ["G90 G80 G40 G49"])
        self.assertTrue(gcodeByToolNumberList[0][1] == 0)

        self.assertTrue(
            gcodeByToolNumberList[1][0]
            == [
                "G0 Z15.00",
                "G90",
                "G0 Z15.00",
                "G0 X10.00 Y10.00",
                "G0 Z10.00",
                "G1 X10.00 Y10.00 Z9.00",
                "G1 X10.00 Y0.00 Z9.00",
                "G1 X0.00 Y0.00 Z9.00",
                "G1 X0.00 Y10.00 Z9.00",
                "G1 X10.00 Y10.00 Z9.00",
                "G1 X10.00 Y10.00 Z8.00",
                "G1 X10.00 Y0.00 Z8.00",
                "G1 X0.00 Y0.00 Z8.00",
                "G1 X0.00 Y10.00 Z8.00",
                "G1 X10.00 Y10.00 Z8.00",
                "G1 X10.00 Y10.00 Z7.00",
                "G1 X10.00 Y0.00 Z7.00",
                "G1 X0.00 Y0.00 Z7.00",
                "G1 X0.00 Y10.00 Z7.00",
                "G1 X10.00 Y10.00 Z7.00",
                "G1 X10.00 Y10.00 Z6.00",
                "G1 X10.00 Y0.00 Z6.00",
                "G1 X0.00 Y0.00 Z6.00",
                "G1 X0.00 Y10.00 Z6.00",
                "G1 X10.00 Y10.00 Z6.00",
                "G1 X10.00 Y10.00 Z5.00",
                "G1 X10.00 Y0.00 Z5.00",
                "G1 X0.00 Y0.00 Z5.00",
                "G1 X0.00 Y10.00 Z5.00",
                "G1 X10.00 Y10.00 Z5.00",
                "G1 X10.00 Y10.00 Z4.00",
                "G1 X10.00 Y0.00 Z4.00",
                "G1 X0.00 Y0.00 Z4.00",
                "G1 X0.00 Y10.00 Z4.00",
                "G1 X10.00 Y10.00 Z4.00",
                "G1 X10.00 Y10.00 Z3.00",
                "G1 X10.00 Y0.00 Z3.00",
                "G1 X0.00 Y0.00 Z3.00",
                "G1 X0.00 Y10.00 Z3.00",
                "G1 X10.00 Y10.00 Z3.00",
                "G1 X10.00 Y10.00 Z2.00",
                "G1 X10.00 Y0.00 Z2.00",
                "G1 X0.00 Y0.00 Z2.00",
                "G1 X0.00 Y10.00 Z2.00",
                "G1 X10.00 Y10.00 Z2.00",
                "G1 X10.00 Y10.00 Z1.00",
                "G1 X10.00 Y0.00 Z1.00",
                "G1 X0.00 Y0.00 Z1.00",
                "G1 X0.00 Y10.00 Z1.00",
                "G1 X10.00 Y10.00 Z1.00",
                "G1 X10.00 Y10.00 Z0.00",
                "G1 X10.00 Y0.00 Z0.00",
                "G1 X0.00 Y0.00 Z0.00",
                "G1 X0.00 Y10.00 Z0.00",
                "G1 X10.00 Y10.00 Z0.00",
                "G0 Z15.00",
                "G90 G80 G40 G49",
            ]
        )
        self.assertTrue(gcodeByToolNumberList[1][1] == 2)


class OutputOrderingTestCases(unittest.TestCase):
    def setUp(self):
        testfile = FreeCAD.getHomePath() + "Mod/Path/PathTests/boxtest.fcstd"
        self.doc = FreeCAD.open(testfile)
        self.job = FreeCAD.ActiveDocument.getObject("Job001")

    def tearDown(self):
        FreeCAD.closeDocument("boxtest")

    def test010(self):
        # Basic postprocessing:

        self.job.Fixtures = ["G54"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Fixture"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)

        outlist = [i.Label for i in self.postlist[0]]

        self.assertTrue(len(self.postlist) == 1)
        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "T3",
            "FifthOp-(T3)",
        ]
        self.assertListEqual(outlist, expected)

    def test020(self):
        # Multiple Fixtures

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Fixture"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)

        self.assertTrue(len(self.postlist) == 1)

        outlist = [i.Label for i in self.postlist[0]]
        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "T3",
            "FifthOp-(T3)",
            "G55",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "T3",
            "FifthOp-(T3)",
        ]

        self.assertListEqual(outlist, expected)

    def test030(self):
        # Multiple Fixtures - Split output

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Fixture"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)

        self.assertTrue(len(self.postlist) == 2)

        outlist = [i.Label for i in self.postlist[0]]
        print(outlist)

        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "T3",
            "FifthOp-(T3)",
        ]
        self.assertListEqual(outlist, expected)

        expected = [
            "G55",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "T3",
            "FifthOp-(T3)",
        ]
        outlist = [i.Label for i in self.postlist[1]]
        self.assertListEqual(outlist, expected)

    def test040(self):
        # Order by 'Tool'

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Tool"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)
        outlist = [i.Label for i in self.postlist[0]]

        self.assertTrue(len(self.postlist) == 1)
        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "G55",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "G54",
            "T3",
            "FifthOp-(T3)",
            "G55",
            "FifthOp-(T3)",
        ]

        self.assertListEqual(outlist, expected)

    def test050(self):
        # Order by 'Tool' and split

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Tool"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)
        outlist = [i.Label for i in self.postlist[0]]

        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
            "G55",
            "FirstOp-(T1)",
            "SecondOp-(T1)",
            "T2",
            "ThirdOp-(T2)",
            "T1",
            "FourthOp-(T1)",
        ]
        self.assertListEqual(outlist, expected)

        outlist = [i.Label for i in self.postlist[1]]

        expected = [
            "G54",
            "T3",
            "FifthOp-(T3)",
            "G55",
            "FifthOp-(T3)",
        ]
        self.assertListEqual(outlist, expected)

    def test060(self):
        # Order by 'Operation'

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = False
        self.job.OrderOutputBy = "Operation"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)
        outlist = [i.Label for i in self.postlist[0]]

        self.assertTrue(len(self.postlist) == 1)
        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "G55",
            "FirstOp-(T1)",
            "G54",
            "T1",
            "SecondOp-(T1)",
            "G55",
            "SecondOp-(T1)",
            "G54",
            "T2",
            "ThirdOp-(T2)",
            "G55",
            "ThirdOp-(T2)",
            "G54",
            "T1",
            "FourthOp-(T1)",
            "G55",
            "FourthOp-(T1)",
            "G54",
            "T3",
            "FifthOp-(T3)",
            "G55",
            "FifthOp-(T3)",
        ]

        self.assertListEqual(outlist, expected)

    def test070(self):
        # Order by 'Operation' and split

        self.job.Fixtures = ["G54", "G55"]
        self.job.SplitOutput = True
        self.job.OrderOutputBy = "Operation"

        cpp = PathPost.CommandPathPost
        self.postlist = cpp.buildPostList(self, self.job)
        self.assertTrue(len(self.postlist) == 5)

        outlist = [i.Label for i in self.postlist[0]]
        expected = [
            "G54",
            "T1",
            "FirstOp-(T1)",
            "G55",
            "FirstOp-(T1)",
        ]
        self.assertListEqual(outlist, expected)

        outlist = [i.Label for i in self.postlist[1]]
        expected = [
            "G54",
            "T1",
            "SecondOp-(T1)",
            "G55",
            "SecondOp-(T1)",
        ]
        self.assertListEqual(outlist, expected)

        outlist = [i.Label for i in self.postlist[2]]
        expected = [
            "G54",
            "T2",
            "ThirdOp-(T2)",
            "G55",
            "ThirdOp-(T2)",
        ]
        self.assertListEqual(outlist, expected)

        outlist = [i.Label for i in self.postlist[3]]
        expected = [
            "G54",
            "T1",
            "FourthOp-(T1)",
            "G55",
            "FourthOp-(T1)",
        ]
        self.assertListEqual(outlist, expected)

        outlist = [i.Label for i in self.postlist[4]]
        expected = [
            "G54",
            "T3",
            "FifthOp-(T3)",
            "G55",
            "FifthOp-(T3)",
        ]
        self.assertListEqual(outlist, expected)
