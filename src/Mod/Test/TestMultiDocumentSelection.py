# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for selection across multiple documents.

To run tests:
    FreeCAD -t TestMultiDocumentSelection.TestMultiDocumentSelection
"""

import time
import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtCore, QtGui, QtWidgets


class TestMultiDocumentSelection(unittest.TestCase):
    def setUp(self):
        self.docs = []

    def tearDown(self):
        FreeCADGui.Selection.clearSelection()
        for doc in reversed(self.docs):
            if doc.Name in FreeCAD.listDocuments():
                FreeCAD.closeDocument(doc.Name)

    def test_global_clear_selection_clears_inactive_document_contexts(self):
        doc_a, obj_a = self._new_document_with_object("Issue30778_A")
        FreeCADGui.Selection.addSelection(doc_a.Name, obj_a.Name)

        doc_b, obj_b = self._new_document_with_object("Issue30778_B")
        FreeCADGui.Selection.addSelection(doc_b.Name, obj_b.Name)

        self.assertEqual(self._selected_names(doc_a), [obj_a.Name])
        self.assertEqual(self._selected_names(doc_b), [obj_b.Name])

        self._activate_document(doc_b)
        FreeCADGui.Selection.clearSelection()

        self.assertEqual(FreeCAD.ActiveDocument.Name, doc_b.Name)
        self.assertEqual(FreeCADGui.ActiveDocument.Document.Name, doc_b.Name)
        self.assertEqual(self._selected_names(doc_a), [])
        self.assertEqual(self._selected_names(doc_b), [])

    def test_empty_viewport_click_clears_inactive_document_contexts(self):
        doc_a, obj_a = self._new_document_with_object("Issue30778_Click_A")
        FreeCADGui.Selection.addSelection(doc_a.Name, obj_a.Name)

        doc_b, _ = self._new_document_with_object("Issue30778_Click_B")
        self._activate_document(doc_b)
        viewport = self._prepare_active_view()

        self.assertEqual(self._selected_names(doc_a), [obj_a.Name])

        self._click_viewport(viewport, viewport.rect().center())

        self.assertEqual(FreeCAD.ActiveDocument.Name, doc_b.Name)
        self.assertEqual(FreeCADGui.ActiveDocument.Document.Name, doc_b.Name)
        self.assertEqual(self._selected_names(doc_a), [])
        self.assertEqual(self._selected_names(doc_b), [])

    def _new_document_with_object(self, name):
        doc = FreeCAD.newDocument(name)
        self.docs.append(doc)
        self._activate_document(doc)
        obj = doc.addObject("App::DocumentObjectGroup", "Rectangle")
        doc.recompute()
        return doc, obj

    def _activate_document(self, doc):
        FreeCAD.setActiveDocument(doc.Name)
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(doc.Name)
        FreeCADGui.updateGui()

    def _selected_names(self, doc):
        return [obj.Name for obj in FreeCADGui.Selection.getSelection(doc.Name)]

    def _prepare_active_view(self):
        view = FreeCADGui.ActiveDocument.ActiveView
        view.getViewer().setEnabledNaviCube(False)
        view.setAxisCross(False)
        view.viewIsometric()
        view.fitAll()

        graphics_view = view.graphicsView()
        viewport = graphics_view.viewport()
        viewport.setFocus(QtCore.Qt.FocusReason.OtherFocusReason)
        self._process_events()
        self._process_events()
        return viewport

    def _click_viewport(self, viewport, pos):
        self._send_mouse_event(
            viewport,
            QtCore.QEvent.Type.MouseMove,
            pos,
            QtCore.Qt.MouseButton.NoButton,
            QtCore.Qt.MouseButton.NoButton,
        )
        self._process_events(10)
        self._send_mouse_event(
            viewport,
            QtCore.QEvent.Type.MouseButtonPress,
            pos,
            QtCore.Qt.MouseButton.LeftButton,
            QtCore.Qt.MouseButton.LeftButton,
        )
        self._process_events(10)
        self._send_mouse_event(
            viewport,
            QtCore.QEvent.Type.MouseButtonRelease,
            pos,
            QtCore.Qt.MouseButton.LeftButton,
            QtCore.Qt.MouseButton.NoButton,
        )
        self._process_events()

    def _send_mouse_event(self, viewport, event_type, pos, button, buttons):
        app = QtWidgets.QApplication.instance()
        global_pos = viewport.mapToGlobal(pos)
        QtGui.QCursor.setPos(global_pos)
        event = QtGui.QMouseEvent(
            event_type,
            QtCore.QPointF(pos),
            QtCore.QPointF(global_pos),
            button,
            buttons,
            QtCore.Qt.KeyboardModifier.NoModifier,
        )
        app.sendEvent(viewport, event)

    def _process_events(self, wait_ms=50):
        FreeCADGui.updateGui()
        app = QtWidgets.QApplication.instance()
        app.processEvents()
        time.sleep(wait_ms / 1000.0)
        app.processEvents()
