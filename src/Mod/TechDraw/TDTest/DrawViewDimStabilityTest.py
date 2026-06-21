#!/usr/bin/env python3

import FreeCAD
import unittest

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewDimStabilityTest(unittest.TestCase):
    """Regression test for issue #19871: dimension references must survive a
    CoarseView (HLR projector) toggle without going to zero."""

    def setUp(self):
        FreeCAD.newDocument("TDDimStability")
        FreeCAD.setActiveDocument("TDDimStability")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDDimStability")
        self.document = FreeCAD.ActiveDocument

        box = self.document.addObject("Part::Box", "Box")
        box.Length = 20.0
        box.Width = 10.0
        box.Height = 30.0

        self.page = createPageWithSVGTemplate(self.document)
        self.page.Scale = 1.0

        self.view = self.document.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [box]
        self.view.CoarseView = False
        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDDimStability")

    def _firstHorizontalEdgeName(self):
        edges = self.view.getVisibleEdges()
        for i, edge in enumerate(edges):
            if edge.Length > 0.0 and len(edge.Vertexes) == 2:
                return "Edge" + str(i), edge.Length
        self.fail("No usable visible edge on view")
        return None, 0.0

    def _coarseCylinderView(self, height):
        # A cylinder under coarse HLR fragments its curved top/bottom edges into
        # many short segments (here ~34 edges vs ~4 under exact HLR), reproducing
        # the projector edge-count churn from issue #19871 that a planar box does
        # not exhibit.
        cyl = self.document.addObject("Part::Cylinder", "Cylinder")
        cyl.Radius = 10.0
        cyl.Height = height
        view = self.document.addObject("TechDraw::DrawViewPart", "CylView")
        self.page.addView(view)
        view.Source = [cyl]
        view.CoarseView = True
        self.document.recompute()
        return view

    def _verticalSilhouetteName(self, view, height):
        # The vertical silhouette of the cylinder projects to a straight Line of
        # length == Height under both projectors. Under coarse HLR it lands at a
        # high index (after all the fragment segments); there are no hidden edges
        # so the visible index equals the index used to resolve the reference.
        edges = view.getVisibleEdges()
        for i, edge in enumerate(edges):
            if (len(edge.Vertexes) == 2
                    and type(edge.Curve).__name__ == "Line"
                    and abs(edge.Length - height) < 1.0e-3):
                return "Edge" + str(i), i, edge.Length
        self.fail("No vertical silhouette line on coarse cylinder view")
        return None, -1, 0.0

    def testDimensionSurvivesCoarseViewToggle(self):
        """Creating a dim under exact HLR and toggling to coarse HLR must keep
        the dimension resolvable (not zero). Repair runs via
        DimensionAutoCorrect against the dim's SavedGeometry."""
        edge_name, _length = self._firstHorizontalEdgeName()
        dim = self.document.addObject("TechDraw::DrawViewDimension", "Dimension")
        self.page.addView(dim)
        dim.Type = "Distance"
        dim.References2D = [(self.view, edge_name)]
        self.document.recompute()
        self.assertTrue("Up-to-date" in dim.State)
        initial_value = dim.getRawValue()
        self.assertGreater(initial_value, 0.0)
        # SavedGeometry is what the autocorrect path matches against. One sub
        # in -> one stored TopoShape, captured on first successful execute.
        self.assertEqual(len(dim.SavedGeometry), 1)
        self.assertFalse(dim.SavedGeometry[0].isNull())

        # Toggle to coarse -- HLR re-runs and the projected edge set may
        # differ. The dim must still resolve to a non-zero value, either
        # because the index is still valid or because autocorrect rebound
        # the reference against SavedGeometry.
        self.view.CoarseView = True
        self.document.recompute()
        self.assertTrue("Up-to-date" in dim.State)
        self.assertGreater(dim.getRawValue(), 0.0)

        # Toggle back to exact -- the dim should recover the original value.
        self.view.CoarseView = False
        self.document.recompute()
        self.assertTrue("Up-to-date" in dim.State)
        self.assertAlmostEqual(dim.getRawValue(), initial_value, places=3)

    def testStaleReferenceRepairedAcrossProjectorToggle(self):
        """The core #19871 case: a reference created under coarse HLR points at a
        high edge index; switching to exact HLR leaves far fewer edges, so that
        index is out of range. okToProceed() must repair the reference against
        SavedGeometry instead of dropping the dimension to a corrupt/zero state."""
        height = 30.0
        view = self._coarseCylinderView(height)
        edge_name, coarse_idx, length = self._verticalSilhouetteName(view, height)

        dim = self.document.addObject("TechDraw::DrawViewDimension", "CylDim")
        self.page.addView(dim)
        dim.Type = "Distance"
        dim.References2D = [(view, edge_name)]
        self.document.recompute()
        self.assertTrue("Up-to-date" in dim.State)
        self.assertAlmostEqual(dim.getRawValue(), length, places=3)
        self.assertEqual(len(dim.SavedGeometry), 1)

        # Switch to exact HLR: the cylinder projects to only a handful of edges,
        # so the coarse index is now beyond the end of the edge list.
        view.CoarseView = False
        self.document.recompute()
        total_edges = len(view.getVisibleEdges()) + len(view.getHiddenEdges())
        self.assertGreater(
            coarse_idx, total_edges - 1,
            "test no longer drives the out-of-range repair path")

        # With the repair, the reference is rebound to the surviving silhouette
        # line: the dimension stays up to date, keeps its value, and the stale
        # subname has been replaced. Without it, the dimension would be corrupt.
        self.assertTrue("Up-to-date" in dim.State)
        self.assertAlmostEqual(dim.getRawValue(), length, places=3)
        self.assertNotEqual(dim.References2D[0][1][0], edge_name)


if __name__ == "__main__":
    unittest.main()
