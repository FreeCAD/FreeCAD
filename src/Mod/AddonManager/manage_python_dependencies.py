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

""" Provides classes and support functions for managing the automatically-installed
Python library dependencies. No support is provided for uninstalling those dependencies
because pip's uninstall function does not support the target directory argument. """

import json
import os
import platform
import shutil
import subprocess
import sys
from functools import partial
from typing import Dict, List, Tuple

import addonmanager_freecad_interface as fci

import FreeCAD
import FreeCADGui
from freecad.utils import get_python_exe
from PySide import QtCore, QtGui, QtWidgets

import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate

# pylint: disable=too-few-public-methods


class PipFailed(Exception):
    """Exception thrown when pip times out or otherwise fails to return valid results"""


class CheckForPythonPackageUpdatesWorker(QtCore.QThread):
    """Perform non-blocking Python library update availability checking"""

    python_package_updates_available = QtCore.Signal()

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        """Usually not called directly: instead, instantiate this class and call its start()
        function in a parent thread. emits a python_package_updates_available signal if updates
        are available for any of the installed Python packages."""

        if check_for_python_package_updates():
            self.python_package_updates_available.emit()


def check_for_python_package_updates() -> bool:
    """Returns True if any of the Python packages installed into the AdditionalPythonPackages
    directory have updates available, or False if they are all up-to-date."""

    vendor_path = os.path.join(FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages")
    package_counter = 0
    try:
        outdated_packages_stdout = call_pip(["list", "-o", "--path", vendor_path])
    except PipFailed as e:
        FreeCAD.Console.PrintError(str(e) + "\n")
        return False
    FreeCAD.Console.PrintLog("Output from pip -o:\n")
    for line in outdated_packages_stdout:
        if len(line) > 0:
            package_counter += 1
        FreeCAD.Console.PrintLog(f"  {line}\n")
    return package_counter > 0


def call_pip(args) -> List[str]:
    """Tries to locate the appropriate Python executable and run pip with version checking
    disabled. Fails if Python can't be found or if pip is not installed."""

    python_exe = get_python_exe()
    pip_failed = False
    if python_exe:
        call_args = [python_exe, "-m", "pip", "--disable-pip-version-check"]
        call_args.extend(args)
        proc = None
        try:
            proc = utils.run_interruptable_subprocess(call_args)
        except subprocess.CalledProcessError:
            pip_failed = True

        result = []
        if not pip_failed:
            data = proc.stdout
            result = data.split("\n")
        elif proc:
            raise PipFailed(proc.stderr)
        else:
            raise PipFailed("pip timed out")
    else:
        raise PipFailed("Could not locate Python executable on this system")
    return result


class PythonPackageManager:

    """A GUI-based pip interface allowing packages to be updated, either individually or all at
    once."""

    class PipRunner(QtCore.QObject):
        """Run pip in a separate thread so the UI doesn't block while it runs"""

        finished = QtCore.Signal()
        error = QtCore.Signal(str)

        def __init__(self, vendor_path, parent=None):
            super().__init__(parent)
            self.all_packages_stdout = []
            self.outdated_packages_stdout = []
            self.vendor_path = vendor_path
            self.package_list = {}

        def process(self):
            """Execute this object."""
            try:
                self.all_packages_stdout = call_pip(["list", "--path", self.vendor_path])
                self.outdated_packages_stdout = call_pip(["list", "-o", "--path", self.vendor_path])
            except PipFailed as e:
                FreeCAD.Console.PrintError(str(e) + "\n")
                self.error.emit(str(e))
            self.finished.emit()

    def __init__(self, addons):
        self.dlg = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "PythonDependencyUpdateDialog.ui")
        )
        self.addons = addons
        self.vendor_path = utils.get_pip_target_directory()
        self.worker_thread = None
        self.worker_object = None
        self.package_list = []

    def show(self):
        """Run the modal dialog"""

        known_python_versions = self.get_known_python_versions()
        if self._current_python_version_is_new() and known_python_versions:
            # pylint: disable=line-too-long
            result = QtWidgets.QMessageBox.question(
                None,
                translate("AddonsInstaller", "New Python Version Detected"),
                translate(
                    "AddonsInstaller",
                    "This appears to be the first time this version of Python has been used with the Addon Manager. "
                    "Would you like to install the same auto-installed dependencies for it?",
                ),
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
            )
            if result == QtWidgets.QMessageBox.Yes:
                self._reinstall_all_packages()

        self._add_current_python_version()
        self._create_list_from_pip()
        self.dlg.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
        self.dlg.tableWidget.setSortingEnabled(False)
        self.dlg.labelInstallationPath.setText(self.vendor_path)
        self.dlg.exec()

    def _create_list_from_pip(self):
        """Uses pip and pip -o to generate a list of installed packages, and creates the user
        interface elements for those packages. Asynchronous, will complete AFTER the window is
        showing in most cases."""

        self.worker_thread = QtCore.QThread()
        self.worker_object = PythonPackageManager.PipRunner(self.vendor_path)
        self.worker_object.moveToThread(self.worker_thread)
        self.worker_object.finished.connect(self._worker_finished)
        self.worker_object.finished.connect(self.worker_thread.quit)
        self.worker_thread.started.connect(self.worker_object.process)
        self.worker_thread.start()

        self.dlg.tableWidget.setRowCount(1)
        self.dlg.tableWidget.setItem(
            0,
            0,
            QtWidgets.QTableWidgetItem(translate("AddonsInstaller", "Processing, please wait...")),
        )
        self.dlg.tableWidget.horizontalHeader().setSectionResizeMode(
            0, QtWidgets.QHeaderView.ResizeToContents
        )

    def _worker_finished(self):
        """Callback for when the worker process has completed"""
        all_packages_stdout = self.worker_object.all_packages_stdout
        outdated_packages_stdout = self.worker_object.outdated_packages_stdout

        self.package_list = self._parse_pip_list_output(
            all_packages_stdout, outdated_packages_stdout
        )
        self.dlg.buttonUpdateAll.clicked.connect(
            partial(self._update_all_packages, self.package_list)
        )

        self.dlg.tableWidget.setRowCount(len(self.package_list))
        updateButtons = []
        counter = 0
        update_counter = 0
        self.dlg.tableWidget.setSortingEnabled(False)
        for package_name, package_details in self.package_list.items():
            dependent_addons = self._get_dependent_addons(package_name)
            dependencies = []
            for addon in dependent_addons:
                if addon["optional"]:
                    dependencies.append(addon["name"] + "*")
                else:
                    dependencies.append(addon["name"])
            self.dlg.tableWidget.setItem(counter, 0, QtWidgets.QTableWidgetItem(package_name))
            self.dlg.tableWidget.setItem(
                counter,
                1,
                QtWidgets.QTableWidgetItem(package_details["installed_version"]),
            )
            self.dlg.tableWidget.setItem(
                counter,
                2,
                QtWidgets.QTableWidgetItem(package_details["available_version"]),
            )
            self.dlg.tableWidget.setItem(
                counter,
                3,
                QtWidgets.QTableWidgetItem(", ".join(dependencies)),
            )
            if len(package_details["available_version"]) > 0:
                updateButtons.append(QtWidgets.QPushButton(translate("AddonsInstaller", "Update")))
                updateButtons[-1].setIcon(QtGui.QIcon(":/icons/button_up.svg"))
                updateButtons[-1].clicked.connect(partial(self._update_package, package_name))
                self.dlg.tableWidget.setCellWidget(counter, 4, updateButtons[-1])
                update_counter += 1
            else:
                self.dlg.tableWidget.removeCellWidget(counter, 3)
            counter += 1
        self.dlg.tableWidget.setSortingEnabled(True)

        self.dlg.tableWidget.horizontalHeader().setStretchLastSection(False)
        self.dlg.tableWidget.horizontalHeader().setSectionResizeMode(
            0, QtWidgets.QHeaderView.Stretch
        )
        self.dlg.tableWidget.horizontalHeader().setSectionResizeMode(
            1, QtWidgets.QHeaderView.ResizeToContents
        )
        self.dlg.tableWidget.horizontalHeader().setSectionResizeMode(
            2, QtWidgets.QHeaderView.ResizeToContents
        )
        self.dlg.tableWidget.horizontalHeader().setSectionResizeMode(
            3, QtWidgets.QHeaderView.ResizeToContents
        )

        if update_counter > 0:
            self.dlg.buttonUpdateAll.setEnabled(True)
        else:
            self.dlg.buttonUpdateAll.setEnabled(False)

    def _get_dependent_addons(self, package):
        dependent_addons = []
        for addon in self.addons:
            # if addon.installed_version is not None:
            if package.lower() in addon.python_requires:
                dependent_addons.append({"name": addon.name, "optional": False})
            elif package.lower() in addon.python_optional:
                dependent_addons.append({"name": addon.name, "optional": True})
        return dependent_addons

    def _parse_pip_list_output(self, all_packages, outdated_packages) -> Dict[str, Dict[str, str]]:
        """Parses the output from pip into a dictionary with update information in it. The pip
        output should be an array of lines of text."""

        # All Packages output looks like this:
        # Package    Version
        # ---------- -------
        # gitdb      4.0.9
        # GitPython  3.1.27
        # setuptools 41.2.0

        # Outdated Packages output looks like this:
        # Package    Version Latest Type
        # ---------- ------- ------ -----
        # pip        21.0.1  22.1.2 wheel
        # setuptools 41.2.0  63.2.0 wheel

        packages = {}
        skip_counter = 0
        for line in all_packages:
            if skip_counter < 2:
                skip_counter += 1
                continue
            entries = line.split()
            if len(entries) > 1:
                package_name = entries[0]
                installed_version = entries[1]
                packages[package_name] = {
                    "installed_version": installed_version,
                    "available_version": "",
                }

        skip_counter = 0
        for line in outdated_packages:
            if skip_counter < 2:
                skip_counter += 1
                continue
            entries = line.split()
            if len(entries) > 1:
                package_name = entries[0]
                installed_version = entries[1]
                available_version = entries[2]
                packages[package_name] = {
                    "installed_version": installed_version,
                    "available_version": available_version,
                }

        return packages

    def _update_package(self, package_name) -> None:
        """Run pip --upgrade on the given package. Updates all dependent packages as well."""
        for line in range(self.dlg.tableWidget.rowCount()):
            if self.dlg.tableWidget.item(line, 0).text() == package_name:
                self.dlg.tableWidget.setItem(
                    line,
                    2,
                    QtWidgets.QTableWidgetItem(translate("AddonsInstaller", "Updating...")),
                )
                break
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        try:
            FreeCAD.Console.PrintLog(
                f"Running 'pip install --upgrade --target {self.vendor_path} {package_name}'\n"
            )
            call_pip(["install", "--upgrade", package_name, "--target", self.vendor_path])
            self._create_list_from_pip()
            while self.worker_thread.isRunning():
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
        except PipFailed as e:
            FreeCAD.Console.PrintError(str(e) + "\n")
            return
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

    def _update_all_packages(self, package_list) -> None:
        """Updates all packages with available updates."""
        updates = []
        for package_name, package_details in package_list.items():
            if (
                len(package_details["available_version"]) > 0
                and package_details["available_version"] != package_details["installed_version"]
            ):
                updates.append(package_name)

        FreeCAD.Console.PrintLog(f"Running update for {len(updates)} Python packages...\n")
        for package_name in updates:
            self._update_package(package_name)

    @classmethod
    def migrate_old_am_installations(cls) -> bool:
        """Move packages installed before the Addon Manager switched to a versioned directory
        structure into the versioned structure. Returns True if a migration was done, or false
        if no migration was needed."""

        migrated = False

        old_directory = os.path.join(FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages")

        new_directory = utils.get_pip_target_directory()
        new_directory_name = new_directory.rsplit(os.path.sep, 1)[1]

        if not os.path.exists(old_directory) or os.path.exists(
            os.path.join(old_directory, "MIGRATION_COMPLETE")
        ):
            # Nothing to migrate
            return False

        if not os.path.exists(new_directory):
            os.makedirs(new_directory)

        for content_item in os.listdir(old_directory):
            if content_item == new_directory_name:
                continue
            old_path = os.path.join(old_directory, content_item)
            new_path = os.path.join(new_directory, content_item)
            FreeCAD.Console.PrintLog(
                f"Moving {content_item} into the new (versioned) directory structure\n"
            )
            FreeCAD.Console.PrintLog(f"   {old_path} --> {new_path}\n")
            shutil.move(old_path, new_path)
            migrated = True

        sys.path.append(new_directory)
        cls._add_current_python_version()

        with open(os.path.join(old_directory, "MIGRATION_COMPLETE"), "w", encoding="utf-8") as f:
            f.write("Files originally installed in this directory have been migrated to:\n")
            f.write(new_directory)
            f.write(
                "\nThe existence of this file prevents the Addon Manager from "
                "attempting the migration again.\n"
            )
        return migrated

    @classmethod
    def get_known_python_versions(cls) -> List[Tuple[int, int, int]]:
        """Get the list of Python versions that the Addon Manager has seen before."""
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        known_python_versions_string = pref.GetString("KnownPythonVersions", "[]")
        known_python_versions = json.loads(known_python_versions_string)
        return known_python_versions

    @classmethod
    def _add_current_python_version(cls) -> None:
        known_python_versions = cls.get_known_python_versions()
        major, minor, _ = platform.python_version_tuple()
        if not [major, minor] in known_python_versions:
            known_python_versions.append((major, minor))
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetString("KnownPythonVersions", json.dumps(known_python_versions))

    @classmethod
    def _current_python_version_is_new(cls) -> bool:
        """Returns True if this is the first time the Addon Manager has seen this version of
        Python"""
        known_python_versions = cls.get_known_python_versions()
        major, minor, _ = platform.python_version_tuple()
        if not [major, minor] in known_python_versions:
            return True
        return False

    def _load_old_package_list(self) -> List[str]:
        """Gets the list of packages from the package installation manifest"""

        known_python_versions = self.get_known_python_versions()
        if not known_python_versions:
            return []
        last_version = known_python_versions[-1]
        expected_directory = f"py{last_version[0]}{last_version[1]}"
        expected_directory = os.path.join(
            FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages", expected_directory
        )
        # For now just do this synchronously
        worker_object = PythonPackageManager.PipRunner(expected_directory)
        worker_object.process()
        packages = self._parse_pip_list_output(
            worker_object.all_packages_stdout, worker_object.outdated_packages_stdout
        )
        return packages.keys()

    def _reinstall_all_packages(self) -> None:
        """Loads the package manifest from another Python version, and installs the same packages
        for the current (presumably new) version of Python."""

        packages = self._load_old_package_list()
        args = ["install"]
        args.extend(packages)
        args.extend(["--target", self.vendor_path])

        try:
            call_pip(args)
        except PipFailed as e:
            FreeCAD.Console.PrintError(str(e) + "\n")
            return
