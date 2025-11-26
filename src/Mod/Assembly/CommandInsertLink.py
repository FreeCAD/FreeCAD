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

import re
import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets
    from PySide.QtGui import QIcon

import UtilsAssembly
import Preferences
import CommandCreateJoint


__title__ = "Assembly Command Insert Component"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


tooltip = QT_TRANSLATE_NOOP(
    "Assembly_InsertLink",
    "<p>Inserts a component into the active assembly. This will create dynamic links to parts, bodies, primitives, and assemblies. To insert external components, make sure that the file is <b>open in the current session</b></p>"
    "<ul>"
    "<li>Insert by left clicking items in the list.</li>"
    "<li>Remove by right clicking items in the list.</li>"
    "<li>Press shift to add several instances of the component while clicking on the view.</li>"
    "</ul>",
)


class CommandGroupInsert:
    def GetCommands(self):
        return ("Assembly_InsertLink", "Assembly_InsertNewPart")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {
            "Pixmap": "Assembly_InsertLink",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_Insert", "Insert Component"),
            "ToolTip": tooltip,
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.isAssemblyCommandActive()


class CommandInsertLink:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_InsertLink",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_InsertLink", "Component"),
            "Accel": "I",
            "ToolTip": tooltip,
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


class InsertLinkObserver:
    def __init__(self, callback):
        self.callback = callback

    def slotDeletedObject(self, obj):
        self.callback(obj)


