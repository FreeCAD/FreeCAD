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

import difflib
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

WriteDebugOutput = False


class TestPathPostTestCases(unittest.TestCase):
    """Test some of the output of the postprocessors.

    At the moment this is just getting started, and only tests
    the linuxcnc postprocessor.  There is one test for a metric
    output and one test which adds the --inches option.
    """

    def setUp(self):
        """Set up the postprocessor tests."""
        testfile = FreeCAD.getHomePath() + "Mod/Path/PathTests/boxtest1.fcstd"
        self.doc = FreeCAD.open(testfile)
        self.job = FreeCAD.ActiveDocument.getObject("Job")
        self.postlist = []
        currTool = None
        for obj in self.job.Operations.Group:
            if not isinstance(obj.Proxy, PathScripts.PathToolController.ToolController):
                tc = PathScripts.PathUtil.toolControllerForOp(obj)
                if tc is not None:
                    if tc.ToolNumber != currTool:
                        self.postlist.append(tc)
                self.postlist.append(obj)

    def tearDown(self):
        """Tear down after the postprocessor tests."""
        FreeCAD.closeDocument("boxtest1")

    def testLinuxCNC(self):
        """
        Test the linuxcnc postprocessor in metric mode (default).

        Returns
        -------
        None.

        """
        from PathScripts.post import linuxcnc_post as postprocessor

        args = (
            # "--no-header --no-comments --no-show-editor --precision=2"
            "--no-header --no-show-editor"
        )
        gcode = postprocessor.export(self.postlist, "gcode.tmp", args)

        referenceFile = (
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_linuxcnc_01.ngc"
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
        """
        Test the linuxcnc postprocessor using the --inches option.

        This uses the same file and job as the testLinuxCNC test but
        adds the --inches option.

        Returns
        -------
        None.

        """
        from PathScripts.post import linuxcnc_post as postprocessor

        args = "--no-header --no-comments --no-show-editor --precision=2 --inches"
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
