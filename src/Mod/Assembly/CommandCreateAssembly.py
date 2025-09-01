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
    from PySide import QtCore, QtGui, QtWidgets

import UtilsAssembly
import Preferences

translate = App.Qt.translate

__title__ = "Assembly Command Create Assembly"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandCreateAssembly:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Geoassembly",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateAssembly", "New Assembly"),
            "Accel": "A",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateAssembly",
                "Creates an assembly object in the current document, or in the current active assembly (if any). Limit of one root assembly per file.",
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
        App.setActiveTransaction("New Assembly")

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


class ActivateAssemblyTaskPanel:
    """A basic TaskPanel to select an assembly to activate."""

    def __init__(self, assemblies):
        self.assemblies = assemblies
        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle(translate("Assembly_ActivateAssembly", "Activate Assembly"))

        layout = QtWidgets.QVBoxLayout(self.form)
        label = QtWidgets.QLabel(
            translate("Assembly_ActivateAssembly", "Select an assembly to activate:")
        )
        self.combo = QtWidgets.QComboBox()

        for asm in self.assemblies:
            # Store the user-friendly Label for display, and the internal Name for activation
            self.combo.addItem(asm.Label, asm.Name)

        layout.addWidget(label)
        layout.addWidget(self.combo)

    def accept(self):
        """Called when the user clicks OK."""
        selected_name = self.combo.currentData()
        if selected_name:
            Gui.doCommand(f"Gui.ActiveDocument.setEdit('{selected_name}')")
        return True

    def reject(self):
        """Called when the user clicks Cancel or closes the panel."""
        return True


class CommandActivateAssembly:
    def __init__(self):
        self.task_panel = None

    def GetResources(self):
        return {
            "Pixmap": "Assembly_ActivateAssembly",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_ActivateAssembly", "Activate Assembly"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_ActivateAssembly", "Sets an assembly as the active one for editing."
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if Gui.Control.activeDialog() or App.ActiveDocument is None:
            return False

        # Command is only active if no assembly is currently active
        if UtilsAssembly.activeAssembly() is not None:
            return False

        # And if there is at least one assembly in the document to activate
        for obj in App.ActiveDocument.Objects:
            if obj.isDerivedFrom("Assembly::AssemblyObject"):
                return True

        return False

    def Activated(self):
        doc = App.ActiveDocument
        assemblies = [o for o in doc.Objects if o.isDerivedFrom("Assembly::AssemblyObject")]

        if len(assemblies) == 1:
            # If there's only one, activate it directly without showing a dialog
            Gui.doCommand(f"Gui.ActiveDocument.setEdit('{assemblies[0].Name}')")
        elif len(assemblies) > 1:
            # If there are multiple, show a task panel to let the user choose
            self.task_panel = ActivateAssemblyTaskPanel(assemblies)
            Gui.Control.showDialog(self.task_panel)


if App.GuiUp:
    Gui.addCommand("Assembly_CreateAssembly", CommandCreateAssembly())
    Gui.addCommand("Assembly_ActivateAssembly", CommandActivateAssembly())
