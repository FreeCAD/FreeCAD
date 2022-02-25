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
import PathScripts
import PathScripts.PathJob
import PathScripts.PathPost
import PathScripts.PathProfileContour
import PathScripts.PathToolController
import PathScripts.PathUtil
import PathScripts.post
import PathScripts.PostUtils as PostUtils

from PathScripts.PathPostProcessor import PostProcessor

# If KEEP_DEBUG_OUTPUT is False, remove the gcode file after the test.
# If KEEP_DEBUG_OUTPUT is True, then leave the gcode file behind in the
# directory where the test is run so it can be looked at easily.
KEEP_DEBUG_OUTPUT = True


class TestPathPost(unittest.TestCase):
    """Test some of the output of the postprocessors.

    At the moment this is just getting started, and only tests
    the linuxcnc postprocessor.  There is one test for a metric
    output and one test which adds the --inches option.
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

    def _generate_gcode(self, job, postprocessor_name, postprocessor_args, gcode_file):
        """Generate the gcode for a job"""

        wcslist = job.Fixtures
        orderby = job.OrderOutputBy
        split = job.SplitOutput

        postlist = []

        if orderby == "Fixture":
            # Order by fixture means all operations and tool changes will be completed in one
            # fixture before moving to the next.

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
                    tc = PathScripts.PathUtil.toolControllerForOp(obj)
                    if tc is not None and PathScripts.PathUtil.opProperty(obj, "Active"):
                        if tc.ToolNumber != currTool or split is True:
                            sublist.append(tc)
                            currTool = tc.ToolNumber
                    sublist.append(obj)
                postlist.append(sublist)

        elif orderby == "Tool":
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
                active = PathScripts.PathUtil.opProperty(obj, "Active")

                tc = PathScripts.PathUtil.toolControllerForOp(obj)
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
            # Order by operation means ops are done in each fixture in
            # sequence.
            currTool = None
            firstFixture = True

            # Now generate the gcode
            for obj in job.Operations.Group:
                if PathScripts.PathUtil.opProperty(obj, "Active"):
                    sublist = []
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
                        tc = PathScripts.PathUtil.toolControllerForOp(obj)
                        if tc is not None:
                            if job.SplitOutput or (tc.ToolNumber != currTool):
                                sublist.append(tc)
                                currTool = tc.ToolNumber
                        sublist.append(obj)
                    postlist.append(sublist)

        if split:
            all_gcode = []
            for slist in postlist:
                processor = PostProcessor.load(postprocessor_name)
                gcode = processor.export(slist, gcode_file, postprocessor_args)
                all_gcode.append(gcode)
            return(all_gcode)
        else:
            finalpostlist = [item for slist in postlist for item in slist]
            processor = PostProcessor.load(postprocessor_name)
            gcode = processor.export(finalpostlist, gcode_file, postprocessor_args)
            return (gcode)

    def _run_a_test(self, freecad_document, postprocessor_file, postprocessor_args, gcode_file, reference_file):
        """
        Run one test based on the arguments.

        Parameters
        ----------
        freecad_document : str
            the name of the FreeCAD document to open
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

        freecad_document_path = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/" + freecad_document + ".fcstd"
        )
        self.doc = FreeCAD.open(freecad_document_path)
        self.job = FreeCAD.ActiveDocument.getObject("Job")
        gcode = self._generate_gcode(self.job,
                                     postprocessor_file,
                                     postprocessor_args,
                                     gcode_file)
        FreeCAD.closeDocument(freecad_document)
        if not KEEP_DEBUG_OUTPUT:
            os.remove(gcode_file)

        reference_file_path = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/" + reference_file + ".ngc"
        )
        with open(reference_file_path, "r") as fp:
            refGCode = fp.read()

        if gcode != refGCode:
            msg = "".join(
                difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True))
            )
            self.fail("linuxcnc output doesn't match: " + msg)

    def test_linuxcnc(self):
        """Test the linuxcnc postprocessor in metric mode (default)."""
        postprocessor_args = (
            # "--no-header --no-comments --no-show-editor --precision=2"
            "--no-header --no-show-editor"
        )
        self._run_a_test(freecad_document="boxtest1",
                         postprocessor_file="linuxcnc",
                         postprocessor_args=postprocessor_args,
                         gcode_file="test_linuxcnc.ngc",
                         reference_file="test_linuxcnc_01")

    def test_linuxcnc_imperial(self):
        """Test the linuxcnc postprocessor in Imperial mode."""
        postprocessor_args = (
            "--no-header --no-show-editor --inches"
        )
        self._run_a_test(freecad_document="boxtest1",
                         postprocessor_file="linuxcnc",
                         postprocessor_args=postprocessor_args,
                         gcode_file="test_linuxcnc_imperial.ngc",
                         reference_file="test_linuxcnc_10")


class TestPathPostUtils(unittest.TestCase):
    """Test the utility functions in the PostUtils.py file."""
    def testSplitArcs(self):
        """
        Tests the PostUtils.splitArcs function.

        Returns
        -------
        None.

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

        results = PostUtils.splitArcs(testpath)
        self.assertTrue(len(results.Commands) == 41)
        self.assertTrue(len([c for c in results.Commands if c.Name in ['G2', 'G3']]) == 0)
