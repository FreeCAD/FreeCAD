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

""" Provides the PackageDetails widget. """

import os
from typing import Optional

from PySide import QtCore, QtGui, QtWidgets

import addonmanager_freecad_interface as fci

import addonmanager_utilities as utils
from addonmanager_metadata import (
    Version,
    get_first_supported_freecad_version,
    get_branch_from_metadata,
)
from addonmanager_workers_startup import GetMacroDetailsWorker, CheckSingleUpdateWorker
from addonmanager_readme_viewer import ReadmeViewer
from addonmanager_git import GitManager, NoGitFound
from Addon import Addon
from change_branch import ChangeBranchDialog
from Widgets.addonmanager_widget_addon_buttons import WidgetAddonButtons

translate = fci.translate


class PackageDetails(QtWidgets.QWidget):
    """The PackageDetails QWidget shows package README information and provides
    install, uninstall, and update buttons."""

    back = QtCore.Signal()
    install = QtCore.Signal(Addon)
    uninstall = QtCore.Signal(Addon)
    update = QtCore.Signal(Addon)
    execute = QtCore.Signal(Addon)
    update_status = QtCore.Signal(Addon)
    check_for_update = QtCore.Signal(Addon)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_PackageDetails()
        self.ui.setupUi(self)

        self.worker = None
        self.repo = None
        self.status_update_thread = None
        try:
            self.git_manager = GitManager()
        except NoGitFound:
            self.git_manager = None

        self.ui.button_bar.back_button.clicked.connect(self.back.emit)
        self.ui.button_bar.run_macro_button.clicked.connect(lambda: self.execute.emit(self.repo))
        self.ui.button_bar.install_button.clicked.connect(lambda: self.install.emit(self.repo))
        self.ui.button_bar.uninstall_button.clicked.connect(lambda: self.uninstall.emit(self.repo))
        self.ui.button_bar.update_button.clicked.connect(lambda: self.update.emit(self.repo))
        self.ui.button_bar.check_for_update_button.clicked.connect(
            lambda: self.check_for_update.emit(self.repo)
        )
        self.ui.button_bar.change_branch_button.clicked.connect(self.change_branch_clicked)
        self.ui.button_bar.enable_button.clicked.connect(self.enable_clicked)
        self.ui.button_bar.disable_button.clicked.connect(self.disable_clicked)

    def show_repo(self, repo: Addon, reload: bool = False) -> None:
        """The main entry point for this class, shows the package details and related buttons
        for the provided repo. If reload is true, then even if this is already the current repo
        the data is reloaded."""

        # If this is the same repo we were already showing, we do not have to do the
        # expensive refetch unless reload is true
        if True or self.repo != repo or reload:
            self.repo = repo

            if self.worker is not None:
                if not self.worker.isFinished():
                    self.worker.requestInterruption()
                    self.worker.wait()

            if repo.repo_type == Addon.Kind.MACRO:
                self.show_macro(repo)
                self.ui.button_bar.run_macro_button.show()
            elif repo.repo_type == Addon.Kind.WORKBENCH:
                self.show_workbench(repo)
                self.ui.button_bar.run_macro_button.hide()
            elif repo.repo_type == Addon.Kind.PACKAGE:
                self.show_package(repo)
                self.ui.button_bar.run_macro_button.hide()

        if repo.status() == Addon.Status.UNCHECKED:
            if not self.status_update_thread:
                self.status_update_thread = QtCore.QThread()
            self.status_create_addon_list_worker = CheckSingleUpdateWorker(repo)
            self.status_create_addon_list_worker.moveToThread(self.status_update_thread)
            self.status_update_thread.finished.connect(
                self.status_create_addon_list_worker.deleteLater
            )
            self.check_for_update.connect(self.status_create_addon_list_worker.do_work)
            self.status_create_addon_list_worker.update_status.connect(self.display_repo_status)
            self.status_update_thread.start()
            self.check_for_update.emit(self.repo)

        self.display_repo_status(self.repo.update_status)

    def display_repo_status(self, status):
        """Updates the contents of the widget to display the current install status of the widget."""
        repo = self.repo
        self.set_change_branch_button_state()
        self.set_disable_button_state()
        if status != Addon.Status.NOT_INSTALLED:
            version = repo.installed_version
            date = ""
            installed_version_string = "<h3>"
            if repo.updated_timestamp:
                date = QtCore.QLocale().toString(
                    QtCore.QDateTime.fromSecsSinceEpoch(int(round(repo.updated_timestamp, 0))),
                    QtCore.QLocale.ShortFormat,
                )
            if version and date:
                installed_version_string += (
                    translate("AddonsInstaller", "Version {version} installed on {date}").format(
                        version=version, date=date
                    )
                    + ". "
                )
            elif version:
                installed_version_string += (
                    translate("AddonsInstaller", "Version {version} installed") + ". "
                ).format(version=version)
            elif date:
                installed_version_string += (
                    translate("AddonsInstaller", "Installed on {date}") + ". "
                ).format(date=date)
            else:
                installed_version_string += translate("AddonsInstaller", "Installed") + ". "

            if status == Addon.Status.UPDATE_AVAILABLE:
                if repo.metadata:
                    name_change = False
                    if repo.installed_metadata:
                        old_branch = get_branch_from_metadata(repo.installed_metadata)
                        new_branch = get_branch_from_metadata(repo.metadata)
                        if old_branch != new_branch:
                            installed_version_string += (
                                "<b>"
                                + translate(
                                    "AddonsInstaller",
                                    "Currently on branch {}, name changed to {}",
                                ).format(old_branch, new_branch)
                            ) + ".</b> "
                            name_change = True
                    if not name_change:
                        installed_version_string += (
                            "<b>"
                            + translate(
                                "AddonsInstaller",
                                "On branch {}, update available to version",
                            ).format(repo.branch)
                            + " "
                        )
                        installed_version_string += str(repo.metadata.version)
                        installed_version_string += ".</b>"
                elif repo.macro and repo.macro.version:
                    installed_version_string += (
                        "<b>" + translate("AddonsInstaller", "Update available to version") + " "
                    )
                    installed_version_string += repo.macro.version
                    installed_version_string += ".</b>"
                else:
                    installed_version_string += (
                        "<b>"
                        + translate(
                            "AddonsInstaller",
                            "An update is available",
                        )
                        + ".</b>"
                    )
            elif status == Addon.Status.NO_UPDATE_AVAILABLE:
                detached_head = False
                branch = repo.branch
                if self.git_manager and repo.repo_type != Addon.Kind.MACRO:
                    basedir = fci.getUserAppDataDir()
                    moddir = os.path.join(basedir, "Mod", repo.name)
                    repo_path = os.path.join(moddir, ".git")
                    if os.path.exists(repo_path):
                        branch = self.git_manager.current_branch(repo_path)
                        if self.git_manager.detached_head(repo_path):
                            tag = self.git_manager.current_tag(repo_path)
                            branch = tag
                            detached_head = True
                if detached_head:
                    installed_version_string += (
                        translate(
                            "AddonsInstaller",
                            "Git tag '{}' checked out, no updates possible",
                        ).format(branch)
                        + "."
                    )
                else:
                    installed_version_string += (
                        translate(
                            "AddonsInstaller",
                            "This is the latest version available for branch {}",
                        ).format(branch)
                        + "."
                    )
            elif status == Addon.Status.PENDING_RESTART:
                installed_version_string += (
                    translate("AddonsInstaller", "Updated, please restart FreeCAD to use") + "."
                )
            elif status == Addon.Status.UNCHECKED:
                pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
                autocheck = pref.GetBool("AutoCheck", False)
                if autocheck:
                    installed_version_string += (
                        translate("AddonsInstaller", "Update check in progress") + "."
                    )
                else:
                    installed_version_string += (
                        translate("AddonsInstaller", "Automatic update checks disabled") + "."
                    )

            installed_version_string += "</h3>"
            self.ui.labelPackageDetails.setText(installed_version_string)
            if repo.status() == Addon.Status.UPDATE_AVAILABLE:
                self.ui.labelPackageDetails.setStyleSheet("color:" + utils.attention_color_string())
            else:
                self.ui.labelPackageDetails.setStyleSheet("color:" + utils.bright_color_string())
            self.ui.labelPackageDetails.show()

            if repo.macro is not None:
                moddir = fci.getUserMacroDir(True)
            else:
                basedir = fci.getUserAppDataDir()
                moddir = os.path.join(basedir, "Mod", repo.name)
            installationLocationString = (
                translate("AddonsInstaller", "Installation location")
                + ": "
                + os.path.normpath(moddir)
            )

            self.ui.labelInstallationLocation.setText(installationLocationString)
            self.ui.labelInstallationLocation.show()
        else:
            self.ui.labelPackageDetails.hide()
            self.ui.labelInstallationLocation.hide()

        if status == Addon.Status.NOT_INSTALLED:
            self.ui.button_bar.install_button.show()
            self.ui.button_bar.uninstall_button.hide()
            self.ui.button_bar.update_button.hide()
            self.ui.button_bar.check_for_update_button.hide()
        elif status == Addon.Status.NO_UPDATE_AVAILABLE:
            self.ui.button_bar.install_button.hide()
            self.ui.button_bar.uninstall_button.show()
            self.ui.button_bar.update_button.hide()
            self.ui.button_bar.check_for_update_button.hide()
        elif status == Addon.Status.UPDATE_AVAILABLE:
            self.ui.button_bar.install_button.hide()
            self.ui.button_bar.uninstall_button.show()
            self.ui.button_bar.update_button.show()
            self.ui.button_bar.check_for_update_button.hide()
        elif status == Addon.Status.UNCHECKED:
            self.ui.button_bar.install_button.hide()
            self.ui.button_bar.uninstall_button.show()
            self.ui.button_bar.update_button.hide()
            self.ui.button_bar.check_for_update_button.show()
        elif status == Addon.Status.PENDING_RESTART:
            self.ui.button_bar.install_button.hide()
            self.ui.button_bar.uninstall_button.show()
            self.ui.button_bar.update_button.hide()
            self.ui.button_bar.check_for_update_button.hide()
        elif status == Addon.Status.CANNOT_CHECK:
            self.ui.button_bar.install_button.hide()
            self.ui.button_bar.uninstall_button.show()
            self.ui.button_bar.update_button.show()
            self.ui.button_bar.check_for_update_button.hide()

        required_version = self.requires_newer_freecad()
        if repo.obsolete:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>" + translate("AddonsInstaller", "WARNING: This addon is obsolete") + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif repo.python2:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon is Python 2 Only")
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif required_version:
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h1>"
                + translate("AddonsInstaller", "WARNING: This addon requires FreeCAD ")
                + required_version
                + "</h1>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())
        elif repo.is_disabled():
            self.ui.labelWarningInfo.show()
            self.ui.labelWarningInfo.setText(
                "<h2>"
                + translate(
                    "AddonsInstaller",
                    "WARNING: This addon is currently installed, but disabled. Use the 'enable' button to re-enable.",
                )
                + "</h2>"
            )
            self.ui.labelWarningInfo.setStyleSheet("color:" + utils.warning_color_string())

        else:
            self.ui.labelWarningInfo.hide()

    def requires_newer_freecad(self) -> Optional[Version]:
        """If the current package is not installed, returns the first supported version of
        FreeCAD, if one is set, or None if no information is available (or if the package is
        already installed)."""

        # If it's not installed, check to see if it's for a newer version of FreeCAD
        if self.repo.status() == Addon.Status.NOT_INSTALLED and self.repo.metadata:
            # Only hide if ALL content items require a newer version, otherwise
            # it's possible that this package actually provides versions of itself
            # for newer and older versions

            first_supported_version = get_first_supported_freecad_version(self.repo.metadata)
            if first_supported_version is not None:
                fc_version = Version(from_list=fci.Version())
                if first_supported_version > fc_version:
                    return first_supported_version
        return None

    def set_change_branch_button_state(self):
        """The change branch button is only available for installed Addons that have a .git directory
        and in runs where the git is available."""

        self.ui.button_bar.change_branch_button.hide()

        pref = fci.ParamGet("User parameter:BaseApp/Preferences/Addons")
        show_switcher = pref.GetBool("ShowBranchSwitcher", False)
        if not show_switcher:
            return

        # Is this repo installed? If not, return.
        if self.repo.status() == Addon.Status.NOT_INSTALLED:
            return

        # Is it a Macro? If so, return:
        if self.repo.repo_type == Addon.Kind.MACRO:
            return

        # Can we actually switch branches? If not, return.
        if not self.git_manager:
            return

        # Is there a .git subdirectory? If not, return.
        basedir = fci.getUserAppDataDir()
        path_to_git = os.path.join(basedir, "Mod", self.repo.name, ".git")
        if not os.path.isdir(path_to_git):
            return

        # If all four above checks passed, then it's possible for us to switch
        # branches, if there are any besides the one we are on: show the button
        self.ui.button_bar.change_branch_button.show()

    def set_disable_button_state(self):
        """Set up the enable/disable button based on the enabled/disabled state of the addon"""
        self.ui.button_bar.enable_button.hide()
        self.ui.button_bar.disable_button.hide()
        status = self.repo.status()
        if status != Addon.Status.NOT_INSTALLED:
            disabled = self.repo.is_disabled()
            if disabled:
                self.ui.button_bar.enable_button.show()
            else:
                self.ui.button_bar.disable_button.show()

    def show_workbench(self, repo: Addon) -> None:
        """loads information of a given workbench"""

        self.ui.textBrowserReadMe.set_addon(repo)

    def show_package(self, repo: Addon) -> None:
        """Show the details for a package (a repo with a package.xml metadata file)"""

        self.ui.textBrowserReadMe.set_addon(repo)

    def show_macro(self, repo: Addon) -> None:
        """loads information of a given macro"""

        if not repo.macro.url:
            # We need to populate the macro information... may as well do it while the user reads the wiki page
            self.worker = GetMacroDetailsWorker(repo)
            self.worker.readme_updated.connect(self.macro_readme_updated)
            self.worker.start()
        else:
            self.macro_readme_updated()

    def macro_readme_updated(self):
        """Update the display of a Macro's README data."""

        self.ui.textBrowserReadMe.set_addon(self.repo)

    def change_branch_clicked(self) -> None:
        """Loads the branch-switching dialog"""
        basedir = fci.getUserAppDataDir()
        path_to_repo = os.path.join(basedir, "Mod", self.repo.name)
        change_branch_dialog = ChangeBranchDialog(path_to_repo, self)
        change_branch_dialog.branch_changed.connect(self.branch_changed)
        change_branch_dialog.exec()

    def enable_clicked(self) -> None:
        """Called by the Enable button, enables this Addon and updates GUI to reflect
        that status."""
        self.repo.enable()
        self.repo.set_status(Addon.Status.PENDING_RESTART)
        self.set_disable_button_state()
        self.update_status.emit(self.repo)
        self.ui.labelWarningInfo.show()
        self.ui.labelWarningInfo.setText(
            "<h3>"
            + translate(
                "AddonsInstaller",
                "This Addon will be enabled next time you restart FreeCAD.",
            )
            + "</h3>"
        )
        self.ui.labelWarningInfo.setStyleSheet("color:" + utils.bright_color_string())

    def disable_clicked(self) -> None:
        """Called by the Disable button, disables this Addon and updates the GUI to
        reflect that status."""
        self.repo.disable()
        self.repo.set_status(Addon.Status.PENDING_RESTART)
        self.set_disable_button_state()
        self.update_status.emit(self.repo)
        self.ui.labelWarningInfo.show()
        self.ui.labelWarningInfo.setText(
            "<h3>"
            + translate(
                "AddonsInstaller",
                "This Addon will be disabled next time you restart FreeCAD.",
            )
            + "</h3>"
        )
        self.ui.labelWarningInfo.setStyleSheet("color:" + utils.attention_color_string())

    def branch_changed(self, name: str) -> None:
        """Displays a dialog confirming the branch changed, and tries to access the
        metadata file from that branch."""
        QtWidgets.QMessageBox.information(
            self,
            translate("AddonsInstaller", "Success"),
            translate(
                "AddonsInstaller",
                "Branch change succeeded, please restart to use the new version.",
            ),
        )
        # See if this branch has a package.xml file:
        basedir = fci.getUserAppDataDir()
        path_to_metadata = os.path.join(basedir, "Mod", self.repo.name, "package.xml")
        if os.path.isfile(path_to_metadata):
            self.repo.load_metadata_file(path_to_metadata)
            self.repo.installed_version = self.repo.metadata.version
        else:
            self.repo.repo_type = Addon.Kind.WORKBENCH
            self.repo.metadata = None
            self.repo.installed_version = None
        self.repo.updated_timestamp = QtCore.QDateTime.currentDateTime().toSecsSinceEpoch()
        self.repo.branch = name
        self.repo.set_status(Addon.Status.PENDING_RESTART)

        installed_version_string = "<h3>"
        installed_version_string += translate(
            "AddonsInstaller", "Changed to git ref '{}' -- please restart to use Addon."
        ).format(name)
        installed_version_string += "</h3>"
        self.ui.labelPackageDetails.setText(installed_version_string)
        self.ui.labelPackageDetails.setStyleSheet("color:" + utils.attention_color_string())
        self.update_status.emit(self.repo)


