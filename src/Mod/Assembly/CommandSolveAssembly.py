# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import UtilsAssembly
import Assembly_rc

# translate = App.Qt.translate

__title__ = "Assembly Command to Solve Assembly"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandSolveAssembly:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_SolveAssembly",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_SolveAssembly", "Solve Assembly"),
            "Accel": "Z",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_SolveAssembly",
                "Solve the currently active assembly.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.isAssemblyCommandActive()

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return

        Gui.addModule("UtilsAssembly")
        App.setActiveTransaction("Solve assembly")
        Gui.doCommand("UtilsAssembly.activeAssembly().solve()")
        App.closeActiveTransaction()


if App.GuiUp:
    Gui.addCommand("Assembly_SolveAssembly", CommandSolveAssembly())
