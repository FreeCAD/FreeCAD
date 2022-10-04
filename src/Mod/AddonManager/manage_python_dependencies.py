# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
#  *  02110-1301  USA                                                       *
# *                                                                         *
# ***************************************************************************

""" Provides classes and support functions for managing the automatically-installed
Python library dependencies. No support is provided for uninstalling those dependencies
because pip's uninstall function does not support the target directory argument. """

from typing import List, Dict

import os
import subprocess
from functools import partial

import FreeCAD
import FreeCADGui
from PySide2 import QtCore, QtGui, QtWidgets

import addonmanager_utilities as utils

translate = FreeCAD.Qt.translate


class CheckForPythonPackageUpdatesWorker(QtCore.QThread):
    """Perform non-blocking Python library update availability checking"""

    python_package_updates_available = QtCore.Signal()

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        """Usually not called directly: instead, instantiate this class and call its start() function
        in a parent thread. emits a python_package_updates_available signal if updates are available
        for any of the installed Python packages."""

        if check_for_python_package_updates():
            self.python_package_updates_available.emit()


def check_for_python_package_updates() -> bool:
    """Returns True if any of the Python packages installed into the AdditionalPythonPackages directory
    have updates available, or False if the are all up-to-date."""

    vendor_path = os.path.join(FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages")
    package_counter = 0
    outdated_packages_stdout = call_pip(["list", "-o", "--path", vendor_path])
    FreeCAD.Console.PrintLog("Output from pip -o:\n")
    for line in outdated_packages_stdout:
        if len(line) > 0:
            package_counter += 1
        FreeCAD.Console.PrintLog(f"  {line}\n")
    return package_counter > 0


def call_pip(args) -> List[str]:
    """Tries to locate the appropriate Python executable and run pip with version checking disabled. Fails
    if Python can't be found or if pip is not installed."""

    python_exe = utils.get_python_exe()
    pip_failed = False
    if python_exe:
        call_args = [python_exe, "-m", "pip", "--disable-pip-version-check"]
        call_args.extend(args)
        proc = None
        try:
            proc = subprocess.run(
                call_args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=True,
                check=True,
                timeout=30,
            )
            if proc.returncode != 0:
                pip_failed = True
        except subprocess.CalledProcessError:
            pip_failed = True
        except subprocess.TimeoutExpired:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "pip took longer than 30 seconds to return results, giving up on it",
                )
                + "\n"
            )
            FreeCAD.Console.PrintLog(" ".join(call_args))
            pip_failed = True

        result = []
        if not pip_failed:
            data = proc.stdout.decode()
            result = data.split("\n")
        elif proc:
            raise Exception(proc.stderr.decode())
        else:
            raise Exception("pip timed out")
    else:
        raise Exception("Could not locate Python executable on this system")
    return result


class PythonPackageManager:

    """A GUI-based pip interface allowing packages to be updated, either individually or all at once."""

    def __init__(self, addons):
        self.dlg = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "PythonDependencyUpdateDialog.ui")
        )
        self.vendor_path = os.path.join(
            FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages"
        )
        self.addons = addons

    def show(self):
        """Run the modal dialog"""

        self._create_list_from_pip()
        self.dlg.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
        self.dlg.tableWidget.setSortingEnabled(False)
        self.dlg.labelInstallationPath.setText(self.vendor_path)
        self.dlg.exec()

    def _create_list_from_pip(self):
        """Uses pip and pip -o to generate a list of installed packages, and creates the user
        interface elements for those packages."""

        all_packages_stdout = call_pip(["list", "--path", self.vendor_path])
        outdated_packages_stdout = call_pip(["list", "-o", "--path", self.vendor_path])
        package_list = self._parse_pip_list_output(
            all_packages_stdout, outdated_packages_stdout
        )
        self.dlg.buttonUpdateAll.clicked.connect(
            partial(self._update_all_packages, package_list)
        )

        self.dlg.tableWidget.setRowCount(len(package_list))
        updateButtons = []
        counter = 0
        update_counter = 0
        self.dlg.tableWidget.setSortingEnabled(False)
        for package_name, package_details in package_list.items():
            dependent_addons = self._get_dependent_addons(package_name)
            dependencies = []
            for addon in dependent_addons:
                if addon["optional"]:
                    dependencies.append(addon['name'] + "*")
                else:
                    dependencies.append(addon['name'])
            self.dlg.tableWidget.setItem(
                counter, 0, QtWidgets.QTableWidgetItem(package_name)
            )
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
                updateButtons.append(
                    QtWidgets.QPushButton(translate("AddonsInstaller", "Update"))
                )
                updateButtons[-1].setIcon(QtGui.QIcon(":/icons/button_up.svg"))
                updateButtons[-1].clicked.connect(
                    partial(self._update_package, package_name)
                )
                self.dlg.tableWidget.setCellWidget(counter, 3, updateButtons[-1])
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
            #if addon.installed_version is not None:
                if package.lower() in addon.python_requires:
                    dependent_addons.append({"name":addon.name,"optional":False})
                elif package.lower() in addon.python_optional:
                    dependent_addons.append({"name":addon.name,"optional":True})
        return dependent_addons

    def _parse_pip_list_output(
        self, all_packages, outdated_packages
    ) -> Dict[str, Dict[str, str]]:
        """Parses the output from pip into a dictionary with update information in it. The pip output should
        be an array of lines of text."""

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
                    QtWidgets.QTableWidgetItem(
                        translate("AddonsInstaller", "Updating...")
                    ),
                )
                break
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        call_pip(["install", "--upgrade", package_name, "--target", self.vendor_path])
        self._create_list_from_pip()
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

    def _update_all_packages(self, package_list) -> None:
        """Updates all packages with available updates."""
        updates = []
        for package_name, package_details in package_list.items():
            if (
                len(package_details["available_version"]) > 0
                and package_details["available_version"]
                != package_details["installed_version"]
            ):
                updates.append(package_name)

        for package_name in updates:
            self._update_package(package_name)
