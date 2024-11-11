# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 The FreeCAD Project Association AISBL         *
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

""" Provides the PackageDetails widget. """

import os
from typing import Optional

from PySide import QtCore, QtWidgets

import addonmanager_freecad_interface as fci

import addonmanager_utilities as utils
from addonmanager_metadata import (
    Version,
    get_first_supported_freecad_version,
    get_branch_from_metadata,
)
from addonmanager_workers_startup import GetMacroDetailsWorker, CheckSingleUpdateWorker
from addonmanager_git import GitManager, NoGitFound
from Addon import Addon
from change_branch import ChangeBranchDialog
from addonmanager_readme_controller import ReadmeController
from Widgets.addonmanager_widget_package_details_view import UpdateInformation, WarningFlags

translate = fci.translate


class PackageDetailsController(QtCore.QObject):
    """Manages the display of the package README information."""

    back = QtCore.Signal()
    install = QtCore.Signal(Addon)
    uninstall = QtCore.Signal(Addon)
    update = QtCore.Signal(Addon)
    execute = QtCore.Signal(Addon)
    update_status = QtCore.Signal(Addon)

    def __init__(self, widget=None):
        super().__init__()
        self.ui = widget
        self.readme_controller = ReadmeController(self.ui.readme_browser)
        self.worker = None
        self.addon = None
        self.update_check_thread = None
        self.original_disabled_state = None
        self.original_status = None
        self.check_for_update_worker = None
        try:
            self.git_manager = GitManager()
        except NoGitFound:
            self.git_manager = None

        self.ui.button_bar.back.clicked.connect(self.back.emit)
        self.ui.button_bar.run_macro.clicked.connect(lambda: self.execute.emit(self.addon))
        self.ui.button_bar.install.clicked.connect(lambda: self.install.emit(self.addon))
        self.ui.button_bar.uninstall.clicked.connect(lambda: self.uninstall.emit(self.addon))
        self.ui.button_bar.update.clicked.connect(lambda: self.update.emit(self.addon))
        self.ui.button_bar.change_branch.clicked.connect(self.change_branch_clicked)
        self.ui.button_bar.enable.clicked.connect(self.enable_clicked)
        self.ui.button_bar.disable.clicked.connect(self.disable_clicked)

    def show_repo(self, repo: Addon) -> None:
        """The main entry point for this class, shows the package details and related buttons
        for the provided repo."""
        self.addon = repo
        self.readme_controller.set_addon(repo)
        self.original_disabled_state = self.addon.is_disabled()
        if repo is not None:
            self.ui.button_bar.show()
        else:
            self.ui.button_bar.hide()

        if self.worker is not None:
            if not self.worker.isFinished():
                self.worker.requestInterruption()
                self.worker.wait()

        installed = self.addon.status() != Addon.Status.NOT_INSTALLED
        self.ui.set_installed(installed)
        update_info = UpdateInformation()
        if installed:
            update_info.unchecked = self.addon.status() == Addon.Status.UNCHECKED
            update_info.update_available = self.addon.status() == Addon.Status.UPDATE_AVAILABLE
            update_info.check_in_progress = False  # TODO: Implement the "check in progress" status
            if repo.metadata:
                update_info.branch = get_branch_from_metadata(repo.metadata)
                update_info.version = repo.metadata.version
            elif repo.macro:
                update_info.version = repo.macro.version
            self.ui.set_update_available(update_info)
            self.ui.set_location(os.path.join(self.addon.mod_directory, self.addon.name))
            self.ui.set_location(os.path.join(self.addon.mod_directory, self.addon.name))
            self.ui.set_disabled(self.addon.is_disabled())
        self.ui.allow_running(repo.repo_type == Addon.Kind.MACRO)
        self.ui.allow_disabling(repo.repo_type != Addon.Kind.MACRO)

        if repo.repo_type == Addon.Kind.MACRO:
            self.update_macro_info(repo)

        if repo.status() == Addon.Status.UNCHECKED:
            self.ui.button_bar.check_for_update.show()
            self.ui.button_bar.check_for_update.setText(
                translate("AddonsInstaller", "Check for " "update")
            )
            self.ui.button_bar.check_for_update.setEnabled(True)
            if not self.update_check_thread:
                self.update_check_thread = QtCore.QThread()
            self.check_for_update_worker = CheckSingleUpdateWorker(repo)
            self.check_for_update_worker.moveToThread(self.update_check_thread)
            self.update_check_thread.finished.connect(self.check_for_update_worker.deleteLater)
            self.ui.button_bar.check_for_update.clicked.connect(
                self.check_for_update_worker.do_work
            )
            self.check_for_update_worker.update_status.connect(self.display_repo_status)
            self.update_check_thread.start()
        else:
            self.ui.button_bar.check_for_update.hide()

        flags = WarningFlags()
        flags.required_freecad_version = self.requires_newer_freecad()
        flags.obsolete = repo.obsolete
        flags.python2 = repo.python2
        self.ui.set_warning_flags(flags)
        self.set_change_branch_button_state()

    def requires_newer_freecad(self) -> Optional[Version]:
        """If the current package is not installed, returns the first supported version of
        FreeCAD, if one is set, or None if no information is available (or if the package is
        already installed)."""

        # If it's not installed, check to see if it's for a newer version of FreeCAD
        if self.addon.status() == Addon.Status.NOT_INSTALLED and self.addon.metadata:
            # Only hide if ALL content items require a newer version, otherwise
            # it's possible that this package actually provides versions of itself
            # for newer and older versions

            first_supported_version = get_first_supported_freecad_version(self.addon.metadata)
            if first_supported_version is not None:
                fc_version = Version(from_list=fci.Version())
                if first_supported_version > fc_version:
                    return first_supported_version
        return None

    def set_change_branch_button_state(self):
        """The change branch button is only available for installed Addons that have a .git directory
        and in runs where the git is available."""

        self.ui.button_bar.change_branch.hide()

        pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
        show_switcher = pref.GetBool("ShowBranchSwitcher", False)
        if not show_switcher:
            return

        # Is this repo installed? If not, return.
        if self.addon.status() == Addon.Status.NOT_INSTALLED:
            return

        # Is it a Macro? If so, return:
        if self.addon.repo_type == Addon.Kind.MACRO:
            return

        # Can we actually switch branches? If not, return.
        if not self.git_manager:
            return

        # Is there a .git subdirectory? If not, return.
        basedir = fci.getUserAppDataDir()
        path_to_git = os.path.join(basedir, "Mod", self.addon.name, ".git")
        if not os.path.isdir(path_to_git):
            return

        # If all four above checks passed, then it's possible for us to switch
        # branches, if there are any besides the one we are on: show the button
        self.ui.button_bar.change_branch.show()

    def update_macro_info(self, repo: Addon) -> None:
        if not repo.macro.url:
            # We need to populate the macro information... may as well do it while the user reads
            # the wiki page
            self.worker = GetMacroDetailsWorker(repo)
            self.worker.readme_updated.connect(self.macro_readme_updated)
            self.worker.start()

    def change_branch_clicked(self) -> None:
        """Loads the branch-switching dialog"""
        basedir = fci.getUserAppDataDir()
        path_to_repo = os.path.join(basedir, "Mod", self.addon.name)
        change_branch_dialog = ChangeBranchDialog(path_to_repo, self.ui)
        change_branch_dialog.branch_changed.connect(self.branch_changed)
        change_branch_dialog.exec()

    def enable_clicked(self) -> None:
        """Called by the Enable button, enables this Addon and updates GUI to reflect
        that status."""
        self.addon.enable()
        self.ui.set_disabled(False)
        if self.original_disabled_state:
            self.ui.set_new_disabled_status(False)
            self.original_status = self.addon.status()
            self.addon.set_status(Addon.Status.PENDING_RESTART)
        else:
            self.addon.set_status(self.original_status)
        self.update_status.emit(self.addon)

    def disable_clicked(self) -> None:
        """Called by the Disable button, disables this Addon and updates the GUI to
        reflect that status."""
        self.addon.disable()
        self.ui.set_disabled(True)
        if not self.original_disabled_state:
            self.ui.set_new_disabled_status(True)
            self.original_status = self.addon.status()
            self.addon.set_status(Addon.Status.PENDING_RESTART)
        else:
            self.addon.set_status(self.original_status)
        self.update_status.emit(self.addon)

    def branch_changed(self, name: str) -> None:
        """Displays a dialog confirming the branch changed, and tries to access the
        metadata file from that branch."""
        QtWidgets.QMessageBox.information(
            self.ui,
            translate("AddonsInstaller", "Success"),
            translate(
                "AddonsInstaller",
                "Branch change succeeded, please restart to use the new version.",
            ),
        )
        # See if this branch has a package.xml file:
        basedir = fci.getUserAppDataDir()
        path_to_metadata = os.path.join(basedir, "Mod", self.addon.name, "package.xml")
        if os.path.isfile(path_to_metadata):
            self.addon.load_metadata_file(path_to_metadata)
            self.addon.installed_version = self.addon.metadata.version
        else:
            self.addon.repo_type = Addon.Kind.WORKBENCH
            self.addon.metadata = None
            self.addon.installed_version = None
        self.addon.updated_timestamp = QtCore.QDateTime.currentDateTime().toSecsSinceEpoch()
        self.addon.branch = name
        self.addon.set_status(Addon.Status.PENDING_RESTART)
        self.ui.set_new_branch(name)
        self.update_status.emit(self.addon)

    def display_repo_status(self, addon):
        self.update_status.emit(self.addon)
        self.show_repo(self.addon)

    def macro_readme_updated(self):
        self.show_repo(self.addon)
