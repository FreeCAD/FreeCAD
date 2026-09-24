# SPDX-License-Identifier: LGPL-2.1-or-later
import os
import tempfile
import unittest
import FreeCAD as App
import Part
import Sketcher
import TechDraw
import SketchAnnotations as Adapter
from .TechDrawTestUtilities import createPageWithSVGTemplate


class SketchAnnotationsTest(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TDAnnotations")
        self.doc.UndoMode = 1
        self.page = createPageWithSVGTemplate(self.doc)
        self.s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.s.addGeometry(
            [
                Part.LineSegment(App.Vector(x, y, 0), App.Vector(u, v, 0))
                for x, y, u, v in ((0, 0, 20, 0), (20, 0, 20, 20), (20, 20, 0, 20), (0, 20, 0, 0))
            ]
        )
        self.text = self.s.addAnnotation(
            {"Type": "Text", "Html": "<b>Linked α</b>", "Position": (3.0, 7.0, 0.0)}
        )
        self.leader = self.s.addAnnotation(
            {"Type": "Leader", "Points": [(1.0, 1.0, 0.0), (5.0, 7.0, 0.0)]}
        )
        self.hatch = self.s.addAnnotation(
            {"Type": "Hatch", "Boundary": [g.Id for g in self.s.GeometryFacadeList]}
        )
        self.view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.s]
        self.view.Direction = App.Vector(0, 0, 1)
        self.view.ScaleType = "Custom"
        self.view.Scale = 1
        self.doc.recompute()
        Adapter.synchronize(self.doc)

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def linked(self, ident):
        return next(o for o in self.doc.Objects if getattr(o, "SourceAnnotationId", 0) == ident)

    def testNativeTypesAndIdempotence(self):
        self.assertTrue(self.linked(self.text).isDerivedFrom("TechDraw::DrawRichAnno"))
        self.assertTrue(self.linked(self.leader).isDerivedFrom("TechDraw::DrawLeaderLine"))
        hatch = self.linked(self.hatch)
        self.assertTrue(hatch.isDerivedFrom("TechDraw::DrawGeomHatch"))
        self.assertEqual(hatch.AnnotationStatus, "Linked")
        self.assertEqual(list(hatch.Source[1]), ["Face0"])
        names = [o.Name for o in self.doc.Objects]
        Adapter.synchronize(self.doc)
        self.assertEqual([o.Name for o in self.doc.Objects], names)
        self.assertEqual(self.s.GeometryCount, 4)
        self.assertEqual(self.s.ConstraintCount, 0)

    def testUpdateScaleConstructionAndOffset(self):
        text = self.linked(self.text)
        name = text.Name
        text.PageOffset = App.Vector(2, 3, 0)
        self.s.updateAnnotation(self.text, {"Html": "<i>Changed</i>"})
        self.view.Scale = 2
        self.view.Rotation = 30
        self.doc.recompute()
        Adapter.synchronize(self.doc)
        p = Adapter._point(self.view, self.s, App.Vector(3, 7, 0)) + text.PageOffset
        self.assertAlmostEqual(text.X.Value, p.x)
        self.assertAlmostEqual(text.Y.Value, p.y)
        self.assertEqual(text.AnnoText, "<i>Changed</i>")
        self.assertAlmostEqual(text.TextHeight.Value, 7)
        leader = self.linked(self.leader)
        start = Adapter._point(self.view, self.s, App.Vector(1, 1, 0), False)
        self.assertAlmostEqual(leader.X.Value, start.x)
        self.assertAlmostEqual(leader.Y.Value, start.y)
        self.s.updateAnnotation(self.text, {"Construction": True})
        Adapter.synchronize(self.doc)
        self.assertEqual(text.AnnoText, "")
        self.s.updateAnnotation(self.text, {"Construction": False})
        Adapter.synchronize(self.doc)
        self.assertEqual(self.linked(self.text).Name, name)
        self.assertEqual(text.AnnoText, "<i>Changed</i>")

    def testBrokenHatchAndUndo(self):
        hatch = self.linked(self.hatch)
        name = hatch.Name
        self.doc.openTransaction("Break boundary")
        self.s.delGeometry(0)
        self.doc.commitTransaction()
        self.doc.recompute()
        Adapter.synchronize(self.doc)
        self.assertNotEqual(hatch.AnnotationStatus, "Linked")
        self.assertEqual(list(hatch.Source[1]), [])
        self.doc.undo()
        self.doc.recompute()
        Adapter.synchronize(self.doc)
        self.assertEqual(self.linked(self.hatch).Name, name)
        self.assertEqual(hatch.AnnotationStatus, "Linked")

    def testPersistenceAndSourceDeletion(self):
        names = {i: self.linked(i).Name for i in (self.text, self.leader, self.hatch)}
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "annotations.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.s = self.doc.Sketch
            self.view = self.doc.View
            self.doc.recompute()
            Adapter.synchronize(self.doc)
            self.assertEqual({i: self.linked(i).Name for i in names}, names)
            self.assertEqual(self.linked(self.text).SourceSketch, self.s)
            self.doc.openTransaction("Delete note")
            self.s.delAnnotations([self.text])
            self.doc.commitTransaction()
            Adapter.synchronize(self.doc)
            self.assertEqual(self.linked(self.text).AnnoText, "")
            self.doc.undo()
            Adapter.synchronize(self.doc)
            self.assertIn("Linked α", self.linked(self.text).AnnoText)

    def testProjectionAndMultipleViews(self):
        self.s.Placement = App.Placement(App.Vector(5, 7, 9), App.Rotation(App.Vector(0, 0, 1), 30))
        self.s.updateAnnotation(self.hatch, {"Pattern": "ANSI37", "Rotation": 27})
        self.view.Direction = App.Vector(0, 1, 1)
        second = self.doc.addObject("TechDraw::DrawViewPart", "SecondView")
        self.page.addView(second)
        second.Source = [self.s]
        second.Direction = App.Vector(0, 0, 1)
        self.doc.recompute()
        Adapter.synchronize(self.doc)
        linked = [o for o in self.doc.Objects if getattr(o, "SourceAnnotationId", 0) == self.hatch]
        self.assertEqual(len(linked), 2)
        self.assertTrue(
            all(o.AnnotationStatus == "Linked" for o in linked),
            [o.AnnotationStatus for o in linked],
        )
        self.assertTrue(all(len(o.AnnotationPattern.strip().splitlines()) == 3 for o in linked))
        self.view.Direction = App.Vector(1, 0, 0)
        self.doc.recompute()
        Adapter.synchronize(self.doc)
        self.assertNotEqual(self.linked(self.hatch).AnnotationStatus, "Linked")

    def testUnchangedSynchronizationDoesNotDirtyDocument(self):
        with tempfile.TemporaryDirectory() as directory:
            self.doc.saveAs(os.path.join(directory, "clean.FCStd"))
            Adapter.synchronize(self.doc)

            class Changes:
                def __init__(self):
                    self.changes = []

                def slotChangedObject(self, obj, prop):
                    self.changes.append((obj.Name, prop))

            changes = Changes()
            App.addDocumentObserver(changes)
            try:
                Adapter.synchronize(self.doc)
                self.assertEqual(changes.changes, [])
            finally:
                App.removeDocumentObserver(changes)
