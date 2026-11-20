import unittest

import FreeCAD

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawPageRemovalTest(unittest.TestCase):
    def setUp(self):
        """Create a page with multiple linked views."""
        self.doc = FreeCAD.newDocument("TDPageRemoval")
        self.page = createPageWithSVGTemplate(self.doc)
        box = self.doc.addObject("Part::Box", "Box")

        for name in ("View1", "View2"):
            view = self.doc.addObject("TechDraw::DrawViewPart", name)
            view.Source = [box]
            self.page.addView(view)

        annotation = self.doc.addObject("TechDraw::DrawViewAnnotation", "Annotation")
        annotation.Text = ["fixture"]
        self.page.addView(annotation)
        self.doc.recompute()

    def tearDown(self):
        if FreeCAD.getDocument("TDPageRemoval"):
            FreeCAD.closeDocument("TDPageRemoval")

    def test_remove_page_with_multiple_views(self):
        """Removing a page must remove all linked views without crashing."""
        self.doc.removeObject(self.page.Name)

        for name in ("Page", "Template", "View1", "View2", "Annotation"):
            self.assertIsNone(self.doc.getObject(name))


if __name__ == "__main__":
    unittest.main()
