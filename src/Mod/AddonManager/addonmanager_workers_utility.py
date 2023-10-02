# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

""" Misc. worker thread classes for the FreeCAD Addon Manager. """

from typing import Optional

import FreeCAD
from PySide import QtCore
import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate


class ConnectionChecker(QtCore.QThread):
    """A worker thread for checking the connection to GitHub as a proxy for overall
    network connectivity. It has two signals: success() and failure(str). The failure
    signal contains a translated error message suitable for display to an end user."""

    success = QtCore.Signal()
    failure = QtCore.Signal(str)

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        """Not generally called directly: create a new ConnectionChecker object and
        call start() on it to spawn a child thread."""

        FreeCAD.Console.PrintLog("Checking network connection...\n")
        result = self.check_network_connection()
        if QtCore.QThread.currentThread().isInterruptionRequested():
            FreeCAD.Console.PrintLog("Connection check cancelled\n")
            return
        if not result:
            self.failure.emit(
                translate(
                    "AddonsInstaller",
                    "Unable to read data from GitHub: check your internet connection and proxy settings and try again.",
                )
            )
            return
        FreeCAD.Console.PrintLog(f"GitHub's zen message response: {result}\n")
        self.success.emit()

    def check_network_connection(self) -> Optional[str]:
        """The main work of this object: returns the decoded result of the connection request, or
        None if the request failed"""
        url = "https://api.github.com/zen"
        result = utils.blocking_get(url)
        if result:
            return result.decode("utf8")
        return None
