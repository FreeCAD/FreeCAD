# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for MDI view type IDs.

To run tests:
    FreeCAD -t TestMDIView.TestMDIView
"""

import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtWidgets


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
