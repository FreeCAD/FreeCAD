import FreeCAD
import unittest

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawAuxiliaryViewTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDAuxiliary")
        FreeCAD.setActiveDocument("TDAuxiliary")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAuxiliary")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.page = createPageWithSVGTemplate()

        self.base = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "BaseView")
        self.page.addView(self.base)
        self.base.Source = [self.box]
        self.base.Direction = (0.0, -1.0, 0.0)
        self.base.XDirection = (1.0, 0.0, 0.0)
        self.base.Scale = 1.0
        FreeCAD.ActiveDocument.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDAuxiliary")

    def makeAuxiliaryView(self):
        auxiliary = FreeCAD.ActiveDocument.addObject("TechDraw::DrawAuxiliaryView", "Auxiliary")
        self.page.addView(auxiliary)
        auxiliary.BaseView = self.base
        auxiliary.Scale = 1.0
        return auxiliary

    def setReference(self, auxiliary, start, end):
        auxiliary.ReferenceStart = start
        auxiliary.ReferenceEnd = end
        auxiliary.AuxiliaryDirection = end - start

    def assertVectorAlmostEqual(self, actual, expected):
        self.assertAlmostEqual(actual.x, expected.x, places=3)
        self.assertAlmostEqual(actual.y, expected.y, places=3)
        self.assertAlmostEqual(actual.z, expected.z, places=3)

    def testAuxiliaryViewAcrossReferenceDirection(self):
        auxiliary = self.makeAuxiliaryView()
        self.setReference(
            auxiliary,
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Vector(1.0, 0.0, 0.0),
        )
        auxiliary.ProjectionMode = "Across"

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.Direction, FreeCAD.Vector(0.0, 0.0, 1.0))
        self.assertVectorAlmostEqual(auxiliary.XDirection, FreeCAD.Vector(1.0, 0.0, 0.0))
        self.assertEqual(auxiliary.Source, self.base.Source)

    def testAuxiliaryViewCanReverseDirection(self):
        auxiliary = self.makeAuxiliaryView()
        self.setReference(
            auxiliary,
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Vector(1.0, 0.0, 0.0),
        )
        auxiliary.ProjectionMode = "Across"
        auxiliary.ReverseDirection = True

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.Direction, FreeCAD.Vector(0.0, 0.0, -1.0))
        self.assertVectorAlmostEqual(auxiliary.XDirection, FreeCAD.Vector(1.0, 0.0, 0.0))

    def testAuxiliaryViewCanProjectAlongReferenceDirection(self):
        auxiliary = self.makeAuxiliaryView()
        self.setReference(
            auxiliary,
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Vector(1.0, 0.0, 0.0),
        )
        auxiliary.ProjectionMode = "Along"

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.Direction, FreeCAD.Vector(1.0, 0.0, 0.0))
        self.assertVectorAlmostEqual(auxiliary.XDirection, FreeCAD.Vector(0.0, 0.0, 1.0))

    def testAuxiliaryViewUsesDefaultReferenceWhenReferenceDirectionIsZero(self):
        auxiliary = self.makeAuxiliaryView()
        auxiliary.AuxiliaryDirection = FreeCAD.Vector(0.0, 0.0, 0.0)
        auxiliary.ProjectionMode = "Across"

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.AuxiliaryDirection, FreeCAD.Vector(0.0, 0.0, 0.0))
        self.assertVectorAlmostEqual(auxiliary.Direction, FreeCAD.Vector(0.0, 0.0, 1.0))

    def testAuxiliaryViewStoresReferenceMarkerEndpoints(self):
        auxiliary = self.makeAuxiliaryView()
        self.setReference(
            auxiliary,
            FreeCAD.Vector(2.0, 3.0, 0.0),
            FreeCAD.Vector(8.0, 3.0, 0.0),
        )

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.ReferenceStart, FreeCAD.Vector(2.0, 3.0, 0.0))
        self.assertVectorAlmostEqual(auxiliary.ReferenceEnd, FreeCAD.Vector(8.0, 3.0, 0.0))
        self.assertTrue(auxiliary.KeepAligned)

    def testAuxiliaryViewKeepsPageOffsetFromBaseView(self):
        auxiliary = self.makeAuxiliaryView()
        self.base.X = 20.0
        self.base.Y = 30.0
        auxiliary.X = 65.0
        auxiliary.Y = 10.0
        FreeCAD.ActiveDocument.recompute()

        self.base.X = 40.0
        self.base.Y = 50.0
        FreeCAD.ActiveDocument.recompute()

        self.assertAlmostEqual(auxiliary.X.Value, 85.0, places=3)
        self.assertAlmostEqual(auxiliary.Y.Value, 30.0, places=3)

        auxiliary.KeepAligned = False
        self.base.X = 60.0
        self.base.Y = 70.0
        FreeCAD.ActiveDocument.recompute()

        self.assertAlmostEqual(auxiliary.X.Value, 85.0, places=3)
        self.assertAlmostEqual(auxiliary.Y.Value, 30.0, places=3)

    def testAuxiliaryViewKeepsZeroPageOffsetFromBaseView(self):
        self.base.X = 20.0
        self.base.Y = 30.0
        auxiliary = self.makeAuxiliaryView()
        auxiliary.X = 20.0
        auxiliary.Y = 30.0
        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.AlignmentOffset, FreeCAD.Vector(0.0, 0.0, 0.0))

        self.base.X = 40.0
        self.base.Y = 50.0
        FreeCAD.ActiveDocument.recompute()

        self.assertAlmostEqual(auxiliary.X.Value, 40.0, places=3)
        self.assertAlmostEqual(auxiliary.Y.Value, 50.0, places=3)

    def testAuxiliaryViewMirrorsBaseSources(self):
        auxiliary = self.makeAuxiliaryView()
        FreeCAD.ActiveDocument.recompute()

        self.assertEqual(auxiliary.Source, [self.box])

        replacement = FreeCAD.ActiveDocument.addObject("Part::Box", "ReplacementBox")
        self.base.Source = [replacement]
        FreeCAD.ActiveDocument.recompute()

        self.assertEqual(auxiliary.Source, [replacement])

        auxiliary.BaseView = None

        self.assertEqual(auxiliary.Source, [])
        self.assertEqual(auxiliary.XSource, [])

    def testReferenceEndpointChangesUpdateProjectionDirection(self):
        auxiliary = self.makeAuxiliaryView()
        auxiliary.ReferenceStart = FreeCAD.Vector(0.0, 0.0, 0.0)
        auxiliary.ReferenceEnd = FreeCAD.Vector(0.0, 1.0, 0.0)
        auxiliary.ProjectionMode = "Across"

        FreeCAD.ActiveDocument.recompute()

        self.assertVectorAlmostEqual(auxiliary.AuxiliaryDirection, FreeCAD.Vector(0.0, 1.0, 0.0))
        self.assertVectorAlmostEqual(auxiliary.Direction, FreeCAD.Vector(-1.0, 0.0, 0.0))


if __name__ == "__main__":
    unittest.main()
