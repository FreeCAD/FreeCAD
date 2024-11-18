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
from PySide.QtGui import QPixmap

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

            # Set signal handlers for accept/reject buttons
            warning_dialog.buttonContinue.clicked.connect(warning_dialog.accept)
            warning_dialog.buttonQuit.clicked.connect(warning_dialog.reject)

            # Show the dialog and check whether the user accepted or canceled
            if warning_dialog.exec() == QtWidgets.QDialog.Accepted:
                # Store warning as read/accepted
                self.readWarning = True
                self.pref.SetBool("readWarning2022", True)

        return self.readWarning
