# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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
import PathScripts.PathJob as PathJob
import PathScripts.PathAdaptive as PathAdaptive
# import PathScripts.PathGeom as PathGeom
from PathTests.PathTestUtils import PathTestBase


class TestPathAdaptive(PathTestBase):
    """Unit tests for the Adaptive operation."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment. 
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """

        doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_adaptive.fcstd')
        # doc_title = "Test_PathAdaptive"
        # doc = FreeCAD.newDocument(doc_title)

        # Create a square donut
        box0 = doc.addObject('Part::Box', 'Box')  # Box
        box0.Length = 50.0
        box0.Width = 50.0
        box0.Height = 10.0
        box1 = doc.addObject('Part::Box', 'Box')  # Box001
        box1.Length = 20.0  # X
        box1.Width = 20.0  # Y
        box1.Height = 20.0  # Z
        box1.Placement = FreeCAD.Placement(FreeCAD.Vector(5.0, 5.0, -5.0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        doc.recompute()
        cut = doc.addObject('Part::Cut', 'Cut')
        cut.Base = box0
        cut.Tool = box1
        cut.Placement = FreeCAD.Placement(FreeCAD.Vector(10.0, 10.0, 0.0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        doc.recompute()

        # Create Job object
        job = PathJob.Create('Job', [doc.Body, doc.Cut], None)
        job.GeometryTolerance.Value = 0.001
        doc.recompute()

        # Instantiate an Adaptive operation for quering available properties
        PathAdaptive.Create("Adaptive_prototype")
        doc.recompute()

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """

        # Comment out to leave test file open and objects and paths intact after all tests finish
        # FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)
        pass

    def setUp(self):
        """setUp()...
        This method is called prior to each test() method.  Add code and objects here
        that are needed for multiple test() methods.
        """
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console
        self.adaptive = self.doc.getObject("Adaptive_prototype")

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def _setDepthsAndHeights(self, op):
        # Set Depths
        # Set start and final depth in order to eliminate effects of stock (and its default values)
        op.setExpression('StartDepth', None)
        op.StartDepth.Value = 10.0
        op.setExpression('FinalDepth', None)
        op.FinalDepth.Value = 0.0

        # Set step down so as to only produce one layer path
        op.setExpression('StepDown', None)
        op.StepDown.Value = 10.0

        # Set Heights
        # default values used
        pass

    def _isTopFace(self, base, feat):
        f = base.Shape.getElement(feat)
        if hasattr(f.Surface, "Axis"):
            if f.Surface.Axis == FreeCAD.Vector(0,0,1) and f.Orientation == 'Forward':
                return feat
        else:
            if f.normalAt(0, 0) == FreeCAD.Vector(0,0,1) and f.Orientation == 'Forward':
                return feat
        return None

    # Methods to identify target points in path command lists
    def _findTargetPoints(self, adaptive, precision, getTextMethod):
        gpt = getattr(self, getTextMethod)  # use method name passed as argument
        pnts = dict()
        z_depth = 0.0
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                x = p.get("X")
                y = p.get("Y")
                z = p.get("Z")
                if x and y:
                    if z:
                        # save last Z value
                        z_depth = round(z, precision)
                    else:
                        xVal = round(x, precision)
                        yVal = round(y, precision)
                        txt = gpt(xVal, yVal, z_depth)
                        if txt:
                            pnts[txt] = True
        # points = sorted(pnts)
        return sorted(pnts)

    def _getPointText_00(self, xVal, yVal, z_depth):
        if ((xVal < 12.7 or xVal > 57.3) and (yVal < 12.7 or yVal > 57.3)):
            txt = "({}, {}, {})".format(xVal, yVal, z_depth)
            return txt

    def _getPointText_03(self, xVal, yVal, z_depth):
        if ((xVal == 12.5 or xVal == 57.5) and (yVal == 10.02 or yVal == 57.3)):
            txt = "({}, {}, {})".format(xVal, yVal, z_depth)
            return txt

    # General method to compare point data of path with verified points
    def _verifyPointData(self, testName, points, verify_points):
        len_pts = len(points)
        len_vrfy_pts = len(verify_points)

        # Verify point counts
        if len_pts < len_vrfy_pts:
            self.con.PrintMessage("{}-points: {}\n".format(testName, points))  # for debugging and coding the test
            self.con.PrintMessage("len_pts, len_vrfy_pts: {}, {}\n".format(len_pts, len_vrfy_pts))  # for debugging and coding the test
            self.assertFalse(len_pts < len_vrfy_pts)
        if len_pts > len_vrfy_pts + 4:
            self.con.PrintMessage("{}-points: {}\n".format(testName, points))  # for debugging and coding the test
            self.con.PrintMessage("len_pts, len_vrfy_pts: {}, {}\n".format(len_pts, len_vrfy_pts))  # for debugging and coding the test
            self.assertFalse(len_pts > len_vrfy_pts + 4)

        # Check for specific points in the path
        for vp in verify_points:
            if vp not in points:
                self.con.PrintMessage("{}-points: {}\n".format(testName, points))  # for debugging and coding the test
                self.con.PrintMessage("len_pts, len_vrfy_pts: {}, {}\n".format(len_pts, len_vrfy_pts))  # for debugging and coding the test
                self.assertEqual(vp, '(-1.0, -1.0, -1.0)')

    # Unit tests
    def test00(self):
        '''Test PartDesign face with bspline hole: default settings'''
        precision = 1

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Body  # self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test00')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.StepOver = 50
        # adaptive.Tolerance = 0.05  # min: 0.001; default: 0.1

        adaptive.recompute()
        self.doc.recompute()

        # Identify target point set from path commands
        getTextMethod = "_getPointText_00"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.6, 12.6, 10.0)', '(12.6, 57.4, 10.0)',
                         '(57.4, 12.6, 10.0)', '(57.4, 57.4, 10.0)']
        self._verifyPointData("test00", points, verify_points)

    def test01(self):
        '''Test PartDesign face with bspline hole: UseOutline=True'''
        precision = 1

        # Verify Use Outline and All Access Extension properties available
        if not hasattr(self.adaptive, "UseOutline"):
            self.con.PrintMessage("TestPathAdaptive:: Skip UseOutline test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Body  # self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test01')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.UseOutline = True

        adaptive.recompute()
        self.doc.recompute()
        
        # Identify target point set from path commands
        getTextMethod = "_getPointText_00"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.6, 12.6, 10.0)', '(12.6, 57.4, 10.0)',
                         '(57.4, 12.6, 10.0)', '(57.4, 57.4, 10.0)']
        self._verifyPointData("test01", points, verify_points)

    def test02(self):
        '''Test Part face with square hole: UseOutline=True'''
        precision = 1

        # Verify Use Outline and All Access Extension properties available
        if not hasattr(self.adaptive, "UseOutline"):
            self.con.PrintMessage("TestPathAdaptive.test02:: Skip UseOutline test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test02')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.UseOutline = True

        adaptive.recompute()
        self.doc.recompute()

        # Identify target point set from path commands
        getTextMethod = "_getPointText_00"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.6, 12.6, 10.0)', '(12.6, 57.4, 10.0)',
                         '(57.4, 12.6, 10.0)', '(57.4, 57.4, 10.0)']
        self._verifyPointData("test02", points, verify_points)

    def test03(self):
        '''Test PartDesign face with bspline hole: ExtensionCorners=False; ExtensionFeature=[base, ["Face10:Edge4"]]'''
        precision = 1
        # Verify Extension Corners and Extension Feature properties available
        if not hasattr(self.adaptive, "ExtensionCorners") or not hasattr(self.adaptive, "ExtensionFeature"):
            self.con.PrintMessage("TestPathAdaptive.test03:: Skip Extensions test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Body  # self.doc.Cut
        subs_list = list()
        face_edge_text = str()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                face_edge_text = name
                subs_list = [name]
                break

        # Identify edge name for front, top edge
        for i in range(0, len(base.Shape.Edges)):
            feat = "Edge{}".format(i+1)
            f = base.Shape.getElement(feat)
            fBB = f.BoundBox
            if (f.Length == 50.0 and round(fBB.ZLength, 6) == 0.0 and
                round(fBB.ZMin, 6) == 10.0 and round(fBB.XMin, 6) == 0.0  and
                round(fBB.YMax, 6) == 0.0):
                face_edge_text += ":" + feat
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test03')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.ExtensionCorners = False
        adaptive.ExtensionFeature = [base, [face_edge_text]]
        adaptive.ExtensionLengthDefault.Value = 2.5

        adaptive.recompute()
        self.doc.recompute()

        # Identify target point set from path commands
        getTextMethod = "_getPointText_03"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.5, 10.2, 10.0)', '(12.5, 57.3, 10.0)',
                         '(57.5, 10.2, 10.0)', '(57.5, 57.3, 10.0)']
        self._verifyPointData("test02", points, verify_points)

    def test04(self):
        '''Test Part face with square hole: default settings'''
        precision = 1

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test04')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0

        adaptive.recompute()
        self.doc.recompute()
        
        # Identify target point set from path commands
        getTextMethod = "_getPointText_00"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.6, 57.4, 10.0)', '(57.4, 12.6, 10.0)', '(57.4, 57.4, 10.0)']
        self._verifyPointData("test04", points, verify_points)

    def test05(self):
        '''Test Part face with square hole: UseOutline=True; StepOver=50'''
        precision = 1

        # Verify Use Outline and All Access Extension properties available
        if not hasattr(self.adaptive, "UseOutline"):
            self.con.PrintMessage("TestPathAdaptive.test05:: Skip UseOutline test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._isTopFace(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('Adaptive-test05')
        self._setDepthsAndHeights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.UseOutline = True
        adaptive.StepOver = 50
        adaptive.Tolerance = 0.005

        adaptive.recompute()
        self.doc.recompute()

        # Identify target point set from path commands
        getTextMethod = "_getPointText_00"
        points = self._findTargetPoints(adaptive, precision, getTextMethod)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(12.6, 12.6, 10.0)', '(12.6, 57.4, 10.0)',
                         '(57.4, 12.6, 10.0)', '(57.4, 57.4, 10.0)']
        self._verifyPointData("test05", points, verify_points)