class Ui_PackageDetails(object):
    """The generated UI from the Qt Designer UI file"""

    def setupUi(self, PackageDetails):
        if not PackageDetails.objectName():
            PackageDetails.setObjectName("PackageDetails")
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(PackageDetails)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.layoutDetailsBackButton = QtWidgets.QHBoxLayout()
        self.layoutDetailsBackButton.setObjectName("layoutDetailsBackButton")

        self.button_bar = WidgetAddonButtons(PackageDetails)
        self.layoutDetailsBackButton.addWidget(self.button_bar)

        self.verticalLayout_2.addLayout(self.layoutDetailsBackButton)

        self.labelPackageDetails = QtWidgets.QLabel(PackageDetails)
        self.labelPackageDetails.hide()

        self.verticalLayout_2.addWidget(self.labelPackageDetails)

        self.labelInstallationLocation = QtWidgets.QLabel(PackageDetails)
        self.labelInstallationLocation.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        self.labelInstallationLocation.hide()

        self.verticalLayout_2.addWidget(self.labelInstallationLocation)

        self.labelWarningInfo = QtWidgets.QLabel(PackageDetails)
        self.labelWarningInfo.hide()

        self.verticalLayout_2.addWidget(self.labelWarningInfo)

        sizePolicy1 = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)

        self.textBrowserReadMe = ReadmeViewer(PackageDetails)
        self.textBrowserReadMe.setObjectName("textBrowserReadMe")

        self.verticalLayout_2.addWidget(self.textBrowserReadMe)

        self.retranslateUi(PackageDetails)

        QtCore.QMetaObject.connectSlotsByName(PackageDetails)

    # setupUi

    def retranslateUi(self, _):
        pass

    # retranslateUi
