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

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

import UtilsAssembly
import Preferences

# translate = App.Qt.translate

__title__ = "Assembly Command Create Assembly"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandCreateAssembly:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Geoassembly",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateAssembly", "Create Assembly"),
            "Accel": "A",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateAssembly",
                "Create an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if Gui.Control.activeDialog():
            return False

        if Preferences.preferences().GetBool("EnforceOneAssemblyRule", True):
            activeAssembly = UtilsAssembly.activeAssembly()

            if UtilsAssembly.isThereOneRootAssembly() and not activeAssembly:
                return False

        return App.ActiveDocument is not None

    def Activated(self):
        App.setActiveTransaction("Create assembly")

        activeAssembly = UtilsAssembly.activeAssembly()
        Gui.addModule("UtilsAssembly")
        if activeAssembly:
            commands = (
                "activeAssembly = UtilsAssembly.activeAssembly()\n"
                'assembly = activeAssembly.newObject("Assembly::AssemblyObject", "Assembly")\n'
            )
        else:
            commands = (
                'assembly = App.ActiveDocument.addObject("Assembly::AssemblyObject", "Assembly")\n'
            )

        commands = commands + 'assembly.Type = "Assembly"\n'
        commands = commands + 'assembly.newObject("Assembly::JointGroup", "Joints")'

        Gui.doCommand(commands)
        if not activeAssembly:
            Gui.doCommandGui("Gui.ActiveDocument.setEdit(assembly)")

        App.closeActiveTransaction()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateAssembly", CommandCreateAssembly())
