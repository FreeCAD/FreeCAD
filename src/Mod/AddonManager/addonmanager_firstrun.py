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

""" Class to display a first-run dialog for the Addon Manager """

import os

from PySide import QtCore, QtWidgets

import FreeCAD
import FreeCADGui

import addonmanager_utilities as utils

# pylint: disable=too-few-public-methods


class FirstRunDialog:
    """Manage the display of the Addon Manager's first-run dialog, setting up some user
    preferences and making sure they are aware that this connects to the internet, downloads
    data, and possibly installs things that run code not affiliated with FreeCAD itself."""

    def __init__(self):
        self.pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.readWarning = self.pref.GetBool("readWarning2022", False)

    def exec(self) -> bool:
        """Display a first-run dialog if needed, and return True to indicate the Addon Manager
        should continue loading, or False if the user cancelled the dialog and wants to exit."""
        if not self.readWarning:
            warning_dialog = FreeCADGui.PySideUic.loadUi(
                os.path.join(os.path.dirname(__file__), "first_run.ui")
            )
            warning_dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
            autocheck = self.pref.GetBool("AutoCheck", False)
            download_macros = self.pref.GetBool("DownloadMacros", False)
            proxy_string = self.pref.GetString("ProxyUrl", "")
            if self.pref.GetBool("NoProxyCheck", True):
                proxy_option = 0
            elif self.pref.GetBool("SystemProxyCheck", False):
                proxy_option = 1
            elif self.pref.GetBool("UserProxyCheck", False):
                proxy_option = 2

            def toggle_proxy_list(option: int):
                if option == 2:
                    warning_dialog.lineEditProxy.show()
                else:
                    warning_dialog.lineEditProxy.hide()

            warning_dialog.checkBoxAutoCheck.setChecked(autocheck)
            warning_dialog.checkBoxDownloadMacroMetadata.setChecked(download_macros)
            warning_dialog.comboBoxProxy.setCurrentIndex(proxy_option)
            toggle_proxy_list(proxy_option)
            if proxy_option == 2:
                warning_dialog.lineEditProxy.setText(proxy_string)

            warning_dialog.comboBoxProxy.currentIndexChanged.connect(toggle_proxy_list)

            warning_dialog.labelWarning.setStyleSheet(
                f"color:{utils.warning_color_string()};font-weight:bold;"
            )

            if warning_dialog.exec() == QtWidgets.QDialog.Accepted:
                self.readWarning = True
                self.pref.SetBool("readWarning2022", True)
                self.pref.SetBool("AutoCheck", warning_dialog.checkBoxAutoCheck.isChecked())
                self.pref.SetBool(
                    "DownloadMacros",
                    warning_dialog.checkBoxDownloadMacroMetadata.isChecked(),
                )
                selected_proxy_option = warning_dialog.comboBoxProxy.currentIndex()
                if selected_proxy_option == 0:
                    self.pref.SetBool("NoProxyCheck", True)
                    self.pref.SetBool("SystemProxyCheck", False)
                    self.pref.SetBool("UserProxyCheck", False)
                elif selected_proxy_option == 1:
                    self.pref.SetBool("NoProxyCheck", False)
                    self.pref.SetBool("SystemProxyCheck", True)
                    self.pref.SetBool("UserProxyCheck", False)
                else:
                    self.pref.SetBool("NoProxyCheck", False)
                    self.pref.SetBool("SystemProxyCheck", False)
                    self.pref.SetBool("UserProxyCheck", True)
                    self.pref.SetString("ProxyUrl", warning_dialog.lineEditProxy.text())
        return self.readWarning
