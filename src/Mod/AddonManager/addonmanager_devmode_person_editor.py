# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

""" Contains a class to handle editing a person (from a Metadata standpoint). """

import os
from typing import Tuple  # Needed until Py 3.9, when tuple supports this directly

from PySide.QtWidgets import QDialog

import FreeCAD
import FreeCADGui

translate = FreeCAD.Qt.translate


class PersonEditor:
    """Create or edit a maintainer or author record."""

    def __init__(self):

        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_people.ui")
        )
        self.dialog.comboBox.clear()
        self.dialog.comboBox.addItem(
            translate("AddonsInstaller", "Maintainer"), userData="maintainer"
        )
        self.dialog.comboBox.addItem(translate("AddonsInstaller", "Author"), userData="author")

    def exec(self) -> Tuple[str, str, str]:
        """Run the dialog, and return a tuple of the person's record type, their name, and their
        email address. Email may be None. If the others are None it's because the user cancelled
        the interaction."""
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            return (
                self.dialog.comboBox.currentData(),
                self.dialog.nameLineEdit.text(),
                self.dialog.emailLineEdit.text(),
            )
        return "", "", ""

    def setup(self, person_type: str = "maintainer", name: str = "", email: str = "") -> None:
        """Configure the dialog"""
        index = self.dialog.comboBox.findData(person_type)
        if index == -1:
            FreeCAD.Console.PrintWarning(f"Internal Error: unrecognized person type {person_type}")
            index = 0
        self.dialog.comboBox.setCurrentIndex(index)
        self.dialog.nameLineEdit.setText(name)
        self.dialog.emailLineEdit.setText(email)
