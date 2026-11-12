# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
from PySide import QtCore, QtGui

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestViewSectionGui(SketcherGuiTestCase):
    """Regression coverage for document-name encoding (FreeCAD #33325)."""

    def check_section_command(self, name):
        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument(name)
        # Exercise a Unicode internal identifier, not just a Unicode Label.
        self.assertEqual(self.doc.Name, name)
        self.assertIs(FreeCAD.getDocument(name), self.doc)
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.ViewObject.SectionView = False
        self.doc.recompute()
        self.flush_gui()
        gui_doc = FreeCADGui.getDocument(self.doc.Name)
        self.assertTrue(gui_doc.setEdit(sketch.Name))
        self.flush_gui()
        self.assertEqual(gui_doc.getInEdit().Object, sketch)
        self.assertIsNotNone(sketch.ViewObject.TempoVis)
        view = gui_doc.activeView()
        self.assertFalse(sketch.ViewObject.SectionView)
        self.assertFalse(view.hasClippingPlane())

        errors = []
        timer = QtCore.QTimer()
        timer.setInterval(10)

        def dismiss_error():
            dialog = QtGui.QApplication.activeModalWidget()
            if isinstance(dialog, QtGui.QMessageBox):
                errors.append(dialog.text())
                dialog.accept()

        timer.timeout.connect(dismiss_error)
        timer.start()
        try:
            for enabled in (True, False):
                # Commands may display Python exceptions in a modal dialog.
                # Dismiss and fail on those too, rather than hanging the suite.
                FreeCADGui.runCommand("Sketcher_ViewSection", 0)
                self.flush_gui()
                self.assertEqual(errors, [])
                self.assertEqual(sketch.ViewObject.SectionView, enabled)
                self.assertEqual(view.hasClippingPlane(), enabled)
        finally:
            timer.stop()

    def test_section_ascii_document_name(self):
        self.check_section_command("Ventilation")

    def test_section_latin1_document_name(self):
        self.check_section_command("Lüftung")

    def test_section_non_latin1_document_name(self):
        self.check_section_command("換気")
