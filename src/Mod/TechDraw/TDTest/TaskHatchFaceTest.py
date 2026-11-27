# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run with TestGui or unittest: TDTest.TaskHatchFaceTest (requires the GUI)."""

from pathlib import Path
import tempfile
import time
import unittest

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtGui

from .TechDrawTestUtilities import createPageWithSVGTemplate


class TaskHatchFaceTest(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("TechDrawWorkbench")
        self.document = App.newDocument("TaskHatchFaceTest")
        self.document_name = self.document.Name
        self.document.UndoMode = 1
        self.preferences = App.ParamGet("User parameter:BaseApp/Preferences/Mod/TechDraw/Hatch")
        self.preference_keys = (
            "HatchDefaultFile",
            "HatchDefaultName",
            "HatchLastUsedFile",
            "HatchLastUsedName",
        )
        self.saved_preferences = {
            key: self.preferences.GetString(key) for key in self.preference_keys
        }
        self.preferences.SetString(
            "HatchDefaultFile", App.getResourceDir() + "Mod/TechDraw/Patterns/simple.svg"
        )
        self.preferences.SetString("HatchDefaultName", "")
        self.page = createPageWithSVGTemplate(self.document)
        box = self.document.addObject("Part::Box", "Box")
        self.view = self.document.addObject("TechDraw::DrawViewPart", "View")
        self.view.Source = [box]
        self.view.Direction = (1, 1, 1)
        self.page.addView(self.view)
        self.document.recompute()
        deadline = time.monotonic() + 5
        while not self.view.getVisibleEdges() and time.monotonic() < deadline:
            Gui.updateGui()
        self.assertTrue(self.view.getVisibleEdges())
        Gui.Selection.clearSelection()

    def tearDown(self):
        if Gui.Control.activeDialog():
            Gui.Control.activeTaskDialog().reject()
            Gui.Control.closeDialog()
        Gui.Selection.clearSelection()
        if self.document_name in App.listDocuments():
            App.closeDocument(self.document_name)
        for key, value in self.saved_preferences.items():
            if value:
                self.preferences.SetString(key, value)
            else:
                self.preferences.RemString(key)

    def make_hatch(self, pattern_type="SVG", faces=("Face0",)):
        object_type = "TechDraw::DrawHatch" if pattern_type == "SVG" else "TechDraw::DrawGeomHatch"
        hatch = self.document.addObject(object_type, "Hatch")
        hatch.Source = (self.view, list(faces))
        self.document.recompute()
        return hatch

    def open_edit(self, hatch, command="TechDraw_HatchFace"):
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(hatch)
        Gui.runCommand(command, 0)
        return self.form()

    def form(self):
        self.assertTrue(Gui.Control.activeDialog())
        form = Gui.getMainWindow().findChild(QtGui.QWidget, "TechDrawGui__TaskHatchFace")
        self.assertIsNotNone(form)
        return form

    def finish(self, accept):
        dialog = Gui.Control.activeTaskDialog()
        if accept:
            dialog.accept()
        else:
            dialog.reject()
        self.assertFalse(Gui.Control.activeDialog())

    def choose_type(self, form, pattern_type):
        combo = form.findChild(QtGui.QComboBox, "patternComboBox")
        for index in range(combo.count()):
            text = combo.itemText(index).lower()
            if (pattern_type == "PAT" and ".pat - " in text) or (
                pattern_type == "SVG" and text == "simple.svg"
            ):
                combo.setCurrentIndex(index)
                expected = (
                    "TechDraw::DrawHatch" if pattern_type == "SVG" else "TechDraw::DrawGeomHatch"
                )
                self.assertTrue(
                    any(obj.TypeId == expected and obj.Source for obj in self.hatches())
                )
                return
        self.fail("No bundled " + pattern_type + " pattern found")

    def hatches(self):
        return [
            obj
            for obj in self.document.Objects
            if obj.TypeId in ("TechDraw::DrawHatch", "TechDraw::DrawGeomHatch")
        ]

    def replace_faces(self, faces=("Face0",)):
        Gui.Selection.clearSelection()
        for face in faces:
            Gui.Selection.addSelection(self.view, face)

        def confirm_replacement():
            dialog = QtGui.QApplication.activeModalWidget()
            if isinstance(dialog, QtGui.QMessageBox):
                dialog.button(QtGui.QMessageBox.Yes).click()

        QtCore.QTimer.singleShot(0, confirm_replacement)
        Gui.runCommand("TechDraw_HatchFace", 0)
        return self.form()

    def test_cancel_replacement_restores_both_hatch_types(self):
        svg = self.make_hatch("SVG", ("Face0", "Face1"))
        pat = self.make_hatch("PAT")
        svg_name, pat_name = svg.Name, pat.Name
        self.replace_faces()
        self.assertEqual(self.document.getObject(svg_name).Source[1], ["Face1"])
        self.assertFalse(any(obj.TypeId == "TechDraw::DrawGeomHatch" for obj in self.hatches()))
        self.finish(False)
        self.assertEqual(self.document.getObject(svg_name).Source[1], ["Face0", "Face1"])
        self.assertEqual(self.document.getObject(pat_name).Source[1], ["Face0"])
        self.assertEqual(len(self.hatches()), 2)

    def test_replacement_is_one_undo_step(self):
        hatch = self.make_hatch()
        name = hatch.Name
        hatch.Label = "Original hatch"
        self.replace_faces()
        self.finish(True)
        self.assertEqual(len(self.hatches()), 1)
        self.document.undo()
        self.assertEqual(len(self.hatches()), 1)
        self.assertEqual(self.document.getObject(name).Label, "Original hatch")
        self.document.redo()
        self.assertEqual(len(self.hatches()), 1)

    def test_cancel_type_switch_restores_original_and_links(self):
        hatch = self.make_hatch()
        name = hatch.Name
        hatch.Label = "Custom hatch label"
        group = self.document.addObject("App::DocumentObjectGroup", "HatchGroup")
        group.addObject(hatch)
        form = self.open_edit(hatch)
        self.choose_type(form, "PAT")
        self.choose_type(form, "SVG")
        self.choose_type(form, "PAT")
        self.finish(False)
        restored = self.document.getObject(name)
        self.assertEqual(restored.Label, "Custom hatch label")
        self.assertEqual(group.Group, [restored])
        self.assertEqual(restored.Source[1], ["Face0"])
        self.assertEqual(len(self.hatches()), 1)

    def test_accept_type_switch_preserves_label_and_group(self):
        hatch = self.make_hatch()
        name = hatch.Name
        hatch.Label = "Custom hatch label"
        group = self.document.addObject("App::DocumentObjectGroup", "HatchGroup")
        group.addObject(hatch)
        form = self.open_edit(hatch)
        self.choose_type(form, "PAT")
        self.finish(True)
        self.assertEqual(len(self.hatches()), 1)
        replacement = self.hatches()[0]
        self.assertEqual(replacement.TypeId, "TechDraw::DrawGeomHatch")
        self.assertEqual(replacement.Label, "Custom hatch label")
        self.assertEqual(group.Group, [replacement])
        self.document.undo()
        self.assertEqual(group.Group, [self.document.getObject(name)])
        self.assertEqual(len(self.hatches()), 1)
        self.document.redo()
        self.assertEqual(group.Group, self.hatches())

    def test_orphan_hatch_can_be_edited_and_cancelled(self):
        for pattern_type in ("SVG", "PAT"):
            with self.subTest(pattern_type=pattern_type):
                hatch = self.make_hatch(pattern_type)
                hatch.Source = None
                form = self.open_edit(hatch)
                form.findChild(QtGui.QDoubleSpinBox, "rotationSpinBox").setValue(37.0)
                rotation = (
                    hatch.ViewObject.HatchRotation
                    if pattern_type == "SVG"
                    else hatch.PatternRotation
                )
                self.assertEqual(rotation, 37.0)
                self.finish(False)
                self.assertIsNone(hatch.Source)
                rotation = (
                    hatch.ViewObject.HatchRotation
                    if pattern_type == "SVG"
                    else hatch.PatternRotation
                )
                self.assertEqual(rotation, 0.0)

    def test_legacy_commands_open_unified_editor(self):
        hatch = self.make_hatch()
        for command in ("TechDraw_Hatch", "TechDraw_GeometricHatch"):
            with self.subTest(command=command):
                self.open_edit(hatch, command)
                self.finish(False)

    def test_edit_uses_hatch_document_after_active_document_changes(self):
        hatch = self.make_hatch()
        form = self.open_edit(hatch)
        other = App.newDocument("OtherHatchDocument")
        try:
            form.findChild(QtGui.QDoubleSpinBox, "rotationSpinBox").setValue(37.0)
            App.setActiveDocument(self.document.Name)
            self.finish(True)
            self.assertEqual(hatch.ViewObject.HatchRotation, 37.0)
            self.assertEqual(other.Objects, [])
        finally:
            App.closeDocument(other.Name)
            App.setActiveDocument(self.document.Name)

    def test_saved_default_uses_file_and_pat_name(self):
        hatch = self.make_hatch("PAT")
        self.preferences.SetString("HatchDefaultFile", hatch.FilePattern)
        self.preferences.SetString("HatchDefaultName", hatch.NamePattern)
        self.replace_faces(("Face1",))
        self.finish(True)
        self.assertEqual(len(self.hatches()), 2)
        replacement = next(obj for obj in self.hatches() if obj.Name != hatch.Name)
        self.assertEqual(replacement.TypeId, "TechDraw::DrawGeomHatch")
        self.assertEqual(Path(replacement.FilePattern), Path(hatch.FilePattern))
        self.assertEqual(replacement.NamePattern, hatch.NamePattern)
        self.assertEqual(self.preferences.GetString("HatchLastUsedName"), hatch.NamePattern)

    def test_switching_back_keeps_original_object(self):
        hatch = self.make_hatch()
        name = hatch.Name
        form = self.open_edit(hatch)
        self.choose_type(form, "PAT")
        self.choose_type(form, "SVG")
        self.finish(True)
        self.assertEqual(self.hatches(), [self.document.getObject(name)])
        self.assertEqual(self.document.getObject(name).Source[1], ["Face0"])

    def test_missing_external_pat_preserves_embedded_pattern(self):
        hatch = self.make_hatch("PAT")
        with tempfile.TemporaryDirectory() as directory:
            pattern = Path(directory) / "custom.pat"
            pattern.write_bytes(Path(hatch.FilePattern).read_bytes())
            hatch.FilePattern = str(pattern)
            included = Path(hatch.PatIncluded).read_bytes()
            pattern.unlink()
            form = self.open_edit(hatch)
            form.findChild(QtGui.QDoubleSpinBox, "rotationSpinBox").setValue(15.0)
            self.finish(True)
            self.assertEqual(hatch.PatternRotation, 15.0)
            self.assertEqual(Path(hatch.PatIncluded).read_bytes(), included)

    def test_closing_document_closes_the_editor(self):
        hatch = self.make_hatch()
        form = self.open_edit(hatch)
        self.choose_type(form, "PAT")
        App.closeDocument(self.document_name)
        self.assertFalse(Gui.Control.activeDialog())

    def test_mixed_selection_and_multiple_views_are_disabled(self):
        command = Gui.Command.get("TechDraw_HatchFace")
        Gui.Selection.addSelection(self.view, "Face0")
        self.assertTrue(command.isActive())
        Gui.Selection.addSelection(self.view, "Edge0")
        self.assertFalse(command.isActive())
        Gui.Selection.clearSelection()
        other = self.document.addObject("TechDraw::DrawViewPart", "OtherView")
        self.page.addView(other)
        Gui.Selection.addSelection(self.view, "Face0")
        Gui.Selection.addSelection(other)
        self.assertFalse(command.isActive())

    def test_missing_saved_default_falls_back_to_available_pattern(self):
        self.preferences.SetString("HatchDefaultFile", "/nonexistent/freecad-hatch-test.svg")
        self.preferences.SetString("HatchLastUsedFile", "/nonexistent/freecad-hatch-test.pat")
        self.replace_faces()
        self.finish(True)
        self.assertEqual(len(self.hatches()), 1)
        hatch = self.hatches()[0]
        path = hatch.HatchPattern if hatch.TypeId == "TechDraw::DrawHatch" else hatch.FilePattern
        self.assertTrue(Path(path).is_file())


if __name__ == "__main__":
    unittest.main()
