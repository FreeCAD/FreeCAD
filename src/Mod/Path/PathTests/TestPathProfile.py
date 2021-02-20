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
import PathScripts.PathJob as PathJob
import PathScripts.PathProfile as PathProfile
import PathScripts.PathGeom as PathGeom
from PathTests.PathTestUtils import PathTestBase
import BasicShapes.Shapes as Shapes


class TestPathProfile(PathTestBase):
    """Unit tests for the Profile operation."""

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

        # Create a new document and create test geometry
        # doc_title = "TestProfile"
        # doc = FreeCAD.newDocument(doc_title)
        # cls._createTestGeometry(doc)

        # Open existing document with test geometry
        doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_profile.fcstd')

        # Create Job object, adding geometry objects from file opened above
        job = PathJob.Create('Job', [doc.Fuse0r, doc.Box2, doc.Body], None)
        job.GeometryTolerance.Value = 0.001
        doc.recompute()

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # FreeCAD.Console.PrintMessage("TestPathProfile.tearDownClass()\n")

        # Comment out to leave test file open and objects and paths intact after all tests finish
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)
        pass

    @staticmethod
    def _createTestGeometry(doc):
        """_createTestGeometry(doc)...
        This contains the instructions to create test geometry for the unit tests in this class.
        A simple example creation is provided.
        """

        # Create a square donut
        box0 = doc.addObject('Part::Box', 'Box0')  # Box
        box0.Length = 50.0
        box0.Width = 50.0
        box0.Height = 10.0
        box1 = doc.addObject('Part::Box', 'Box1')  # Box001
        box1.Length = 10.0  # X
        box1.Width = 10.0  # Y
        box1.Height = 20.0  # Z
        box1.Placement = FreeCAD.Placement(FreeCAD.Vector(10.0, 10.0, -5.0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        cut0 = doc.addObject('Part::Cut', 'Cut0')
        cut0.Base = box0
        cut0.Tool = box1
        doc.recompute()

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    # Unit tests
    def test00(self):
        '''Profile all Job modeles: processHoles=True'''

        # Identify base feature to be used for operation's Base Geometry
        """
        base = self.doc.Fuse0
        subs_list = list()
        for i in range(0, len(base.Shape.Faces)):
            feat = "Face{}".format(i+1)
            f = base.Shape.getElement(feat)
            if f.Surface.Axis == FreeCAD.Vector(0,0,1) and f.Orientation == 'Forward':
                subs_list.append(feat)
                break
        """

        # Instantiate a Profile operation and set Base Geometry
        profile = PathProfile.Create('Profile')
        # profile.Base = (base, subs_list)

        # Set additional operation properties
        self._set_depths_and_heights(profile)
        profile.Label = "test00_"
        profile.Comment = "Profile whole model: processHoles=True"
        # profile.processCircles = True
        profile.processHoles = True

        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1" or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(-10.05, 3.31, 0.0)', '(-13.37, 45.51, 0.0)', '(-18.29, 40.72, 0.0)',
              '(-18.29, 8.35, 0.0)', '(-2.5, 0.0, 0.0)', '(-2.5, 2.0, 0.0)',
              '(-31.71, 40.72, 0.0)', '(-31.71, 8.35, 0.0)', '(-36.63, 3.55, 0.0)',
              '(-4.39, 7.56, 0.0)', '(-41.19, 17.83, 0.0)', '(-41.19, 31.24, 0.0)',
              '(-45.98, 36.16, 0.0)', '(-8.81, 17.83, 0.0)', '(-8.81, 31.24, 0.0)',
              '(0.0, -2.5, 0.0)', '(0.0, 4.5, 0.0)', '(10.0, 7.5, 0.0)',
              '(2.0, -2.5, 0.0)', '(2.0, 4.5, 0.0)', '(3.77, 3.77, 0.0)',
              '(4.5, 0.0, 0.0)', '(4.5, 2.0, 0.0)', '(42.32, 62.5, 0.0)',
              '(60.0, 62.5, 0.0)', '(60.0, 7.5, 0.0)', '(62.5, 10.0, 0.0)',
              '(62.5, 60.0, 0.0)', '(7.5, 10.0, 0.0)', '(7.5, 60.0, 0.0)',
              '(7.54, 9.58, 0.0)']
        self._verifyPointData("test00", points, verify_points)

    def test01(self):
        '''Profile two top continuous linear open edges: ["Edge15", "Edge21"], FinalDepth=5.0'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r  # Refined version of Fusion
        subs_list = ["Edge15", "Edge21"]

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test01_"
        profile.Comment = "Profile two top continuous linear open edges: ['Edge15', 'Edge21'], FinalDepth=5.0"
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0

        # profile.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(10.0, 7.5, 5.0)', '(40.0, 7.5, 5.0)',
                         '(42.5, 10.0, 5.0)', '(42.5, 60.0, 5.0)']
        self._verifyPointData("test01", points, verify_points)

    def test02(self):
        '''Profile two top continuous linear open edges with offset: ['Edge15', 'Edge21']; FinalDepth=5.0; offsetExtra=2.0'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r  # Refined version of Fusion
        subs_list = ["Edge15", "Edge21"]

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test02_"
        profile.Comment = "Profile two top continuous linear open edges with offset: ['Edge15', 'Edge21']; FinalDepth=5.0; offsetExtra=2.0"
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0
        profile.OffsetExtra.Value = 2.0

        # profile.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(10.0, 5.5, 5.0)', '(40.0, 5.5, 5.0)',
                         '(44.5, 10.0, 5.0)', '(44.5, 60.0, 5.0)']
        self._verifyPointData("test02", points, verify_points)

    def test03(self):
        '''Profile single top linear open edge with micro bit: FinalDepth=7.5; ToolBit.Diameter.Value=0.1'''

        bit_diameter = self.doc.ToolBit.Diameter.Value

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Box2
        subs_list = list()
        for i in range(0, len(base.Shape.Edges)):
            feat = "Edge{}".format(i+1)
            f = base.Shape.getElement(feat)
            fBB = f.BoundBox
            if (PathGeom.isRoughly(fBB.ZMin, 10.0) and
                PathGeom.isRoughly(fBB.XMin, 0.0) and
                PathGeom.isRoughly(fBB.YMin, 0.0) and
                PathGeom.isRoughly(fBB.XMax, 2.0)):
                subs_list.append(feat)
                break

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test03_"
        profile.Comment = "Profile single top linear open edge with micro bit: FinalDepth=7.5; ToolBit.Diameter.Value=0.1"
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 7.5

        # Adjust Toolbit
        self.doc.ToolBit.Diameter.Value = 0.1  # micro bit

        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            if c.Name == "G1":
                p = c.Parameters
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(0.0, -0.05, 7.5)', '(2.0, -0.05, 7.5)']
        self._verifyPointData("test03", points, verify_points)

        # Restore toolbit diameter for Job
        self.doc.ToolBit.Diameter.Value = bit_diameter
        self.doc.recompute()

    def test04(self):
        '''Test face-edge filter capability. Profile top face and single top open edge \
        simultaneously: ['Face11', 'Edge15']; Side='Outside'; FinalDepth=5.0.'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r
        subs_list = ['Face11', 'Edge15']

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test04_"
        profile.Comment = "Test face-edge filter capability. \
            Profile top face and single hole top open edge simultaneously: \
            ['Face11', 'Edge15']; Side='Outside'; FinalDepth=5.0."
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0
        profile.Side = "Outside"

        self.doc.recompute()
        
        # Identify points on outer linear profile paths and single open-edge path
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1" or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        # First point is transition from open-edge to outer face profiles
        verify_points = ['(10.0, 7.5, 5.0)', '(40.0, 7.5, 5.0)', '(42.5, 10.0, 5.0)',
                         '(42.5, 60.09, 5.0)', '(7.5, 10.0, 5.0)', '(7.5, 60.0, 5.0)']
        self._verifyPointData("test02", points, verify_points)

    def test05(self):
        '''Profile single top linear open edge: ['Edge33']; FinalDepth=5.0.
        This profile SHOULD FAIL due to inaccessibility bug, producing zero path points for verification.'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r  # Refined version of Fusion
        subs_list = ['Edge33']

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test05_"
        profile.Comment = "Profile single top linear open edge: ['Edge33']; FinalDepth=5.0."
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0

        # profile.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        # The third point is dummy point until bug is fixed
        verify_points = ['(12.5, 32.5, 5.0)', '(32.5, 32.5, 5.0)', '(22.5, 32.5, 5.0)']
        len_pts = len(points)
        len_vrfy_pts = len(verify_points)

        # Verify point count
        if len_vrfy_pts > len_pts:
            self.con.PrintMessage("verify_points: {}\n".format(verify_points))
            self.con.PrintMessage("...... points: {}\n".format(points))
        self.assertTrue(len_vrfy_pts > len_pts)  # self.assertFalse(len_vrfy_pts > len_pts)

        # Verify each point.  All should be missing due to bug.
        # When the bug is fixed, the third point in `verify_points` list above will need deleted.
        missing = False
        for vp in verify_points:
            if vp not in points:
                missing = True
                break
        self.assertTrue(missing)

    def test06(self):
        '''Profile loop of four vertical faces: ['Face12', 'Face13', 'Face14', 'Face15']; FinalDepth=5.0.'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r
        subs_list = ['Face12', 'Face13', 'Face14', 'Face15']

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test06_"
        profile.Comment = "Profile loop of four vertical faces: ['Face12', 'Face13', 'Face14', 'Face15']; FinalDepth=5.0."
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0

        # profile.recompute()
        self.doc.recompute()
        
        # Identify points on inner linear profile paths
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        # Points to verify in path commands
        verify_points = ['(17.5, 17.5, 5.0)', '(17.5, 32.5, 5.0)',
                         '(32.5, 17.5, 5.0)', '(32.5, 32.5, 5.0)']
        self._verifyPointData("test06", points, verify_points)

    def test07(self):
        '''Profile two top continuous linear open edges with offset: ['Edge15', 'Edge21']; FinalDepth=5.0; offsetExtra=3.0. \
        This profile WILL FAIL until the bug is fixed.'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r  # Refined version of Fusion
        subs_list = ["Edge15", "Edge21"]

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test07_"
        profile.Comment = "Profile two top continuous linear open edges with offset: ['Edge15', 'Edge21']; FinalDepth=5.0; offsetExtra=3.0. \
            This profile WILL FAIL until the bug is fixed."
        self._set_depths_and_heights(profile)

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set Operation
        profile.FinalDepth.Value = 5.0
        profile.OffsetExtra.Value = 3.0

        # profile.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(10.0, 4.5, 5.0)', '(40.0, 4.5, 5.0)',
                         '(45.5, 10.0, 5.0)', '(45.5, 60.0, 5.0)']

        len_pts = len(points)
        len_vrfy_pts = len(verify_points)

        # Verify point count
        if len_vrfy_pts > len_pts:
            self.con.PrintMessage("verify_points: {}\n".format(verify_points))
            self.con.PrintMessage("...... points: {}\n".format(points))
        self.assertFalse(len_vrfy_pts > len_pts)

        # Verify each point.  Some should be missing due to bug.
        missing = False
        for vp in verify_points:
            if vp not in points:
                missing = True
                break
        self.assertTrue(missing)

    def test08(self):
        '''Profile single top open edge: ['Edge15']; default settings. \
        This profile SHOULD FAIL because open edges require top-edge selection and manual Final Depth.'''

        # Identify base feature to be used for operation's Base Geometry
        base = self.doc.Fuse0r  # Refined version of Fusion
        subs_list = ['Edge15']

        # Instantiate a Profile operation
        profile = PathProfile.Create('Profile')
        profile.Label = "test08_"
        profile.Comment = "Profile single bottom linear open edge: ['Edge15']; default settings. \
            This profile SHOULD FAIL because open edges require top-edge selection and manual Final Depth."

        # Set Base Geometry
        profile.Base = (base, subs_list)

        # Set other operation properties pertaining to test
        # self._set_depths_and_heights(profile)
        # profile.FinalDepth.Value = 5.0

        # profile.recompute()
        self.doc.recompute()
        
        pnts = dict()
        for c in profile.Path.Commands:
            p = c.Parameters
            if c.Name == "G1":  # or c.Name == "G2" or c.Name == "G3":
                if p.get("X") and p.get("Y") and p.get("Z"):
                    pnts[self._format_point(p)] = True
        points = sorted(pnts)
        # self.con.PrintMessage("points: {}\n".format(points))

        verify_points = ['(10.0, 7.5, 0.0)', '(40.0, 7.5, 0.0)']
        len_pts = len(points)
        len_vrfy_pts = len(verify_points)

        # Verify point count
        self.con.PrintMessage("verify_points: {}\n".format(verify_points))
        self.con.PrintMessage("...... points: {}\n".format(points))
        self.assertTrue(len_vrfy_pts > len_pts)  # because only two points should be correct if profile were to be correct

        # Verify each point.  All should be missing due to unmet open-edge settings.
        missing = False
        for vp in verify_points:
            if vp not in points:
                missing = True
                break
        self.assertTrue(missing)

    # Support methods
    def _set_depths_and_heights(self, op):
        """_set_depths_and_heights(self, op)... Sets default depths and heights for `op` passed to it"""

        # Set start and final depth in order to eliminate effects of stock (and its default values)
        op.setExpression('StartDepth', None)
        op.StartDepth.Value = 15.0
        op.setExpression('FinalDepth', None)
        op.FinalDepth.Value = 0.0

        # Set step down so as to only produce one layer path
        op.setExpression('StepDown', None)
        op.StepDown.Value = 20.0

        # Set Heights
        # default values used
        pass

    def _format_point(self, cmd):
        """_format_point(cmd)... Accepts command dict and returns point string coordinate"""
        x = round(cmd["X"], 2)
        y = round(cmd["Y"], 2)
        z = round(cmd["Z"], 2)
        return "({}, {}, {})".format(x, y, z)

    def _verifyPointData(self, testName, points, verify_points):
        """_verifyPointData(testName, points, verify_points)...
        General method to compare point data of path with verified points."""

        len_pts = len(points)
        len_vrfy_pts = len(verify_points)

        # Verify point count
        if len_vrfy_pts > len_pts:
            self.con.PrintMessage("verify_points: {}\n".format(verify_points))
            self.con.PrintMessage("...... points: {}\n".format(points))
        self.assertFalse(len_vrfy_pts > len_pts)

        # Verify each point, excluding arcs
        for vp in verify_points:
            if vp not in points:
                self.con.PrintMessage("verify_points: {}\n".format(verify_points))
                self.con.PrintMessage("...... points: {}\n".format(points))
                self.assertEqual(vp, "(-10.0, -10.0, -10.0)")
