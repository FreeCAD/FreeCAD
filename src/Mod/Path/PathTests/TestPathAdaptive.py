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
from PathTests.PathTestUtils import PathTestBase


class TestPathAdaptive(PathTestBase):
    """Unit tests for the Adaptive operation."""

    @staticmethod
    def _setup_test_environment():
        """_setup_test_environment()...
        This method is upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.
        This method does not have access to the class `self` reference.
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

        # Create Job object
        job = PathJob.Create('Job', [doc.Body, doc.Cut], None)
        job.GeometryTolerance.Value = 0.001
        doc.recompute()

        # Instantiate an Adaptive operation for quering available properties
        adaptive = PathAdaptive.Create("Adaptive")
        doc.recompute()
        if adaptive:  # satisfy LGTM
            return

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # FreeCAD.Console.PrintMessage("TestPathAdaptive.setUpClass()\n")
        cls._setup_test_environment()

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # FreeCAD.Console.PrintMessage("TestPathAdaptive.tearDownClass()\n")
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)

    def setUp(self):
        """setUp()...
        This method is called prior to each test() method.  Add code and objects here
        that are needed for multiple test() methods.
        """
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console
        self.adaptive = self.doc.getObject("Adaptive")

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    def _set_depths_and_heights(self, op):
        # Set Depths
        # set start and final depth in order to eliminate effects of stock (and its default values)
        op.setExpression('StartDepth', None)
        op.StartDepth.Value = 10.0
        op.setExpression('FinalDepth', None)
        op.FinalDepth.Value = 0.0
        op.setExpression('StepDown', None)
        op.StepDown.Value = 10.0

        # Set Heights
        # default values used

    def _is_top_face(self, base, feat):
        f = base.Shape.getElement(feat)
        if hasattr(f.Surface, "Axis"):
            if f.Surface.Axis == FreeCAD.Vector(0,0,1) and f.Orientation == 'Forward':
                return feat
        else:
            if f.normalAt(0, 0) == FreeCAD.Vector(0,0,1) and f.Orientation == 'Forward':
                return feat
        return None

    def test00(self):
        '''test00()...
        Test adaptive operation with:
            - PartDesign: top donut curved hole face selected
            - default settings
        '''
        precision = 1

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Body  # self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._is_top_face(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('adaptive-test00-PD-default')
        self._set_depths_and_heights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0

        adaptive.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                k = p.keys()
                if "'X', 'Y'" in str(k)[9:]:
                    x = round(p["X"], precision)
                    y = round(p["Y"], precision)
                    if (x == 2.5 or x == 47.5) and (y < 2.8 or y > 47.1):
                        txt = "({}, {})".format(x, y)
                        pnts[txt] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        # Verify minimum point count
        self.assertTrue(len(points) > 5)

        # Check for specific points in the path
        verify_points = ['(2.5, 2.7)', '(2.5, 47.3)',
                         '(47.5, 2.6)', '(47.5, 2.7)',
                         '(47.5, 47.2)', '(47.5, 47.3)']
        for vp in verify_points:
            if vp not in points:
                self.assertEqual(1.0, 2.0)
                return

    def test01(self):
        '''test01()...
        Test adaptive operation with:
            - PartDesign: top donut curved hole face selected
            - UseOutline = True
        '''
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
            name = self._is_top_face(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('adaptive-test01-PD-default_useoutline')
        self._set_depths_and_heights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.UseOutline = True

        adaptive.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                k = p.keys()
                if "'X', 'Y'" in str(k)[9:]:
                    x = round(p["X"], precision)
                    y = round(p["Y"], precision)
                    if (x == 2.5 or x == 47.5) and (y < 2.8 or y > 47.1):
                        txt = "({}, {})".format(x, y)
                        pnts[txt] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        # Verify minimum point count
        self.assertTrue(len(points) > 3)

        # Check for specific points in the path
        verify_points = ['(2.5, 2.7)', '(2.5, 47.3)','(47.5, 2.6)', '(47.5, 47.3)']
        for vp in verify_points:
            if vp not in points:
                self.assertEqual(1.0, 2.0)
                return

    def test02(self):
        '''test02()...
        Test adaptive operation with:
            - Part: top donut square hole face selected
            - UseOutline = True
        '''
        precision = 1

        # Verify Use Outline and All Access Extension properties available
        if not hasattr(self.adaptive, "UseOutline"):
            self.con.PrintMessage("TestPathAdaptive:: Skip UseOutline test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._is_top_face(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('adaptive-test01-PRT-default_useoutline')
        self._set_depths_and_heights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0
        adaptive.UseOutline = True

        adaptive.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                k = p.keys()
                if "'X', 'Y'" in str(k)[9:]:
                    x = round(p["X"], precision)
                    y = round(p["Y"], precision)
                    if (x == 2.5 or x == 47.5) and (y < 2.8 or y > 47.1):
                        txt = "({}, {})".format(x, y)
                        pnts[txt] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        # Verify minimum point count
        self.assertTrue(len(points) > 3)

        # Check for specific points in the path
        verify_points = ['(2.5, 2.7)', '(2.5, 47.3)','(47.5, 2.6)', '(47.5, 47.3)']
        for vp in verify_points:
            if vp not in points:
                self.assertEqual(1.0, 2.0)
                return

    def test03(self):
        '''test03()...
        Test adaptive operation with:
            - PartDesign: top donut curved hole face selected
            - ExtensionCorners = False
            - ExtensionFeature = [base, ["Face10:Edge4"]]
            - ExtensionLengthDefault.Value = 2.5
        '''
        precision = 1

        # Verify Extension Corners and Extension Feature properties available
        if not hasattr(self.adaptive, "ExtensionCorners") or not hasattr(self.adaptive, "ExtensionFeature"):
            self.con.PrintMessage("TestPathAdaptive:: Skip Extensions test.  Feature not available.\n")
            return

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Body  # self.doc.Cut
        subs_list = list()
        face_edge_text = str()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._is_top_face(base, feat)
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
        adaptive = PathAdaptive.Create('adaptive-test03-PD-default_extensions_Face10_Edge4')
        self._set_depths_and_heights(adaptive)

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

        pnts = dict()
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                k = p.keys()
                if "'X', 'Y'" in str(k)[9:]:  # str(k)[9:] == "(['F', 'X', 'Y'])":
                    x = round(p["X"], precision)
                    y = round(p["Y"], precision)
                    if (x == 2.5 or x == 47.5) and (y == 0.2 or y == 47.3):
                        txt = "({}, {})".format(x, y)
                        pnts[txt] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))  # for debugging and coding the test

        verify_points = ['(2.5, 0.2)', '(2.5, 47.3)', '(47.5, 0.2)', '(47.5, 47.3)']
        # Verify point count
        self.assertEqual(len(verify_points), len(points))
        # Verify each line-segment point, excluding arcs
        for i in range(len(verify_points)):
            self.assertEqual(points[i], verify_points[i])

    def test04(self):
        '''test04()...
        Test adaptive operation with:
            - Part: top donut square hole face selected
            - default settings
        '''
        precision = 1

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Cut
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            name = self._is_top_face(base, feat)
            if name:
                subs_list = [name]
                break

        # Instantiate an Adaptive operation
        adaptive = PathAdaptive.Create('adaptive-test04-PRT-default')
        self._set_depths_and_heights(adaptive)

        # Set Base Geometry
        adaptive.Base = (base, subs_list)

        # Set Operation
        adaptive.StartDepth.Value = 11.0
        adaptive.FinalDepth.Value = 10.0

        adaptive.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in adaptive.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                k = p.keys()
                if "'X', 'Y'" in str(k)[9:]:
                    x = round(p["X"], precision)
                    y = round(p["Y"], precision)
                    if (x < 2.7 or x > 47.3) and (y < 2.8 or y > 47.1):
                        txt = "({}, {})".format(x, y)
                        pnts[txt] = True
        points = sorted(pnts)
        self.con.PrintMessage("test04-points: {}\n".format(points))  # for debugging and coding the test

        # Verify minimum point count
        self.assertTrue(len(points) > 5)

        # Check for specific points in the path
        verify_points = ['(2.5, 47.3)', '(2.6, 47.4)', '(47.4, 2.6)',
                         '(47.5, 2.7)', '(47.5, 47.3)']
        for vp in verify_points:
            if vp not in points:
                self.assertEqual(vp, '(-1.0, -1.0)')
                return
