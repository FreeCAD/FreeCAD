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

import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import UtilsAssembly

# translate = App.Qt.translate

__title__ = "Assembly Command Insert Link"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandInsertLink:
    def __init__(self):
        pass

    def GetResources(self):
        tooltip = "<p>Insert a Link into the assembly. "
        tooltip += "This will create dynamic links to parts/bodies/primitives/assemblies."
        tooltip += "To insert external objects, make sure that the file "
        tooltip += "is <b>open in the current session</b></p>"
        tooltip += "<p>Press shift to add several links while clicking on the view."

        return {
            "Pixmap": "Assembly_InsertLink",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_InsertLink", "Insert Link"),
            "Accel": "I",
            "ToolTip": QT_TRANSLATE_NOOP("Assembly_InsertLink", tooltip),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyInsertLink(assembly, view)
        Gui.Control.showDialog(self.panel)


class TaskAssemblyInsertLink(QtCore.QObject):
    def __init__(self, assembly, view):
        super().__init__()

        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyInsertLink.ui")
        self.form.installEventFilter(self)

        # Actions
        self.form.openFileButton.clicked.connect(self.openFiles)
        self.form.partList.itemClicked.connect(self.onItemClicked)
        self.form.filterPartList.textChanged.connect(self.onFilterChange)

        self.allParts = []
        self.partsDoc = []
        self.numberOfAddedParts = 0
        self.translation = 0
        self.partMoving = False

        self.buildPartList()

        App.setActiveTransaction("Insert Link")

    def accept(self):
        App.closeActiveTransaction()
        self.deactivated()
        return True

    def reject(self):
        App.closeActiveTransaction(True)
        self.deactivated()
        return True

    def deactivated(self):
        if self.partMoving:
            self.endMove()

    def buildPartList(self):
        self.allParts.clear()
        self.partsDoc.clear()

        docList = App.listDocuments().values()

        for doc in docList:
            if UtilsAssembly.isDocTemporary(doc):
                continue

            # Build list of current assembly's parents, including the current assembly itself
            parents = self.assembly.Parents
            if parents:
                root_parent, sub = parents[0]
                parents_names, _ = UtilsAssembly.getObjsNamesAndElement(root_parent.Name, sub)
            else:
                parents_names = [self.assembly.Name]

            for obj in doc.findObjects("App::Part"):
                # we don't want to link to itself or parents.
                if obj.Name not in parents_names:
                    self.allParts.append(obj)
                    self.partsDoc.append(doc)

            for obj in doc.findObjects("PartDesign::Body"):
                # but only those at top level (not nested inside other containers)
                if obj.getParentGeoFeatureGroup() is None:
                    self.allParts.append(obj)
                    self.partsDoc.append(doc)

        self.form.partList.clear()
        for part in self.allParts:
            newItem = QtGui.QListWidgetItem()
            newItem.setText(part.Document.Name + " - " + part.Name)
            newItem.setIcon(part.ViewObject.Icon)
            self.form.partList.addItem(newItem)

    def onFilterChange(self):
        filter_str = self.form.filterPartList.text().strip().lower()

        for i in range(self.form.partList.count()):
            item = self.form.partList.item(i)
            item_text = item.text().lower()

            # Check if the item's text contains the filter string
            is_visible = filter_str in item_text if filter_str else True

            item.setHidden(not is_visible)

    def openFiles(self):
        selected_files, _ = QtGui.QFileDialog.getOpenFileNames(
            None,
            "Select FreeCAD documents to import parts from",
            "",
            "Supported Formats (*.FCStd *.fcstd);;All files (*)",
        )

        for filename in selected_files:
            requested_file = os.path.split(filename)[1]
            import_doc_is_open = any(
                requested_file == os.path.split(doc.FileName)[1]
                for doc in App.listDocuments().values()
            )

            if not import_doc_is_open:
                if filename.lower().endswith(".fcstd"):
                    App.openDocument(filename)
                    App.setActiveDocument(self.doc.Name)
                    self.buildPartList()

    def onItemClicked(self, item):
        for selected in self.form.partList.selectedIndexes():
            selectedPart = self.allParts[selected.row()]
        if not selectedPart:
            return

        if self.partMoving:
            self.endMove()

        # check that the current document had been saved or that it's the same document as that of the selected part
        if not self.doc.FileName != "" and not self.doc == selectedPart.Document:
            print("The current document must be saved before inserting an external part")
            return

        self.createdLink = self.assembly.newObject("App::Link", selectedPart.Name)
        self.createdLink.LinkedObject = selectedPart
        self.createdLink.Placement.Base = self.getTranslationVec(selectedPart)
        self.createdLink.recompute()

        self.numberOfAddedParts += 1

        # highlight the link
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.doc.Name, self.assembly.Name, self.createdLink.Name + ".")

        # Start moving the part if user brings mouse on view
        self.initMove()

    def initMove(self):
        self.callbackMove = self.view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackClick = self.view.addEventCallback("SoMouseButtonEvent", self.clickMouse)
        self.callbackKey = self.view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)
        self.partMoving = True

        # Selection filter to avoid selecting the part while it's moving
        # filter = Gui.Selection.Filter('SELECT ???')
        # Gui.Selection.addSelectionGate(filter)

    def endMove(self):
        self.view.removeEventCallback("SoLocation2Event", self.callbackMove)
        self.view.removeEventCallback("SoMouseButtonEvent", self.callbackClick)
        self.view.removeEventCallback("SoKeyboardEvent", self.callbackKey)
        self.partMoving = False
        self.doc.recompute()
        # Gui.Selection.removeSelectionGate()

    def moveMouse(self, info):
        newPos = self.view.getPoint(*info["Position"])
        self.createdLink.Placement.Base = newPos

    def clickMouse(self, info):
        if info["Button"] == "BUTTON1" and info["State"] == "DOWN":
            if info["ShiftDown"]:
                # Create a new link and moves this one now
                currentPos = self.createdLink.Placement.Base
                selectedPart = self.createdLink.LinkedObject
                self.createdLink = self.assembly.newObject("App::Link", selectedPart.Name)
                self.createdLink.LinkedObject = selectedPart
                self.createdLink.Placement.Base = currentPos
            else:
                self.endMove()

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            self.endMove()
            self.doc.removeObject(self.createdLink.Name)

    # Taskbox keyboard event handler
    def eventFilter(self, watched, event):
        if watched == self.form and event.type() == QtCore.QEvent.KeyPress:
            if event.key() == QtCore.Qt.Key_Escape and self.partMoving:
                self.endMove()
                self.doc.removeObject(self.createdLink.Name)
                return True  # Consume the event
        return super().eventFilter(watched, event)

    def getTranslationVec(self, part):
        bb = part.Shape.BoundBox
        if bb:
            self.translation += (bb.XMax + bb.YMax + bb.ZMax) * 0.15
        else:
            self.translation += 10
        return App.Vector(self.translation, self.translation, self.translation)


if App.GuiUp:
    Gui.addCommand("Assembly_InsertLink", CommandInsertLink())
