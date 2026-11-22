# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""GUI regression tests for wrapping 3D views as PySide widgets.

To run tests:
    FreeCAD -t TestGraphicsViewWrapping.TestGraphicsViewWrapping
"""

import unittest

import FreeCAD
import FreeCADGui
from PySide6 import QtWidgets


class TestGraphicsViewWrapping(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestGraphicsViewWrapping")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test_active_view_wraps_as_qgraphics_view(self):
        view = FreeCADGui.ActiveDocument.ActiveView

        # graphicsView() wraps a C++ QGraphicsView through Shiboken. Some
        # Shiboken builds do not resolve Qt widget RTTI names, so this used to
        # raise "RuntimeError: Failed to wrap widget" instead of falling back to
        # PySide's public type names.
        graphics_view = view.graphicsView()

        self.assertIsInstance(graphics_view, QtWidgets.QGraphicsView)
        self.assertIsNotNone(graphics_view.viewport())
