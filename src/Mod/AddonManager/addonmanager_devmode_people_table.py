# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Contains a wrapper class for a table listing authors and maintainers """

import os

from PySide.QtWidgets import QTableWidgetItem
from PySide.QtGui import QIcon
from PySide.QtCore import Qt

import FreeCAD
import FreeCADGui

from addonmanager_devmode_person_editor import PersonEditor

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class PeopleTable:
    """A QTableWidget and associated buttons for managing the list of authors and maintainers."""

    def __init__(self):
        self.widget = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_people_table.ui")
        )

        self.widget.addButton.setIcon(QIcon.fromTheme("add", QIcon(":/icons/list-add.svg")))
        self.widget.removeButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )

        self.widget.addButton.clicked.connect(self._add_clicked)
        self.widget.removeButton.clicked.connect(self._remove_clicked)
        self.widget.tableWidget.itemSelectionChanged.connect(self._selection_changed)
        self.widget.tableWidget.itemDoubleClicked.connect(self._edit)
        self.metadata = None

    def show(self, metadata):
        """Set up the widget based on incoming metadata"""
        self.metadata = metadata
        self._populate_from_metadata()
        self.widget.removeButton.setDisabled(True)
        self.widget.show()

    def _populate_from_metadata(self):
        """Use the passed metadata object to populate the maintainers and authors"""
        self.widget.tableWidget.setRowCount(0)
        row = 0
        for maintainer in self.metadata.Maintainer:
            name = maintainer["name"]
            email = maintainer["email"]
            self._add_row(row, "maintainer", name, email)
            row += 1
        for author in self.metadata.Author:
            name = author["name"]
            email = author["email"]
            self._add_row(row, "author", name, email)
            row += 1

    def _add_row(self, row, person_type, name, email):
        """Add this person to the tableWidget at row given"""
        person_type_translation = {
            "maintainer": translate("AddonsInstaller", "Maintainer"),
            "author": translate("AddonsInstaller", "Author"),
        }
        self.widget.tableWidget.insertRow(row)
        item = QTableWidgetItem(person_type_translation[person_type])
        item.setData(Qt.UserRole, person_type)
        self.widget.tableWidget.setItem(row, 0, item)
        self.widget.tableWidget.setItem(row, 1, QTableWidgetItem(name))
        self.widget.tableWidget.setItem(row, 2, QTableWidgetItem(email))

    def _add_clicked(self):
        """Callback: the Add Person button was clicked"""
        dlg = PersonEditor()
        person_type, name, email = dlg.exec()
        if person_type and name:
            self._add_row(self.widget.tableWidget.rowCount(), person_type, name, email)
            if person_type == "maintainer":
                self.metadata.addMaintainer(name, email)
            else:
                self.metadata.addAuthor(name, email)

    def _remove_clicked(self):
        """Callback: the Remove Person button was clicked"""
        items = self.widget.tableWidget.selectedIndexes()
        if items:
            # We only support single-selection, so can just pull the row # from
            # the first entry
            row = items[0].row()
            person_type = self.widget.tableWidget.item(row, 0).data(Qt.UserRole)
            name = self.widget.tableWidget.item(row, 1).text()
            email = self.widget.tableWidget.item(row, 2).text()
            self.widget.tableWidget.removeRow(row)
            if person_type == "maintainer":
                self.metadata.removeMaintainer(name, email)
            else:
                self.metadata.removeAuthor(name, email)

    def _edit(self, item):
        """Callback: a row in the tableWidget was double-clicked"""
        row = item.row()
        person_type = self.widget.tableWidget.item(row, 0).data(Qt.UserRole)
        name = self.widget.tableWidget.item(row, 1).text()
        email = self.widget.tableWidget.item(row, 2).text()

        dlg = PersonEditor()
        dlg.setup(person_type, name, email)
        new_person_type, new_name, new_email = dlg.exec()

        if new_person_type and new_name:
            self.widget.tableWidget.removeRow(row)
            if person_type == "maintainer":
                self.metadata.removeMaintainer(name, email)
            else:
                self.metadata.removeAuthor(name, email)
            self._add_row(row, new_person_type, new_name, email)
            if new_person_type == "maintainer":
                self.metadata.addMaintainer(new_name, new_email)
            else:
                self.metadata.addAuthor(new_name, new_email)
            self.widget.tableWidget.selectRow(row)

    def _selection_changed(self):
        """Callback: the current selection in the tableWidget changed"""
        items = self.widget.tableWidget.selectedItems()
        if items:
            self.widget.removeButton.setDisabled(False)
        else:
            self.widget.removeButton.setDisabled(True)
