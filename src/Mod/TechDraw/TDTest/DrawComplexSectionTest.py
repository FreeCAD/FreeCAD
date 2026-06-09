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

        self.profile = FreeCAD.ActiveDocument.addObject("Part::Feature", "SectionProfile")
        self.profile.Shape = Part.makeLine(
            FreeCAD.Vector(2.0, 5.0, 5.0),
            FreeCAD.Vector(8.0, 5.0, 5.0),
        )

        self.closedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "ClosedSectionProfile"
        )
        self.closedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(2.0, 5.0, 2.0),
                FreeCAD.Vector(8.0, 5.0, 2.0),
                FreeCAD.Vector(8.0, 5.0, 8.0),
                FreeCAD.Vector(2.0, 5.0, 8.0),
                FreeCAD.Vector(2.0, 5.0, 2.0),
            ]
        )

        self.outsideProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "OutsideSectionProfile"
        )
        self.outsideProfile.Shape = Part.makeLine(
            FreeCAD.Vector(2.0, 20.0, 5.0),
            FreeCAD.Vector(8.0, 20.0, 5.0),
        )

        self.holedBar = FreeCAD.ActiveDocument.addObject("Part::Feature", "HoledBar")
        holeBox = Part.makeBox(12.0, 8.0, 8.0)
        throughHole = Part.makeCylinder(
            1.5,
            14.0,
            FreeCAD.Vector(-1.0, 4.0, 4.0),
            FreeCAD.Vector(1.0, 0.0, 0.0),
        )
        self.holedBar.Shape = holeBox.cut(throughHole)

        self.holeProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "HoleSectionProfile"
        )
        self.holeProfile.Shape = Part.makeLine(
            FreeCAD.Vector(1.0, 4.0, 4.0),
            FreeCAD.Vector(11.0, 4.0, 4.0),
        )

        self.tangentCylinder = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "TangentCylinder"
        )
        self.tangentCylinder.Shape = Part.makeCylinder(
            2.0,
            10.0,
            FreeCAD.Vector(0.0, 5.0, 5.0),
            FreeCAD.Vector(1.0, 0.0, 0.0),
        )

        self.tangentProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "TangentSectionProfile"
        )
        self.tangentProfile.Shape = Part.makeLine(
            FreeCAD.Vector(0.0, 7.0, 5.0),
            FreeCAD.Vector(10.0, 7.0, 5.0),
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
        FreeCAD.ActiveDocument.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDComplexSection")

    def makePartView(self, source, name):
        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", name)
        self.page.addView(view)
        view.Source = [source]
        view.Direction = (0.0, 0.0, 1.0)
        view.Rotation = 0.0
        view.X = 30.0
        view.Y = 150.0
        FreeCAD.ActiveDocument.recompute()
        return view

    def makeComplexSection(self, strategy, profile=None, source=None, baseView=None, origin=None):
        if source is None:
            source = self.box
        if baseView is None:
            baseView = self.view
        if origin is None:
            origin = (5.0, 5.0, 5.0)

        section = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawComplexSection", "ComplexSection"
        )
        self.page.addView(section)
        section.Source = [source]
        section.BaseView = baseView
        section.CuttingToolWireObject = profile if profile else self.profile
        section.Direction = (0.0, 1.0, 0.0)
        section.SectionNormal = (0.0, 1.0, 0.0)
        section.SectionOrigin = origin
        section.ProjectionStrategy = strategy
        FreeCAD.ActiveDocument.recompute()
        return section

    def assertFaceOnlyBoxSection(self, section, message):
        self.assertEqual(len(section.getVisibleEdges()), 4, message)
        self.assertEqual(
            len(section.getHiddenEdges()),
            0,
            "Face-only complex section should not include outlines behind the section plane",
        )
        self.assertTrue("Up-to-date" in section.State)

    def assertBlankSection(self, section, message):
        self.assertEqual(len(section.getVisibleEdges()), 0, message)
        self.assertEqual(len(section.getHiddenEdges()), 0, message)
        self.assertTrue("Up-to-date" in section.State)

    def testRemovedComplexSectionShowsOnlyCutFace(self):
        section = self.makeComplexSection("Removed")

        self.assertFaceOnlyBoxSection(
            section,
            "Removed complex section should show only the cut face",
        )

    def testRotatedComplexSectionShowsOnlyCutFace(self):
        section = self.makeComplexSection("Rotated")

        self.assertFaceOnlyBoxSection(
            section,
            "Rotated complex section should show only the cut face",
        )

    def testFaceOnlyComplexSectionSupportsClosedProfiles(self):
        section = self.makeComplexSection("Removed", self.closedProfile)

        edges = section.getVisibleEdges()
        self.assertTrue(edges, "Closed profile section should not produce a blank view")
        self.assertTrue("Up-to-date" in section.State)

    def testFaceOnlyComplexSectionPreservesThroughHoleEdges(self):
        holeView = self.makePartView(self.holedBar, "HoleView")
        section = self.makeComplexSection(
            "Removed",
            self.holeProfile,
            self.holedBar,
            holeView,
            (6.0, 4.0, 4.0),
        )

        self.assertEqual(
            len(section.getVisibleEdges()),
            8,
            "Through-hole section should include the cut face and hole edges",
        )
        self.assertEqual(len(section.getHiddenEdges()), 0)
        self.assertTrue("Up-to-date" in section.State)

    def testFaceOnlyComplexSectionIgnoresTangentEdgeContacts(self):
        tangentView = self.makePartView(self.tangentCylinder, "TangentView")
        section = self.makeComplexSection(
            "Removed",
            self.tangentProfile,
            self.tangentCylinder,
            tangentView,
            (5.0, 7.0, 5.0),
        )

        self.assertBlankSection(
            section,
            "Tangent contact should not create a face-only section",
        )

    def testFaceOnlyComplexSectionCanRecomputeFromStrategyChange(self):
        section = self.makeComplexSection("Offset")
        self.assertNotEqual(
            len(section.getVisibleEdges()),
            4,
            "Offset complex section should include more than the cut face",
        )

        section.ProjectionStrategy = "Removed"
        FreeCAD.ActiveDocument.recompute()

        self.assertFaceOnlyBoxSection(
            section,
            "Strategy change should recompute face-only geometry",
        )

    def testFaceOnlyComplexSectionWithoutIntersectionStaysUpToDate(self):
        section = self.makeComplexSection("Removed", self.outsideProfile)
        section.SectionOrigin = (5.0, 20.0, 5.0)
        FreeCAD.ActiveDocument.recompute()

        self.assertBlankSection(
            section,
            "Non-intersecting face-only section should be blank",
        )

    def testFaceOnlyComplexSectionClearsStaleGeometry(self):
        section = self.makeComplexSection("Removed")
        self.assertEqual(len(section.getVisibleEdges()), 4)

        section.CuttingToolWireObject = self.outsideProfile
        section.SectionOrigin = (5.0, 20.0, 5.0)
        FreeCAD.ActiveDocument.recompute()

        self.assertBlankSection(
            section,
            "Blank face-only recompute should clear old edges",
        )


if __name__ == "__main__":
    unittest.main()
