# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of App.                                          *
#                                                                           *
#    App is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    App is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with App. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP
from ContextCreatorLibrary import ContextCreationSystem

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import UtilsAssembly
import Preferences

transparency_level = 75

# translate = App.Qt.translate

__title__ = "Assembly Command Update Assembly Context"
__author__ = "drwho495"
__url__ = "https://github.com/drwho495/FreeCAD-Context-Fork"

class CommandUpdateAssemblyContext:
    def __init__(self):
        print("Assembly Context Updator Loaded")
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_AssemblyContextUpdate",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_AssemblyContextUpdate", "Update Assembly Context"),
            "Accel": "C",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_UpdateAssemblyContext",
                "Update an assembly context (EXPERIMENTAL)",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if Preferences.preferences().GetBool("EnforceOneAssemblyRule", True):
            return Gui.Selection.getSelection() is not None

        return App.ActiveDocument is not None

    def Activated(self):
        App.setActiveTransaction("Update Assembly Context")

        selection = Gui.Selection.getSelection()

        if selection:
            contextCreationSystem = ContextCreationSystem(None, None)
            contextCreationSystem.UpdateContext()

    
    


if App.GuiUp:
    Gui.addCommand("Assembly_UpdateAssemblyContext", CommandUpdateAssemblyContext())
