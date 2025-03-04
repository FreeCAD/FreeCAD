# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2025 The FreeCAD Project Association AISBL         *
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

"""The Change Branch dialog and utility classes and methods"""

import os
from typing import Dict

import addonmanager_freecad_interface as fci
import addonmanager_utilities as utils

from addonmanager_git import initialize_git, GitFailed

try:
    from PySide import QtWidgets, QtCore
except ImportError:
    try:
        from PySide6 import QtWidgets, QtCore
    except ImportError:
        from PySide2 import QtWidgets, QtCore  # pylint: disable=deprecated-module

translate = fci.translate


class ChangeBranchDialog(QtWidgets.QWidget):
    """A dialog that displays available git branches and allows the user to select one to change
    to. Includes code that does that change, as well as some modal dialogs to warn them of the
    possible consequences and display various error messages."""

    branch_changed = QtCore.Signal(str, str)

    def __init__(self, path: str, parent=None):
        super().__init__(parent)

        self.ui = utils.loadUi(os.path.join(os.path.dirname(__file__), "change_branch.ui"))

        self.item_filter = ChangeBranchDialogFilter()
        self.ui.tableView.setModel(self.item_filter)

        self.item_model = ChangeBranchDialogModel(path, self)
        self.item_filter.setSourceModel(self.item_model)
        self.ui.tableView.sortByColumn(
            2, QtCore.Qt.DescendingOrder
        )  # Default to sorting by remote last-changed date

        # Figure out what row gets selected:
        git_manager = initialize_git()
        if git_manager is None:
            return

        row = 0
        self.current_ref = git_manager.current_branch(path)
        selection_model = self.ui.tableView.selectionModel()
        for ref in self.item_model.branches:
            if ref["ref_name"] == self.current_ref:
                index = self.item_filter.mapFromSource(self.item_model.index(row, 0))
                selection_model.select(index, QtCore.QItemSelectionModel.ClearAndSelect)
                selection_model.select(index.siblingAtColumn(1), QtCore.QItemSelectionModel.Select)
                selection_model.select(index.siblingAtColumn(2), QtCore.QItemSelectionModel.Select)
                break
            row += 1

        # Make sure the column widths are OK:
        header = self.ui.tableView.horizontalHeader()
        header.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)

    def exec(self):
        """Run the Change Branch dialog and its various sub-dialogs. May result in the branch
        being changed. Code that cares if that happens should connect to the branch_changed
        signal."""
        if self.ui.exec() == QtWidgets.QDialog.Accepted:

            selection = self.ui.tableView.selectedIndexes()
            index = self.item_filter.mapToSource(selection[0])
            ref = self.item_model.data(index, ChangeBranchDialogModel.RefAccessRole)

            if ref["ref_name"] == self.item_model.current_branch:
                # This is the one we are already on... just return
                return

            result = QtWidgets.QMessageBox.critical(
                self,
                translate("AddonsInstaller", "DANGER: Developer feature"),
                translate(
                    "AddonsInstaller",
                    "DANGER: Switching branches is intended for developers and beta testers, "
                    "and may result in broken, non-backwards compatible documents, instability, "
                    "crashes, and/or the premature heat death of the universe. Are you sure you "
                    "want to continue?",
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
                QtWidgets.QMessageBox.Cancel,
            )
            if result == QtWidgets.QMessageBox.Cancel:
                return
            if self.item_model.dirty:
                result = QtWidgets.QMessageBox.critical(
                    self,
                    translate("AddonsInstaller", "There are local changes"),
                    translate(
                        "AddonsInstaller",
                        "WARNING: This repo has uncommitted local changes. Are you sure you want "
                        "to change branches (bringing the changes with you)?",
                    ),
                    QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
                    QtWidgets.QMessageBox.Cancel,
                )
                if result == QtWidgets.QMessageBox.Cancel:
                    return

            self.change_branch(self.item_model.path, ref)

    def change_branch(self, path: str, ref: Dict[str, str]) -> None:
        """Change the git clone in `path` to git ref `ref`. Emits the branch_changed signal
        on success."""
        remote_name = ref["ref_name"]
        _, _, local_name = ref["ref_name"].rpartition("/")
        gm = initialize_git()
        if gm is None:
            self._show_no_git_dialog()
            return

        try:
            if ref["upstream"]:
                gm.checkout(path, remote_name)
            else:
                gm.checkout(path, remote_name, args=["-b", local_name])
            self.branch_changed.emit(self.current_ref, local_name)
        except GitFailed:
            self._show_git_failed_dialog()

    def _show_no_git_dialog(self):
        QtWidgets.QMessageBox.critical(
            self,
            translate("AddonsInstaller", "Cannot find git"),
            translate(
                "AddonsInstaller",
                "Could not find git executable: cannot change branch",
            ),
            QtWidgets.QMessageBox.Ok,
            QtWidgets.QMessageBox.Ok,
        )

    def _show_git_failed_dialog(self):
        QtWidgets.QMessageBox.critical(
            self,
            translate("AddonsInstaller", "git operation failed"),
            translate(
                "AddonsInstaller",
                "Git returned an error code when attempting to change branch. There may be "
                "more details in the Report View.",
            ),
            QtWidgets.QMessageBox.Ok,
            QtWidgets.QMessageBox.Ok,
        )


class ChangeBranchDialogModel(QtCore.QAbstractTableModel):
    """The data for the dialog comes from git: this model handles the git interactions and
    returns branch information as its rows. Use user data in the RefAccessRole to get information
    about the git refs. RefAccessRole data is a dictionary defined by the GitManager class as the
    results of a `get_branches_with_info()` call."""

    branches = []
    DataSortRole = QtCore.Qt.UserRole
    RefAccessRole = QtCore.Qt.UserRole + 1

    def __init__(self, path: str, parent=None) -> None:
        super().__init__(parent)

        gm = initialize_git()
        self.path = path
        self.branches = gm.get_branches_with_info(path)
        self.current_branch = gm.current_branch(path)
        self.dirty = gm.dirty(path)
        self._remove_tracking_duplicates()

    def rowCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """Returns the number of rows in the model, e.g. the number of branches."""
        if parent.isValid():
            return 0
        return len(self.branches)

    def columnCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """Returns the number of columns in the model, e.g. the number of entries in the git ref
        structure (currently 3, 'ref_name', 'upstream', and 'date')."""
        if parent.isValid():
            return 0
        return 3  # Local name, remote name, date

    def data(self, index: QtCore.QModelIndex, role: int = QtCore.Qt.DisplayRole):
        """The data access method for this model. Supports four roles: ToolTipRole, DisplayRole,
        DataSortRole, and RefAccessRole."""
        if not index.isValid():
            return None
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.ToolTipRole:
            return self.branches[row]["author"] + ": " + self.branches[row]["subject"]
        if role == QtCore.Qt.DisplayRole:
            return self._data_display_role(column, row)
        if role == ChangeBranchDialogModel.DataSortRole:
            return self._data_sort_role(column, row)
        if role == ChangeBranchDialogModel.RefAccessRole:
            return self.branches[row]
        return None

    def _data_display_role(self, column, row):
        dd = self.branches[row]
        if column == 2:
            if dd["date"] is not None:
                q_date = QtCore.QDateTime.fromString(dd["date"], QtCore.Qt.DateFormat.RFC2822Date)
                return QtCore.QLocale().toString(q_date, QtCore.QLocale.ShortFormat)
            return None
        if column == 0:
            return dd["ref_name"]
        if column == 1:
            return dd["upstream"]
        return None

    def _data_sort_role(self, column, row):
        if column == 2:
            if self.branches[row]["date"] is not None:
                q_date = QtCore.QDateTime.fromString(
                    self.branches[row]["date"], QtCore.Qt.DateFormat.RFC2822Date
                )
                return q_date
            return None
        if column == 0:
            return self.branches[row]["ref_name"]
        if column == 1:
            return self.branches[row]["upstream"]
        return None

    def headerData(
        self,
        section: int,
        orientation: QtCore.Qt.Orientation,
        role: int = QtCore.Qt.DisplayRole,
    ):
        """Returns the header information for the data in this model."""
        if orientation == QtCore.Qt.Vertical:
            return None
        if role != QtCore.Qt.DisplayRole:
            return None
        if section == 0:
            return translate(
                "AddonsInstaller",
                "Local",
                "Table header for local git ref name",
            )
        if section == 1:
            return translate(
                "AddonsInstaller",
                "Remote tracking",
                "Table header for git remote tracking branch name",
            )
        if section == 2:
            return translate(
                "AddonsInstaller",
                "Last Updated",
                "Table header for git update date",
            )
        return None

    def _remove_tracking_duplicates(self):
        remote_tracking_branches = []
        branches_to_keep = []
        for branch in self.branches:
            if branch["upstream"]:
                remote_tracking_branches.append(branch["upstream"])
        for branch in self.branches:
            if (
                "HEAD" not in branch["ref_name"]
                and branch["ref_name"] not in remote_tracking_branches
            ):
                branches_to_keep.append(branch)
        self.branches = branches_to_keep


class ChangeBranchDialogFilter(QtCore.QSortFilterProxyModel):
    """Uses the DataSortRole in the model to provide a comparison method to sort the data."""

    def lessThan(self, left: QtCore.QModelIndex, right: QtCore.QModelIndex):
        """Compare two git refs according to the DataSortRole in the model."""
        left_data = self.sourceModel().data(left, ChangeBranchDialogModel.DataSortRole)
        right_data = self.sourceModel().data(right, ChangeBranchDialogModel.DataSortRole)
        if left_data is None or right_data is None:
            return right_data is not None
        return left_data < right_data
