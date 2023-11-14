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

import re
import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import UtilsAssembly
import Preferences

# translate = App.Qt.translate

__title__ = "Assembly Command Insert Link"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandInsertLink:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_InsertLink",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_InsertLink", "Insert Link"),
            "Accel": "I",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_InsertLink",
                "Insert a Link into the currently active assembly. This will create dynamic links to parts/bodies/primitives/assemblies. To insert external objects, make sure that the file is <b>open in the current session</b>",
            )
            + "</p><p><ul><li>"
            + QT_TRANSLATE_NOOP("Assembly_InsertLink", "Insert by left clicking items in the list.")
            + "</li><li>"
            + QT_TRANSLATE_NOOP("Assembly_InsertLink", "Undo by right clicking items in the list.")
            + "</li><li>"
            + QT_TRANSLATE_NOOP(
                "Assembly_InsertLink",
                "Press shift to add several links while clicking on the view.",
            )
            + "</li></ul></p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.isAssemblyCommandActive()

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
        self.form.partList.installEventFilter(self)

        pref = Preferences.preferences()
        self.form.CheckBox_InsertInParts.setChecked(pref.GetBool("InsertInParts", True))

        # Actions
        self.form.openFileButton.clicked.connect(self.openFiles)
        self.form.partList.itemClicked.connect(self.onItemClicked)
        self.form.filterPartList.textChanged.connect(self.onFilterChange)

        self.allParts = []
        self.partsDoc = []
        self.translation = 0
        self.partMoving = False
        self.totalTranslation = App.Vector()

        self.insertionStack = []  # used to handle cancellation of insertions.

        self.buildPartList()

        App.setActiveTransaction("Insert Link")

    def accept(self):
        self.deactivated()

        if self.partMoving:
            self.endMove()

        App.closeActiveTransaction()
        return True

    def reject(self):
        self.deactivated()

        if self.partMoving:
            self.dismissPart()

        App.closeActiveTransaction(True)
        return True

    def deactivated(self):
        pref = Preferences.preferences()
        pref.SetBool("InsertInParts", self.form.CheckBox_InsertInParts.isChecked())

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

            for obj in doc.findObjects("Part::Feature"):
                # but only those at top level (not nested inside other containers)
                if obj.getParentGeoFeatureGroup() is None:
                    self.allParts.append(obj)
                    self.partsDoc.append(doc)

        self.form.partList.clear()
        for part in self.allParts:
            newItem = QtGui.QListWidgetItem()
            newItem.setText(part.Label + " (" + part.Document.Name + ".FCStd)")
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
            msgBox = QtWidgets.QMessageBox()
            msgBox.setIcon(QtWidgets.QMessageBox.Warning)
            msgBox.setText("The current document must be saved before inserting external parts.")
            msgBox.setWindowTitle("Save Document")
            saveButton = msgBox.addButton("Save", QtWidgets.QMessageBox.AcceptRole)
            cancelButton = msgBox.addButton("Cancel", QtWidgets.QMessageBox.RejectRole)

            msgBox.exec_()

            if not (msgBox.clickedButton() == saveButton and Gui.ActiveDocument.saveAs()):
                return

        objectWhereToInsert = self.assembly

        if self.form.CheckBox_InsertInParts.isChecked() and selectedPart.TypeId != "App::Part":
            objectWhereToInsert = self.assembly.newObject("App::Part", "Part_" + selectedPart.Label)

        createdLink = objectWhereToInsert.newObject("App::Link", selectedPart.Label)
        createdLink.LinkedObject = selectedPart
        createdLink.recompute()

        addedObject = createdLink
        if self.form.CheckBox_InsertInParts.isChecked() and selectedPart.TypeId != "App::Part":
            addedObject = objectWhereToInsert

        insertionDict = {}
        insertionDict["item"] = item
        insertionDict["addedObject"] = addedObject
        self.insertionStack.append(insertionDict)
        self.increment_counter(item)

        translation = self.getTranslationVec(addedObject)
        insertionDict["translation"] = translation
        self.totalTranslation += translation
        addedObject.Placement.Base = self.totalTranslation

        # highlight the link
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.doc.Name, addedObject.Name, "")

        # Start moving the part if user brings mouse on view
        self.initMove()

        self.form.partList.setItemSelected(item, False)

    def increment_counter(self, item):
        text = item.text()
        match = re.search(r"(\d+) inserted$", text)

        if match:
            # Counter exists, increment it
            counter = int(match.group(1)) + 1
            new_text = re.sub(r"\d+ inserted$", f"{counter} inserted", text)
        else:
            # Counter does not exist, add it
            new_text = f"{text} : 1 inserted"

        item.setText(new_text)

    def decrement_counter(self, item):
        text = item.text()
        match = re.search(r"(\d+) inserted$", text)

        if match:
            counter = int(match.group(1)) - 1
            if counter > 0:
                # Update the counter
                new_text = re.sub(r"\d+ inserted$", f"{counter} inserted", text)
            elif counter == 0:
                # Remove the counter part from the text
                new_text = re.sub(r" : \d+ inserted$", "", text)
            else:
                return

            item.setText(new_text)

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
        self.insertionStack[-1]["addedObject"].Placement.Base = newPos

    def clickMouse(self, info):
        if info["Button"] == "BUTTON1" and info["State"] == "DOWN":
            Gui.Selection.clearSelection()
            if info["ShiftDown"]:
                # Create a new link and moves this one now
                addedObject = self.insertionStack[-1]["addedObject"]
                currentPos = addedObject.Placement.Base
                selectedPart = addedObject
                if addedObject.TypeId == "App::Link":
                    selectedPart = addedObject.LinkedObject

                addedObject = self.assembly.newObject("App::Link", selectedPart.Label)
                addedObject.LinkedObject = selectedPart
                addedObject.Placement.Base = currentPos

                insertionDict = {}
                insertionDict["translation"] = App.Vector()
                insertionDict["item"] = self.insertionStack[-1]["item"]
                insertionDict["addedObject"] = addedObject
                self.insertionStack.append(insertionDict)

            else:
                self.endMove()

        elif info["Button"] == "BUTTON2" and info["State"] == "DOWN":
            self.dismissPart()

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            self.dismissPart()

    def dismissPart(self):
        self.endMove()
        stack_item = self.insertionStack.pop()
        self.totalTranslation -= stack_item["translation"]
        UtilsAssembly.removeObjAndChilds(stack_item["addedObject"])
        self.decrement_counter(stack_item["item"])

    # Taskbox keyboard event handler
    def eventFilter(self, watched, event):
        if watched == self.form and event.type() == QtCore.QEvent.KeyPress:
            if event.key() == QtCore.Qt.Key_Escape and self.partMoving:
                self.dismissPart()
                return True  # Consume the event

        if event.type() == QtCore.QEvent.ContextMenu and watched is self.form.partList:
            item = watched.itemAt(event.pos())

            if item:
                # Iterate through the insertionStack in reverse
                for i in reversed(range(len(self.insertionStack))):
                    stack_item = self.insertionStack[i]

                    if stack_item["item"] == item:
                        if self.partMoving:
                            self.endMove()

                        self.totalTranslation -= stack_item["translation"]
                        UtilsAssembly.removeObjAndChilds(stack_item["addedObject"])
                        self.decrement_counter(item)
                        del self.insertionStack[i]
                        self.form.partList.setItemSelected(item, False)

                        return True

        return super().eventFilter(watched, event)

    def getTranslationVec(self, part):
        bb = part.Shape.BoundBox
        if bb:
            translation = (bb.XMax + bb.YMax + bb.ZMax) * 0.15
        else:
            translation = 10
        return App.Vector(translation, translation, translation)


if App.GuiUp:
    Gui.addCommand("Assembly_InsertLink", CommandInsertLink())
