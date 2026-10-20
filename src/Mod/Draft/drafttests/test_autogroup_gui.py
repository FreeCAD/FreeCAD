# SPDX-License-Identifier: LGPL-2.1-or-later

"""Document-switching regressions for the Draft AutoGroup toolbar."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui

if App.GuiUp:
    import DraftGui


@unittest.skipUnless(App.GuiUp, "AutoGroup toolbar tests require GUI mode")
class DraftAutoGroup(unittest.TestCase):
    def setUp(self):
        self.documents = []
        self.toolbar = DraftGui.DraftToolBar()

    def tearDown(self):
        App.removeDocumentObserver(self.toolbar)
        for document in reversed(self.documents):
            if document in App.listDocuments():
                App.closeDocument(document)
        self.toolbar.tray.deleteLater()
        self.toolbar.baseWidget.deleteLater()
        Gui.updateGui()

    def new_document(self, name):
        document = App.newDocument(name)
        self.documents.append(document.Name)
        return document

    def add_group(self, document, label):
        group = document.addObject("App::DocumentObjectGroup", "AutoGroup")
        group.Label = label
        self.toolbar.setAutoGroup(group.Name)
        return group

    def test_switching_documents_restores_each_group_and_label(self):
        first = self.new_document("AutoGroupFirst")
        first_group = self.add_group(first, "First floor")
        second = self.new_document("AutoGroupSecond")
        self.assertIsNone(self.toolbar.autogroup)

        second_group = self.add_group(second, "Second floor")
        self.assertEqual(first_group.Name, second_group.Name)
        App.setActiveDocument(first.Name)
        self.assertEqual(self.toolbar.autogroup, first_group.Name)
        self.assertEqual(self.toolbar.autoGroupButton.text(), first_group.Label)

        App.setActiveDocument(second.Name)
        self.assertEqual(self.toolbar.autogroup, second_group.Name)
        self.assertEqual(self.toolbar.autoGroupButton.text(), second_group.Label)

    def test_closing_last_document_clears_toolbar(self):
        document = self.new_document("AutoGroupLast")
        self.add_group(document, "Last floor")
        App.closeDocument(document.Name)

        self.assertIsNone(self.toolbar.autogroup)
        self.assertFalse(self.toolbar.tray.isEnabled())
        self.assertEqual(
            self.toolbar.autoGroupButton.toolTip(), DraftGui.translate("draft", "Autogroup off")
        )

    def test_closed_document_state_is_not_reused(self):
        first = self.new_document("AutoGroupReused")
        first_name = first.Name
        self.add_group(first, "Old group")
        self.new_document("AutoGroupOther")
        App.closeDocument(first_name)

        replacement = self.new_document(first_name)
        replacement.addObject("App::DocumentObjectGroup", "AutoGroup")
        self.assertIsNone(self.toolbar.autogroup)

    def test_deleted_group_is_not_restored(self):
        first = self.new_document("AutoGroupDeleted")
        group = self.add_group(first, "Deleted group")
        self.new_document("AutoGroupOther")
        first.removeObject(group.Name)

        App.setActiveDocument(first.Name)
        self.assertIsNone(self.toolbar.autogroup)

    def test_closing_inactive_document_preserves_active_group(self):
        first = self.new_document("AutoGroupInactive")
        self.add_group(first, "Inactive floor")
        second = self.new_document("AutoGroupActive")
        group = self.add_group(second, "Active floor")

        App.closeDocument(first.Name)

        self.assertEqual(self.toolbar.autogroup, group.Name)
        self.assertEqual(self.toolbar.autoGroupButton.text(), group.Label)
        self.assertTrue(self.toolbar.tray.isEnabled())

    def test_cleared_selection_remains_cleared_after_switch(self):
        first = self.new_document("AutoGroupCleared")
        self.add_group(first, "Cleared group")
        self.toolbar.setAutoGroup(None)
        self.new_document("AutoGroupOther")

        App.setActiveDocument(first.Name)
        self.assertIsNone(self.toolbar.autogroup)
