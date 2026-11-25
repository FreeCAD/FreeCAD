# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for MDI view type IDs and the main window title.

To run tests:
    FreeCAD -t TestMDIView
"""

import time
import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtCore, QtWidgets


class PythonView:
    def __init__(self):
        self._widget = QtWidgets.QWidget()

    def widget(self):
        return self._widget


class TestMDIView(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestMDIView")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test_3d_view_type_id(self):
        view = FreeCADGui.ActiveDocument.ActiveView

        self.assertEqual(view.getTypeId(), "Gui::View3DInventor")

    def test_python_view_type_id(self):
        main_window = FreeCADGui.getMainWindow()
        view = main_window.addWindow(PythonView())

        try:
            self.assertEqual(view.getTypeId(), "Gui::MDIViewPyWrap")
        finally:
            main_window.removeWindow(view)


class TestMainWindowTitle(unittest.TestCase):
    """Only development builds with an 8-digit revision show it in the title."""

    def setUp(self):
        self.addCleanup(FreeCAD.ConfigSet, "BuildRevision", FreeCAD.ConfigGet("BuildRevision"))
        self.set_show_version(True)

    def set_show_version(self, value):
        group = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
        if "ShowVersionInTitle" in group.GetBools():
            self.addCleanup(
                group.SetBool, "ShowVersionInTitle", group.GetBool("ShowVersionInTitle")
            )
        else:
            self.addCleanup(group.RemBool, "ShowVersionInTitle")
        group.SetBool("ShowVersionInTitle", value)

    def assert_title(self, revision, expected_name):
        """Check the title of a new document that is created with the given build revision."""
        FreeCAD.ConfigSet("BuildRevision", revision)
        doc = FreeCAD.newDocument("TestMainWindowTitle")
        try:
            main_window = FreeCADGui.getMainWindow()
            expected = f"[*] {doc.Label} - {expected_name}"
            # the label is set after the view was created, the title follows with a delay
            deadline = time.monotonic() + 3.0
            while main_window.windowTitle() != expected and time.monotonic() < deadline:
                QtWidgets.QApplication.processEvents(
                    QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 50
                )
                time.sleep(0.01)
            self.assertEqual(main_window.windowTitle(), expected)
        finally:
            FreeCAD.closeDocument(doc.Name)

    def name_with_version(self):
        version = ".".join(
            FreeCAD.ConfigGet(key)
            for key in ("BuildVersionMajor", "BuildVersionMinor", "BuildVersionPoint")
        )
        suffix = FreeCAD.ConfigGet("BuildVersionSuffix")
        return f"{QtCore.QCoreApplication.applicationName()} {version}{suffix}"

    def test_eight_digit_revision(self):
        expected = self.name_with_version()
        # only development builds show it, releases and release candidates keep their version
        if FreeCAD.ConfigGet("BuildVersionSuffix") == "dev":
            expected += " 20261117"

        for revision in ("20261117 (Git shallow)", "20261117"):
            with self.subTest(revision=revision):
                self.assert_title(revision, expected)

    def test_other_revisions(self):
        for revision in (
            "",
            "Unknown",
            "Dev Build",
            "49586",
            "49586 (Git)",
            "49583 +3 (Git)",
            "14555 (Git shallow)",
            "2026111 (Git shallow)",
            "202611170 (Git shallow)",
            "20261117 (Git)",
            "20261117 +3 (Git)",
            "2026/11/17",
            "r20261117",
        ):
            with self.subTest(revision=revision):
                self.assert_title(revision, self.name_with_version())

    def test_hidden_version(self):
        self.set_show_version(False)

        self.assert_title("20261117 (Git shallow)", QtCore.QCoreApplication.applicationName())
