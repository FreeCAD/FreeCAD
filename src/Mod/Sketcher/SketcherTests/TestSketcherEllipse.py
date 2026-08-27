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
