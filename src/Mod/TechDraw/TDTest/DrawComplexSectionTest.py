import FreeCAD
import Part
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawComplexSectionTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDComplexSection")
        FreeCAD.setActiveDocument("TDComplexSection")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDComplexSection")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.box.Length = 10.0
        self.box.Width = 10.0
        self.box.Height = 10.0

        self.cavityPart = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "BlindInternalCavity"
        )
        self.cavityPart.Shape = Part.makeBox(10.0, 10.0, 10.0).cut(
            Part.makeCylinder(
                1.0,
                5.0,
                FreeCAD.Vector(5.0, 5.0, 0.0),
                FreeCAD.Vector(0.0, 0.0, 1.0),
            )
        )

        self.openProfile = FreeCAD.ActiveDocument.addObject("Part::Feature", "OpenProfile")
        self.openProfile.Shape = Part.makeLine(
            FreeCAD.Vector(1.0, 1.0, 10.0),
            FreeCAD.Vector(4.0, 1.0, 10.0),
        )

        self.closedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "ClosedProfile"
        )
        self.closedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(1.0, 1.0, 10.0),
                FreeCAD.Vector(4.0, 1.0, 10.0),
                FreeCAD.Vector(4.0, 4.0, 10.0),
                FreeCAD.Vector(1.0, 4.0, 10.0),
                FreeCAD.Vector(1.0, 1.0, 10.0),
            ]
        )

        self.wideClosedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "WideClosedProfile"
        )
        self.wideClosedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(2.0, 2.0, 10.0),
                FreeCAD.Vector(8.0, 2.0, 10.0),
                FreeCAD.Vector(8.0, 8.0, 10.0),
                FreeCAD.Vector(2.0, 8.0, 10.0),
                FreeCAD.Vector(2.0, 2.0, 10.0),
            ]
        )

        self.misalignedClosedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "MisalignedClosedProfile"
        )
        self.misalignedClosedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(3.0, 0.0, 3.0),
                FreeCAD.Vector(7.0, 0.0, 3.0),
                FreeCAD.Vector(7.0, 0.0, 7.0),
                FreeCAD.Vector(3.0, 0.0, 7.0),
                FreeCAD.Vector(3.0, 0.0, 3.0),
            ]
        )

        self.internalClosedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "InternalClosedProfile"
        )
        self.internalClosedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(1.0, 1.0, 5.0),
                FreeCAD.Vector(4.0, 1.0, 5.0),
                FreeCAD.Vector(4.0, 4.0, 5.0),
                FreeCAD.Vector(1.0, 4.0, 5.0),
                FreeCAD.Vector(1.0, 1.0, 5.0),
            ]
        )

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.box]
        self.view.Direction = (0.0, 0.0, 1.0)
        self.view.Rotation = 0.0
        self.view.X = 30.0
        self.view.Y = 150.0

        self.cavityView = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewPart", "CavityView"
        )
        self.page.addView(self.cavityView)
        self.cavityView.Source = [self.cavityPart]
        self.cavityView.Direction = (0.0, 0.0, 1.0)
        self.cavityView.Rotation = 0.0
        self.cavityView.X = 90.0
        self.cavityView.Y = 150.0

        FreeCAD.ActiveDocument.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDComplexSection")

    def makeComplexSection(
        self,
        profile=None,
        strategy="BrokenOut",
        depth=4.0,
        direction=(0.0, 0.0, 1.0),
        source=None,
        baseView=None,
    ):
        section = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawComplexSection", "ComplexSection"
        )
        source = source if source else self.box
        baseView = baseView if baseView else self.view
        self.page.addView(section)
        section.Source = [source]
        section.BaseView = baseView
        section.CuttingToolWireObject = profile if profile else self.closedProfile
        section.Direction = direction
        section.SectionNormal = direction
        section.SectionOrigin = (5.0, 5.0, 10.0)
        section.ProjectionStrategy = strategy
        section.BrokenOutDepth = depth
        FreeCAD.ActiveDocument.recompute()
        return section

    def visibleBounds(self, section):
        xs = []
        ys = []
        for edge in section.getVisibleEdges():
            for vertex in edge.Vertexes:
                xs.append(vertex.Point.x)
                ys.append(vertex.Point.y)
        return (min(xs), max(xs), min(ys), max(ys))

    def assertBoundsAlmostEqual(self, bounds, expected):
        for actual, target in zip(bounds, expected):
            self.assertAlmostEqual(actual, target, places=6)

    def scaledBounds(self, bounds):
        scale = self.page.Scale
        return tuple(value * scale for value in bounds)

    def edgeBounds(self, edge):
        xs = []
        ys = []
        for vertex in edge.Vertexes:
            xs.append(vertex.Point.x)
            ys.append(vertex.Point.y)
        return (min(xs), max(xs), min(ys), max(ys))

    def innerBoundaryBounds(self, section):
        outer = self.scaledBounds((-5.0, 5.0, -5.0, 5.0))

        def touchesOuter(edge_bounds):
            return any(
                abs(actual - target) < 1e-6
                for actual in edge_bounds
                for target in outer
            )

        inner_edges = []
        for edge in section.getVisibleEdges():
            bounds = self.edgeBounds(edge)
            if not touchesOuter(bounds):
                inner_edges.append(edge)

        self.assertEqual(
            len(inner_edges),
            4,
            "Broken-out section should include four local boundary edges",
        )

        xs = []
        ys = []
        for edge in inner_edges:
            for vertex in edge.Vertexes:
                xs.append(vertex.Point.x)
                ys.append(vertex.Point.y)
        return (min(xs), max(xs), min(ys), max(ys))

    def assertUpToDate(self, section):
        self.assertTrue("Up-to-date" in section.State)

    def assertBlankSection(self, section, message):
        self.assertEqual(len(section.getVisibleEdges()), 0, message)
        self.assertEqual(len(section.getHiddenEdges()), 0, message)
        self.assertUpToDate(section)

    def testBrokenOutClosedProfileProducesGeometry(self):
        section = self.makeComplexSection()

        self.assertEqual(
            len(section.getVisibleEdges()),
            8,
            "Broken-out section should show base outline plus the local broken-out boundary",
        )
        self.assertEqual(len(section.getHiddenEdges()), 0)
        self.assertBoundsAlmostEqual(
            self.visibleBounds(section), self.scaledBounds((-5.0, 5.0, -5.0, 5.0))
        )
        self.assertBoundsAlmostEqual(
            self.innerBoundaryBounds(section), self.scaledBounds((-4.0, -1.0, 1.0, 4.0))
        )
        self.assertUpToDate(section)

    def testBrokenOutDepthChangeInvalidatesAndRecomputes(self):
        section = self.makeComplexSection(depth=3.0)
        self.assertFalse(section.MustExecute)

        section.BrokenOutDepth = 6.0
        self.assertTrue(section.MustExecute)

        FreeCAD.ActiveDocument.recompute()
        self.assertFalse(section.MustExecute)
        self.assertEqual(len(section.getVisibleEdges()), 8)
        self.assertBoundsAlmostEqual(
            self.visibleBounds(section), self.scaledBounds((-5.0, 5.0, -5.0, 5.0))
        )
        self.assertBoundsAlmostEqual(
            self.innerBoundaryBounds(section), self.scaledBounds((-4.0, -1.0, 1.0, 4.0))
        )
        self.assertUpToDate(section)

    def testBrokenOutDepthRevealsInternalGeometry(self):
        shallow = self.makeComplexSection(
            self.wideClosedProfile,
            depth=3.0,
            source=self.cavityPart,
            baseView=self.cavityView,
        )
        deep = self.makeComplexSection(
            self.wideClosedProfile,
            depth=7.0,
            source=self.cavityPart,
            baseView=self.cavityView,
        )

        self.assertEqual(len(shallow.getVisibleEdges()), 8)
        self.assertGreater(
            len(deep.getVisibleEdges()),
            len(shallow.getVisibleEdges()),
            "Deeper broken-out cuts should reveal internal geometry",
        )
        self.assertEqual(len(shallow.getHiddenEdges()), 0)
        self.assertEqual(len(deep.getHiddenEdges()), 0)
        self.assertUpToDate(shallow)
        self.assertUpToDate(deep)

    def testBrokenOutRequiresClosedProfile(self):
        section = self.makeComplexSection(self.openProfile)

        self.assertBlankSection(
            section,
            "Broken-out section with an open profile should stay blank without crashing",
        )

    def testBrokenOutRequiresProfileOnBaseViewPlane(self):
        section = self.makeComplexSection(self.misalignedClosedProfile)

        self.assertBlankSection(
            section,
            "Broken-out section with a profile off the base view plane should stay blank",
        )

    def testBrokenOutRequiresProfileOnSourceSurface(self):
        section = self.makeComplexSection(self.internalClosedProfile)

        self.assertBlankSection(
            section,
            "Broken-out section with a profile inside the source solid should stay blank",
        )

    def testBrokenOutRequiresBaseViewDirection(self):
        section = self.makeComplexSection(direction=(0.0, 1.0, 0.0))

        self.assertBlankSection(
            section,
            "Broken-out section should use the base view projection direction",
        )

    def testBrokenOutRequiresPositiveDepth(self):
        section = self.makeComplexSection(depth=0.0)

        self.assertBlankSection(
            section,
            "Broken-out section with zero depth should stay blank",
        )

    def testBrokenOutClearsStaleGeometry(self):
        section = self.makeComplexSection()
        self.assertGreater(len(section.getVisibleEdges()), 0)

        section.CuttingToolWireObject = self.openProfile
        FreeCAD.ActiveDocument.recompute()

        self.assertBlankSection(
            section,
            "Broken-out section should clear stale edges when the profile becomes invalid",
        )

    def testOffsetComplexSectionStillProducesGeometry(self):
        section = self.makeComplexSection(self.openProfile, "Offset")

        self.assertGreater(
            len(section.getVisibleEdges()),
            0,
            "Existing offset complex sections should continue to produce geometry",
        )
        self.assertUpToDate(section)


if __name__ == "__main__":
    unittest.main()
