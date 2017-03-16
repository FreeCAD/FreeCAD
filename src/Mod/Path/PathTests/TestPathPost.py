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
import Path
import PathScripts
import PathScripts.PathContour
import PathScripts.PathJob
import PathScripts.PathLoadTool
import PathScripts.PathPost
import PathScripts.PathUtils
import difflib
import unittest

class PathPostTestCases(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PathPostTest")

    def tearDown(self):
        FreeCAD.closeDocument("PathPostTest")

    def testLinuxCNC(self):
        # first create something to generate a path for
        box = self.doc.addObject("Part::Box", "Box")

        # Create job and setup tool library + default tool
        job = self.doc.addObject("Path::FeatureCompoundPython", "Job")
        PathScripts.PathJob.ObjectPathJob(job)
        job.Base = self.doc.Box
        PathScripts.PathLoadTool.CommandPathLoadTool.Create(job.Name, False)
        tool1 = Path.Tool()
        tool1.Diameter = 5.0
        tool1.Name = "Default Tool"
        tool1.CuttingEdgeHeight = 15.0
        tool1.ToolType = "EndMill"
        tool1.Material = "HighSpeedSteel"

        tc = FreeCAD.ActiveDocument.addObject("Path::FeaturePython",'TC')
        PathScripts.PathLoadTool.LoadTool(tc)
        PathScripts.PathUtils.addToJob(tc, "Job")
        tc.Tooltable.setTool(2, tool1)
        tc.ToolNumber = 2

        self.failUnless(True)

        self.doc.getObject("TC").ToolNumber = 2
        self.doc.recompute()

        contour = self.doc.addObject("Path::FeaturePython", "Contour")
        PathScripts.PathContour.ObjectContour(contour)
        contour.Active = True
        contour.ClearanceHeight = 20.0
        contour.StepDown = 1.0
        contour.StartDepth= 10.0
        contour.FinalDepth=0.0
        contour.SafeHeight = 12.0
        contour.OffsetExtra = 0.0
        contour.Direction = 'CW'
        contour.ToolController = tc
        contour.UseComp = True
        PathScripts.PathUtils.addToJob(contour)
        PathScripts.PathContour.ObjectContour.setDepths(contour.Proxy, contour)
        self.doc.recompute()

        job.PostProcessor = 'linuxcnc'
        job.PostProcessorArgs = '--no-header --no-line-numbers --no-comments --no-show-editor'

        post = PathScripts.PathPost.CommandPathPost()
        (fail, gcode) = post.exportObjectsWith([job], job, False)
        self.assertFalse(fail)

        referenceFile = FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_linuxcnc_00.ngc'
        with open(referenceFile, 'r') as fp:
            refGCode = fp.read()

        # Use if this test fails in order to have a real good look at the changes
        if False:
            with open('tab.tmp', 'w') as fp:
                fp.write(gcode)


        if gcode != refGCode:
            msg = ''.join(difflib.ndiff(gcode.splitlines(True), refGCode.splitlines(True)))
            self.fail("linuxcnc output doesn't match: " + msg)

