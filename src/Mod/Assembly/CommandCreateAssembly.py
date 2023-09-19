# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
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
# ***************************************************************************/

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

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
                "Create an assembly object in the current document.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return App.ActiveDocument is not None

    def Activated(self):
        App.setActiveTransaction("Create assembly")
        assembly = App.ActiveDocument.addObject("App::Part", "Assembly")
        assembly.Type = "Assembly"
        Gui.ActiveDocument.ActiveView.setActiveObject("part", assembly)
        assembly.newObject("App::DocumentObjectGroup", "Joints")
        App.closeActiveTransaction()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateAssembly", CommandCreateAssembly())
