# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module contains FreeCAD commands for the BIM workbench"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_Welcome:
    def GetResources(self):
        return {
            "Pixmap": "BIM_Welcome.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Welcome", "BIM Welcome Screen"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Welcome", "Shows the BIM workbench welcome screen"
            ),
        }

    def Activated(self):
        self.form = FreeCADGui.PySideUic.loadUi(":ui/dialogWelcome.ui")

        # handle the tutorial links
        self.form.label_4.linkActivated.connect(self.handleLink)
        self.form.label_7.linkActivated.connect(self.handleLink)

        self.form.adjustSize()

        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        # show dialog and run setup dialog afterwards if OK was pressed
        result = self.form.exec_()
        if result:
            FreeCADGui.runCommand("BIM_Setup")

        # remove first time flag
        PARAMS.SetBool(
            "FirstTime", False
        )

    def handleLink(self, link):
        from PySide import QtCore, QtGui

        if hasattr(self, "form"):
            self.form.hide()
            if "BIM_Start_Tutorial" in link:
                FreeCADGui.runCommand("BIM_Tutorial")
            else:
                # print("Opening link:",link)
                url = QtCore.QUrl(link)
                QtGui.QDesktopServices.openUrl(url)


FreeCADGui.addCommand("BIM_Welcome", BIM_Welcome())
