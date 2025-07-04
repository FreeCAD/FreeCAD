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
    from PySide.QtWidgets import QPushButton, QMenu

import UtilsAssembly
import Preferences
from functools import partial

__title__ = "Assembly Command Create Bill of Materials"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"

translate = App.Qt.translate

TranslatedColumnNames = [
    translate("Assembly", "Index (auto)"),
    translate("Assembly", "Name (auto)"),
    translate("Assembly", "Description"),
    translate("Assembly", "File Name (auto)"),
    translate("Assembly", "Quantity (auto)"),
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
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateBom", "Bill of Materials"),
            "Accel": "O",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateBom",
                "Creates a bill of materials of the current assembly. If an assembly is active, it will be a BOM of this assembly. Else it will be a BOM of the whole document.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateBom",
                "The BOM object is a document object that stores the settings of your BOM. It is also a spreadsheet object so you can easily visualize the BOM. If you do not need the BOM object to be saved as a document object, you can simply export and cancel the task.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateBom",
                "The columns 'Index', 'Name', 'File Name' and 'Quantity' are automatically generated on recompute. The 'Description' and custom columns are not overwritten.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return True

    def Activated(self):
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

        self.form.btnAddColumn.clicked.connect(self.showAddColumnMenu)
        self.form.btnExport.clicked.connect(self.export)

        self.form.helpButton.clicked.connect(self.showHelpDialog)

        pref = Preferences.preferences()

        if bomObj:
            App.setActiveTransaction("Edit Bill Of Materials")

            for name in bomObj.columnsNames:
                if name in ColumnNames:
                    index = ColumnNames.index(name)
                    name = TranslatedColumnNames[index]

                self.addColItem(name)

            self.bomObj = bomObj
            self.form.CheckBox_onlyParts.setChecked(bomObj.onlyParts)
            self.form.CheckBox_detailParts.setChecked(bomObj.detailParts)
            self.form.CheckBox_detailSubAssemblies.setChecked(bomObj.detailSubAssemblies)

        else:
            App.setActiveTransaction("Create Bill Of Materials")

            # Add the columns
            for name in TranslatedColumnNames:
                self.addColItem(name)

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

        item = self.addColItem(new_name)

        # Ensure the new item is selected and starts editing
        self.form.columnList.setCurrentItem(item)
        self.form.columnList.editItem(item)
        self.updateColumnList()

    def addColItem(self, name):
        item = QtWidgets.QListWidgetItem(name)

        isCustomCol = self.isCustomColumn(name)

        if isCustomCol:
            item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
        else:
            font = item.font()
            font.setBold(True)
            item.setFont(font)

        self.form.columnList.addItem(item)
        return item

    def showAddColumnMenu(self):
        menu = QtWidgets.QMenu()
        # Get the current columns in the list
        current_columns = [
            self.form.columnList.item(i).text() for i in range(self.form.columnList.count())
        ]

        # Add actions for columns that are not currently in the list
        noneAdded = True
        for name in TranslatedColumnNames:
            if name not in current_columns:
                action = QtWidgets.QAction(f"Add '{name}' column", self)
                action.triggered.connect(partial(self.addColItem, name))
                menu.addAction(action)
                noneAdded = False

        if noneAdded:
            self.addColumn()
            return

        # Add the action for adding a custom column
        action = QtWidgets.QAction("Add custom column", self)
        action.triggered.connect(self.addColumn)
        menu.addAction(action)

        # Show the menu below the button
        menu.exec_(
            self.form.btnAddColumn.mapToGlobal(QtCore.QPoint(0, self.form.btnAddColumn.height()))
        )

    def isCustomColumn(self, name):
        isCustomCol = True
        if name in TranslatedColumnNames:
            # Description column is currently not auto generated so it's a custom column
            index = TranslatedColumnNames.index(name)
            if ColumnNames[index] != "Description":
                isCustomCol = False
        return isCustomCol

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
            isCustomCol = self.isCustomColumn(new_text)

            if not isCustomCol:
                font = item.font()
                font.setBold(True)
                item.setFont(font)
                # Use a single-shot timer to defer changing the flags (else FC crashes)
                QtCore.QTimer.singleShot(0, lambda: self.makeItemNonEditable(item))

            self.updateColumnList()

    def makeItemNonEditable(self, item):
        item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)

    def isNameDuplicate(self, name):
        for i in range(self.form.columnList.count()):
            if self.form.columnList.item(i).text() == name:
                return True
        return False

    def createBomObject(self):
        assembly = UtilsAssembly.activeAssembly()
        Gui.addModule("UtilsAssembly")
        if assembly is not None:
            commands = (
                "assembly = UtilsAssembly.activeAssembly()\n"
                "bom_group = UtilsAssembly.getBomGroup(assembly)\n"
                'bomObj = bom_group.newObject("Assembly::BomObject", "Bill of Materials")'
            )
        else:
            commands = 'bomObj = App.activeDocument().addObject("Assembly::BomObject", "Bill of Materials")'
        Gui.doCommand(commands)
        self.bomObj = Gui.doCommandEval("bomObj")

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

    def showHelpDialog(self):
        help_dialog = QtWidgets.QDialog(self.form)
        help_dialog.setWindowFlags(QtCore.Qt.Popup)
        help_dialog.setWindowModality(QtCore.Qt.NonModal)
        help_dialog.setAttribute(QtCore.Qt.WA_DeleteOnClose)

        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(10, 10, 10, 10)

        options_title = QtWidgets.QLabel("<b>" + translate("Assembly", "Options") + "</b>")
        options_text = QtWidgets.QLabel(
            " - "
            + translate(
                "Assembly",
                "Sub-assembly children: the children of sub-assemblies will be included in the bill of materials",
            )
            + "\n"
            " - "
            + translate(
                "Assembly",
                "Parts children: the children of parts will be added to the bill of materials",
            )
            + "\n"
            " - "
            + translate(
                "Assembly",
                "Only parts: adds only part containers and sub-assemblies to the bill of materials. Solids like Part Design bodies, fasteners, or Part workbench primitives are ignored.",
            )
            + "\n"
        )
        columns_title = QtWidgets.QLabel("<b>" + translate("Assembly", "Columns") + "</b>")
        columns_text = QtWidgets.QLabel(
            " - "
            + translate(
                "Assembly",
                "Auto columns :  (Index, Quantity, Name...) are populated automatically. Any modification you make will be overridden. These columns cannot be renamed.",
            )
            + "\n"
            " - "
            + translate(
                "Assembly",
                "Custom columns : 'Description' and other custom columns you add by clicking on 'Add column' will not have their data overwritten. If a column name starts with '.' followed by a property name (e.g. '.Length'), it will be auto-populated with that property value. These columns can be renamed by double-clicking or pressing F2 (renaming a column will currently lose its data).",
            )
            + "\n"
            "\n"
            + translate(
                "Assembly",
                "Any column (custom or not), can be deleted by pressing the Delete key",
            )
            + "\n"
        )
        export_title = QtWidgets.QLabel("<b>" + translate("Assembly", "Export") + "</b>")
        export_text = QtWidgets.QLabel(
            " - "
            + translate(
                "Assembly",
                "The exported file format can be customized in the Spreadsheet workbench preferences",
            )
            + "\n"
        )

        options_text.setWordWrap(True)
        columns_text.setWordWrap(True)
        export_text.setWordWrap(True)

        layout.addWidget(options_title)
        layout.addWidget(options_text)
        layout.addWidget(columns_title)
        layout.addWidget(columns_text)
        layout.addWidget(export_title)
        layout.addWidget(export_text)

        help_dialog.setLayout(layout)
        help_dialog.setFixedWidth(500)

        help_dialog.show()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateBom", CommandCreateBom())
