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
    from PySide import QtCore, QtGui, QtWidgets, QtUiTools
    from PySide.QtWidgets import QPushButton, QMenu

import UtilsAssembly
import Preferences

# translate = App.Qt.translate

__title__ = "Assembly Command Create Bill of Materials"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"

translate = App.Qt.translate

TranslatedColumnNames = [
    translate("Assembly", "Index"),
    translate("Assembly", "Name"),
    translate("Assembly", "Description"),
    translate("Assembly", "File Name"),
    translate("Assembly", "Quantity"),
]

ColumnNames = [
    "Index",
    "Name",
    "Description",
    "File Name",
    "Quantity",
]


class CommandCreateBom:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_BillOfMaterials",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_BillOfMaterials", "Create Bill of Materials"),
            "Accel": "O",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateView",
                "Create a bill of materials of the current assembly.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateView",
                "The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the bom. If you don't need the BOM object to be saved as a document object, you can simply export and cancel the task.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateView",
                "The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwriten.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            UtilsAssembly.isAssemblyCommandActive()
            and UtilsAssembly.assembly_has_at_least_n_parts(1)
        )

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return

        self.panel = TaskAssemblyCreateBom()
        Gui.Control.showDialog(self.panel)


######### Create Exploded View Task ###########
class TaskAssemblyCreateBom(QtCore.QObject):
    def __init__(self, bomObj=None):
        super().__init__()

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateBom.ui")

        # Set the QListWidget properties to support drag and drop
        self.form.columnList.setEditTriggers(
            QtWidgets.QAbstractItemView.DoubleClicked | QtWidgets.QAbstractItemView.EditKeyPressed
        )
        self.form.columnList.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.form.columnList.setDragEnabled(True)
        self.form.columnList.setAcceptDrops(True)
        self.form.columnList.setDropIndicatorShown(True)
        self.form.columnList.setDragDropMode(QtWidgets.QAbstractItemView.InternalMove)

        self.form.columnList.installEventFilter(self)

        self.form.btnAddColumn.clicked.connect(self.addColumn)
        self.form.btnExport.clicked.connect(self.export)

        self.assembly = UtilsAssembly.activeAssembly()

        pref = Preferences.preferences()

        if bomObj:
            App.setActiveTransaction("Edit Bill Of Materials")

            names = []
            for i in range(len(bomObj.columnsNames)):
                text = bomObj.columnsNames[i]
                if text in ColumnNames:
                    index = ColumnNames.index(text)
                    text = TranslatedColumnNames[index]
                names.append(text)
            self.form.columnList.addItems(names)

            self.bomObj = bomObj
            self.form.CheckBox_onlyParts.setChecked(bomObj.onlyParts)
            self.form.CheckBox_detailParts.setChecked(bomObj.detailParts)
            self.form.CheckBox_detailSubAssemblies.setChecked(bomObj.detailSubAssemblies)

        else:
            App.setActiveTransaction("Create Bill Of Materials")
            self.form.columnList.addItems(TranslatedColumnNames)

            self.createBomObject()
            self.form.CheckBox_onlyParts.setChecked(pref.GetBool("BOMOnlyParts", False))
            self.form.CheckBox_detailParts.setChecked(pref.GetBool("BOMDetailParts", True))
            self.form.CheckBox_detailSubAssemblies.setChecked(
                pref.GetBool("BOMDetailSubAssemblies", True)
            )

        self.form.columnList.model().rowsMoved.connect(self.onItemsReordered)
        self.form.columnList.itemChanged.connect(self.itemUpdated)

        self.form.CheckBox_onlyParts.stateChanged.connect(self.onIncludeSolids)
        self.form.CheckBox_detailParts.stateChanged.connect(self.onDetailParts)
        self.form.CheckBox_detailSubAssemblies.stateChanged.connect(self.onDetailSubAssemblies)

        self.updateColumnList()

    def accept(self):
        self.deactivate()
        App.closeActiveTransaction()

        self.bomObj.recompute()

        self.bomObj.ViewObject.showSheetMdi()

        return True

    def reject(self):
        self.deactivate()
        App.closeActiveTransaction(True)
        return True

    def deactivate(self):
        pref = Preferences.preferences()
        pref.SetBool("BOMOnlyParts", self.form.CheckBox_onlyParts.isChecked())
        pref.SetBool("BOMDetailParts", self.form.CheckBox_detailParts.isChecked())
        pref.SetBool("BOMDetailSubAssemblies", self.form.CheckBox_detailSubAssemblies.isChecked())

        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()

    def onIncludeSolids(self, val):
        self.bomObj.onlyParts = val

    def onDetailParts(self, val):
        self.bomObj.detailParts = val

    def onDetailSubAssemblies(self, val):
        self.bomObj.detailSubAssemblies = val

    def addColumn(self):
        new_name = translate("Assembly", "Default")
        if self.isNameDuplicate(new_name):
            # Find a unique name
            counter = 1
            while self.isNameDuplicate(f"{new_name}_{counter}"):
                counter += 1
            new_name = f"{new_name}_{counter}"

        new_item = QtWidgets.QListWidgetItem(new_name)
        new_item.setFlags(new_item.flags() | QtCore.Qt.ItemIsEditable)
        self.form.columnList.addItem(new_item)

        # Ensure the new item is selected and starts editing
        self.form.columnList.setCurrentItem(new_item)
        self.form.columnList.editItem(new_item)
        self.updateColumnList()

    def onItemsReordered(self, parent, start, end, destination, row):
        self.updateColumnList()

    def updateColumnList(self):
        if self.bomObj:
            new_names = []
            for i in range(self.form.columnList.count()):
                text = self.form.columnList.item(i).text()
                if text in TranslatedColumnNames:
                    index = TranslatedColumnNames.index(text)
                    text = ColumnNames[index]
                new_names.append(text)
            self.bomObj.columnsNames = new_names

    def itemUpdated(self, item):
        new_text = item.text()
        item_row = self.form.columnList.row(item)

        # Check for duplicate names
        duplicate_found = False
        for i in range(self.form.columnList.count()):
            if i != item_row and self.form.columnList.item(i).text() == new_text:
                duplicate_found = True
                break

        if duplicate_found:
            QtWidgets.QMessageBox.warning(
                self.form,
                translate("Assembly", "Duplicate Name"),
                translate("Assembly", "This name is already used. Please choose a different name."),
            )

            # Revert the change
            old_text = (
                self.bomObj.columnsNames[item_row]
                if self.bomObj and item_row < len(self.bomObj.columnsNames)
                else ""
            )
            item.setText(old_text)
        else:
            self.updateColumnList()

    def isNameDuplicate(self, name):
        for i in range(self.form.columnList.count()):
            if self.form.columnList.item(i).text() == name:
                return True
        return False

    def createBomObject(self):
        bom_group = UtilsAssembly.getBomGroup(self.assembly)
        self.bomObj = bom_group.newObject("Assembly::BomObject", "Bill of Materials")

    def export(self):
        self.bomObj.recompute()
        self.bomObj.ViewObject.exportAsFile()

    def eventFilter(self, watched, event):
        if self.form is not None and watched == self.form.columnList:
            if event.type() == QtCore.QEvent.ShortcutOverride:
                if event.key() == QtCore.Qt.Key_Delete:
                    event.accept()  # Accept the event only if the key is Delete
                    return True  # Indicate that the event has been handled
                return False

            elif event.type() == QtCore.QEvent.KeyPress:
                if event.key() == QtCore.Qt.Key_Delete:
                    selected_indexes = self.form.columnList.selectedIndexes()
                    items_to_remove = []

                    for index in selected_indexes:
                        self.form.columnList.takeItem(index.row())

                    self.updateColumnList()
                    return True  # Consume the event

        return super().eventFilter(watched, event)


if App.GuiUp:
    Gui.addCommand("Assembly_CreateBom", CommandCreateBom())
