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

"""Class to manage installation of sets of Python dependencies."""

import os
import subprocess
from typing import List

import addonmanager_freecad_interface as fci
from addonmanager_pyside_interface import QObject, Signal, is_interruption_requested

import addonmanager_utilities as utils
from addonmanager_installer import AddonInstaller, MacroInstaller
from Addon import Addon

translate = fci.translate


class DependencyInstaller(QObject):
    """Install Python dependencies using pip. Intended to be instantiated and then moved into a
    QThread: connect the run() function to the QThread's started() signal."""

    no_python_exe = Signal()
    no_pip = Signal(str)  # Attempted command
    failure = Signal(str, str)  # Short message, detailed message
    finished = Signal()

    def __init__(
        self,
        addons: List[Addon],
        python_requires: List[str],
        python_optional: List[str],
        location: os.PathLike = None,
    ):
        """Install the various types of dependencies that might be specified. If an optional
         dependency fails this is non-fatal, but other failures are considered fatal. If location
        is specified it overrides the FreeCAD user base directory setting: this is used mostly
        for testing purposes and shouldn't be set by normal code in most circumstances.
        """
        super().__init__()
        self.addons = addons
        self.python_requires = python_requires
        self.python_optional = python_optional
        self.location = location

    def run(self):
        """Normally not called directly, but rather connected to the worker thread's started
        signal."""
        if self._verify_pip():
            if self.python_requires or self.python_optional:
                if not is_interruption_requested():
                    self._install_python_packages()
        if not is_interruption_requested():
            self._install_addons()
        self.finished.emit()

    def _install_python_packages(self):
        """Install required and optional Python dependencies using pip."""

        if self.location:
            vendor_path = os.path.join(self.location, "AdditionalPythonPackages")
        else:
            vendor_path = utils.get_pip_target_directory()
        if not os.path.exists(vendor_path):
            os.makedirs(vendor_path)

        self._install_required(vendor_path)
        self._install_optional(vendor_path)

    def _verify_pip(self) -> bool:
        """Ensure that pip is working -- returns True if it is, or False if not. Also emits the
        no_pip signal if pip cannot execute."""
        python_exe = self._get_python()
        if not python_exe:
            return False
        try:
            proc = self._run_pip(["--version"])
            fci.Console.PrintMessage(proc.stdout + "\n")
        except subprocess.CalledProcessError:
            self.no_pip.emit(f"{python_exe} -m pip --version")
            return False
        return True

    def _install_required(self, vendor_path: str) -> bool:
        """Install the required Python package dependencies. If any fail a failure
        signal is emitted and the function exits without proceeding with any additional
        installations."""
        for pymod in self.python_requires:
            if is_interruption_requested():
                return False
            try:
                proc = self._run_pip(
                    [
                        "install",
                        "--disable-pip-version-check",
                        "--target",
                        vendor_path,
                        pymod,
                    ]
                )
                fci.Console.PrintMessage(proc.stdout + "\n")
            except subprocess.CalledProcessError as e:
                fci.Console.PrintError(str(e) + "\n")
                self.failure.emit(
                    translate(
                        "AddonsInstaller",
                        "Installation of Python package {} failed",
                    ).format(pymod),
                    str(e),
                )
                return False
        return True

    def _install_optional(self, vendor_path: str):
        """Install the optional Python package dependencies. If any fail a message is printed to
        the console, but installation of the others continues."""
        for pymod in self.python_optional:
            if is_interruption_requested():
                return
            try:
                proc = self._run_pip(
                    [
                        "install",
                        "--disable-pip-version-check",
                        "--target",
                        vendor_path,
                        pymod,
                    ]
                )
                fci.Console.PrintMessage(proc.stdout + "\n")
            except subprocess.CalledProcessError as e:
                fci.Console.PrintError(
                    translate("AddonsInstaller", "Installation of optional package failed")
                    + ":\n"
                    + str(e)
                    + "\n"
                )

    def _run_pip(self, args):
        python_exe = self._get_python()
        final_args = [python_exe, "-m", "pip"]
        final_args.extend(args)
        return self._subprocess_wrapper(final_args)

    @staticmethod
    def _subprocess_wrapper(args) -> subprocess.CompletedProcess:
        """Wrap subprocess call so test code can mock it."""
        return utils.run_interruptable_subprocess(args)

    def _get_python(self) -> str:
        """Wrap Python access so test code can mock it."""
        python_exe = utils.get_python_exe()
        if not python_exe:
            self.no_python_exe.emit()
        return python_exe

    def _install_addons(self):
        for addon in self.addons:
            if is_interruption_requested():
                return
            fci.Console.PrintMessage(
                translate("AddonsInstaller", "Installing required dependency {}").format(addon.name)
                + "\n"
            )
            if addon.macro is None:
                installer = AddonInstaller(addon)
            else:
                installer = MacroInstaller(addon)
            result = installer.run()  # Run in this thread, which should be off the GUI thread
            if not result:
                self.failure.emit(
                    translate("AddonsInstaller", "Installation of Addon {} failed").format(
                        addon.name
                    ),
                    "",
                )
                return
