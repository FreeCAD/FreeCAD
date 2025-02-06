# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 The FreeCAD Project Association AISBL              *
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

"""Test the Change Branch GUI code"""

# pylint: disable=wrong-import-position, deprecated-module, too-many-return-statements

import sys
import unittest
from unittest.mock import patch, Mock, MagicMock

# So that when run standalone, the Addon Manager classes imported below are available
sys.path.append("../..")

from AddonManagerTest.gui.gui_mocks import DialogWatcher, AsynchronousMonitor

from change_branch import ChangeBranchDialog

from addonmanager_freecad_interface import translate
from addonmanager_git import GitFailed

try:
    from PySide import QtCore, QtWidgets
except ImportError:
    try:
        from PySide6 import QtCore, QtWidgets
    except ImportError:
        from PySide2 import QtCore, QtWidgets


class MockFilter(QtCore.QSortFilterProxyModel):
    """Replaces a filter with a non-filter that simply always returns whatever it's given"""

    def mapToSource(self, something):
        return something


class MockChangeBranchDialogModel(QtCore.QAbstractTableModel):
    """Replace a data-connected model with a static one for testing"""

    branches = [
        {"ref_name": "ref1", "upstream": "us1"},
        {"ref_name": "ref2", "upstream": "us2"},
        {"ref_name": "ref3", "upstream": "us3"},
    ]
    current_branch = "ref1"
    DataSortRole = QtCore.Qt.UserRole
    RefAccessRole = QtCore.Qt.UserRole + 1

    def __init__(self, _: str, parent=None) -> None:
        super().__init__(parent)

    def rowCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """Number of rows: should always return 3"""
        if parent.isValid():
            return 0
        return len(self.branches)

    def columnCount(self, parent: QtCore.QModelIndex = QtCore.QModelIndex()) -> int:
        """Number of columns (identical to non-mocked version)"""
        if parent.isValid():
            return 0
        return 3  # Local name, remote name, date

    def data(self, index: QtCore.QModelIndex, role: int = QtCore.Qt.DisplayRole):
        """Mock returns static untranslated strings for DisplayRole, no tooltips at all, and
        otherwise matches the non-mock version"""
        if not index.isValid():
            return None
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.DisplayRole:
            if column == 2:
                return "date"
            if column == 0:
                return "ref_name"
            if column == 1:
                return "upstream"
            return None
        if role == MockChangeBranchDialogModel.DataSortRole:
            return None
        if role == MockChangeBranchDialogModel.RefAccessRole:
            return self.branches[row]
        return None

    def headerData(
        self,
        section: int,
        orientation: QtCore.Qt.Orientation,
        role: int = QtCore.Qt.DisplayRole,
    ):
        """Mock returns untranslated strings for DisplayRole, and no tooltips at all"""
        if orientation == QtCore.Qt.Vertical:
            return None
        if role != QtCore.Qt.DisplayRole:
            return None
        if section == 0:
            return "Local"
        if section == 1:
            return "Remote tracking"
        if section == 2:
            return "Last Updated"
        return None

    def currentBranch(self) -> str:
        """Mock returns a static string stored in the class: that string could be modified to
        return something else by tests that require it."""
        return self.current_branch


class TestChangeBranchGui(unittest.TestCase):
    """Tests for the ChangeBranch GUI code"""

    MODULE = "test_change_branch"  # file name without extension

    def setUp(self):
        pass

    def tearDown(self):
        pass

    @patch("change_branch.ChangeBranchDialogModel", new=MockChangeBranchDialogModel)
    @patch("change_branch.initialize_git", new=Mock(return_value=None))
    def test_no_git(self):
        """If git is not present, a dialog saying so is presented"""
        # Arrange
        gui = ChangeBranchDialog("/some/path")
        ref = {"ref_name": "foo/bar", "upstream": "us1"}
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Cannot find git"),
            QtWidgets.QDialogButtonBox.Ok,
        )

        # Act
        gui.change_branch("/foo/bar/baz", ref)

        # Assert
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")

    @patch("change_branch.ChangeBranchDialogModel", new=MockChangeBranchDialogModel)
    @patch("change_branch.initialize_git")
    def test_git_failed(self, init_git: MagicMock):
        """If git fails when attempting to change branches, a dialog saying so is presented"""
        # Arrange
        git_manager = MagicMock()
        git_manager.checkout = MagicMock()
        git_manager.checkout.side_effect = GitFailed()
        init_git.return_value = git_manager
        gui = ChangeBranchDialog("/some/path")
        ref = {"ref_name": "foo/bar", "upstream": "us1"}
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "git operation failed"),
            QtWidgets.QDialogButtonBox.Ok,
        )

        # Act
        gui.change_branch("/foo/bar/baz", ref)

        # Assert
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")

    @patch("change_branch.ChangeBranchDialogModel", new=MockChangeBranchDialogModel)
    @patch("change_branch.initialize_git", new=MagicMock)
    def test_branch_change_succeeded(self):
        """If nothing gets thrown, then the process is assumed to have worked, and the appropriate
        signal is emitted."""

        # Arrange
        gui = ChangeBranchDialog("/some/path")
        ref = {"ref_name": "foo/bar", "upstream": "us1"}
        monitor = AsynchronousMonitor(gui.branch_changed)

        # Act
        gui.change_branch("/foo/bar/baz", ref)

        # Assert
        monitor.wait_for_at_most(10)  # Should be effectively instantaneous
        self.assertTrue(monitor.good())

    @patch("change_branch.ChangeBranchDialogFilter", new=MockFilter)
    @patch("change_branch.ChangeBranchDialogModel", new=MockChangeBranchDialogModel)
    @patch("change_branch.initialize_git", new=MagicMock)
    def test_warning_is_shown_when_dialog_is_accepted(self):
        """If the dialog is accepted (e.g. a branch change is requested) then a warning dialog is
        displayed, and gives the opportunity to cancel. If cancelled, no signal is emitted."""
        # Arrange
        gui = ChangeBranchDialog("/some/path")
        gui.ui.exec = MagicMock()
        gui.ui.exec.return_value = QtWidgets.QDialog.Accepted
        gui.ui.tableView.selectedIndexes = MagicMock()
        gui.ui.tableView.selectedIndexes.return_value = [MagicMock()]
        gui.ui.tableView.selectedIndexes.return_value[0].isValid = MagicMock()
        gui.ui.tableView.selectedIndexes.return_value[0].isValid.return_value = True
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "DANGER: Developer feature"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        monitor = AsynchronousMonitor(gui.branch_changed)

        # Act
        gui.exec()

        # Assert
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertFalse(monitor.good())  # The watcher cancelled the op, so no signal is emitted


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    QtCore.QTimer.singleShot(0, unittest.main)
    if hasattr(app, "exec"):
        app.exec()  # PySide6
    else:
        app.exec_()  # PySide2
