# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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

        self.form.buttonBox.accepted.connect(self.save_properties)
        self.form.buttonBox.rejected.connect(self.reject)

        # Connect text changed signal to update window title
        self.form.lineEditLibraryName.textChanged.connect(self.update_window_title)

        # Make the OK button the default so Enter key works
        ok_button = self.form.buttonBox.button(QtWidgets.QDialogButtonBox.Ok)
        cancel_button = self.form.buttonBox.button(QtWidgets.QDialogButtonBox.Cancel)

        if cancel_button:
            cancel_button.setDefault(False)
            cancel_button.setAutoDefault(False)

        if ok_button:
            ok_button.setDefault(True)
            ok_button.setAutoDefault(True)
            ok_button.setFocus()  # Also set focus to the OK button

        # Set minimum width for the dialog
        self.setMinimumWidth(450)

        # Set focus to the text input so user can start typing immediately
        self.form.lineEditLibraryName.setFocus()
        self.form.lineEditLibraryName.selectAll()  # Select all text for easy replacement

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
