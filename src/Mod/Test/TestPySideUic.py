# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors

from pathlib import Path
import shutil
import tempfile
import unittest

import FreeCADGui
from PySide import QtWidgets


class PySideUicTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("pyside6-uic"), "pyside6-uic is not available")
    def test_ui_file_loading(self):
        ui = """<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>TestWidget</class>
 <widget class="QWidget" name="TestWidget">
  <widget class="QLabel" name="label">
   <property name="text">
    <string>My String property</string>
   </property>
  </widget>
 </widget>
 <resources/>
 <connections/>
</ui>
"""

        with tempfile.TemporaryDirectory() as directory:
            ui_file = Path(directory, "TestWidget.ui")
            ui_file.write_text(ui, encoding="utf-8")

            form_class, base_class = FreeCADGui.PySideUic.loadUiType(str(ui_file))

        self.assertEqual(form_class.__name__, "Ui_TestWidget")
        self.assertIs(base_class, QtWidgets.QWidget)

        widget = base_class()
        form = form_class()
        form.setupUi(widget)
        self.assertEqual(form.label.text(), "My String property")
