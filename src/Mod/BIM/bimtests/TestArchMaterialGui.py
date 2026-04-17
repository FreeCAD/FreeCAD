# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 Czarflix                                           *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""GUI tests for the ArchMaterial module."""

from PySide import QtGui

import Arch
import FreeCADGui

from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchMaterialGui(TestArchBaseGui):
    def setUp(self):
        super().setUp()
        self.panel = None

        self.material = Arch.makeMaterial(name="Material")
        self.material_001 = Arch.makeMaterial(name="Material001")
        self.multi_material = Arch.makeMultiMaterial(name="TestMultiMaterial")
        self.multi_material.Names = ["Solid panel", "Frame"]
        self.multi_material.Materials = [self.material, self.material_001]
        self.multi_material.Thicknesses = [200.0, 100.0]
        self.document.recompute()

    def tearDown(self):
        if self.panel:
            self.panel.form.close()
            self.panel = None
        super().tearDown()

    def _find_multimaterial_tree(self):
        main_window = FreeCADGui.getMainWindow()
        for tree in main_window.findChildren(QtGui.QTreeView, "tree"):
            if not tree.isVisible():
                continue
            model = tree.model()
            if model is None or model.columnCount() < 3 or model.rowCount() < 2:
                continue
            header_item = model.horizontalHeaderItem(2)
            if header_item and "Thickness" in header_item.text():
                return tree
        self.fail("Could not find the visible MultiMaterial task panel tree.")

    @staticmethod
    def _visible_input_fields(tree):
        return [
            editor
            for editor in tree.findChildren(QtGui.QWidget)
            if editor.metaObject().className() == "Gui::InputField" and editor.isVisible()
        ]

    def test_multimaterial_delegate_reserves_inputfield_height(self):
        """The real edit lifecycle should grow only the active thickness row, then shrink it again."""
        self.assertTrue(self.multi_material.ViewObject.Proxy.setEdit(self.multi_material.ViewObject, 0))
        self.pump_gui_events()

        tree = self._find_multimaterial_tree()
        self.assertFalse(tree.uniformRowHeights())
        index = tree.model().index(0, 2)
        other_index = tree.model().index(1, 2)
        tree.setCurrentIndex(index)
        tree.openPersistentEditor(index)
        self.pump_gui_events()

        editors = self._visible_input_fields(tree)
        self.assertTrue(editors, "Expected an active Gui::InputField editor in the thickness column.")
        editor = editors[0]
        editor.ensurePolished()
        editor_height = max(editor.sizeHint().height(), editor.minimumSizeHint().height())

        row_height = tree.visualRect(index).height()
        other_row_height = tree.visualRect(other_index).height()
        self.assertGreaterEqual(
            row_height,
            editor_height,
            f"Thickness row height {row_height} is smaller than the InputField height {editor_height}.",
        )
        self.assertLess(
            other_row_height,
            row_height,
            "Only the edited thickness row should expand for the InputField editor.",
        )

        tree.closePersistentEditor(index)
        self.pump_gui_events()

        self.assertFalse(
            self._visible_input_fields(tree),
            "Expected the active Gui::InputField editor to close after ending the edit.",
        )
        closed_row_height = tree.visualRect(index).height()
        self.assertLess(
            closed_row_height,
            row_height,
            "The edited thickness row should shrink again after the InputField editor closes.",
        )
        self.assertLessEqual(
            closed_row_height,
            other_row_height,
            "Closing the editor should restore the edited row to the non-edited row height.",
        )
