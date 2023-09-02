# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

import os

import FreeCAD
import FreeCADGui

from PySide import QtWidgets, QtCore

translate = FreeCAD.Qt.translate

try:
    import git

    NO_GIT = False
except Exception:
    NO_GIT = True


class ChangeBranchDialog(QtWidgets.QWidget):

    branch_changed = QtCore.Signal(str)

    def __init__(self, path: os.PathLike, parent=None):
        super().__init__(parent)

        self.ui = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "change_branch.ui")
        )
        self.ui.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        self.item_filter = ChangeBranchDialogFilter()
        self.ui.tableView.setModel(self.item_filter)

        self.item_model = ChangeBranchDialogModel(path, self)
        self.item_filter.setSourceModel(self.item_model)
        self.ui.tableView.sortByColumn(
            4, QtCore.Qt.DescendingOrder
        )  # Default to sorting by remote last-changed date

        # Figure out what row gets selected:
        row = 0
        current_ref = self.item_model.repo.head.ref
        selection_model = self.ui.tableView.selectionModel()
        for ref in self.item_model.refs:
            if ref == current_ref:
                index = self.item_filter.mapFromSource(self.item_model.index(row, 0))
                selection_model.select(index, QtCore.QItemSelectionModel.ClearAndSelect)
                selection_model.select(index.siblingAtColumn(1), QtCore.QItemSelectionModel.Select)
                selection_model.select(index.siblingAtColumn(2), QtCore.QItemSelectionModel.Select)
                selection_model.select(index.siblingAtColumn(3), QtCore.QItemSelectionModel.Select)
                selection_model.select(index.siblingAtColumn(4), QtCore.QItemSelectionModel.Select)
                break
            row += 1

        # Make sure the column widths are OK:
        header = self.ui.tableView.horizontalHeader()
        header.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)

    def exec(self):
        if self.ui.exec() == QtWidgets.QDialog.Accepted:

            selection = self.ui.tableView.selectedIndexes()
            index = self.item_filter.mapToSource(selection[0])
            ref = self.item_model.data(index, ChangeBranchDialogModel.RefAccessRole)

            if ref == self.item_model.repo.head.ref:
                # This is the one we are already on... just return
                return

            result = QtWidgets.QMessageBox.critical(
                self,
                translate("AddonsInstaller", "DANGER: Developer feature"),
                translate(
                    "AddonsInstaller",
                    "DANGER: Switching branches is intended for developers and beta testers, and may result in broken, non-backwards compatible documents, instability, crashes, and/or the premature heat death of the universe. Are you sure you want to continue?",
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
                QtWidgets.QMessageBox.Cancel,
            )
            if result == QtWidgets.QMessageBox.Cancel:
                return
            if self.item_model.repo.is_dirty():
                result = QtWidgets.QMessageBox.critical(
                    self,
                    translate("AddonsInstaller", "There are local changes"),
                    translate(
                        "AddonsInstaller",
                        "WARNING: This repo has uncommitted local changes. Are you sure you want to change branches (bringing the changes with you)?",
                    ),
                    QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
                    QtWidgets.QMessageBox.Cancel,
                )
                if result == QtWidgets.QMessageBox.Cancel:
                    return

            if isinstance(ref, git.TagReference):
                # Detach the head
                self.item_model.repo.head.reference = ref
                self.item_model.repo.head.reset(index=True, working_tree=True)
            elif isinstance(ref, git.RemoteReference):
                # Set up a local tracking branch
                slash_index = ref.name.find("/")
                if slash_index != -1:
                    local_name = ref.name[slash_index + 1 :]
                else:
                    local_name = ref.name
                self.item_model.repo.create_head(local_name, ref)
                self.item_model.repo.heads[local_name].set_tracking_branch(ref)
                self.item_model.repo.heads[local_name].checkout()
            else:
                # It's already a local branch, just check it out
                ref.checkout()

            self.branch_changed.emit(ref.name)


class ChangeBranchDialogModel(QtCore.QAbstractTableModel):

    refs = []
    display_data = []
    DataSortRole = QtCore.Qt.UserRole
    RefAccessRole = QtCore.Qt.UserRole + 1

    def __init__(self, path: os.PathLike, parent=None) -> None:
        super().__init__(parent)
        self.repo = git.Repo(path)

        self.refs = []
        tracking_refs = []
        for ref in self.repo.refs:
            row = ["", None, None, None, None]
            if "HEAD" in ref.name:
                continue
            if isinstance(ref, git.RemoteReference):
                if ref.name in tracking_refs:
                    # Already seen, it's the remote part of a remote tracking branch
                    continue
                else:
                    # Just a remote branch, not tracking:
                    row[0] = translate("AddonsInstaller", "Branch", "git terminology")
                    row[2] = ref.name
                    if hasattr(ref, "commit") and hasattr(ref.commit, "committed_date"):
                        row[4] = ref.commit.committed_date
                    else:
                        row[4] = ref.log_entry(0).time[0]
            elif isinstance(ref, git.TagReference):
                # Tags are simple, there is no tracking to worry about
                row[0] = translate("AddonsInstaller", "Tag", "git terminology")
                row[1] = ref.name
                row[3] = ref.commit.committed_date
            elif isinstance(ref, git.Head):
                if hasattr(ref, "tracking_branch") and ref.tracking_branch():
                    # This local branch tracks a remote: we have all five pieces of data...
                    row[0] = translate("AddonsInstaller", "Branch", "git terminology")
                    row[1] = ref.name
                    row[2] = ref.tracking_branch().name
                    row[3] = ref.commit.committed_date
                    row[4] = ref.tracking_branch().commit.committed_date
                    tracking_refs.append(ref.tracking_branch().name)
                else:
                    # Just a local branch, no remote tracking:
                    row[0] = translate("AddonsInstaller", "Branch", "git terminology")
                    row[1] = ref.name
                    if hasattr(ref, "commit") and hasattr(ref.commit, "committed_date"):
                        row[3] = ref.commit.committed_date
                    else:
                        row[3] = ref.log_entry(0).time[0]
            else:
                continue

            self.display_data.append(row.copy())
            self.refs.append(ref)

    def rowCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return len(self.refs)

    def columnCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return 5  # Type, local name, tracking name, local update, and remote update

    def data(self, index: QtCore.QModelIndex, role: int = QtCore.Qt.DisplayRole):
        if not index.isValid():
            return None
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.ToolTipRole:
            tooltip = ""
            # TODO: What should the tooltip be for these items? Last commit message?
            return tooltip
        elif role == QtCore.Qt.DisplayRole:
            dd = self.display_data[row]
            if column == 3 or column == 4:
                if dd[column] is not None:
                    qdate = QtCore.QDateTime.fromTime_t(dd[column])
                    return QtCore.QLocale().toString(qdate, QtCore.QLocale.ShortFormat)
            elif column < len(dd):
                return dd[column]
            else:
                return None
        elif role == ChangeBranchDialogModel.DataSortRole:
            if column == 0:
                if self.refs[row] in self.repo.heads:
                    return 0
                else:
                    return 1
            elif column < len(self.display_data[row]):
                return self.display_data[row][column]
            else:
                return None
        elif role == ChangeBranchDialogModel.RefAccessRole:
            return self.refs[row]

    def headerData(
        self,
        section: int,
        orientation: QtCore.Qt.Orientation,
        role: int = QtCore.Qt.DisplayRole,
    ):
        if orientation == QtCore.Qt.Vertical:
            return None
        if role != QtCore.Qt.DisplayRole:
            return None
        if section == 0:
            return translate(
                "AddonsInstaller",
                "Kind",
                "Table header for git ref type (e.g. either Tag or Branch)",
            )
        elif section == 1:
            return translate("AddonsInstaller", "Local name", "Table header for git ref name")
        elif section == 2:
            return translate(
                "AddonsInstaller",
                "Tracking",
                "Table header for git remote tracking branch name name",
            )
        elif section == 3:
            return translate(
                "AddonsInstaller",
                "Local updated",
                "Table header for git update time of local branch",
            )
        elif section == 4:
            return translate(
                "AddonsInstaller",
                "Remote updated",
                "Table header for git update time of remote branch",
            )
        else:
            return None


class ChangeBranchDialogFilter(QtCore.QSortFilterProxyModel):
    def lessThan(self, left: QtCore.QModelIndex, right: QtCore.QModelIndex):
        leftData = self.sourceModel().data(left, ChangeBranchDialogModel.DataSortRole)
        rightData = self.sourceModel().data(right, ChangeBranchDialogModel.DataSortRole)
        if leftData is None or rightData is None:
            if rightData is not None:
                return True
            else:
                return False
        return leftData < rightData
