# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from PySide import QtWidgets
import FreeCADGui
import FreeCAD
from ..models.library import Library


class LibraryPropertyDialog(QtWidgets.QDialog):
    def __init__(self, library: Library, new=False, parent=None):
        super(LibraryPropertyDialog, self).__init__(parent)
        self.library = library

        # Load the UI file into a QWidget
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/LibraryProperties.ui")

        # Create a layout for the dialog and add the loaded form widget
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.form)
        self.setLayout(layout)

        # Connect signals and set initial values using the loaded form
        self.form.lineEditLibraryName.setText(self.library.label)
        self.update_window_title()

        if new:
            label = FreeCAD.Qt.translate("CAM", "Create Library")
            self.form.pushButtonSave.setText(label)

        self.form.buttonBox.accepted.connect(self.accept)
        self.form.buttonBox.rejected.connect(self.reject)
        self.form.pushButtonSave.clicked.connect(self.save_properties)

        # Connect text changed signal to update window title
        self.form.lineEditLibraryName.textChanged.connect(self.update_window_title)

        # Set minimum width for the dialog
        self.setMinimumWidth(450)

    def update_window_title(self):
        # Update title based on current text in the line edit
        current_name = self.form.lineEditLibraryName.text()
        title = FreeCAD.Qt.translate(
            "LibraryPropertyDialog", f"Library Properties - {current_name or self.library.label}"
        )
        self.setWindowTitle(title)

    def save_properties(self):
        new_name = self.form.lineEditLibraryName.text()
        if new_name != self.library.label:
            self.library._label = new_name
        # Additional logic to save other properties if added later
        self.accept()
