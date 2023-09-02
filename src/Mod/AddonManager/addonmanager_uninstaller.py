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

""" Contains the classes to manage Addon removal: intended as a stable API, safe for
external code to call and to rely upon existing. See classes AddonUninstaller and
MacroUninstaller for details."""

import os
from typing import List

import addonmanager_freecad_interface as fci
from addonmanager_pyside_interface import QObject, Signal

import addonmanager_utilities as utils
from Addon import Addon

translate = fci.translate

# pylint: disable=too-few-public-methods


class InvalidAddon(RuntimeError):
    """Raised when an object that cannot be uninstalled is passed to the constructor"""


class AddonUninstaller(QObject):
    """The core, non-GUI uninstaller class for non-macro addons. Usually instantiated
    and moved to its own thread, otherwise it will block the GUI (if the GUI is
    running) -- since all it does is delete files this is not a huge problem,
    but in some cases the Addon might be quite large, and deletion may take a
    non-trivial amount of time.

    In all cases in this class, the generic Python 'object' argument to the init
    function is intended to be an Addon-like object that provides, at a minimum,
    a 'name' attribute. The Addon manager uses the Addon class for this purpose,
    but external code may use any other class that meets that criterion.

    Recommended Usage (when running with the GUI up, so you don't block the GUI thread):

        addon_to_remove = MyAddon() # Some class with 'name' attribute

        self.worker_thread = QThread()
        self.uninstaller = AddonUninstaller(addon_to_remove)
        self.uninstaller.moveToThread(self.worker_thread)
        self.uninstaller.success.connect(self.removal_succeeded)
        self.uninstaller.failure.connect(self.removal_failed)
        self.uninstaller.finished.connect(self.worker_thread.quit)
        self.worker_thread.started.connect(self.uninstaller.run)
        self.worker_thread.start() # Returns immediately

        # On success, the connections above result in self.removal_succeeded being
        emitted, and # on failure, self.removal_failed is emitted.


    Recommended non-GUI usage (blocks until complete):

        addon_to_remove = MyAddon() # Some class with 'name' attribute
        uninstaller = AddonInstaller(addon_to_remove)
        uninstaller.run()

    """

    # Signals: success and failure Emitted when the installation process is complete.
    # The object emitted is the object that the installation was requested for.
    success = Signal(object)
    failure = Signal(object, str)

    # Finished: regardless of the outcome, this is emitted when all work that is
    # going to be done is done (i.e. whatever thread this is running in can quit).
    finished = Signal()

    def __init__(self, addon: Addon):
        """Initialize the uninstaller."""
        super().__init__()
        self.addon_to_remove = addon
        self.installation_path = fci.DataPaths().mod_dir
        self.macro_installation_path = fci.DataPaths().macro_dir

    def run(self) -> bool:
        """Remove an addon. Returns True if the addon was removed cleanly, or False
        if not. Emits either success or failure prior to returning."""
        success = False
        error_message = translate("AddonsInstaller", "An unknown error occurred")
        if hasattr(self.addon_to_remove, "name") and self.addon_to_remove.name:
            # Make sure we don't accidentally remove the Mod directory
            path_to_remove = os.path.normpath(
                os.path.join(self.installation_path, self.addon_to_remove.name)
            )
            if os.path.exists(path_to_remove) and not os.path.samefile(
                path_to_remove, self.installation_path
            ):
                try:
                    self.run_uninstall_script(path_to_remove)
                    self.remove_extra_files(path_to_remove)
                    success = utils.rmdir(path_to_remove)
                    if (
                        hasattr(self.addon_to_remove, "contains_workbench")
                        and self.addon_to_remove.contains_workbench()
                    ):
                        self.addon_to_remove.desinstall_workbench()
                except OSError as e:
                    error_message = str(e)
            else:
                error_message = translate(
                    "AddonsInstaller",
                    "Could not find addon {} to remove it.",
                ).format(self.addon_to_remove.name)
        if success:
            self.success.emit(self.addon_to_remove)
        else:
            self.failure.emit(self.addon_to_remove, error_message)
        self.addon_to_remove.set_status(Addon.Status.NOT_INSTALLED)
        self.finished.emit()
        return success

    @staticmethod
    def run_uninstall_script(path_to_remove):
        """Run the addon's uninstaller.py script, if it exists"""
        uninstall_script = os.path.join(path_to_remove, "uninstall.py")
        if os.path.exists(uninstall_script):
            # pylint: disable=broad-exception-caught
            try:
                with open(uninstall_script, encoding="utf-8") as f:
                    exec(f.read())
            except Exception:
                fci.Console.PrintError(
                    translate(
                        "AddonsInstaller",
                        "Execution of Addon's uninstall.py script failed. Proceeding with uninstall...",
                    )
                    + "\n"
                )

    @staticmethod
    def remove_extra_files(path_to_remove):
        """When installing, an extra file called AM_INSTALLATION_DIGEST.txt may be
        created, listing extra files that the installer put into place. Remove those
        files."""
        digest = os.path.join(path_to_remove, "AM_INSTALLATION_DIGEST.txt")
        if not os.path.exists(digest):
            return
        with open(digest, encoding="utf-8") as f:
            lines = f.readlines()
            for line in lines:
                stripped = line.strip()
                if len(stripped) > 0 and stripped[0] != "#" and os.path.exists(stripped):
                    try:
                        os.unlink(stripped)
                        fci.Console.PrintMessage(
                            translate("AddonsInstaller", "Removed extra installed file {}").format(
                                stripped
                            )
                            + "\n"
                        )
                    except FileNotFoundError:
                        pass  # Great, no need to remove then!
                    except OSError as e:
                        # Strange error to receive here, but just continue and print
                        # out an error to the console
                        fci.Console.PrintWarning(
                            translate(
                                "AddonsInstaller",
                                "Error while trying to remove extra installed file {}",
                            ).format(stripped)
                            + "\n"
                        )
                        fci.Console.PrintWarning(str(e) + "\n")


