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

import time

from PySide import QtGui

import Arch
import FreeCADGui

from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchMaterialGui(TestArchBaseGui):
    def setUp(self):
        super().setUp()

        self.material = Arch.makeMaterial(name="Material")
        self.material_001 = Arch.makeMaterial(name="Material001")
        self.multi_material = Arch.makeMultiMaterial(name="TestMultiMaterial")
        self.multi_material.Names = ["Solid panel", "Frame"]
        self.multi_material.Materials = [self.material, self.material_001]
        self.multi_material.Thicknesses = [200.0, 100.0]
        self.document.recompute()

    def _matching_multimaterial_tree(self):
        expected_names = list(self.multi_material.Names)
        expected_materials = [material.Label for material in self.multi_material.Materials]
        main_window = FreeCADGui.getMainWindow()
        for tree in main_window.findChildren(QtGui.QTreeView, "tree"):
            if not tree.isVisible():
                continue
            model = tree.model()
            if model is None or model.columnCount() < 3 or model.rowCount() != len(expected_names):
                continue
            names = [model.index(row, 0).data() for row in range(model.rowCount())]
            materials = [model.index(row, 1).data() for row in range(model.rowCount())]
            if names == expected_names and materials == expected_materials:
                return tree
        return None

    def _wait_until(self, predicate, message, timeout_ms=2000, step_ms=50):
        deadline = time.monotonic() + (timeout_ms / 1000.0)
        while time.monotonic() < deadline:
            try:
                if predicate():
                    return
            except RuntimeError:
                pass
            self.pump_gui_events(step_ms)

        try:
            if predicate():
                return
        except RuntimeError:
            pass

        self.fail(message)

    def _best_effort_wait(self, predicate, timeout_ms=1000, step_ms=50):
        deadline = time.monotonic() + (timeout_ms / 1000.0)
        while time.monotonic() < deadline:
            try:
                if predicate():
                    return True
            except RuntimeError:
                return True
            self.pump_gui_events(step_ms)

        try:
            return predicate()
        except RuntimeError:
            return True

    def _find_multimaterial_tree(self):
        holder = {}

        def _tree_available():
            tree = self._matching_multimaterial_tree()
            if tree is None:
                return False
            holder["tree"] = tree
            return True

        self._wait_until(
            _tree_available, "Could not find the visible MultiMaterial task panel tree."
        )
        return holder["tree"]

    @staticmethod
    def _visible_input_fields(tree):
        return [
            editor
            for editor in tree.findChildren(QtGui.QWidget)
            if editor.metaObject().className() == "Gui::InputField" and editor.isVisible()
        ]

    def _wait_for_visible_input_field(self, tree):
        holder = {}

        def _editor_available():
            editors = self._visible_input_fields(tree)
            if not editors:
                return False
            holder["editor"] = editors[0]
            return True

        self._wait_until(
            _editor_available, "Expected an active Gui::InputField editor in the thickness column."
        )
        return holder["editor"]

    def _assert_editor_row_state(self, tree, active_index, inactive_index):
        tree.setCurrentIndex(active_index)
        tree.openPersistentEditor(active_index)

        try:
            editor = self._wait_for_visible_input_field(tree)
            editor.ensurePolished()
            editor_height = max(editor.sizeHint().height(), editor.minimumSizeHint().height())

            self._wait_until(
                lambda: tree.visualRect(active_index).height() >= editor_height,
                f"Thickness row did not grow to the InputField height {editor_height}.",
            )
            row_height = tree.visualRect(active_index).height()
            other_row_height = tree.visualRect(inactive_index).height()
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
        finally:
            try:
                tree.closePersistentEditor(active_index)
            except RuntimeError:
                return
            self._best_effort_wait(lambda: not self._visible_input_fields(tree))

    def _close_multimaterial_edit_session(self):
        try:
            self.multi_material.ViewObject.Proxy.unsetEdit(self.multi_material.ViewObject, 0)
        except RuntimeError:
            return
        self._best_effort_wait(lambda: self._matching_multimaterial_tree() is None)

    def test_multimaterial_delegate_reserves_inputfield_height(self):
        """The real edit lifecycle should grow only the active thickness row to fit the editor."""
        edit_started = False
        try:
            self.assertTrue(
                self.multi_material.ViewObject.Proxy.setEdit(self.multi_material.ViewObject, 0)
            )
            edit_started = True
            tree = self._find_multimaterial_tree()
            self.assertFalse(tree.uniformRowHeights())
            index = tree.model().index(0, 2)
            other_index = tree.model().index(1, 2)
            self._assert_editor_row_state(tree, index, other_index)
        finally:
            if edit_started:
                self._close_multimaterial_edit_session()
