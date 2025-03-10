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

__title__ = "Assembly Command Create Assembly Context"
__author__ = "drwho495"
__url__ = "https://github.com/drwho495/my-App-Fork"

class ContextGuiManager(QtCore.QObject):
    def __init__(self, assembly, view):
        super().__init__()

        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateContext.ui")
        self.form.installEventFilter(self)
        self.form.partList.installEventFilter(self)

        pref = Preferences.preferences()
        # Actions
        self.form.partList.itemClicked.connect(self.onItemClicked)
        self.form.filterPartList.textChanged.connect(self.onFilterChange)

        self.allParts = []
        self.translation = 0
        self.selectedPart = None
        self.partMoving = False
        self.totalTranslation = App.Vector()
        self.groundedObj = None

        self.insertionStack = []  # used to handle cancellation of insertions.

        self.buildPartList()

        App.setActiveTransaction("Insert Assembly Context")

    def onItemClicked(self, item):
        for selected in self.form.partList.selectedIndexes():
            selectedPart = self.allParts[selected.row()]
            self.selectedPart = selectedPart
            print(selectedPart)
        if not selectedPart:
            return
    
    def onFilterChange(self):
        pass

    def buildPartList(self):
        for part in self.assembly.OutList:
            if hasattr(part, 'Placement'):
                self.allParts.append(part)

        self.form.partList.clear()
        for part in self.allParts:
            newItem = QtGui.QListWidgetItem()
            newItem.setText(part.Label)
            newItem.setIcon(part.ViewObject.Icon)
            self.form.partList.addItem(newItem)
    
    def accept(self, resetEdit=True):
        print("Creating context...")
        Gui.Control.closeDialog()
        contextCreationSystem = ContextCreationSystem(self.selectedPart, self.assembly)
        
        self.linked_edit_obj = contextCreationSystem.getLinkedObj(self.selectedPart)
        self.target_document = contextCreationSystem.getLinkedDoc(self.selectedPart)
        self.target_placement = self.selectedPart.Placement
        self.objects = contextCreationSystem.getCopyableObjectsInAssembly(self.assembly)
        #linked_object, target_document, self.objects, linked_placement, self.edit_selection
        contextCreationSystem.createContext(self.linked_edit_obj, self.target_document, self.objects, self.target_placement, self.selectedPart)

class CommandCreateAssemblyContext:
    def __init__(self):
        print("Assembly Context Creator Loaded")
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_AssemblyContextCreate",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_AssemblyContextCreate", "Create Assembly Context"),
            "Accel": "C",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateAssemblyContext",
                "Create an assembly context (EXPERIMENTAL)",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if Preferences.preferences().GetBool("EnforceOneAssemblyRule", True):
            activeAssembly = UtilsAssembly.activeAssembly()

            return activeAssembly != None and Gui.Control.activeDialog() == False

        return App.ActiveDocument is not None

    def Activated(self):
        App.setActiveTransaction("Create Assembly Context")

        print("Open Assembly Context UI")
        activeAssembly = UtilsAssembly.activeAssembly()
        if activeAssembly:
            print("Open Assembly Context UI")
            self.panel = ContextGuiManager(activeAssembly, Gui.activeDocument().activeView())
            if Gui.Control.activeDialog() != False:
                App.Console.PrintWarning("The task menu is already being used!")
            else:
                Gui.Control.showDialog(self.panel)
        else:
            App.Console.PrintWarning("You need to have an active assembly selected!")
    
    


if App.GuiUp:
    Gui.addCommand("Assembly_CreateAssemblyContext", CommandCreateAssemblyContext())