class TaskAssemblyInsertLink(QtCore.QObject):
    def __init__(self, assembly, view):
        super().__init__()

        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument
        self.showHidden = False

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyInsertLink.ui")
        self.form.installEventFilter(self)
        self.form.partList.installEventFilter(self)

        pref = Preferences.preferences()
        self.form.CheckBox_ShowOnlyParts.setChecked(pref.GetBool("InsertShowOnlyParts", False))
        self.form.CheckBox_RigidSubAsm.setChecked(pref.GetBool("InsertRigidSubAssemblies", True))

        # Actions
        self.form.openFileButton.clicked.connect(self.openFiles)
        self.form.partList.itemClicked.connect(self.onItemClicked)
        self.form.filterPartList.textChanged.connect(self.onFilterChange)
        self.form.CheckBox_ShowOnlyParts.stateChanged.connect(self.buildPartList)

        self.form.partList.header().hide()

        self.translation = 0
        # self.partMoving = False
        self.totalTranslation = App.Vector()
        self.prevScreenCenter = App.Vector()
        self.groundedObj = None

        self.insertionStack = []  # used to handle cancellation of insertions.
        self.doc_item_map = {}

        self.buildPartList()

        App.setActiveTransaction("Insert Component")

        # Listen for external deletions to keep the list in sync
        self.docObserver = InsertLinkObserver(self.onObjectDeleted)
        App.addDocumentObserver(self.docObserver)

    def accept(self):
        self.deactivated()

        Gui.addModule("UtilsAssembly")
        commands = "assembly = UtilsAssembly.activeAssembly()\n"
        for insertionItem in self.insertionStack:
            object = insertionItem["addedObject"]
            translation = insertionItem["translation"]

            # Check if object.Name & object.LinkedObject.Name exists
            if (
                not hasattr(object, "Name")
                or not hasattr(object, "LinkedObject")
                or not hasattr(object.LinkedObject, "Name")
            ):
                continue

            commands = commands + (
                f'item = assembly.newObject("App::Link", "{object.Name}")\n'
                f'item.LinkedObject = App.ActiveDocument.getObject("{object.LinkedObject.Name}")\n'
                f'item.Label = "{object.Label}"\n'
            )

            if translation != App.Vector():
                commands = commands + (
                    f"item.Placement.base = App.Vector({translation.x},"
                    f"{translation.y},"
                    f"{translation.z})\n"
                )

        # Ground the first item if that happened
        if self.groundedObj:
            commands = (
                commands
                + f'CommandCreateJoint.createGroundedJoint(App.ActiveDocument.getObject("{self.groundedObj.Name}"))\n'
            )

        Gui.doCommandSkip(commands[:-1])  # Get rid of last \n
        App.closeActiveTransaction()
        return True

    def reject(self):
        self.deactivated()

        App.closeActiveTransaction(True)
        return True

    def deactivated(self):
        if hasattr(self, "docObserver") and self.docObserver:
            App.removeDocumentObserver(self.docObserver)
            self.docObserver = None

        pref = Preferences.preferences()
        pref.SetBool("InsertShowOnlyParts", self.form.CheckBox_ShowOnlyParts.isChecked())
        pref.SetBool("InsertRigidSubAssemblies", self.form.CheckBox_RigidSubAsm.isChecked())
        Gui.Selection.clearSelection()

    def buildPartList(self):
        self.form.partList.clear()

        docList = App.listDocuments().values()

        for doc in docList:
            # Create a new tree item for the document
            docItem = QtGui.QTreeWidgetItem()
            itemName = doc.Label
            icon = QIcon.fromTheme("add", QIcon(":/icons/Document.svg"))
            if doc.Partial:
                itemName = (
                    itemName + " (" + QT_TRANSLATE_NOOP("Assembly_Insert", "Partially loaded") + ")"
                )
                icon = self.createDisabledIcon(icon)
            docItem.setText(0, itemName)
            docItem.setIcon(0, icon)
            self.doc_item_map[docItem] = doc

            if not any(
                (child.isDerivedFrom("Part::Feature") or child.isDerivedFrom("App::Part"))
                for child in doc.Objects
            ):
                continue  # Skip this doc if no relevant objects

            self.form.partList.addTopLevelItem(docItem)

            def process_objects(objs, item):
                onlyParts = self.form.CheckBox_ShowOnlyParts.isChecked()
                for obj in objs:
                    if obj == self.assembly:
                        continue  # Skip current assembly

                    if obj in self.assembly.InListRecursive:
                        continue  # Prevent dependency loop.
                        # For instance if asm1/asm2 with asm2 active, we don't want to have asm1 in the list

                    if not obj.ViewObject.ShowInTree and not self.showHidden:
                        continue

                    if (
                        obj.isDerivedFrom("Part::Feature")
                        or obj.isDerivedFrom("App::Part")
                        or obj.isDerivedFrom("App::DocumentObjectGroup")
                    ):
                        # Special handling for DocumentObjectGroup: only add if it contains relevant child objects
                        if obj.isDerivedFrom("App::DocumentObjectGroup"):
                            if not any(
                                (
                                    (not onlyParts and child.isDerivedFrom("Part::Feature"))
                                    or child.isDerivedFrom("App::Part")
                                )
                                for child in obj.ViewObject.claimChildrenRecursive()
                            ):
                                continue  # Skip this object if no relevant children

                        if obj.isDerivedFrom("Part::Feature"):
                            if onlyParts:
                                continue  # Ignore solids if we show only Parts

                        # Now add the object under the document item
                        objItem = QtGui.QTreeWidgetItem(item)
                        objItem.setText(0, obj.Label)
                        objItem.setIcon(
                            0, obj.ViewObject.Icon if hasattr(obj, "ViewObject") else QtGui.QIcon()
                        )  # Use object's icon if available

                        if not obj.isDerivedFrom("App::DocumentObjectGroup"):
                            objItem.setData(0, QtCore.Qt.UserRole, obj)

                        if obj.isDerivedFrom("App::Part") or obj.isDerivedFrom(
                            "App::DocumentObjectGroup"
                        ):
                            process_objects(obj.ViewObject.claimChildren(), objItem)

            guiDoc = Gui.getDocument(doc.Name)
            process_objects(guiDoc.TreeRootObjects, docItem)
            self.form.partList.expandAll()

        self.adjustTreeWidgetSize()

    def adjustTreeWidgetSize(self):
        # Adjust the height of the part list based on item count
        item_count = 1

        def count_items(item):
            nonlocal item_count
            item_count += 1
            for i in range(item.childCount()):
                count_items(item.child(i))

        for i in range(self.form.partList.topLevelItemCount()):
            count_items(self.form.partList.topLevelItem(i))

        item_height = self.form.partList.sizeHintForRow(0)
        total_height = item_count * item_height
        max_height = 500

        self.form.partList.setMinimumHeight(min(total_height, max_height))

    def onFilterChange(self):
        filter_str = self.form.filterPartList.text().strip().lower()

        def filter_tree_item(item):
            # This function recursively filters items based on the filter string.
            item_text = item.text(0).lower()  # Assuming the relevant text is in the first column
            is_visible = filter_str in item_text if filter_str else True

            child_count = item.childCount()
            for i in range(child_count):
                child = item.child(i)
                child_is_visible = filter_tree_item(child)  # Recursively filter children
                is_visible = (
                    is_visible or child_is_visible
                )  # Parent is visible if any child matches

            item.setHidden(not is_visible)
            return is_visible

        root_count = self.form.partList.topLevelItemCount()
        for i in range(root_count):
            root_item = self.form.partList.topLevelItem(i)
            filter_tree_item(root_item)  # Filter from each root item

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
                    App.openDocument(filename, True)
                    App.setActiveDocument(self.doc.Name)
                    self.buildPartList()

    def onItemClicked(self, item):
        selectedPart = item.data(0, QtCore.Qt.UserRole)
        if not selectedPart:
            # If there's no part associated, toggle the expanded state
            item.setExpanded(not item.isExpanded())
            return

        # check that the current document had been saved or that it's the same document as that of the selected part
        if not self.doc == selectedPart.Document:
            if self.doc.FileName == "":
                msgBox = QtWidgets.QMessageBox()
                msgBox.setIcon(QtWidgets.QMessageBox.Warning)
                msgBox.setText(
                    "The current document must be saved before inserting external parts."
                )
                msgBox.setWindowTitle("Save Document")
                saveButton = msgBox.addButton("Save", QtWidgets.QMessageBox.AcceptRole)
                cancelButton = msgBox.addButton("Cancel", QtWidgets.QMessageBox.RejectRole)

                msgBox.exec_()

                if not (msgBox.clickedButton() == saveButton and Gui.ActiveDocument.saveAs()):
                    return

            # check that the selectedPart document is saved.
            if selectedPart.Document.FileName == "":
                msgBox = QtWidgets.QMessageBox()
                msgBox.setIcon(QtWidgets.QMessageBox.Warning)
                msgBox.setText("The selected object's document must be saved before inserting it.")
                msgBox.setWindowTitle("Save Document")
                saveButton = msgBox.addButton("Save", QtWidgets.QMessageBox.AcceptRole)
                cancelButton = msgBox.addButton("Cancel", QtWidgets.QMessageBox.RejectRole)

                msgBox.exec_()

                if not (
                    msgBox.clickedButton() == saveButton
                    and selectedPart.ViewObject.Document.saveAs()
                ):
                    return

                # Update the document item text - useless because Document.Name still return 'Unnamed'
                """documentItem = item
                while documentItem.parent() is not None:
                    documentItem = documentItem.parent()
                newDocName = selectedPart.Document.Name
                print(selectedPart.Document.Name)
                documentItem.setText(0, f"{newDocName}.FCStd")"""

        if selectedPart.isDerivedFrom("Assembly::AssemblyObject"):
            objType = "Assembly::AssemblyLink"
        else:
            objType = "App::Link"

        addedObject = self.assembly.newObject(objType, selectedPart.Label)

        # set placement of the added object to the center of the screen.
        view = Gui.activeView()
        x, y = view.getSize()
        screenCenter = view.getPointOnFocalPlane(x // 2, y // 2)
        screenCorner = view.getPointOnFocalPlane(x, y)

        addedObject.LinkedObject = selectedPart
        addedObject.Label = selectedPart.Label  # non-ASCII characters fails with newObject. #12164
        addedObject.recompute()

        insertionDict = {}
        insertionDict["item"] = item
        insertionDict["addedObject"] = addedObject
        self.insertionStack.append(insertionDict)
        self.increment_counter(item)

        translation = App.Vector()
        resetThreshold = (screenCorner - screenCenter).Length * 0.1
        if len(self.insertionStack) == 1:
            translation = App.Vector()  # No translation for first object.
        elif (self.prevScreenCenter - screenCenter).Length > resetThreshold:
            self.totalTranslation = App.Vector()
            self.prevScreenCenter = screenCenter
        else:
            translation = self.getTranslationVec(addedObject)

        insertionDict["translation"] = translation
        self.totalTranslation += translation

        originX, originY = view.getPointOnViewport(App.Vector() + translation)
        if originX > 0 and originX < x and originY > 0 and originY < y:
            # If the origin is within view then we insert at the origin.
            addedObject.Placement.Base = self.totalTranslation
        else:
            #
            bboxCenter = addedObject.ViewObject.getBoundingBox().Center
            addedObject.Placement.Base = screenCenter - bboxCenter + self.totalTranslation

        self.prevScreenCenter = screenCenter

        # We turn it flexible after changing the position so that it uses the logic in
        # AssemblyLink::onChanged to handle positioning correctly.
        if selectedPart.isDerivedFrom("Assembly::AssemblyObject"):
            addedObject.Rigid = self.form.CheckBox_RigidSubAsm.isChecked()

        # highlight the link
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.doc.Name, addedObject.Name, "")

        item.setSelected(False)

        if len(self.insertionStack) == 1 and not UtilsAssembly.isAssemblyGrounded():
            self.handleFirstInsertion()

    def handleFirstInsertion(self):
        pref = Preferences.preferences()
        fixPart = False
        fixPartPref = pref.GetInt("GroundFirstPart", 0)
        if fixPartPref == 0:  # unset
            msgBox = QtWidgets.QMessageBox()
            msgBox.setWindowTitle("Ground Part?")
            msgBox.setText(
                "Do you want to ground the first inserted part automatically?\nYou need at least one grounded part in your assembly."
            )
            msgBox.setIcon(QtWidgets.QMessageBox.Question)

            yesButton = msgBox.addButton("Yes", QtWidgets.QMessageBox.YesRole)
            noButton = msgBox.addButton("No", QtWidgets.QMessageBox.RejectRole)
            yesAlwaysButton = msgBox.addButton("Always", QtWidgets.QMessageBox.YesRole)
            noAlwaysButton = msgBox.addButton("Never", QtWidgets.QMessageBox.NoRole)

            msgBox.exec_()

            clickedButton = msgBox.clickedButton()
            if clickedButton == yesButton:
                fixPart = True
            elif clickedButton == yesAlwaysButton:
                fixPart = True
                pref.SetInt("GroundFirstPart", 1)
            elif clickedButton == noAlwaysButton:
                pref.SetInt("GroundFirstPart", 2)

        elif fixPartPref == 1:  # Yes always
            fixPart = True

        if fixPart:
            # Create groundedJoint.
            if len(self.insertionStack) != 1:
                return

            self.groundedObj = self.insertionStack[0]["addedObject"]
            self.groundedJoint = CommandCreateJoint.createGroundedJoint(self.groundedObj)

    def increment_counter(self, item):
        text = item.text(0)
        match = re.search(r"(\d+) inserted$", text)

        if match:
            # Counter exists, increment it
            counter = int(match.group(1)) + 1
            new_text = re.sub(r"\d+ inserted$", f"{counter} inserted", text)
        else:
            # Counter does not exist, add it
            new_text = f"{text} : 1 inserted"

        item.setText(0, new_text)

    def decrement_counter(self, item):
        text = item.text(0)
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

            item.setText(0, new_text)

    """def clickMouse(self, info):
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
            self.dismissPart()"""

    # Taskbox keyboard event handler
    def eventFilter(self, watched, event):

        if event.type() == QtCore.QEvent.ContextMenu and watched is self.form.partList:
            item = watched.itemAt(event.pos())

            if item:
                if item.parent() is None:
                    doc = self.doc_item_map.get(item)
                    if doc and doc.Partial:
                        menu = QtWidgets.QMenu()
                        load_action_text = QT_TRANSLATE_NOOP(
                            "Assembly_Insert", "Fully load document"
                        )
                        load_action = menu.addAction(load_action_text)
                        load_action.triggered.connect(lambda: self.fullyLoadDocument(doc))
                        menu.exec_(event.globalPos())
                        return True  # Event was handled

                # Iterate through the insertionStack in reverse
                for i in reversed(range(len(self.insertionStack))):
                    stack_item = self.insertionStack[i]

                    if stack_item["item"] == item:
                        obj = stack_item["addedObject"]

                        # ONLY remove the object from the document.
                        # The Observer (onObjectDeleted) will handle the rest.
                        if obj and obj.Document:
                            UtilsAssembly.removeObjAndChilds(obj)

                        return True
            else:
                menu = QtWidgets.QMenu()

                # Add the checkbox action
                showHiddenAction = QtWidgets.QAction("Show objects hidden in tree view", menu)
                showHiddenAction.setCheckable(True)
                showHiddenAction.setChecked(self.showHidden)

                # Connect the action to toggle `self.showHidden`
                showHiddenAction.toggled.connect(self.toggleShowHidden)
                menu.addAction(showHiddenAction)
                menu.exec_(event.globalPos())
                return True

        return super().eventFilter(watched, event)

    def fullyLoadDocument(self, doc_to_load):
        """Closes and re-opens a document to load it fully."""
        if not doc_to_load.FileName:
            return

        # Save UI state
        scrollbar = self.form.partList.verticalScrollBar()
        scroll_position = scrollbar.value()

        # Perform the reload
        App.open(doc_to_load.FileName)
        App.setActiveDocument(self.doc.Name)

        # Refresh the UI
        self.buildPartList()

        # Restore UI state
        scrollbar.setValue(scroll_position)

    def createDisabledIcon(self, icon):
        if icon.isNull():
            return QIcon()

        # Get a pixmap from the icon at a standard size
        pixmap = icon.pixmap(icon.actualSize(QtCore.QSize(16, 16)))

        # Ask the application's style to generate a disabled version of the pixmap
        style = QtWidgets.QApplication.style()
        disabled_pixmap = style.generatedIconPixmap(
            QtGui.QIcon.Disabled, pixmap, QtWidgets.QStyleOption()
        )

        return QIcon(disabled_pixmap)

    def toggleShowHidden(self, checked):
        self.showHidden = checked
        self.buildPartList()

    def getTranslationVec(self, part):
        bb = part.Shape.BoundBox
        if bb:
            translation = (bb.XMax + bb.YMax + bb.ZMax) * 0.15
        else:
            translation = 10
        return App.Vector(translation, translation, translation)

    def onObjectDeleted(self, obj):
        """
        Handles cleanup when an object is deleted (via Right Click OR Tree View).
        """
        # Iterate backwards to safely delete
        for i in reversed(range(len(self.insertionStack))):
            stack_item = self.insertionStack[i]

            if stack_item["addedObject"] == obj:
                # 1. Revert translation
                self.totalTranslation -= stack_item["translation"]

                # 2. Update UI counter
                item = stack_item["item"]
                self.decrement_counter(item)

                # 3. Handle Grounded Joint cleanup
                if self.groundedObj == obj:
                    if self.groundedJoint:
                        try:
                            # Remove the joint if it still exists
                            if self.groundedJoint.Document:
                                self.groundedJoint.Document.removeObject(self.groundedJoint.Name)
                        except Exception:
                            pass
                    self.groundedObj = None
                    self.groundedJoint = None

                # 4. Remove from stack
                del self.insertionStack[i]

                # 5. Clear selection
                if item:
                    item.setSelected(False)


if App.GuiUp:
    Gui.addCommand("Assembly_InsertLink", CommandInsertLink())
    Gui.addCommand("Assembly_Insert", CommandGroupInsert())
