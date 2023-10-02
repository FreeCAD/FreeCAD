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

""" Contains a class to manage selection of a license for an Addon. """

import os
from datetime import date
from typing import Optional, Tuple

import FreeCAD
import FreeCADGui

from PySide.QtWidgets import QFileDialog, QDialog
from PySide.QtGui import QDesktopServices
from PySide.QtCore import QUrl, QFile, QIODevice

try:
    from PySide.QtGui import (
        QRegularExpressionValidator,
    )
    from PySide.QtCore import QRegularExpression

    RegexWrapper = QRegularExpression
    RegexValidatorWrapper = QRegularExpressionValidator
except ImportError:
    QRegularExpressionValidator = None
    QRegularExpression = None
    from PySide.QtGui import (
        QRegExpValidator,
    )
    from PySide.QtCore import QRegExp

    RegexWrapper = QRegExp
    RegexValidatorWrapper = QRegExpValidator

translate = FreeCAD.Qt.translate


class LicenseSelector:
    """Choose from a selection of licenses, or provide your own. Includes the capability to create
    the license file itself for a variety of popular open-source licenses, as well as providing
    links to opensource.org's page about the various licenses (which often link to other resources).
    """

    licenses = {
        "Apache-2.0": (
            "Apache License, Version 2.0",
            "https://opensource.org/licenses/Apache-2.0",
        ),
        "BSD-2-Clause": (
            "The 2-Clause BSD License",
            "https://opensource.org/licenses/BSD-2-Clause",
        ),
        "BSD-3-Clause": (
            "The 3-Clause BSD License",
            "https://opensource.org/licenses/BSD-3-Clause",
        ),
        "CC0v1": (
            "No Rights Reserved/Public Domain",
            "https://creativecommons.org/choose/zero/",
        ),
        "GPLv2": (
            "GNU General Public License version 2",
            "https://opensource.org/licenses/GPL-2.0",
        ),
        "GPLv3": (
            "GNU General Public License version 3",
            "https://opensource.org/licenses/GPL-3.0",
        ),
        "LGPLv2.1": (
            "GNU Lesser General Public License version 2.1",
            "https://opensource.org/licenses/LGPL-2.1",
        ),
        "LGPLv3": (
            "GNU Lesser General Public License version 3",
            "https://opensource.org/licenses/LGPL-3.0",
        ),
        "MIT": (
            "The MIT License",
            "https://opensource.org/licenses/MIT",
        ),
        "MPL-2.0": (
            "Mozilla Public License 2.0",
            "https://opensource.org/licenses/MPL-2.0",
        ),
    }

    def __init__(self, path_to_addon):
        self.other_label = translate(
            "AddonsInstaller",
            "Other...",
            "For providing a license other than one listed",
        )
        self.path_to_addon = path_to_addon
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_license.ui")
        )
        self.pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        for short_code, details in LicenseSelector.licenses.items():
            self.dialog.comboBox.addItem(f"{short_code}: {details[0]}", userData=short_code)
        self.dialog.comboBox.addItem(self.other_label)
        self.dialog.otherLineEdit.hide()
        self.dialog.otherLabel.hide()

        # Connections:
        self.dialog.comboBox.currentIndexChanged.connect(self._selection_changed)
        self.dialog.aboutButton.clicked.connect(self._about_clicked)
        self.dialog.browseButton.clicked.connect(self._browse_clicked)
        self.dialog.createButton.clicked.connect(self._create_clicked)

        # Set up the first selection to whatever the user chose last time
        short_code = self.pref.GetString("devModeLastSelectedLicense", "LGPLv2.1")
        self.set_license(short_code)

    def exec(self, short_code: str = None, license_path: str = "") -> Optional[Tuple[str, str]]:
        """The main method for executing this dialog, as a modal that returns a tuple of the
        license's "short code" and optionally the path to the license file. Returns a tuple
        of None,None if the user cancels the operation."""

        if short_code:
            self.set_license(short_code)
        self.dialog.pathLineEdit.setText(license_path)
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            new_short_code = self.dialog.comboBox.currentData()
            new_license_path = self.dialog.pathLineEdit.text()
            if not new_short_code:
                new_short_code = self.dialog.otherLineEdit.text()
            self.pref.SetString("devModeLastSelectedLicense", new_short_code)
            return new_short_code, new_license_path
        return None

    def set_license(self, short_code):
        """Set the currently-selected license."""
        index = self.dialog.comboBox.findData(short_code)
        if index != -1:
            self.dialog.comboBox.setCurrentIndex(index)
        else:
            self.dialog.comboBox.setCurrentText(self.other_label)
            self.dialog.otherLineEdit.setText(short_code)

    def _selection_changed(self, _: int):
        """Callback: when the license selection changes, the UI is updated here."""
        if self.dialog.comboBox.currentText() == self.other_label:
            self.dialog.otherLineEdit.clear()
            self.dialog.otherLineEdit.show()
            self.dialog.otherLabel.show()
            self.dialog.aboutButton.setDisabled(True)
        else:
            self.dialog.otherLineEdit.hide()
            self.dialog.otherLabel.hide()
            self.dialog.aboutButton.setDisabled(False)

    def _current_short_code(self) -> str:
        """Gets the currently-selected license short code"""
        short_code = self.dialog.comboBox.currentData()
        if not short_code:
            short_code = self.dialog.otherLineEdit.text()
        return short_code

    def _about_clicked(self):
        """Callback: when the About button is clicked, try to launch a system-default web browser
        and display the OSI page about the currently-selected license."""
        short_code = self.dialog.comboBox.currentData()
        if short_code in LicenseSelector.licenses:
            url = LicenseSelector.licenses[short_code][1]
            QDesktopServices.openUrl(QUrl(url))
        else:
            FreeCAD.Console.PrintWarning(
                f"Internal Error: unrecognized license short code {short_code}\n"
            )

    def _browse_clicked(self):
        """Callback: browse for an existing license file."""
        start_dir = os.path.join(
            self.path_to_addon,
            self.dialog.pathLineEdit.text().replace("/", os.path.sep),
        )
        license_path, _ = QFileDialog.getOpenFileName(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Select the corresponding license file in your Addon",
            ),
            dir=start_dir,
        )
        if license_path:
            self._set_path(self.path_to_addon, license_path)

    def _set_path(self, start_dir: str, license_path: str):
        """Sets the value displayed in the path widget to the relative path from
        start_dir to license_path"""
        license_path = license_path.replace("/", os.path.sep)
        base_dir = start_dir.replace("/", os.path.sep)
        if base_dir[-1] != os.path.sep:
            base_dir += os.path.sep
        if not license_path.startswith(base_dir):
            FreeCAD.Console.PrintError("Selected file not in Addon\n")
            # Eventually offer to copy it?
            return
        relative_path = license_path[len(base_dir) :]
        relative_path = relative_path.replace(os.path.sep, "/")
        self.dialog.pathLineEdit.setText(relative_path)

    def _create_clicked(self):
        """Asks the users for the path to save the new license file to, then copies our internal
        copy of the license text to that file."""
        start_dir = os.path.join(
            self.path_to_addon,
            self.dialog.pathLineEdit.text().replace("/", os.path.sep),
        )
        license_path, _ = QFileDialog.getSaveFileName(
            parent=self.dialog,
            caption=translate(
                "AddonsInstaller",
                "Location for new license file",
            ),
            dir=os.path.join(start_dir, "LICENSE"),
        )
        if license_path:
            self._set_path(start_dir, license_path)
            short_code = self._current_short_code()
            qf = QFile(f":/licenses/{short_code}.txt")
            if qf.exists():
                qf.open(QIODevice.ReadOnly)
                byte_data = qf.readAll()
                qf.close()

                string_data = str(byte_data, encoding="utf-8")

                if "<%%YEAR%%>" in string_data or "<%%COPYRIGHT HOLDER%%>" in string_data:
                    info_dlg = FreeCADGui.PySideUic.loadUi(
                        os.path.join(
                            os.path.dirname(__file__),
                            "developer_mode_copyright_info.ui",
                        )
                    )
                    info_dlg.yearLineEdit.setValidator(
                        RegexValidatorWrapper(RegexWrapper("^[12]\\d{3}$"))
                    )
                    info_dlg.yearLineEdit.setText(str(date.today().year))
                    result = info_dlg.exec()
                    if result != QDialog.Accepted:
                        return  # Don't create the file, just bail out

                    holder = info_dlg.copyrightHolderLineEdit.text()
                    year = info_dlg.yearLineEdit.text()

                    string_data = string_data.replace("<%%YEAR%%>", year)
                    string_data = string_data.replace("<%%COPYRIGHT HOLDER%%>", holder)

                with open(license_path, "w", encoding="utf-8") as f:
                    f.write(string_data)
            else:
                FreeCAD.Console.PrintError(f"Cannot create license file of type {short_code}\n")
