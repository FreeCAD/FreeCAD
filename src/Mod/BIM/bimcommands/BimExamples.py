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

"""The BIM Examples command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Examples:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Help",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Examples", "BIM Examples"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Examples",
                "Download examples of BIM files made with FreeCAD",
            ),
        }

    def Activated(self):
        from PySide import QtGui
        QtGui.QDesktopServices.openUrl("https://github.com/yorikvanhavre/FreeCAD-BIM-examples")


FreeCADGui.addCommand("BIM_Examples", BIM_Examples())
