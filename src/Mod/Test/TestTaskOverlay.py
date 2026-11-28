# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""Regression tests for Tasks overlay settings."""

import unittest

import FreeCAD as App
from PySide6.QtTest import QTest


class TestTaskOverlay(unittest.TestCase):
    def setUp(self):
        self.prefs = App.ParamGet("User parameter:BaseApp/MainWindow/DockWindows/OverlayRight")
        for key in ("AutoHide", "EditHide", "TaskShow", "EditShow"):
            if key in self.prefs.GetBools():
                self.addCleanup(self.prefs.SetBool, key, self.prefs.GetBool(key))
            else:
                self.addCleanup(self.prefs.RemBool, key)
        self.prefs.SetBool("AutoHide", False)
        self.prefs.SetBool("EditHide", False)
        self.prefs.SetBool("TaskShow", False)

    def _restore_legacy_mode(self):
        self.prefs.SetBool("EditShow", True)
        for _ in range(20):
            if "EditShow" not in self.prefs.GetBools():
                break
            QTest.qWait(50)
        self.assertNotIn("EditShow", self.prefs.GetBools())

    def test_show_on_edit_migrates_to_auto_task(self):
        self._restore_legacy_mode()
        self.assertTrue(self.prefs.GetBool("TaskShow"))

    def test_show_on_edit_migration_preserves_auto_hide_priority(self):
        self.prefs.SetBool("AutoHide", True)
        self._restore_legacy_mode()
        self.assertTrue(self.prefs.GetBool("AutoHide"))
        self.assertFalse(self.prefs.GetBool("TaskShow"))

    def test_show_on_edit_migration_preserves_hide_on_edit_priority(self):
        self.prefs.SetBool("EditHide", True)
        self._restore_legacy_mode()
        self.assertTrue(self.prefs.GetBool("EditHide"))
        self.assertFalse(self.prefs.GetBool("TaskShow"))