class MacroUninstaller(QObject):
    """The core, non-GUI uninstaller class for macro addons. May be run directly on
    the GUI thread if desired, since macros are intended to be relatively small and
    shouldn't have too many files to delete. However, it is a QObject so may also be
    moved into a QThread -- see AddonUninstaller documentation for details of that
    implementation.

    The Python object passed in is expected to provide a "macro" subobject,
    which itself is required to provide at least a "filename" attribute, and may also
    provide an "icon", "xpm", and/or "other_files" attribute. All filenames provided
    by those attributes are expected to be relative to the installed location of the
    "filename" macro file (usually the main FreeCAD user macros directory)."""

    # Signals: success and failure Emitted when the removal process is complete. The
    # object emitted is the object that the removal was requested for.
    success = Signal(object)
    failure = Signal(object, str)

    # Finished: regardless of the outcome, this is emitted when all work that is
    # going to be done is done (i.e. whatever thread this is running in can quit).
    finished = Signal()

    def __init__(self, addon):
        super().__init__()
        self.installation_location = fci.DataPaths().macro_dir
        self.addon_to_remove = addon
        if (
            not hasattr(self.addon_to_remove, "macro")
            or not self.addon_to_remove.macro
            or not hasattr(self.addon_to_remove.macro, "filename")
            or not self.addon_to_remove.macro.filename
        ):
            raise InvalidAddon()

    def run(self):
        """Execute the removal process."""
        success = True
        errors = []
        directories = set()
        for f in self._get_files_to_remove():
            normed = os.path.normpath(f)
            full_path = os.path.join(self.installation_location, normed)
            if "/" in f:
                directories.add(os.path.dirname(full_path))
            try:
                os.unlink(full_path)
                fci.Console.PrintLog(f"Removed macro file {full_path}\n")
            except FileNotFoundError:
                pass  # Great, no need to remove then!
            except OSError as e:
                # Probably permission denied, or something like that
                errors.append(
                    translate(
                        "AddonsInstaller",
                        "Error while trying to remove macro file {}: ",
                    ).format(full_path)
                    + str(e)
                )
                success = False

        self._cleanup_directories(directories)

        if success:
            self.success.emit(self.addon_to_remove)
        else:
            self.failure.emit(self.addon_to_remove, "\n".join(errors))
        self.addon_to_remove.set_status(Addon.Status.NOT_INSTALLED)
        self.finished.emit()

    def _get_files_to_remove(self) -> List[os.PathLike]:
        """Get the list of files that should be removed"""
        files_to_remove = [self.addon_to_remove.macro.filename]
        if self.addon_to_remove.macro.icon:
            files_to_remove.append(self.addon_to_remove.macro.icon)
        if self.addon_to_remove.macro.xpm:
            files_to_remove.append(self.addon_to_remove.macro.name.replace(" ", "_") + "_icon.xpm")
        for f in self.addon_to_remove.macro.other_files:
            files_to_remove.append(f)
        return files_to_remove

    @staticmethod
    def _cleanup_directories(directories):
        """Clean up any extra directories that are leftover and are empty"""
        for directory in directories:
            if os.path.isdir(directory):
                utils.remove_directory_if_empty(directory)
