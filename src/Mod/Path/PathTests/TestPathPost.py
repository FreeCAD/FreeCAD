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
import importlib
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

    def _run_a_test(self, freecad_document, postprocesser_file, postprocessor_args, gcode_file, reference_file):
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
        self.postlist = []
        currTool = None
        for obj in self.job.Operations.Group:
            if not isinstance(obj.Proxy, PathScripts.PathToolController.ToolController):
                tc = PathScripts.PathUtil.toolControllerForOp(obj)
                if tc is not None:
                    if tc.ToolNumber != currTool:
                        self.postlist.append(tc)
                self.postlist.append(obj)

        full_postprocessor_file_name = "PathScripts.post." + postprocesser_file
        postprocessor = importlib.import_module(name=full_postprocessor_file_name)
        gcode = postprocessor.export(self.postlist, gcode_file, postprocessor_args)
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
                         postprocesser_file="linuxcnc_post",
                         postprocessor_args=postprocessor_args,
                         gcode_file="test_linuxcnc.ngc",
                         reference_file="test_linuxcnc_01")

    def test_linuxcnc_imperial(self):
        """Test the linuxcnc postprocessor in Imperial mode."""
        postprocessor_args = (
            "--no-header --no-comments --no-show-editor --precision=2 --inches"
        )
        self._run_a_test(freecad_document="boxtest1",
                         postprocesser_file="linuxcnc_post",
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
