# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathPost
import PathScripts.PathToolController
import PathScripts.PathUtil
import difflib
import unittest

WriteDebugOutput = False

class PathPostTestCases(unittest.TestCase):

    def setUp(self):
        testfile = FreeCAD.getHomePath() + 'Mod/Path/PathTests/boxtest.fcstd'
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
        args = '--no-header --no-line-numbers --no-comments --no-show-editor --precision=2'
        gcode = postprocessor.export(self.postlist, 'gcode.tmp', args)

        referenceFile = FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_linuxcnc_00.ngc'
        with open(referenceFile, 'r') as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open('testLinuxCNC.tmp', 'w') as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = ''.join(difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True)))
            self.fail("linuxcnc output doesn't match: " + msg)

    def testLinuxCNCImperial(self):
        from PathScripts.post import linuxcnc_post as postprocessor
        args = '--no-header --no-line-numbers --no-comments --no-show-editor --precision=2 --inches'
        gcode = postprocessor.export(self.postlist, 'gcode.tmp', args)

        referenceFile = FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_linuxcnc_10.ngc'
        with open(referenceFile, 'r') as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open('testLinuxCNCImplerial.tmp', 'w') as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = ''.join(difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True)))
            self.fail("linuxcnc output doesn't match: " + msg)

    def testCentroid(self):
        from PathScripts.post import centroid_post as postprocessor
        args = '--no-header --no-line-numbers --no-comments --no-show-editor --axis-precision=2 --feed-precision=2'
        gcode = postprocessor.export(self.postlist, 'gcode.tmp', args)

        referenceFile = FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_centroid_00.ngc'
        with open(referenceFile, 'r') as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if WriteDebugOutput:
            with open('testCentroid.tmp', 'w') as fp:
                fp.write(gcode)

        if gcode != refGCode:
            msg = ''.join(difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True)))
            self.fail("linuxcnc output doesn't match: " + msg)
