#**************************************************************************
#   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************

import FreeCAD, unittest, Part
import copy
import math
from FreeCAD import Units
from FreeCAD import Base
App = FreeCAD

from parttests.Geom2d_tests import Geom2dTests
from parttests.regression_tests import RegressionTests
from parttests.TopoShapeListTest import TopoShapeListTest

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------
def getCoincidentVertexes(vtx1, vtx2):
    pairs = []
    tol = Part.Precision.confusion()
    for i in vtx1:
        for j in vtx2:
            if i.Point.distanceToPoint(j.Point) < tol:
                pairs.append((i, j))

    return pairs


class PartTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartTest")

    def testBoxCase(self):
        self.Box = self.Doc.addObject("Part::Box","Box")
        self.Doc.recompute()
        self.assertEqual(len(self.Box.Shape.Faces), 6)

    def testIssue2985(self):
        v1 = App.Vector(0.0,0.0,0.0)
        v2 = App.Vector(10.0,0.0,0.0)
        v3 = App.Vector(10.0,0.0,10.0)
        v4 = App.Vector(0.0,0.0,10.0)
        edge1 = Part.makeLine(v1, v2)
        edge2 = Part.makeLine(v2, v3)
        edge3 = Part.makeLine(v3, v4)
        edge4 = Part.makeLine(v4, v1)
        # Travis build confirms the crash under macOS
        #result = Part.makeFilledFace([edge1,edge2,edge3,edge4])
        #self.Doc.addObject("Part::Feature","Face").Shape = result
        #self.assertTrue(isinstance(result.Surface, Part.BSplineSurface))

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartTest")
        #print ("omit closing document for debugging")

