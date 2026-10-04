# SPDX-License-Identifier: LGPL-2.1-or-later
import os
import tempfile
from pathlib import Path
import FreeCAD as App
import FreeCADGui as Gui
import Part
import TechDraw
import TechDrawGui
import SketchAnnotations as Adapter
from SketcherTests.GuiTestCase import SketcherGuiTestCase
from .TechDrawTestUtilities import createPageWithSVGTemplate


class SketchAnnotationsGuiTest(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        Gui.activateWorkbench("TechDrawWorkbench")
        self.doc = App.newDocument("VisualAnnotations")
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
            {"Type": "Text", "Html": "<b>Linked annotation</b>", "Position": (0, 25, 0)}
        )
        self.s.addAnnotation({"Type": "Leader", "Points": [(5, 5, 0), (10, 25, 0), (17, 25, 0)]})
        self.s.addAnnotation(
            {"Type": "Hatch", "Boundary": [g.Id for g in self.s.GeometryFacadeList], "Rotation": 45}
        )
        self.view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.s]
        self.view.Direction = App.Vector(0, 0, 1)
        self.view.ScaleType = "Custom"
        self.view.Scale = 2
        self.doc.recompute()
        self.assertTrue(self.wait_until(lambda: len(self.linked()) == 3, 5000))
        self.assertTrue(
            self.wait_until(
                lambda: all(o.AnnotationStatus == "Linked" for o in self.linked()), 5000
            )
        )

    def linked(self):
        return [o for o in self.doc.Objects if hasattr(o, "SourceAnnotationId")]

    def testAutomaticUpdateAndExport(self):
        self.s.updateAnnotation(self.text, {"Html": "<b>Automatic update</b>"})
        self.assertTrue(
            self.wait_until(
                lambda: any(
                    getattr(o, "AnnoText", "") == "<b>Automatic update</b>" for o in self.linked()
                ),
                3000,
            )
        )
        Gui.activeDocument().getObject(self.page.Name).show()
        self.flush_gui(250)
        hatch = next(o for o in self.linked() if o.isDerivedFrom("TechDraw::DrawGeomHatch"))
        strokes = TechDraw.makeGeomHatch(
            self.view.getFaces()[0].Faces[0],
            hatch.ScalePattern,
            hatch.NamePattern,
            hatch.PatIncluded,
        )
        self.assertGreater(len(strokes.Edges), 0)
        with tempfile.TemporaryDirectory() as directory:
            svg = os.path.join(directory, "page.svg")
            pdf = os.path.join(directory, "page.pdf")
            TechDrawGui.exportPageAsSvg(self.page, svg)
            TechDrawGui.exportPageAsPdf(self.page, pdf)
            self.assertIn("Automatic update", Path(svg).read_text())
            self.assertGreater(Path(pdf).stat().st_size, 1000)
        self.s.updateAnnotation(self.text, {"Construction": True})
        self.assertTrue(
            self.wait_until(
                lambda: all(not getattr(o, "AnnoText", "") for o in self.linked()), 3000
            )
        )

    def testDeletedViewAndSourceUndo(self):
        names = [o.Name for o in self.linked()]
        for obj in (self.view, self.s):
            self.doc.openTransaction("Delete source")
            self.doc.removeObject(obj.Name)
            self.doc.commitTransaction()
            self.doc.recompute()
            self.assertTrue(
                self.wait_until(
                    lambda: all(not o.ViewObject.Visibility for o in self.linked()), 3000
                )
            )
            self.doc.undo()
            self.doc.recompute()
            self.assertTrue(
                self.wait_until(
                    lambda: len(self.linked()) == 3
                    and all(o.AnnotationStatus == "Linked" for o in self.linked()),
                    5000,
                )
            )
            self.assertEqual([o.Name for o in self.linked()], names)

    def testCosmeticVisibilityUpdatesLinkedContent(self):
        text = next(o for o in self.linked() if o.SourceAnnotationId == self.text)
        self.doc.openTransaction("Hide cosmetic")
        self.s.ViewObject.HiddenAnnotations = [self.text]
        self.doc.commitTransaction()
        self.assertTrue(self.wait_until(lambda: not text.ViewObject.Visibility, 3000))
        self.assertEqual(text.AnnoText, "")
        self.assertTrue(all(o.ViewObject.Visibility for o in self.linked() if o != text))
        self.doc.undo()
        self.assertTrue(self.wait_until(lambda: text.ViewObject.Visibility, 3000))
        self.assertIn("Linked annotation", text.AnnoText)
        self.s.ViewObject.HiddenAnnotations = [self.text]
        self.flush_gui(200)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "hidden.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.s = self.doc.getObject("Sketch")
            self.assertEqual(self.s.ViewObject.HiddenAnnotations, [self.text])
            self.assertTrue(
                self.wait_until(
                    lambda: all(
                        not o.ViewObject.Visibility
                        for o in self.linked()
                        if o.SourceAnnotationId == self.text
                    ),
                    3000,
                )
            )
