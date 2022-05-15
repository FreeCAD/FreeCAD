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

from PathScripts import PathLog
from PathScripts import PathPost
from PathScripts import PostUtils

from PathScripts.PathPostProcessor import PostProcessor

# If KEEP_DEBUG_OUTPUT is False, remove the gcode file after the test.
# If KEEP_DEBUG_OUTPUT is True, then leave the gcode file behind so it
# can be looked at easily.
KEEP_DEBUG_OUTPUT = False

PathPost.LOG_MODULE = PathLog.thisModule()

PathLog.setLevel(PathLog.Level.INFO, PathPost.LOG_MODULE)


class TestPathPost(unittest.TestCase):
    """Test some of the output of the postprocessors.

    So far there are three tests each for the linuxcnc
    and centroid postprocessors.
    """

    #
    # This variable is used when there is more than one
    # file output while testing a postprocessor.  This may
    # happen when the "split%s" test is run, for example.
    #
    subpart = 1

    def setUp(self):
        """Set up the postprocessor tests."""
        pass

    def tearDown(self):
        """Tear down after the postprocessor tests."""
        pass

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
            # (freecad_document, job_name, postprocessor_arguments, output_file_id)
            #
            # test in metric mode (default)
            ("boxtest1", "Job", "--no-header --no-show-editor", "metric"),
            # test in Imperial mode
            ("boxtest1", "Job", "--no-header --no-show-editor --inches", "imperial"),
            # test in metric, G55, M4, the other way around the part
            ("boxtest1", "Job001", "--no-header --no-show-editor", "other_way"),
            # test in metric, split by fixtures, G54, G55, G56
            ("boxtest1", "Job002", "--no-header --no-show-editor", "split%s"),
        )
        #
        # The postprocessors to test.
        # You can comment out any postprocessors that you don't want
        # to test.
        #
        postprocessors_to_test = (
            "centroid",
            "linuxcnc",
        )
        #
        # Enough of the path to where the tests are stored so that
        # they can be found by the python interpreter.
        #
        PATHTESTS_LOCATION = "Mod/Path/PathTests/"
        #
        # The following code tries to re-use an open FreeCAD document
        # as much as possible.  It compares the current document with
        # the document for the next test.  If the names are different
        # then the current document is closed and the new document is
        # opened.  The final document is closed at the end of the code.
        #
        current_document = ""
        for (
            freecad_document,
            job_name,
            postprocessor_arguments,
            output_file_id,
        ) in tests_to_perform:
            if current_document != freecad_document:
                if current_document != "":
                    FreeCAD.closeDocument(current_document)
                current_document = freecad_document
                current_document_path = (
                    FreeCAD.getHomePath() + PATHTESTS_LOCATION + current_document + ".fcstd"
                )
                FreeCAD.open(current_document_path)
            job = FreeCAD.ActiveDocument.getObject(job_name)
            # Create a list of lists of objects to be written by the postprocessor.
            postlist = PathPost.CommandPathPost.buildPostList(self, job)
            for postprocessor_id in postprocessors_to_test:
                print(
                    "\nRunning %s test on %s postprocessor:\n" % (output_file_id, postprocessor_id)
                )
                processor = PostProcessor.load(postprocessor_id)
                output_file_pattern = "test_%s_%s.ngc" % (postprocessor_id, output_file_id)
                output_file_path = FreeCAD.getHomePath() + PATHTESTS_LOCATION + output_file_pattern
                self.subpart = 1
                for slist in postlist:
                    output_filename = PathPost.CommandPathPost.processFileNameSubstitutions(
                        self, job, output_file_path
                    )
                    file_path, extension = os.path.splitext(output_filename)
                    reference_file_name = "%s%s%s" % (file_path, "_ref", extension)
                    gcode = processor.export(slist, output_filename, postprocessor_arguments)
                    with open(reference_file_name, "r") as fp:
                        reference_gcode = fp.read()
                    if gcode != reference_gcode:
                        msg = "".join(
                            difflib.ndiff(gcode.splitlines(True), reference_gcode.splitlines(True))
                        )
                        self.fail(postprocessor_id + " output doesn't match: " + msg)
                    if not KEEP_DEBUG_OUTPUT:
                        os.remove(output_filename)
                self.subpart = 1
        if current_document != "":
            FreeCAD.closeDocument(current_document)


class TestPathPostUtils(unittest.TestCase):
    """Test the utility functions in the PostUtils.py file."""

    def testSplitArcs(self):
        """
        Tests the PostUtils.splitArcs function.

        Returns
        -------
        None
        """
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
        self.assertTrue(opList[0].Name == "Custom", "Expected first operation to be Custom")
        self.assertTrue(opList[1].Name == "Custom001", "Expected second operation to be Custom001")

        if close:
            FreeCAD.closeDocument(doc.Name)

    def test004(self):
        """test004()... Verify g-code imported with g-code pre-processor"""

        self.test003(close=False)

        doc = FreeCAD.ActiveDocument
        op1 = doc.Job.Operations.Group[0]
        op2 = doc.Job.Operations.Group[1]

        # Verify g-code sizes
        self.assertTrue(op1.Path.Size == 4, "Expected Custom g-code command count to be 4.")
        self.assertTrue(op2.Path.Size == 60, "Expected Custom g-code command count to be 60.")

        # Verify g-code commands
        op1_code = "(Custom_test_centroid_00)\n(Begin Custom)\nG90 G49.000000\n(End Custom)\n"
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