class PartTestBSplineCurve(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartTest")

        poles = [[0, 0, 0], [1, 1, 0], [2, 0, 0]]
        self.spline = Part.BSplineCurve()
        self.spline.buildFromPoles(poles)

        poles = [[0, 0, 0], [1, 1, 0], [2, 0, 0], [1, -1, 0]]
        self.nurbs = Part.BSplineCurve()
        self.nurbs.buildFromPolesMultsKnots(poles, (3, 1, 3),(0, 0.5, 1), False, 2)

    def testProperties(self):
        self.assertEqual(self.spline.Continuity, 'CN')
        self.assertEqual(self.spline.Degree, 2)
        self.assertEqual(self.spline.EndPoint, App.Vector(2, 0, 0))
        self.assertEqual(self.spline.FirstParameter, 0.0)
        self.assertEqual(self.spline.FirstUKnotIndex, 1)
        self.assertEqual(self.spline.KnotSequence, [0.0, 0.0, 0.0, 1.0, 1.0, 1.0])
        self.assertEqual(self.spline.LastParameter, 1.0)
        self.assertEqual(self.spline.LastUKnotIndex, 2)
        max_degree = self.spline.MaxDegree
        self.assertEqual(self.spline.NbKnots, 2)
        self.assertEqual(self.spline.NbPoles, 3)
        self.assertEqual(self.spline.StartPoint, App.Vector(0.0, 0.0, 0.0))

    def testGetters(self):
        '''only check if the function doesn't crash'''
        self.spline.getKnot(1)
        self.spline.getKnots()
        self.spline.getMultiplicities()
        self.spline.getMultiplicity(1)
        self.spline.getPole(1)
        self.spline.getPoles()
        self.spline.getPolesAndWeights()
        self.spline.getResolution(0.5)
        self.spline.getWeight(1)
        self.spline.getWeights()

    def testSetters(self):
        spline = copy.copy(self.spline)
        spline.setKnot(1, 0.1)
        spline.setPeriodic()
        spline.setNotPeriodic()
        # spline.setKnots()
        # spline.setOrigin(2)   # not working?
        self.spline.setPole(1, App.Vector([1, 0, 0])) # first parameter 0 gives occ error

    def testIssue2671(self):
        self.Doc = App.newDocument("Issue2671")
        Box = self.Doc.addObject("Part::Box","Box")
        Mirroring = self.Doc.addObject("Part::Mirroring", 'Mirroring')
        Spreadsheet = self.Doc.addObject('Spreadsheet::Sheet', 'Spreadsheet')
        Mirroring.Source = Box
        Mirroring.Base = (8, 5, 25)
        Mirroring.Normal = (0.5, 0.2, 0.9)
        Spreadsheet.set('A1', '=Mirroring.Base.x')
        Spreadsheet.set('B1', '=Mirroring.Base.y')
        Spreadsheet.set('C1', '=Mirroring.Base.z')
        Spreadsheet.set('A2', '=Mirroring.Normal.x')
        Spreadsheet.set('B2', '=Mirroring.Normal.y')
        Spreadsheet.set('C2', '=Mirroring.Normal.z')
        self.Doc.recompute()
        self.assertEqual(Spreadsheet.A1, Units.Quantity('8 mm'))
        self.assertEqual(Spreadsheet.B1, Units.Quantity('5 mm'))
        self.assertEqual(Spreadsheet.C1, Units.Quantity('25 mm'))
        self.assertEqual(Spreadsheet.A2, Units.Quantity('0.5 mm'))
        self.assertEqual(Spreadsheet.B2, Units.Quantity('0.2 mm'))
        self.assertEqual(Spreadsheet.C2, Units.Quantity('0.9 mm'))
        App.closeDocument("Issue2671")

    def testIssue2876(self):
        self.Doc = App.newDocument("Issue2876")
        Cylinder = self.Doc.addObject("Part::Cylinder", "Cylinder")
        Cylinder.Radius = 5
        Pipe = self.Doc.addObject("Part::Thickness", "Pipe")
        Pipe.Faces = (Cylinder, ["Face2", "Face3"])
        Pipe.Mode = 1
        Pipe.Value = -1 # negative wall thickness
        Spreadsheet = self.Doc.addObject('Spreadsheet::Sheet', 'Spreadsheet')
        Spreadsheet.set('A1', 'Pipe OD')
        Spreadsheet.set('B1', 'Pipe WT')
        Spreadsheet.set('C1', 'Pipe ID')
        Spreadsheet.set('A2', '=2*Cylinder.Radius')
        Spreadsheet.set('B2', '=-Pipe.Value')
        Spreadsheet.set('C2', '=2*(Cylinder.Radius + Pipe.Value)')
        self.Doc.recompute()
        self.assertEqual(Spreadsheet.B2, Units.Quantity('1 mm'))
        self.assertEqual(Spreadsheet.C2, Units.Quantity('8 mm'))
        App.closeDocument("Issue2876")

    def testSubElements(self):
        box = Part.makeBox(1, 1, 1)
        with self.assertRaises(ValueError):
            box.getElement("InvalidName")
        with self.assertRaises(ValueError):
            box.getElement("Face6_abc")
        # getSubTopoShape now catches this before it gets to OCC, so the error changes:
        # with self.assertRaises(Part.OCCError):
        with self.assertRaises(IndexError):
            box.getElement("Face7")

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartTest")

class PartTestCurveToNurbs(unittest.TestCase):
    def testCircleToNurbs(self):
        mat = Base.Matrix()
        mat.rotateX(1)
        mat.rotateY(1)
        mat.rotateZ(1)

        circle = Part.Circle()
        circle.Radius = 5

        circle.transform(mat)
        nurbs = circle.toNurbs()
        self.assertEqual(circle.value(0), nurbs.value(0))

        arc = circle.trim(0, 2)
        nurbs = arc.toNurbs()
        self.assertEqual(circle.value(0), nurbs.value(0))

        spline = circle.toBSpline()
        self.assertAlmostEqual(circle.value(0).distanceToPoint(spline.value(0)), 0)

    def testEllipseToNurbs(self):
        mat = Base.Matrix()
        mat.rotateX(1)
        mat.rotateY(1)
        mat.rotateZ(1)

        ellipse = Part.Ellipse()
        ellipse.MajorRadius = 5
        ellipse.MinorRadius = 3

        ellipse.transform(mat)
        nurbs = ellipse.toNurbs()
        self.assertEqual(ellipse.value(0), nurbs.value(0))

        arc = ellipse.trim(0, 2)
        nurbs = arc.toNurbs()
        self.assertEqual(ellipse.value(0), nurbs.value(0))

        spline = ellipse.toBSpline()
        self.assertAlmostEqual(ellipse.value(0).distanceToPoint(spline.value(0)), 0)

class PartTestBSplineSurface(unittest.TestCase):
    def testTorusToSpline(self):
        to = Part.Toroid()
        bs = to.toBSpline()
        bs.setUPeriodic()
        bs.setVPeriodic()
        self.assertGreater(len(bs.UKnotSequence), 0)
        self.assertGreater(len(bs.VKnotSequence), 0)

    def testBounds(self):
        to = Part.Toroid()
        bs = to.toBSpline()
        self.assertAlmostEqual(bs.bounds()[1], 2 * math.pi)
        self.assertAlmostEqual(bs.bounds()[3], 2 * math.pi)
        bs.scaleKnotsToBounds(0.0, 1.0, 0.0, 1.0)
        self.assertAlmostEqual(bs.bounds()[1], 1.0)
        self.assertAlmostEqual(bs.bounds()[3], 1.0)

class PartTestNormals(unittest.TestCase):
    def setUp(self):
        self.face = Part.makePlane(1, 1)

    def testFaceNormal(self):
        self.assertEqual(self.face.normalAt(0, 0), Base.Vector(0, 0, 1))
        self.assertEqual(self.face.Surface.normal(0, 0), Base.Vector(0, 0, 1))

    def testReverseOrientation(self):
        self.face.reverse()
        self.assertEqual(self.face.normalAt(0, 0), Base.Vector(0, 0, -1))
        self.assertEqual(self.face.Surface.normal(0, 0), Base.Vector(0, 0, 1))

    def testPlacement(self):
        self.face.reverse()
        self.face.Placement.Rotation.Angle = 1
        self.face.Placement.Rotation.Axis = (1,1,1)
        vec = Base.Vector(-0.63905, 0.33259, -0.69353)
        self.assertGreater(self.face.normalAt(0, 0).dot(vec), 0.9999)
        self.assertLess(self.face.Surface.normal(0, 0).dot(vec), -0.9999)

    def tearDown(self):
        pass

class PartTestShapeRotate(unittest.TestCase):
    def testPlacement(self):
        tol = 1e-12

        box = Part.makeBox(1, 1, 1)
        box.Placement.Base = Base.Vector(10, 10, 10)
        box.rotate((0, 0, 0), (0, 0, 1), 90)

        p1 = Base.Placement()
        p1.Base = Base.Vector(10, 10, 10)

        p2 = Base.Placement()
        p2.Rotation.Angle = math.radians(90)
        self.assertTrue(box.Placement.isSame(p2 * p1, tol))

        p3 = p1.copy()
        p3.rotate((0, 0, 0), (0, 0, 1), 90)
        self.assertTrue(p3.isSame(p1 * p2, tol))
        self.assertFalse(box.Placement.isSame(p3, tol))

        p4 = p1.copy()
        p4.rotate((0, 0, 0), (0, 0, 1), 90, True)
        self.assertTrue(p4.isSame(p2 * p1, tol))
        self.assertTrue(box.Placement.isSame(p4, tol))

class PartTestCircle2D(unittest.TestCase):
    def testValidCircle(self):
        p1 = App.Base.Vector2d(0.01, 0.01)
        p2 = App.Base.Vector2d(0.02, 0.02)
        p3 = App.Base.Vector2d(0.01, -0.01)
        Part.Geom2d.Circle2d.getCircleCenter(p1, p2, p3)

    def testCollinearPoints(self):
        p1 = App.Base.Vector2d(0.01, 0.01)
        p2 = App.Base.Vector2d(0.02, 0.02)
        p3 = App.Base.Vector2d(0.04, 0.0399)
        with self.assertRaises(ValueError):
            Part.Geom2d.Circle2d.getCircleCenter(p1, p2, p3)

class PartTestCone(unittest.TestCase):
    def testderivatives(self):
        def get_dn(surface, u, v):
            pos = surface.value(u, v)
            v10 = surface.getDN(u, v, 1, 0)
            v01 = surface.getDN(u, v, 0, 1)
            v11 = surface.getDN(u, v, 1, 1)
            return (pos, v10, v01, v11)

        cone = Part.Cone()
        cone.SemiAngle = 0.2
        cone.Radius = 2.0

        u, v = (5.0, 5.0)
        vp, v1, v2, v3 = get_dn(cone, u, v)

        shape = cone.toShape(0, 2*math.pi, 0, 10)
        shape = shape.toNurbs()
        spline = shape.Face1.Surface

        u, v = spline.parameter(vp)
        wp, w1, w2, w3 = get_dn(spline, u, v)

        self.assertAlmostEqual(vp.distanceToPoint(wp), 0)
        self.assertAlmostEqual(v1.getAngle(w1), 0)
        self.assertAlmostEqual(v2.getAngle(w2), 0)
        self.assertAlmostEqual(v3.getAngle(w3), 0)

class PartTestChFi2dAlgos(unittest.TestCase):
    def testChFi2d_FilletAlgo(self):
        v = FreeCAD.Vector
        edge1 = Part.makeLine(v(0,0,0), v(0,10,0))
        edge2 = Part.makeLine(v(0,10,0), v(10,10,0))
        wire = Part.Wire([edge1, edge2])
        pln = Part.Plane()

        with self.assertRaises(TypeError):
            alg = Part.ChFi2d.FilletAlgo(pln)

        alg = Part.ChFi2d.FilletAlgo()
        with self.assertRaises(TypeError):
            alg.init()

        print (alg)
        # Test without shape
        with self.assertRaises(Base.CADKernelError):
            alg.perform(1)

        with self.assertRaises(TypeError):
            alg.perform()

        alg = Part.ChFi2d.FilletAlgo(wire, pln)
        alg.init(edge1, edge2, pln)
        alg.init(wire, pln)

        alg = Part.ChFi2d.FilletAlgo(edge1, edge2, pln)
        alg.perform(1.0)

        with self.assertRaises(TypeError):
            alg.numberOfResults()

        with self.assertRaises(TypeError):
            alg.result(1)

        self.assertEqual(alg.numberOfResults(Base.Vector(0,10,0)), 1)
        result = alg.result(Base.Vector(0,10,0))
        curve = result[0].Curve
        self.assertEqual(type(curve), Part.Circle)
        self.assertEqual(curve.Axis, pln.Axis)
        self.assertEqual(curve.Radius, 1.0)

    def testChFi2d_AnaFilletAlgo(self):
        v = FreeCAD.Vector
        edge1 = Part.makeLine(v(0,0,0), v(0,10,0))
        edge2 = Part.makeLine(v(0,10,0), v(10,10,0))
        wire = Part.Wire([edge1, edge2])
        pln = Part.Plane()

        with self.assertRaises(TypeError):
            alg = Part.ChFi2d.AnaFilletAlgo(pln)

        alg = Part.ChFi2d.AnaFilletAlgo()
        with self.assertRaises(TypeError):
            alg.init()

        print (alg)
        # Test without shape
        self.assertFalse(alg.perform(1))

        with self.assertRaises(TypeError):
            alg.perform()

        alg = Part.ChFi2d.AnaFilletAlgo(wire, pln)
        alg.init(edge1, edge2, pln)
        alg.init(wire, pln)

        alg = Part.ChFi2d.AnaFilletAlgo(edge1, edge2, pln)
        alg.perform(1.0)

        with self.assertRaises(TypeError):
            alg.result(1)

        result = alg.result()
        curve = result[0].Curve
        self.assertEqual(type(curve), Part.Circle)
        self.assertEqual(curve.Radius, 1.0)

    def testChFi2d_ChamferAPI(self):
        v = FreeCAD.Vector
        edge1 = Part.makeLine(v(0,0,0), v(0,10,0))
        edge2 = Part.makeLine(v(0,10,0), v(10,10,0))
        wire = Part.Wire([edge1, edge2])

        with self.assertRaises(TypeError):
            alg = Part.ChFi2d.ChamferAPI(edge1)

        alg = Part.ChFi2d.ChamferAPI(wire)
        with self.assertRaises(TypeError):
            alg.init()

        print (alg)

        with self.assertRaises(TypeError):
            alg.perform(1)

        alg = Part.ChFi2d.ChamferAPI(wire)
        alg.init(edge1, edge2)
        alg.init(wire)

        alg = Part.ChFi2d.ChamferAPI(edge1, edge2)
        alg.perform()

        with self.assertRaises(TypeError):
            alg.result(1)

        result = alg.result(1.0, 1.0)
        curve = result[0].Curve
        self.assertEqual(type(curve), Part.Line)

class PartTestRuledSurface(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument()

    def testRuledSurfaceFromTwoObjects(self):
        line1 = Part.makeLine(FreeCAD.Vector(-70,-30,0), FreeCAD.Vector(-50,40,0))
        line2 = Part.makeLine(FreeCAD.Vector(-40,-30,0), FreeCAD.Vector(-40,10,0))
        plm1 = FreeCAD.Placement()
        plm1.Rotation = FreeCAD.Rotation(0.7071067811865476, 0.0, 0.0, 0.7071067811865475)
        line1.Placement = plm1
        fea1 = self.Doc.addObject("Part::Feature")
        fea2 = self.Doc.addObject("Part::Feature")
        fea1.Shape = line1
        fea2.Shape = line2
        ruled = self.Doc.addObject("Part::RuledSurface")
        ruled.Curve1 = fea1
        ruled.Curve2 = fea2

        self.Doc.recompute()

        same1 = getCoincidentVertexes(fea1.Shape.Vertexes, ruled.Shape.Vertexes)
        same2 = getCoincidentVertexes(fea2.Shape.Vertexes, ruled.Shape.Vertexes)
        self.assertEqual(len(same1), 2)
        self.assertEqual(len(same2), 2)

    def testRuledSurfaceFromOneObjects(self):
        sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        sketch.Placement = FreeCAD.Placement(FreeCAD.Vector(0.000000, 0.000000, 0.000000), App.Rotation(0.707107, 0.000000, 0.000000, 0.707107))
        sketch.MapMode = "Deactivated"

        sketch.addGeometry(Part.LineSegment(App.Vector(-43.475811,34.364464,0),App.Vector(-65.860519,-20.078733,0)),False)
        sketch.addGeometry(Part.LineSegment(App.Vector(14.004498,27.390331,0),App.Vector(33.577049,-27.952749,0)),False)

        ruled = self.Doc.addObject('Part::RuledSurface', 'Ruled Surface')
        ruled.Curve1 = (sketch,['Edge1'])
        ruled.Curve2 = (sketch,['Edge2'])
        self.Doc.recompute()

        same = getCoincidentVertexes(sketch.Shape.Vertexes, ruled.Shape.Vertexes)
        self.assertEqual(len(same), 4)

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

class PartTestShapeFix(unittest.TestCase):
    def testShapeFix_Root(self):
        with self.assertRaises(TypeError):
            Part.ShapeFix.Root([])

        fix = Part.ShapeFix.Root()
        print (fix)

        fix.Precision = 0.0
        self.assertEqual(fix.Precision, 0.0)

        fix.MinTolerance = 0.0
        self.assertEqual(fix.MinTolerance, 0.0)

        fix.MaxTolerance = 0.5
        self.assertEqual(fix.MaxTolerance, 0.5)

        self.assertEqual(fix.limitTolerance(0.25), 0.25)

    def testShapeFix_Shape(self):
        surface = Part.Plane()
        face = surface.toShape(-1, 1, -1, 1)

        with self.assertRaises(TypeError):
            Part.ShapeFix.Shape([])

        fix = Part.ShapeFix.Shape(face)
        fix.init(face)
        print (fix)
        fix.shape()
        fix.fixSolidTool()
        fix.fixShellTool()
        fix.fixFaceTool()
        fix.fixWireTool()
        fix.fixEdgeTool()

        fix.FixSolidMode = True
        self.assertEqual(fix.FixSolidMode, True)

        fix.FixFreeShellMode = True
        self.assertEqual(fix.FixFreeShellMode, True)

        fix.FixFreeFaceMode = True
        self.assertEqual(fix.FixFreeFaceMode, True)

        fix.FixFreeWireMode = True
        self.assertEqual(fix.FixFreeWireMode, True)

        fix.FixSameParameterMode = True
        self.assertEqual(fix.FixSameParameterMode, True)

        fix.FixVertexPositionMode = True
        self.assertEqual(fix.FixVertexPositionMode, True)

        fix.FixVertexTolMode = True
        self.assertEqual(fix.FixVertexTolMode, True)

        fix.perform()

    def testShapeFix_Edge(self):
        surface = Part.Plane()
        face = surface.toShape(-1, 1, -1, 1)

        with self.assertRaises(TypeError):
            Part.ShapeFix.Edge([])

        wirefix = Part.ShapeFix.Wire(face.OuterWire, face, 1e-7)
        fix = wirefix.fixEdgeTool()
        print (fix)

        fix.fixRemovePCurve(face.Edge1, face)
        fix.fixRemovePCurve(face.Edge1, face.Surface, face.Placement)
        with self.assertRaises(TypeError):
            fix.fixRemovePCurve(face)

        fix.fixRemoveCurve3d(face.Edge1)
        fix.fixAddCurve3d(face.Edge1)

        fix.fixAddPCurve(face.Edge1, face, False)
        fix.fixAddPCurve(face.Edge1, face.Surface, face.Placement, False)
        with self.assertRaises(TypeError):
            fix.fixAddPCurve(face)

        fix.fixVertexTolerance(face.Edge1)
        fix.fixVertexTolerance(face.Edge1, face)

        fix.fixReversed2d(face.Edge1, face)
        fix.fixReversed2d(face.Edge1, face.Surface, face.Placement)
        with self.assertRaises(TypeError):
            fix.fixReversed2d(face)

        fix.fixSameParameter(face.Edge1)
        fix.fixSameParameter(face.Edge1, face)
        with self.assertRaises(TypeError):
            fix.fixSameParameter(face)

    def testShapeFix_Face(self):
        surface = Part.Plane()
        face = surface.toShape(-1, 1, -1, 1)

        Part.ShapeFix.Face()
        Part.ShapeFix.Face(surface, 0.00001, True)
        with self.assertRaises(TypeError):
            Part.ShapeFix.Face([])

        fix = Part.ShapeFix.Face(face)
        print (fix)

        fix.fixOrientation()
        fix.fixAddNaturalBound()
        fix.fixMissingSeam()
        fix.fixSmallAreaWire(True)
        fix.fixLoopWire()
        fix.fixIntersectingWires()
        fix.fixWiresTwoCoincidentEdges()
        fix.fixPeriodicDegenerated()
        fix.perform()

        fix.add(face.OuterWire)
        current = fix.face()
        result = fix.result()
        fix.fixWireTool()

        fix.FixWireMode = True
        self.assertEqual(fix.FixWireMode, True)

        fix.FixOrientationMode = True
        self.assertEqual(fix.FixOrientationMode, True)

        fix.FixAddNaturalBoundMode = True
        self.assertEqual(fix.FixAddNaturalBoundMode, True)

        fix.FixMissingSeamMode = True
        self.assertEqual(fix.FixMissingSeamMode, True)

        fix.FixSmallAreaWireMode = True
        self.assertEqual(fix.FixSmallAreaWireMode, True)

        fix.RemoveSmallAreaFaceMode = True
        self.assertEqual(fix.RemoveSmallAreaFaceMode, True)

        fix.FixIntersectingWiresMode = True
        self.assertEqual(fix.FixIntersectingWiresMode, True)

        fix.FixLoopWiresMode = True
        self.assertEqual(fix.FixLoopWiresMode, True)

        fix.FixSplitFaceMode = True
        self.assertEqual(fix.FixSplitFaceMode, True)

        fix.AutoCorrectPrecisionMode = True
        self.assertEqual(fix.AutoCorrectPrecisionMode, True)

        fix.FixPeriodicDegeneratedMode = True
        self.assertEqual(fix.FixPeriodicDegeneratedMode, True)

        fix.clearModes()

    def testShapeFix_Shell(self):
        surface = Part.Plane()
        face = surface.toShape(-1, 1, -1, 1)
        shell = Part.Shell([face])

        Part.ShapeFix.Shell()
        with self.assertRaises(TypeError):
            Part.ShapeFix.Face([])

        fix = Part.ShapeFix.Shell(shell)
        fix.init(shell)
        print (fix)
        fix.perform()
        fix.shell()
        fix.shape()
        fix.fixFaceTool()

        fix.setNonManifoldFlag(True)
        fix.fixFaceOrientation(shell)

        self.assertEqual(len(fix.errorFaces().Faces), 0)

        self.assertEqual(fix.numberOfShells(), 1)

        fix.FixFaceMode = True
        self.assertEqual(fix.FixFaceMode, True)

        fix.FixOrientationMode = True
        self.assertEqual(fix.FixOrientationMode, True)

    def testShapeFix_Solid(self):
        box = Part.makeBox(1, 1, 1)
        with self.assertRaises(TypeError):
            Part.ShapeFix.Solid([])

        fix = Part.ShapeFix.Solid()
        fix.init(box)
        print (fix)

        fix.perform()
        fix.solid()
        fix.shape()
        fix.fixShellTool()
        fix.solidFromShell(box.Shells[0])

        fix.FixShellMode = True
        self.assertEqual(fix.FixShellMode, True)

        fix.FixShellOrientationMode = True
        self.assertEqual(fix.FixShellOrientationMode, True)

        fix.CreateOpenSolidMode = True
        self.assertEqual(fix.CreateOpenSolidMode, True)

    def testShapeFix_Wire(self):
        with self.assertRaises(TypeError):
            Part.ShapeFix.Wire([])

        surface = Part.Plane()
        face = surface.toShape(-1, 1, -1, 1)
        Part.ShapeFix.Wire(face.OuterWire, face, 1e-7)
        fix = Part.ShapeFix.Wire()
        fix.init(face.OuterWire, face, 1e-7)
        fix.load(face.OuterWire)
        fix.setSurface(surface)
        fix.setSurface(surface, face.Placement)
        fix.setFace(face)
        fix.setMaxTailAngle(math.pi)
        fix.setMaxTailWidth(10.0)
        fix.fixEdgeTool()

        self.assertEqual(fix.isLoaded(), True)
        self.assertEqual(fix.isReady(), True)
        self.assertEqual(fix.numberOfEdges(), 4)

        print (fix)
        fix.clearModes()
        fix.clearStatuses()

        fix.wire()
        fix.wireAPIMake()
        fix.face()

        fix.ModifyTopologyMode = True
        self.assertEqual(fix.ModifyTopologyMode, True)

        fix.ModifyGeometryMode = True
        self.assertEqual(fix.ModifyGeometryMode, True)

        fix.ModifyRemoveLoopMode = True
        self.assertEqual(fix.ModifyRemoveLoopMode, True)

        fix.ClosedWireMode = True
        self.assertEqual(fix.ClosedWireMode, True)

        fix.PreferencePCurveMode = True
        self.assertEqual(fix.PreferencePCurveMode, True)

        fix.FixGapsByRangesMode = True
        self.assertEqual(fix.FixGapsByRangesMode, True)

        fix.FixReorderMode = True
        self.assertEqual(fix.FixReorderMode, True)

        fix.FixSmallMode = True
        self.assertEqual(fix.FixSmallMode, True)

        fix.FixConnectedMode = True
        self.assertEqual(fix.FixConnectedMode, True)

        fix.FixEdgeCurvesMode = True
        self.assertEqual(fix.FixEdgeCurvesMode, True)

        fix.FixDegeneratedMode = True
        self.assertEqual(fix.FixDegeneratedMode, True)

        fix.FixSelfIntersectionMode = True
        self.assertEqual(fix.FixSelfIntersectionMode, True)

        fix.FixLackingMode = True
        self.assertEqual(fix.FixLackingMode, True)

        fix.FixGaps3dMode = True
        self.assertEqual(fix.FixGaps3dMode, True)

        fix.FixGaps2dMode = True
        self.assertEqual(fix.FixGaps2dMode, True)

        fix.FixReversed2dMode = True
        self.assertEqual(fix.FixReversed2dMode, True)

        fix.FixRemovePCurveMode = True
        self.assertEqual(fix.FixRemovePCurveMode, True)

        fix.FixAddPCurveMode = True
        self.assertEqual(fix.FixAddPCurveMode, True)

        fix.FixRemoveCurve3dMode = True
        self.assertEqual(fix.FixRemoveCurve3dMode, True)

        fix.FixAddCurve3dMode = True
        self.assertEqual(fix.FixAddCurve3dMode, True)

        fix.FixSeamMode = True
        self.assertEqual(fix.FixSeamMode, True)

        fix.FixShiftedMode = True
        self.assertEqual(fix.FixShiftedMode, True)

        fix.FixSameParameterMode = True
        self.assertEqual(fix.FixSameParameterMode, True)

        fix.FixVertexToleranceMode = True
        self.assertEqual(fix.FixVertexToleranceMode, True)

        fix.FixNotchedEdgesMode = True
        self.assertEqual(fix.FixNotchedEdgesMode, True)

        fix.FixSelfIntersectingEdgeMode = True
        self.assertEqual(fix.FixSelfIntersectingEdgeMode, True)

        fix.FixIntersectingEdgesMode = True
        self.assertEqual(fix.FixIntersectingEdgesMode, True)

        fix.FixNonAdjacentIntersectingEdgesMode = True
        self.assertEqual(fix.FixNonAdjacentIntersectingEdgesMode, True)

        fix.FixTailMode = True
        self.assertEqual(fix.FixTailMode, True)

        fix.perform()
        fix.fixReorder()
        fix.fixSmall(True)
        fix.fixSmall(1, True, 1e-7)
        fix.fixConnected()
        fix.fixConnected(1, True)
        fix.fixEdgeCurves()
        fix.fixDegenerated()
        fix.fixDegenerated(1)
        fix.fixSelfIntersection()
        fix.fixLacking()
        fix.fixLacking(1, False)
        fix.fixClosed()
        fix.fixGaps3d()
        fix.fixGaps2d()
        fix.fixSeam(1)
        fix.fixShifted()
        fix.fixNotchedEdges()
        fix.fixGap3d(1, False)
        fix.fixGap2d(1, False)
        fix.fixTails()

class PartBOPTestContainer(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument()

    def testMakeFuse(self):
        box = self.Doc.addObject("Part::Box", "Box")
        cyl = self.Doc.addObject("Part::Cylinder", "Cylinder")
        part = self.Doc.addObject("App::Part", "Part")
        part.addObject(box)
        part.addObject(cyl)
        from BOPTools import BOPFeatures
        bp = BOPFeatures.BOPFeatures(self.Doc)
        fuse = bp.make_multi_fuse([cyl.Name, box.Name])
        self.assertEqual(part, fuse.getParent())

    def testMakeCut(self):
        box = self.Doc.addObject("Part::Box", "Box")
        cyl = self.Doc.addObject("Part::Cylinder", "Cylinder")
        part = self.Doc.addObject("App::Part", "Part")
        part.addObject(box)
        part.addObject(cyl)
        from BOPTools import BOPFeatures
        bp = BOPFeatures.BOPFeatures(self.Doc)
        fuse = bp.make_cut([cyl.Name, box.Name])
        self.assertEqual(part, fuse.getParent())

    def testMakeCommon(self):
        box = self.Doc.addObject("Part::Box", "Box")
        cyl = self.Doc.addObject("Part::Cylinder", "Cylinder")
        part = self.Doc.addObject("App::Part", "Part")
        part.addObject(box)
        part.addObject(cyl)
        from BOPTools import BOPFeatures
        bp = BOPFeatures.BOPFeatures(self.Doc)
        fuse = bp.make_multi_common([cyl.Name, box.Name])
        self.assertEqual(part, fuse.getParent())

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

class BSplineCurve2d(unittest.TestCase):
    def setUp(self):
        vec2 = FreeCAD.Base.Vector2d
        self.pts = [vec2(0, 0), vec2(1, 0)]
        self.bs = Part.Geom2d.BSplineCurve2d()

    def testInterpolate(self):
        self.bs.interpolate(Points=self.pts)

    def testApproximate(self):
        self.bs.approximate(Points=self.pts)

class GeometryCurve(unittest.TestCase):
    def testProject(self):
        line = Part.Line()
        line.projectPoint(FreeCAD.Vector())

class EmptyEdge(unittest.TestCase):
    def testParameterByLength(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.getParameterByLength(0)

    def testValueAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.valueAt(0)

    def testParameters(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            face = Part.Face()
            edge.parameters(face)

    def testParameterAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            vertex = Part.Vertex(0, 0, 0)
            edge.parameterAt(vertex)

    def testTangentAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.tangentAt(0)

    def testCurvatureAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.curvatureAt(0)

    def testCenterOfCurvatureAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.centerOfCurvatureAt(0)

    def testDerivative1At(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.derivative1At(0)

    def testDerivative2At(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.derivative2At(0)

    def testDerivative3At(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.derivative3At(0)

    def testNormalAt(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.normalAt(0)

    def testFirstVertex(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.firstVertex()

    def testLastVertex(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.lastVertex()

    def testGetTolerance(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.Tolerance

    def testSetTolerance(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.Tolerance = 0.01

    def testLength(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.Length

    def testCurve(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.Curve

    def testParameterRange(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.ParameterRange

    def testFirstParameter(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.FirstParameter

    def testLastParameter(self):
        with self.assertRaises(ValueError):
            edge = Part.Edge()
            edge.LastParameter

class EmptyFace(unittest.TestCase):
    def testMakeOffset(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.makeOffset(1)

    def testValueAt(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.valueAt(0, 0)

    def testNormalAt(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.normalAt(0, 0)

    def testTangentAt(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.tangentAt(0, 0)

    def testCurvatureAt(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.curvatureAt(0, 0)

    def testDerivative1At(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.derivative1At(0, 0)

    def testDerivative2At(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.derivative2At(0, 0)

    def testCutHoles(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            circle = Part.Circle()
            wire = Part.Wire(Part.Edge(circle))
            face.cutHoles([wire])

    def testUVNodes(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.getUVNodes()

    def testGetTolerance(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.Tolerance

    def testSetTolerance(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.Tolerance = 0.01

    def testParameterRange(self):
        with self.assertRaises(ValueError):
            face = Part.Face()
            face.ParameterRange
