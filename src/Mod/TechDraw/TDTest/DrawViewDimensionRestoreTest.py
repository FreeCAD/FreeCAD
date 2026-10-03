# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD


class _Observer:
    def __init__(self):
        self.hits = []

    def slotChangedObject(self, obj, prop):
        document = getattr(obj, "Document", None)
        if document is not None and getattr(document, "Restoring", False):
            return
        if prop in ("References2D", "SavedGeometry", "BoxCorners"):
            self.hits.append((obj.Name, prop))


class DrawViewDimensionRestoreTest(unittest.TestCase):
    def test_reopen_does_not_rewrite_reference(self):
        """Opening a drawing must not rewrite an unchanged dimension reference."""
        from TDTest.TechDrawTestUtilities import createPageWithSVGTemplate

        doc = FreeCAD.newDocument("TDDimRestore")
        try:
            doc.addObject("Part::Box", "Box")
            page = createPageWithSVGTemplate(doc)
            view = doc.addObject("TechDraw::DrawViewPart", "View")
            page.addView(view)
            view.Source = [doc.Box]
            dimension = doc.addObject("TechDraw::DrawViewDimension", "Dimension")
            page.addView(dimension)
            dimension.Type = "Distance"
            dimension.References2D = [(view, "Edge1")]
            doc.recompute()
            self.assertEqual(dimension.References2D[0][1], ("Edge1",))

            saved = os.path.join(tempfile.mkdtemp(prefix="td-dim-restore-"), "dim.FCStd")
            doc.saveAs(saved)
        finally:
            FreeCAD.closeDocument("TDDimRestore")

        observer = _Observer()
        FreeCAD.addDocumentObserver(observer)
        try:
            reopened = FreeCAD.openDocument(saved)
            reopened.recompute()
            self.assertEqual(observer.hits, [])
            restored = reopened.getObject("Dimension")
            self.assertEqual(restored.References2D[0][1], ("Edge1",))
            # A real reference change must still be stored.
            restored.References2D = [(reopened.getObject("View"), "Edge2")]
            self.assertEqual(restored.References2D[0][1], ("Edge2",))
            FreeCAD.closeDocument(reopened.Name)
        finally:
            FreeCAD.removeDocumentObserver(observer)


if __name__ == "__main__":
    unittest.main()
