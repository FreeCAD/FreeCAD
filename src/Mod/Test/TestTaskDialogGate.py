# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for selection gates of task dialogs.

To run tests:
    FreeCAD -t TestTaskDialogGate.TestTaskDialogGate
"""

import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtWidgets


class RejectAllGate:
    def allow(self, doc, obj, sub):
        return False


class GatedPanel:
    def __init__(self):
        self.form = QtWidgets.QWidget()
        FreeCADGui.Selection.addSelectionGate(RejectAllGate())


class TestTaskDialogGate(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestTaskDialogGate")
        self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        FreeCADGui.Selection.clearSelection()

    def tearDown(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Selection.removeSelectionGate()
        FreeCADGui.Selection.clearSelection()
        FreeCAD.closeDocument(self.doc.Name)

    def is_box_selectable(self):
        FreeCADGui.Selection.addSelection(self.doc.Name, "Box")
        selected = bool(FreeCADGui.Selection.getSelection())
        FreeCADGui.Selection.clearSelection()
        return selected

    def test_gate_set_before_show_dialog_stays_active(self):
        FreeCADGui.Control.showDialog(GatedPanel())

        self.assertFalse(self.is_box_selectable())

    def test_gate_stays_active_when_document_is_activated_again(self):
        FreeCADGui.Control.showDialog(GatedPanel())
        FreeCAD.setActiveDocument(self.doc.Name)
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)

        self.assertFalse(self.is_box_selectable())
