# ***************************************************************************
# *   Copyright (c) 2023 David Carter <dcarter@dvidcarter.ca>               *
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

__title__ = "FreeCAD material value editor"
__author__ = "David Carter"
__url__ = "http://www.freecad.org"

import os
from PySide import QtCore, QtGui, QtSvg

import FreeCAD
import FreeCADGui

class ValueEditor:

    def __init__(self):
        # load the UI file from the same directory as this script
        filePath = os.path.dirname(__file__) + os.sep
        self.widget = FreeCADGui.PySideUic.loadUi(filePath + "ValueEditor.ui")

        # remove unused Help button
        self.widget.setWindowFlags(self.widget.windowFlags()
                                   & ~QtCore.Qt.WindowContextHelpButtonHint)

        # restore size and position
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        width = param.GetInt("ValueEditorWidth", 441)
        height = param.GetInt("ValueEditorHeight", 626)
        self.widget.resize(width, height)

        # # additional UI fixes and tweaks
        widget = self.widget
        standardButtons = widget.standardButtons

        standardButtons.rejected.connect(self.reject)

    def accept(self):
        ""

        self.storeSize()
        QtGui.QDialog.accept(self.widget)

    def reject(self):
        ""

        self.storeSize()
        QtGui.QDialog.reject(self.widget)

    def storeSize(self):
        "stores the widget size"
        # store widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material")
        p.SetInt("ValueEditorWidth", self.widget.width())
        p.SetInt("ValueEditorHeight", self.widget.height())

    def show(self):
        return self.widget.show()

    def exec_(self):
        return self.widget.exec_()
