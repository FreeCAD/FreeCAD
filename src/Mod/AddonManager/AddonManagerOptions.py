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

""" Contains a the Addon Manager's preferences dialog management class """

import os

import FreeCAD
import FreeCADGui

from PySide import QtCore
from PySide.QtGui import QIcon
from PySide.QtWidgets import (
    QWidget,
    QCheckBox,
    QComboBox,
    QDialog,
    QHeaderView,
    QRadioButton,
    QLineEdit,
    QTextEdit,
)

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class AddonManagerOptions:
    """A class containing a form element that is inserted as a FreeCAD preference page."""

    def __init__(self, _=None):
        self.form = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManagerOptions.ui")
        )
        self.table_model = CustomRepoDataModel()
        self.form.customRepositoriesTableView.setModel(self.table_model)

        self.form.addCustomRepositoryButton.setIcon(
            QIcon.fromTheme("add", QIcon(":/icons/list-add.svg"))
        )
        self.form.removeCustomRepositoryButton.setIcon(
            QIcon.fromTheme("remove", QIcon(":/icons/list-remove.svg"))
        )

        self.form.customRepositoriesTableView.horizontalHeader().setStretchLastSection(False)
        self.form.customRepositoriesTableView.horizontalHeader().setSectionResizeMode(
            0, QHeaderView.Stretch
        )
        self.form.customRepositoriesTableView.horizontalHeader().setSectionResizeMode(
            1, QHeaderView.ResizeToContents
        )

        self.form.addCustomRepositoryButton.clicked.connect(self._add_custom_repo_clicked)
        self.form.removeCustomRepositoryButton.clicked.connect(self._remove_custom_repo_clicked)
        self.form.customRepositoriesTableView.doubleClicked.connect(self._row_double_clicked)

    def saveSettings(self):
        """Required function: called by the preferences dialog when Apply or Save is clicked,
        saves out the preference data by reading it from the widgets."""
        for widget in self.form.children():
            self.recursive_widget_saver(widget)
        self.table_model.save_model()

    def recursive_widget_saver(self, widget):
        """Writes out the data for this widget and all of its children, recursively."""
        if isinstance(widget, QWidget):
            # See if it's one of ours:
            pref_path = widget.property("prefPath")
            pref_entry = widget.property("prefEntry")
            if pref_path and pref_entry:
                pref_path = pref_path.data()
                pref_entry = pref_entry.data()
                pref_access_string = f"User parameter:BaseApp/Preferences/{str(pref_path,'utf-8')}"
                pref = FreeCAD.ParamGet(pref_access_string)
                if isinstance(widget, QCheckBox):
                    checked = widget.isChecked()
                    pref.SetBool(str(pref_entry, "utf-8"), checked)
                elif isinstance(widget, QRadioButton):
                    checked = widget.isChecked()
                    pref.SetBool(str(pref_entry, "utf-8"), checked)
                elif isinstance(widget, QComboBox):
                    new_index = widget.currentIndex()
                    pref.SetInt(str(pref_entry, "utf-8"), new_index)
                elif isinstance(widget, QTextEdit):
                    text = widget.toPlainText()
                    pref.SetString(str(pref_entry, "utf-8"), text)
                elif isinstance(widget, QLineEdit):
                    text = widget.text()
                    pref.SetString(str(pref_entry, "utf-8"), text)
                elif widget.metaObject().className() == "Gui::PrefFileChooser":
                    filename = str(widget.property("fileName"))
                    filename = pref.SetString(str(pref_entry, "utf-8"), filename)

        # Recurse over children
        if isinstance(widget, QtCore.QObject):
            for child in widget.children():
                self.recursive_widget_saver(child)

    def loadSettings(self):
        """Required function: called by the preferences dialog when it is launched,
        loads the preference data and assigns it to the widgets."""
        for widget in self.form.children():
            self.recursive_widget_loader(widget)
        self.table_model.load_model()

    def recursive_widget_loader(self, widget):
        """Loads the data for this widget and all of its children, recursively."""
        if isinstance(widget, QWidget):
            # See if it's one of ours:
            pref_path = widget.property("prefPath")
            pref_entry = widget.property("prefEntry")
            if pref_path and pref_entry:
                pref_path = pref_path.data()
                pref_entry = pref_entry.data()
                pref_access_string = f"User parameter:BaseApp/Preferences/{str(pref_path,'utf-8')}"
                pref = FreeCAD.ParamGet(pref_access_string)
                if isinstance(widget, QCheckBox):
                    widget.setChecked(pref.GetBool(str(pref_entry, "utf-8")))
                elif isinstance(widget, QRadioButton):
                    if pref.GetBool(str(pref_entry, "utf-8")):
                        widget.setChecked(True)
                elif isinstance(widget, QComboBox):
                    new_index = pref.GetInt(str(pref_entry, "utf-8"))
                    widget.setCurrentIndex(new_index)
                elif isinstance(widget, QTextEdit):
                    text = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setText(text)
                elif isinstance(widget, QLineEdit):
                    text = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setText(text)
                elif widget.metaObject().className() == "Gui::PrefFileChooser":
                    filename = pref.GetString(str(pref_entry, "utf-8"))
                    widget.setProperty("fileName", filename)

        # Recurse over children
        if isinstance(widget, QtCore.QObject):
            for child in widget.children():
                self.recursive_widget_loader(child)

    def _add_custom_repo_clicked(self):
        """Callback: show the Add custom repo dialog"""
        dlg = CustomRepositoryDialog()
        url, branch = dlg.exec()
        if url and branch:
            self.table_model.appendData(url, branch)

    def _remove_custom_repo_clicked(self):
        """Callback: when the remove button is clicked, get the current selection and remove it."""
        item = self.form.customRepositoriesTableView.currentIndex()
        if not item.isValid():
            return
        row = item.row()
        self.table_model.removeRows(row, 1, QtCore.QModelIndex())

    def _row_double_clicked(self, item):
        """Edit the row that was double-clicked"""
        row = item.row()
        dlg = CustomRepositoryDialog()
        url_index = self.table_model.createIndex(row, 0)
        branch_index = self.table_model.createIndex(row, 1)
        dlg.dialog.urlLineEdit.setText(self.table_model.data(url_index))
        dlg.dialog.branchLineEdit.setText(self.table_model.data(branch_index))
        url, branch = dlg.exec()
        if url and branch:
            self.table_model.setData(url_index, url)
            self.table_model.setData(branch_index, branch)


