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

""" System for checking the network connection status asynchronously. """

import FreeCAD

from PySide import QtCore, QtWidgets

import NetworkManager
from addonmanager_workers_utility import ConnectionChecker

translate = FreeCAD.Qt.translate


class ConnectionCheckerGUI(QtCore.QObject):
    """Determine whether there is an active network connection, showing a progress message if it
    starts to take too long, and an error message if the network cannot be accessed."""

    connection_available = QtCore.Signal()
    check_complete = QtCore.Signal()

    def __init__(self):
        super().__init__()

        # Check the connection in a new thread, so FreeCAD stays responsive
        self.connection_checker = ConnectionChecker()
        self.connection_checker.success.connect(self._check_succeeded)
        self.connection_checker.failure.connect(self._network_connection_failed)

        self.connection_message_timer = None
        self.connection_check_message = None

    def start(self):
        """Start the connection check"""
        self.connection_checker.start()

        # If it takes longer than a half second to check the connection, show a message:
        self.connection_message_timer = QtCore.QTimer.singleShot(
            500, self._show_connection_check_message
        )

    def _show_connection_check_message(self):
        """Display a message informing the user that the check is in process"""
        if not self.connection_checker.isFinished():
            self.connection_check_message = QtWidgets.QMessageBox(
                QtWidgets.QMessageBox.Information,
                translate("AddonsInstaller", "Checking connection"),
                translate("AddonsInstaller", "Checking for connection to GitHub..."),
                QtWidgets.QMessageBox.Cancel,
            )
            self.connection_check_message.buttonClicked.connect(self.cancel_network_check)
            self.connection_check_message.show()

    def cancel_network_check(self, _):
        """Cancel the check"""
        if not self.connection_checker.isFinished():
            self.connection_checker.success.disconnect(self._check_succeeded)
            self.connection_checker.failure.disconnect(self._network_connection_failed)
            self.connection_checker.requestInterruption()
            self.connection_checker.wait(500)
            self.connection_check_message.close()
            self.check_complete.emit()

    def _network_connection_failed(self, message: str) -> None:
        """Callback for failed connection check. Displays an error message, then emits the
        check_complete signal (but not the connection available signal)."""
        # This must run on the main GUI thread
        if hasattr(self, "connection_check_message") and self.connection_check_message:
            self.connection_check_message.close()
        if NetworkManager.HAVE_QTNETWORK:
            QtWidgets.QMessageBox.critical(
                None, translate("AddonsInstaller", "Connection failed"), message
            )
        else:
            # pylint: disable=line-too-long
            QtWidgets.QMessageBox.critical(
                None,
                translate("AddonsInstaller", "Missing dependency"),
                translate(
                    "AddonsInstaller",
                    "Could not import QtNetwork -- see Report View for details. Addon Manager unavailable.",
                ),
            )

        self.check_complete.emit()

    def _check_succeeded(self):
        """Emit both the connection_available and the check_complete signals, in that order."""

        if hasattr(self, "connection_check_message") and self.connection_check_message:
            self.connection_check_message.close()

        self.connection_available.emit()
        self.check_complete.emit()
