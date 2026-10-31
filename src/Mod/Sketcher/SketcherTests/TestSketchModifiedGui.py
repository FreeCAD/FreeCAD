# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Part
import Sketcher

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestSketchModifiedGui(SketcherGuiTestCase):
    """Regression tests for issue #34015.

    Merely opening a sketch (entering edit mode) used to write the freshly solved
    geometry back through ``solve(true)`` and run the visibility automation. Both
    touched objects and flagged the document as modified even though the user changed
    nothing, so the window caption gained a "*" and FreeCAD prompted to save on close.
    These tests assert that visiting a sketch without editing it leaves the document
    unmodified.
    """

    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("SketchModifiedGuiTest")

    def _add_closed_rectangle(self, name):
        sketch = self.doc.addObject("Sketcher::SketchObject", name)
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)), False
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0)), False
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(10, 10, 0), FreeCAD.Vector(0, 10, 0)), False
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 10, 0), FreeCAD.Vector(0, 0, 0)), False
        )
        sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 0))
        sketch.addConstraint(Sketcher.Constraint("Vertical", 1))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 2, 20.0))
        sketch.addConstraint(Sketcher.Constraint("DistanceY", 0, 2, 15.0))
        return sketch

    def _gui_document(self):
        return FreeCADGui.getDocument(self.doc.Name)

    def _clean_sketch(self, name):
        """Return a fully-defined sketch with the document in an unmodified state.

        A freshly created document is flagged as modified. Clear the flag to model a
        saved, clean project so the tests can detect any spurious modified state that
        entering or leaving edit mode introduces.
        """
        sketch = self._add_closed_rectangle(name)
        self.doc.recompute()
        self.flush_gui()

        gui_doc = self._gui_document()
        gui_doc.Modified = False
        self.flush_gui()
        return sketch

    def test_opening_sketch_does_not_mark_document_modified(self):
        sketch = self._clean_sketch("OpenOnly")
        gui_doc = self._gui_document()
        self.assertFalse(
            gui_doc.Modified, "Baseline: a clean document must be unmodified."
        )

        FreeCADGui.ActiveDocument = gui_doc
        gui_doc.setEdit(sketch.Name)
        self.flush_gui()
        try:
            self.assertFalse(
                gui_doc.Modified,
                "Merely opening a sketch must not mark the document as modified (#34015).",
            )
        finally:
            gui_doc.resetEdit()
            self.flush_gui()

    def test_opening_then_closing_sketch_does_not_mark_document_modified(self):
        sketch = self._clean_sketch("OpenThenClose")
        gui_doc = self._gui_document()
        self.assertFalse(
            gui_doc.Modified, "Baseline: a clean document must be unmodified."
        )

        FreeCADGui.ActiveDocument = gui_doc
        gui_doc.setEdit(sketch.Name)
        self.flush_gui()
        gui_doc.resetEdit()
        self.flush_gui()

        self.assertFalse(
            gui_doc.Modified,
            "Opening then closing a sketch without changes must not mark the document "
            "as modified (#34015).",
        )