class CustomRepoDataModel(QtCore.QAbstractTableModel):
    """The model for the custom repositories: wraps the underlying preference data and uses that
    as its main data store."""

    def __init__(self):
        super().__init__()
        pref_access_string = "User parameter:BaseApp/Preferences/Addons"
        self.pref = FreeCAD.ParamGet(pref_access_string)
        self.load_model()

    def load_model(self):
        """Load the data from the preferences entry"""
        pref_entry: str = self.pref.GetString("CustomRepositories", "")

        # The entry is saved as a space- and newline-delimited text block: break it into its
        # constituent parts
        lines = pref_entry.split("\n")
        self.model = []
        for line in lines:
            if not line:
                continue
            split_data = line.split()
            if len(split_data) > 1:
                branch = split_data[1]
            else:
                branch = "master"
            url = split_data[0]
            self.model.append([url, branch])

    def save_model(self):
        """Save the data into a preferences entry"""
        entry = ""
        for row in self.model:
            entry += f"{row[0]} {row[1]}\n"
        self.pref.SetString("CustomRepositories", entry)

    def rowCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """The number of rows"""
        if parent.isValid():
            return 0
        return len(self.model)

    def columnCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """The number of columns (which is always 2)"""
        if parent.isValid():
            return 0
        return 2

    def data(self, index, role=QtCore.Qt.DisplayRole):
        """The data at an index."""
        if role != QtCore.Qt.DisplayRole:
            return None
        row = index.row()
        column = index.column()
        if row > len(self.model):
            return None
        if column > 1:
            return None
        return self.model[row][column]

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        """Get the row and column header data."""
        if role != QtCore.Qt.DisplayRole:
            return None
        if orientation == QtCore.Qt.Vertical:
            return section + 1
        if section == 0:
            return translate(
                "AddonsInstaller",
                "Repository URL",
                "Preferences header for custom repositories",
            )
        if section == 1:
            return translate(
                "AddonsInstaller",
                "Branch name",
                "Preferences header for custom repositories",
            )
        return None

    def removeRows(self, row, count, parent):
        """Remove rows"""
        self.beginRemoveRows(parent, row, row + count - 1)
        for _ in range(count):
            self.model.pop(row)
        self.endRemoveRows()

    def insertRows(self, row, count, parent):
        """Insert blank rows"""
        self.beginInsertRows(parent, row, row + count - 1)
        for _ in range(count):
            self.model.insert(["", ""])
        self.endInsertRows()

    def appendData(self, url, branch):
        """Append this url and branch to the end of the list"""
        row = self.rowCount()
        self.beginInsertRows(QtCore.QModelIndex(), row, row)
        self.model.append([url, branch])
        self.endInsertRows()

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        """Set the data at this index"""
        if role != QtCore.Qt.EditRole:
            return
        self.model[index.row()][index.column()] = value
        self.dataChanged.emit(index, index)


class CustomRepositoryDialog:
    """A dialog for setting up a custom repository, with branch information"""

    def __init__(self):
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManagerOptions_AddCustomRepository.ui")
        )

    def exec(self):
        """Run the dialog modally, and return either None or a tuple or (url,branch)"""
        result = self.dialog.exec()
        if result == QDialog.Accepted:
            url = self.dialog.urlLineEdit.text()
            branch = self.dialog.branchLineEdit.text()
            return (url, branch)
        return (None, None)
