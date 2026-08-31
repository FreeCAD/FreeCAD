# SPDX-License-Identifier: LGPL-2.1-or-later

"""Sketcher-specific edit-mode assertions for GUI tests."""

from __future__ import annotations

from typing import Any

import FreeCAD
from Gui.Harness import FreeCADGui
from Gui.TestCase import FreeCADGuiTestCase


class SketcherGuiTestCase(FreeCADGuiTestCase):
    """Add exact Sketcher edit-target assertions to the generic test base."""

    def enter_sketch_edit(self, document: FreeCAD.Document, sketch: Any) -> Any:
        """Enter ``sketch`` edit mode and return its GUI document."""
        gui_document = self.gui.enter_edit(document, sketch.Name)
        self.assert_sketch_edit_active(gui_document, sketch)
        return gui_document

    @staticmethod
    def _active_gui_document(gui_document: Any | None = None) -> Any | None:
        return gui_document if gui_document is not None else FreeCADGui.ActiveDocument

    def assert_sketch_edit_active(
        self,
        gui_document: Any | None = None,
        sketch: Any | None = None,
    ) -> None:
        """Assert that Sketcher is editing, optionally checking its exact object."""
        gui_document = self._active_gui_document(gui_document)
        edit = gui_document.getInEdit() if gui_document is not None else None
        self.assertIsNotNone(edit, "Expected sketch edit mode to remain active")
        self.assertTrue(
            edit.isDerivedFrom("SketcherGui::ViewProviderSketch"),
            "Expected a Sketcher view provider to remain in edit mode",
        )
        if sketch is not None:
            edit_object = getattr(edit, "Object", None)
            self.assertEqual(
                getattr(edit_object, "Name", None),
                sketch.Name,
                "Expected the requested sketch to be the active edit target",
            )

    def assert_sketch_edit_inactive(self, gui_document: Any | None = None) -> None:
        """Assert that the selected GUI document is not in Sketcher edit mode."""
        gui_document = self._active_gui_document(gui_document)
        edit = gui_document.getInEdit() if gui_document is not None else None
        self.assertIsNone(edit, "Expected sketch edit mode to be inactive")
