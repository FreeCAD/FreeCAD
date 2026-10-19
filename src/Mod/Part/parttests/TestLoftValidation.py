import unittest
import FreeCAD as App
import Part


class LoftValidationTest(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("LoftValidation")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def _section(self, shape):
        obj = self.doc.addObject("Part::Feature")
        obj.Shape = shape
        return obj

    def testSolidLoftFromOpenSectionsReportsError(self):
        # two OPEN wires + Solid=True -> cannot form a solid -> must error
        s1 = self._section(Part.makePolygon([App.Vector(0, 0, 0),  App.Vector(10, 0, 0)]))
        s2 = self._section(Part.makePolygon([App.Vector(0, 0, 10), App.Vector(10, 0, 10)]))
        loft = self.doc.addObject("Part::Loft", "Loft")
        loft.Sections = [s1, s2]
        loft.Solid = True
        self.doc.recompute()
        self.assertFalse(loft.isValid())           # execute() errored -> overlay shown

    def testSolidLoftFromClosedSectionsStaysValid(self):
        s1 = self._section(Part.Wire(Part.makeCircle(5, App.Vector(0, 0, 0)).Edges))
        s2 = self._section(Part.Wire(Part.makeCircle(5, App.Vector(0, 0, 10)).Edges))
        loft = self.doc.addObject("Part::Loft", "Loft")
        loft.Sections = [s1, s2]
        loft.Solid = True
        self.doc.recompute()
        self.assertTrue(loft.isValid())
        self.assertTrue(loft.Shape.Solids)         # genuinely produced a solid

    def testOpenSectionsAsSurfaceStillValid(self):
        # regression guard: Solid=False must NOT be rejected
        s1 = self._section(Part.makePolygon([App.Vector(0, 0, 0),  App.Vector(10, 0, 0)]))
        s2 = self._section(Part.makePolygon([App.Vector(0, 0, 10), App.Vector(10, 0, 10)]))
        loft = self.doc.addObject("Part::Loft", "Loft")
        loft.Sections = [s1, s2]
        loft.Solid = False
        self.doc.recompute()
        self.assertTrue(loft.isValid())


if __name__ == "__main__":
    unittest.main()