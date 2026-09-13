import math
import unittest
import FreeCAD
import Part
import Sketcher

App = FreeCAD


# ---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Sketcher module
# ---------------------------------------------------------------------------


class TestSketcherEllipse(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("SketchEllipse")

    def tearDown(self):
        # FreeCAD.closeDocument(self.doc.Name)
        pass

    def addExternalEllipse(self, tilt):
        n = App.Vector(math.sin(math.radians(tilt)), 0, math.cos(math.radians(tilt)))
        plane = Part.makePlane(100, 100)
        plane.Placement = App.Placement(App.Vector(-20, -50), App.Rotation(App.Vector(0, 0, 1), n))
        cylinder = Part.makeCylinder(1.6, 200, App.Vector(0, 0, -100))
        section = cylinder.section(plane)

        obj = self.doc.addObject("Part::Feature", "Section")
        obj.Shape = section

        sk = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sk.addExternal(obj.Name, "Edge1")
        self.doc.recompute()

        geo = sk.ExternalGeo[-1]
        self.assertEqual(type(geo), Part.Circle)

    def testProjectEllipse1(self):
        self.addExternalEllipse(30)

    def testProjectEllipse2(self):
        self.addExternalEllipse(44)

    def testProjectEllipse3(self):
        self.addExternalEllipse(44.9)

    def testProjectEllipse4(self):
        self.addExternalEllipse(45)

    def testProjectEllipse5(self):
        self.addExternalEllipse(45.1)

    def testProjectEllipse6(self):
        self.addExternalEllipse(60)

    def testProjectEllipse7(self):
        ell = self.doc.addObject("Part::Ellipse", "Ellipse")
        ell.Placement.Rotation.setYawPitchRoll(-61.194, -36.5151, -66.0633)
        ell.MinorRadius = 2.0
        self.doc.recompute()

        c = ell.Shape.Edge1.Curve
        angle = math.acos(c.MinorRadius / c.MajorRadius)
        R = App.Rotation(c.YAxis, Radian=angle)
        proj_dir = R * c.Axis

        line = self.doc.addObject("Part::Feature", "Axis")
        line.Shape = Part.makeLine(c.Location, c.Location + 10 * proj_dir)

        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.MapReversed = False
        sketch.AttachmentSupport = [(line, "Edge1")]
        sketch.MapPathParameter = 0.000000
        sketch.MapMode = "NormalToEdge"

        sketch.addExternal(ell.Name, "Edge1")
        self.doc.recompute()

        geo = sketch.ExternalGeo[-1]
        self.assertEqual(type(geo), Part.Circle)
