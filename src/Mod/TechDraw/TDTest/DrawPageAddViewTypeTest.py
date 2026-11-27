# SPDX-License-Identifier: LGPL-2.1-or-later
"""DrawPage.addView/removeView accept the objects a page can hold (views, links to
views, sketches) and raise TypeError for any other object, as they did for
non-views when they only accepted views, instead of quietly returning -1."""

import unittest

import FreeCAD
import Part  # noqa: F401
import TechDraw  # noqa: F401

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawPageAddViewTypeTest(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TDAddViewType")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.page = createPageWithSVGTemplate(self.doc)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testNonViewIsRejected(self):
        with self.assertRaises(TypeError):
            self.page.addView(self.box)
        with self.assertRaises(TypeError):
            self.page.removeView(self.box)
        self.assertEqual(self.page.Views, [])

    def testViewIsAccepted(self):
        view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        view.Source = [self.box]
        self.assertGreater(self.page.addView(view), 0)
        self.assertIn(view, self.page.Views)
        self.assertGreaterEqual(self.page.removeView(view), 0)
        self.assertNotIn(view, self.page.Views)

    def testLinkToViewIsAccepted(self):
        view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        view.Source = [self.box]
        link = self.doc.addObject("App::Link", "LinkToView")
        link.LinkedObject = view
        self.assertGreater(self.page.addView(link), 0)
        self.assertIn(link, self.page.Views)
        self.assertGreaterEqual(self.page.removeView(link), 0)
        self.assertNotIn(link, self.page.Views)

    def testLinkToNonViewIsRejected(self):
        link = self.doc.addObject("App::Link", "LinkToBox")
        link.LinkedObject = self.box
        with self.assertRaises(TypeError):
            self.page.addView(link)
        self.assertEqual(self.page.Views, [])

    def testHeldLinkStaysRemovable(self):
        # a link the page holds may since point at a non-view: it must still be removable
        view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        view.Source = [self.box]
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = view
        self.assertGreater(self.page.addView(link), 0)
        link.LinkedObject = self.box
        self.assertGreaterEqual(self.page.removeView(link), 0)
        self.assertNotIn(link, self.page.Views)

    def testObjectOfClosedDocumentIsRejected(self):
        # the wrapper outlives its object: it must raise, not crash
        other = FreeCAD.newDocument("TDAddViewTypeOther")
        view = other.addObject("TechDraw::DrawViewPart", "View")
        FreeCAD.closeDocument(other.Name)
        with self.assertRaises(ValueError):
            self.page.addView(view)
        with self.assertRaises(ValueError):
            self.page.removeView(view)
        self.assertEqual(self.page.Views, [])

    def testSketchIsAccepted(self):
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.assertGreater(self.page.addView(sketch), 0)
        self.assertIn(sketch, self.page.Views)


if __name__ == "__main__":
    unittest.main()
