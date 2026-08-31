# SPDX-License-Identifier: LGPL-2.1-or-later

"""High-value regression tests for the shared Python GUI support."""

from types import SimpleNamespace
import time
from unittest.mock import Mock, patch

from PySide import QtCore, QtWidgets

import FreeCAD
from Gui.TestCase import FreeCADGuiTestCase
from Support import temporary_preference


class TestGuiSupport(FreeCADGuiTestCase):
    def setUp(self):
        super().setUp()

        self.window = QtWidgets.QDialog(self.gui.main_window())
        self.window.setObjectName("GuiTestSupportWindow")
        self.window.setModal(False)
        self.window.setGeometry(10, 10, 320, 120)
        self.window.show()
        self.window.activateWindow()
        self.window.raise_()
        self.gui.flush_gui()

    def tearDown(self):
        try:
            self.window.close()
            self.window.deleteLater()
            self.gui.flush_gui(20)
        finally:
            super().tearDown()

    def test_preference_guard_restores_original_value(self):
        path = "User parameter:BaseApp/Preferences/Mod/Test/GuiSupport"
        key = "TemporaryInteger"
        group = FreeCAD.ParamGet(path)
        original = next(
            (entry for entry in (group.GetContents() or []) if entry[1] == key),
            None,
        )

        with temporary_preference(path, key, 42):
            self.assertEqual(group.GetInt(key), 42)

        restored = next(
            (entry for entry in (group.GetContents() or []) if entry[1] == key),
            None,
        )
        self.assertEqual(restored, original)

    def test_preference_guard_restores_original_across_type_change(self):
        path = "User parameter:BaseApp/Preferences/Mod/Test/GuiSupport"
        key = "TemporaryMixedType"
        group = FreeCAD.ParamGet(path)

        with temporary_preference(path, key, "original", value_type="String"):
            original = tuple(entry for entry in (group.GetContents() or []) if entry[1] == key)

            with temporary_preference(path, key, 42):
                self.assertEqual(group.GetInt(key), 42)

            restored = tuple(entry for entry in (group.GetContents() or []) if entry[1] == key)
            self.assertEqual(restored, original)

    def test_enter_edit_rejects_different_current_object(self):
        document = SimpleNamespace(Name="GuiSupportEditTarget")
        current_edit = SimpleNamespace(Object=SimpleNamespace(Name="FirstSketch"))
        gui_document = Mock()
        gui_document.getInEdit.return_value = current_edit
        gui_api = Mock()
        gui_api.getDocument.return_value = gui_document

        with patch("Gui.Harness.FreeCADGui", gui_api), patch(
            "Gui.Harness.Wait.gui_available", return_value=True
        ):
            with self.assertRaisesRegex(RuntimeError, "already being edited"):
                self.gui.enter_edit(document, "SecondSketch")

        gui_document.setEdit.assert_not_called()

    def test_late_modal_is_rejected_by_modal_watchdog(self):
        late = QtWidgets.QDialog(self.window)
        late.setModal(True)

        def open_late():
            late.exec()

        def operation():
            QtCore.QTimer.singleShot(60, open_late)
            self.gui.pump(250)

        try:
            state = self.gui.run_with_modal_action(
                operation,
                lambda dialog: dialog.accept(),
                timeout_ms=20,
            )
            self.assertTrue(state["expired"])
            self.assertFalse(late.isVisible())
        finally:
            late.close()
            late.deleteLater()
            self.gui.flush_gui(20)

    def test_modal_action_cancelled_before_deadline_is_not_expired(self):
        state = self.gui.run_with_modal_action(
            lambda: None,
            lambda dialog: dialog.accept(),
            timeout_ms=500,
        )

        self.assertFalse(state["seen"])
        self.assertFalse(state["expired"])

    def test_modal_action_records_expiry_after_blocking_operation(self):
        state = self.gui.run_with_modal_action(
            lambda: time.sleep(0.05),
            lambda dialog: dialog.accept(),
            timeout_ms=10,
        )

        self.assertFalse(state["seen"])
        self.assertTrue(state["expired"])

    def test_wait_for_focus_returns_control_that_owns_focus(self):
        spinbox = QtWidgets.QSpinBox(self.window)
        spinbox.setObjectName("GuiTestSupportSpinBox")
        spinbox.show()
        self.window.activateWindow()

        self.gui.click(spinbox, spinbox.rect().center())
        focused = self.gui.wait_for_focus(
            QtWidgets.QAbstractSpinBox,
            parent=self.window,
            timeout_ms=500,
        )
        self.assertIs(focused, spinbox)

    def test_wait_until_processes_deferred_gui_work(self):
        state = {"ready": False}
        QtCore.QTimer.singleShot(20, lambda: state.update(ready=True))

        self.assertTrue(
            self.gui.wait_until(lambda: state["ready"], timeout_ms=500),
            "Expected the deferred GUI callback to run",
        )
