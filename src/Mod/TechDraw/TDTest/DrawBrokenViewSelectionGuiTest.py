# SPDX-License-Identifier: LGPL-2.1-or-later
"""TechDraw_BrokenView creates a broken view from a selected object and break sketch."""

from pathlib import Path
import unittest
from uuid import uuid4

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtWidgets


@unittest.skipUnless("TechDrawWorkbench" in Gui.listWorkbenches(), "TechDraw is unavailable")
class DrawBrokenViewSelectionGuiTest(unittest.TestCase):
    def setUp(self):
        self.previous_document = App.activeDocument().Name if App.activeDocument() else None
        workbench = Gui.activeWorkbench()
        self.previous_workbench = workbench.name() if workbench else None
        self.doc_name = None
        self.addCleanup(self._cleanup)
        Gui.activateWorkbench("TechDrawWorkbench")
        self.doc = App.newDocument("BrokenViewSel_" + uuid4().hex[:10])
        self.doc_name = self.doc.Name
        self.closer = QtCore.QTimer()
        self.closer.timeout.connect(self._closeModal)

    def _cleanup(self):
        self.closer.stop()
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        Gui.Selection.clearSelection()
        if self.doc_name in App.listDocuments():
            App.closeDocument(self.doc_name)
        if self.previous_document in App.listDocuments():
            App.setActiveDocument(self.previous_document)
        if self.previous_workbench:
            Gui.activateWorkbench(self.previous_workbench)

    @staticmethod
    def _closeModal():
        # never leave a modal message box open
        dialog = QtWidgets.QApplication.instance().activeModalWidget()
        if dialog is not None:
            dialog.reject()

    def test_object_and_break_sketch_create_broken_view(self):
        import Part
        import Sketcher  # noqa: F401

        page = self.doc.addObject("TechDraw::DrawPage", "Page")
        template = self.doc.addObject("TechDraw::DrawSVGTemplate", "Template")
        template.Template = str(Path(__file__).with_name("TestTemplate.svg"))
        page.Template = template
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 200
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.addGeometry(Part.LineSegment(App.Vector(80, -5, 0), App.Vector(80, 15, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(120, -5, 0), App.Vector(120, 15, 0)))
        self.doc.recompute()

        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.doc.Name, box.Name)
        Gui.Selection.addSelection(self.doc.Name, sketch.Name)
        self.closer.start(100)
        Gui.runCommand("TechDraw_BrokenView")
        self.closer.stop()

        views = [obj for obj in self.doc.Objects if obj.isDerivedFrom("TechDraw::DrawBrokenView")]
        self.assertEqual(len(views), 1)
        view = views[0]
        self.assertEqual(view.Source, [box])
        self.assertEqual(view.Breaks, [sketch])
        self.assertIn(view, page.Views)
        self.assertFalse(Gui.Control.activeDialog())


if __name__ == "__main__":
    unittest.main()
