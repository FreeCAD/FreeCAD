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

import FreeCAD
import FreeCADGui

from addonmanager_devmode_license_selector import LicenseSelector

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class LicensesTable:
    """A QTableWidget and associated buttons for managing the list of authors and maintainers."""

    def __init__(self):
        self.widget = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "developer_mode_licenses_table.ui")
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
        self.path_to_addon = ""

    def show(self, metadata, path_to_addon):
        """Set up the widget based on incoming metadata"""
        self.metadata = metadata
        self.path_to_addon = path_to_addon
        self._populate_from_metadata()
        self.widget.removeButton.setDisabled(True)
        self.widget.show()

    def _populate_from_metadata(self):
        """Use the passed metadata object to populate the maintainers and authors"""
        self.widget.tableWidget.setRowCount(0)
        row = 0
        for lic in self.metadata.License:
            shortcode = lic["name"]
            path = lic["file"]
            self._add_row(row, shortcode, path)
            row += 1

    def _add_row(self, row, shortcode, path):
        """Add this license to the tableWidget at row given"""
        self.widget.tableWidget.insertRow(row)
        self.widget.tableWidget.setItem(row, 0, QTableWidgetItem(shortcode))
        self.widget.tableWidget.setItem(row, 1, QTableWidgetItem(path))

    def _add_clicked(self):
        """Callback: the Add License button was clicked"""
        dlg = LicenseSelector(self.path_to_addon)
        shortcode, path = dlg.exec()
        if shortcode and path:
            self._add_row(self.widget.tableWidget.rowCount(), shortcode, path)
            self.metadata.addLicense(shortcode, path)

    def _remove_clicked(self):
        """Callback: the Remove License button was clicked"""
        items = self.widget.tableWidget.selectedIndexes()
        if items:
            # We only support single-selection, so can just pull the row # from
            # the first entry
            row = items[0].row()
            shortcode = self.widget.tableWidget.item(row, 0).text()
            path = self.widget.tableWidget.item(row, 1).text()
            self.widget.tableWidget.removeRow(row)
            self.metadata.removeLicense(shortcode, path)

    def _edit(self, item):
        """Callback: a row in the tableWidget was double-clicked"""
        row = item.row()
        shortcode = self.widget.tableWidget.item(row, 0).text()
        path = self.widget.tableWidget.item(row, 1).text()

        dlg = LicenseSelector(self.path_to_addon)
        new_shortcode, new_path = dlg.exec(shortcode, path)

        if new_shortcode and new_path:
            self.widget.tableWidget.removeRow(row)
            self.metadata.removeLicense(new_shortcode, new_path)

            self._add_row(row, new_shortcode, new_path)
            self.metadata.addLicense(new_shortcode, new_path)

            self.widget.tableWidget.selectRow(row)

    def _selection_changed(self):
        """Callback: the current selection in the tableWidget changed"""
        items = self.widget.tableWidget.selectedItems()
        if items:
            self.widget.removeButton.setDisabled(False)
        else:
            self.widget.removeButton.setDisabled(True)
