# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Dan Falk <ddfalck@gmail.com>                       *
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
from PathScripts import PathUtil
from PathScripts import PostUtils

from PathScripts.PathPostProcessor import PostProcessor

# If KEEP_DEBUG_OUTPUT is False, remove the gcode file after the test.
# If KEEP_DEBUG_OUTPUT is True, then leave the gcode file behind in the
# directory where the test is run so it can be looked at easily.
KEEP_DEBUG_OUTPUT = False

PathPost.LOG_MODULE = PathLog.thisModule()

PathLog.setLevel(PathLog.Level.INFO, PathPost.LOG_MODULE)


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

    class _TempObject:
        # pylint: disable=no-init
        Path = None
        Name = "Fixture"
        InList = []
        Label = "Fixture"

    def _generate_gcode(self, job):
        """
        Generate the gcode for the passed-in job.

        Parameters
        ----------
        job : FreeCAD job object
            the job to generate the gcode for

        Returns
        -------
        split : bool
            True if need to split output
        postlist : list
            the list of objects to postprocess

        Note:  This code is copied from PathScripts/PathPost.py
               with the intent of refactoring PathPost.py in the future
               so there only needs to be one copy of this code
               (in the PathPost.py file).
               Now is not a good time to do this.
        """

        PathLog.debug("about to postprocess job: {}".format(job.Name))

        wcslist = job.Fixtures
        orderby = job.OrderOutputBy
        split = job.SplitOutput

        postlist = []

        if orderby == "Fixture":
            PathLog.debug("Ordering by Fixture")
            # Order by fixture means all operations and tool changes will be
            # completed in one fixture before moving to the next.

            currTool = None
            for index, f in enumerate(wcslist):
                # create an object to serve as the fixture path
                fobj = self._TempObject()
                c1 = Path.Command(f)
                fobj.Path = Path.Path([c1])
                if index != 0:
                    c2 = Path.Command(
                        "G0 Z"
                        + str(
                            job.Stock.Shape.BoundBox.ZMax
                            + job.SetupSheet.ClearanceHeightOffset.Value
                        )
                    )
                    fobj.Path.addCommands(c2)
                fobj.InList.append(job)
                sublist = [fobj]

                # Now generate the gcode
                for obj in job.Operations.Group:
                    tc = PathUtil.toolControllerForOp(obj)
                    if tc is not None and PathUtil.opProperty(obj, "Active"):
                        if tc.ToolNumber != currTool or split is True:
                            sublist.append(tc)
                            PathLog.debug("Appending TC: {}".format(tc.Name))
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append(sublist)

        elif orderby == "Tool":
            PathLog.debug("Ordering by Tool")
            # Order by tool means tool changes are minimized.
            # all operations with the current tool are processed in the current
            # fixture before moving to the next fixture.

            currTool = None
            fixturelist = []
            for f in wcslist:
                # create an object to serve as the fixture path
                fobj = self._TempObject()
                c1 = Path.Command(f)
                c2 = Path.Command(
                    "G0 Z"
                    + str(
                        job.Stock.Shape.BoundBox.ZMax
                        + job.SetupSheet.ClearanceHeightOffset.Value
                    )
                )
                fobj.Path = Path.Path([c1, c2])
                fobj.InList.append(job)
                fixturelist.append(fobj)

            # Now generate the gcode
            curlist = []  # list of ops for tool, will repeat for each fixture
            sublist = []  # list of ops for output splitting

            for idx, obj in enumerate(job.Operations.Group):

                # check if the operation is active
                active = PathUtil.opProperty(obj, "Active")

                tc = PathUtil.toolControllerForOp(obj)
                if tc is None or tc.ToolNumber == currTool and active:
                    curlist.append(obj)
                elif (
                    tc.ToolNumber != currTool and currTool is None and active
                ):  # first TC
                    sublist.append(tc)
                    curlist.append(obj)
                    currTool = tc.ToolNumber
                elif (
                    tc.ToolNumber != currTool and currTool is not None and active
                ):  # TC
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append(sublist)
                    sublist = [tc]
                    curlist = [obj]
                    currTool = tc.ToolNumber

                if idx == len(job.Operations.Group) - 1:  # Last operation.
                    for fixture in fixturelist:
                        sublist.append(fixture)
                        sublist.extend(curlist)
                    postlist.append(sublist)

        elif orderby == "Operation":
            PathLog.debug("Ordering by Operation")
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None
            firstFixture = True

            # Now generate the gcode
            for obj in job.Operations.Group:
                if PathUtil.opProperty(obj, "Active"):
                    sublist = []
                    PathLog.debug("obj: {}".format(obj.Name))
                    for f in wcslist:
                        fobj = self._TempObject()
                        c1 = Path.Command(f)
                        fobj.Path = Path.Path([c1])
                        if not firstFixture:
                            c2 = Path.Command(
                                "G0 Z"
                                + str(
                                    job.Stock.Shape.BoundBox.ZMax
                                    + job.SetupSheet.ClearanceHeightOffset.Value
                                )
                            )
                            fobj.Path.addCommands(c2)
                        fobj.InList.append(job)
                        sublist.append(fobj)
                        firstFixture = False
                        tc = PathUtil.toolControllerForOp(obj)
                        if tc is not None:
                            if job.SplitOutput or (tc.ToolNumber != currTool):
                                sublist.append(tc)
                                currTool = tc.ToolNumber
                        sublist.append(obj)
                    postlist.append(sublist)
        return(split, postlist)

    def _run_a_test(
        self, freecad_document, job_name, postprocessor_file, postprocessor_args, gcode_file, reference_file
    ):
        """
        Run one test based on the arguments.

        Parameters
        ----------
        freecad_document : str
            the name of the FreeCAD document to open
        job_name : str
            the name of the job to postprocess in the FreeCAD document
        postprocessor_file : str
            the name of the postprocessor file to test
        postprocessor_args : str
            the arguments to pass to the postprocessor
        gcode_file : str
            the name of the file the postprocessor writes the gcode to
        reference_file : str
            the name of the file that the gcode is compared to

        Returns
        -------
        None

        """
        PATHTESTS_LOCATION = "Mod/Path/PathTests/"

        freecad_document_path = (
            FreeCAD.getHomePath() + PATHTESTS_LOCATION + freecad_document + ".fcstd"
        )
        self.doc = FreeCAD.open(freecad_document_path)
        self.job = FreeCAD.ActiveDocument.getObject(job_name)
        (split, postlist) = self._generate_gcode(self.job)
        if split:
            gcode_list = []
            for slist in postlist:
                processor = PostProcessor.load(postprocessor_file)
                part_gcode = processor.export(slist, gcode_file, postprocessor_args)
                gcode_list.append(part_gcode)
            gcode = "".join(gcode_list)
        else:
            finalpostlist = [item for slist in postlist for item in slist]
            processor = PostProcessor.load(postprocessor_file)
            gcode = processor.export(finalpostlist, gcode_file, postprocessor_args)
        FreeCAD.closeDocument(freecad_document)
        if not KEEP_DEBUG_OUTPUT:
            os.remove(gcode_file)

        reference_file_path = (
            FreeCAD.getHomePath() + PATHTESTS_LOCATION + reference_file + ".ngc"
        )
        with open(reference_file_path, "r") as fp:
            refGCode = fp.read()

        if gcode != refGCode:
            msg = "".join(difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True)))
            self.fail(postprocessor_file + " output doesn't match: " + msg)

    def test_linuxcnc(self):
        """Test the linuxcnc postprocessor in metric mode (default)."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job",
            postprocessor_file="linuxcnc",
            postprocessor_args="--no-header --no-show-editor",
            gcode_file="test_linuxcnc.ngc",
            reference_file="test_linuxcnc_01",
        )

    def test_linuxcnc_imperial(self):
        """Test the linuxcnc postprocessor in Imperial mode."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job",
            postprocessor_file="linuxcnc",
            postprocessor_args="--no-header --no-show-editor --inches",
            gcode_file="test_linuxcnc_imperial.ngc",
            reference_file="test_linuxcnc_11",
        )

    def test_linuxcnc_02(self):
        """Test the linuxcnc postprocessor metric, G55, M4, other way around part."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job001",
            postprocessor_file="linuxcnc",
            postprocessor_args="--no-header --no-show-editor",
            gcode_file="test_linuxcnc_2.ngc",
            reference_file="test_linuxcnc_02",
        )

    def test_centroid(self):
        """Test the centroid postprocessor in metric (default) mode."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job",
            postprocessor_file="centroid",
            postprocessor_args="--no-header --no-show-editor",
            gcode_file="test_centroid.ngc",
            reference_file="test_centroid_01",
        )

    def test_centroid_imperial(self):
        """Test the centroid postprocessor in Imperial mode."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job",
            postprocessor_file="centroid",
            postprocessor_args="--no-header --no-show-editor --inches",
            gcode_file="test_centroid_imperial.ngc",
            reference_file="test_centroid_11",
        )

    def test_centroid_02(self):
        """Test the centroid postprocessor metric, G55, M4, other way around part."""
        self._run_a_test(
            freecad_document="boxtest1",
            job_name="Job001",
            postprocessor_file="centroid",
            postprocessor_args="--no-header --no-show-editor",
            gcode_file="test_centroid_2.ngc",
            reference_file="test_centroid_02",
        )


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
        self.assertTrue(
            len([c for c in testpath.Commands if c.Name in ["G2", "G3"]]) == 4
        )

        results = PostUtils.splitArcs(testpath, deflection=0.01)
        self.assertTrue(len(results.Commands) == 41)
        self.assertTrue(
            len([c for c in results.Commands if c.Name in ["G2", "G3"]]) == 0
        )
