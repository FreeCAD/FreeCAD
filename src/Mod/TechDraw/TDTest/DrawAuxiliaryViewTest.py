#!/usr/bin/env python3

# test script for DrawAuxiliaryView
# creates a page, a base view and an auxiliary view


import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore


def wait_for_projection():
    loop = QtCore.QEventLoop()
    timer = QtCore.QTimer()
    timer.setSingleShot(True)
    timer.timeout.connect(loop.quit)
    timer.start(2000)
    loop.exec_()


class DrawAuxiliaryViewTest(unittest.TestCase):
    def setUp(self):
        """Creates a page and a base view"""
        FreeCAD.newDocument("TDAuxiliary")
        FreeCAD.setActiveDocument("TDAuxiliary")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAuxiliary")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.box.Length = 10.0
        self.box.Width = 6.0
        self.box.Height = 3.0

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        print("DrawAuxiliaryView test: page created")

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.box]
        self.view.Direction = (0.0, 0.0, 1.0)
        self.view.XDirection = (1.0, 0.0, 0.0)
        self.view.Rotation = 0.0
        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()
        print("DrawAuxiliaryView test: base view created")

    def tearDown(self):
        print("DrawAuxiliaryView test finished")
        FreeCAD.closeDocument("TDAuxiliary")

    def testMakeDrawAuxiliaryView(self):
        """Tests if an auxiliary view can be added to a page"""
        auxiliary = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawAuxiliaryView", "Auxiliary"
        )
        self.page.addView(auxiliary)
        auxiliary.BaseView = self.view
        auxiliary.AuxiliaryDirection = (1.0, 0.0, 0.0)
        auxiliary.AuxiliaryOrientation = "Along"
        auxiliary.ReferenceLabel = "A"
        auxiliary.ReferenceStart = (0.0, 0.0, 0.0)
        auxiliary.ReferenceEnd = (10.0, 0.0, 0.0)

        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()
        print("DrawAuxiliaryView test: auxiliary view created")

        edges = auxiliary.getVisibleEdges()
        self.assertGreater(len(edges), 0, "DrawAuxiliaryView has no visible edges")
        self.assertTrue(
            "Up-to-date" in auxiliary.State,
            "DrawAuxiliaryView is not Up-to-date",
        )
        self.assertEqual(auxiliary.BaseView, self.view)
        self.assertAlmostEqual(auxiliary.Direction.x, 1.0)
        self.assertAlmostEqual(auxiliary.XDirection.y, 1.0)
        self.assertTrue(auxiliary.KeepAligned)
        self.assertGreater(auxiliary.X, self.view.X)
        self.assertAlmostEqual(auxiliary.Y, self.view.Y)

        aligned_offset = auxiliary.X - self.view.X
        self.view.X = self.view.X + 10.0
        self.assertAlmostEqual(auxiliary.X - self.view.X, aligned_offset)
        self.assertAlmostEqual(auxiliary.Y, self.view.Y)

        auxiliary.KeepAligned = False
        auxiliary.X = 12.0
        auxiliary.Y = 34.0
        self.view.X = self.view.X + 10.0
        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()

        self.assertAlmostEqual(auxiliary.X, 12.0)
        self.assertAlmostEqual(auxiliary.Y, 34.0)

        auxiliary.ReverseDirection = True
        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()

        self.assertTrue(
            "Up-to-date" in auxiliary.State,
            "DrawAuxiliaryView is not Up-to-date after reversing direction",
        )
        self.assertAlmostEqual(auxiliary.Direction.x, -1.0)

        auxiliary.ReverseDirection = False
        auxiliary.AuxiliaryOrientation = "Across"
        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()

        self.assertAlmostEqual(auxiliary.Direction.y, 1.0)

    def testProjectionGroupItemCanBeBaseView(self):
        """Tests if an auxiliary view can use a projection group item as base"""
        group = FreeCAD.ActiveDocument.addObject("TechDraw::DrawProjGroup", "ProjGroup")
        self.page.addView(group)
        group.Source = [self.box]
        group.addProjection("Front")
        group.Anchor.Direction = (0.0, 0.0, 1.0)
        group.Anchor.RotationVector = (1.0, 0.0, 0.0)

        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()

        base_item = group.Anchor
        self.assertIsNotNone(base_item, "Projection group did not create an anchor item")
        self.assertGreater(
            len(base_item.getVisibleEdges()),
            0,
            "Projection group anchor has no visible edges",
        )

        auxiliary = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawAuxiliaryView", "AuxiliaryFromProjection"
        )
        self.page.addView(auxiliary)
        auxiliary.BaseView = base_item
        auxiliary.AuxiliaryDirection = (1.0, 0.0, 0.0)
        auxiliary.AuxiliaryOrientation = "Along"
        auxiliary.ReferenceLabel = "B"
        auxiliary.ReferenceStart = (0.0, 0.0, 0.0)
        auxiliary.ReferenceEnd = (10.0, 0.0, 0.0)

        FreeCAD.ActiveDocument.recompute()
        wait_for_projection()
        print("DrawAuxiliaryView test: projection group auxiliary view created")

        edges = auxiliary.getVisibleEdges()
        self.assertGreater(
            len(edges),
            0,
            "DrawAuxiliaryView from projection group item has no visible edges",
        )
        self.assertTrue(
            "Up-to-date" in auxiliary.State,
            "DrawAuxiliaryView from projection group item is not Up-to-date",
        )
        self.assertEqual(auxiliary.BaseView, base_item)


if __name__ == "__main__":
    unittest.main()
