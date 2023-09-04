#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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
import functools
import tempfile
import threading
import json
from datetime import date
from typing import Dict

from PySide import QtGui, QtCore, QtWidgets
import FreeCAD
import FreeCADGui

from addonmanager_workers_startup import (
    CreateAddonListWorker,
    LoadPackagesFromCacheWorker,
    LoadMacrosFromCacheWorker,
    CheckWorkbenchesForUpdatesWorker,
    CacheMacroCodeWorker,
)
from addonmanager_workers_installation import (
    UpdateMetadataCacheWorker,
)
from addonmanager_installer_gui import AddonInstallerGUI, MacroInstallerGUI
from addonmanager_uninstaller_gui import AddonUninstallerGUI
from addonmanager_update_all_gui import UpdateAllGUI
import addonmanager_utilities as utils
import AddonManager_rc  # This is required by Qt, it's not unused
from package_list import PackageList, PackageListItemModel
from package_details import PackageDetails
from Addon import Addon
from manage_python_dependencies import (
    PythonPackageManager,
)
from addonmanager_cache import local_cache_needs_update
from addonmanager_devmode import DeveloperMode
from addonmanager_firstrun import FirstRunDialog
from addonmanager_connection_checker import ConnectionCheckerGUI
from addonmanager_devmode_metadata_checker import MetadataValidators

import NetworkManager

from AddonManagerOptions import AddonManagerOptions

translate = FreeCAD.Qt.translate


def QT_TRANSLATE_NOOP(_, txt):
    return txt


__title__ = "FreeCAD Addon Manager Module"
__author__ = "Yorik van Havre", "Jonathan Wiedemann", "Kurt Kremitzki", "Chris Hennes"
__url__ = "http://www.freecad.org"

"""
FreeCAD Addon Manager Module

Fetches various types of addons from a variety of sources. Built-in sources are:
* https://github.com/FreeCAD/FreeCAD-addons
* https://github.com/FreeCAD/FreeCAD-macros
* https://wiki.freecad.org/

Additional git sources may be configure via user preferences.

You need a working internet connection, and optionally git -- if git is not available, ZIP archives
are downloaded instead.
"""

#  \defgroup ADDONMANAGER AddonManager
#  \ingroup ADDONMANAGER
#  \brief The Addon Manager allows users to install workbenches and macros made by other users
#  @{

INSTANCE = None


class CommandAddonManager:
    """The main Addon Manager class and FreeCAD command"""

    workers = [
        "create_addon_list_worker",
        "check_worker",
        "show_worker",
        "showmacro_worker",
        "macro_worker",
        "update_metadata_cache_worker",
        "load_macro_metadata_worker",
        "update_all_worker",
        "check_for_python_package_updates_worker",
    ]

    lock = threading.Lock()
    restart_required = False

    def __init__(self):
        QT_TRANSLATE_NOOP("QObject", "Addon Manager")
        FreeCADGui.addPreferencePage(
            AddonManagerOptions,
            "Addon Manager",
        )

        self.check_worker = None
        self.check_for_python_package_updates_worker = None
        self.update_all_worker = None
        self.developer_mode = None
        self.installer_gui = None

        self.update_cache = False
        self.dialog = None
        self.startup_sequence = []

        # Set up the connection checker
        self.connection_checker = ConnectionCheckerGUI()
        self.connection_checker.connection_available.connect(self.launch)

        # Give other parts of the AM access to the current instance
        global INSTANCE
        INSTANCE = self

    def GetResources(self) -> Dict[str, str]:
        """FreeCAD-required function: get the core resource information for this Mod."""
        return {
            "Pixmap": "AddonManager",
            "MenuText": QT_TRANSLATE_NOOP("Std_AddonMgr", "&Addon manager"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Std_AddonMgr",
                "Manage external workbenches, macros, and preference packs",
            ),
            "Group": "Tools",
        }

    def Activated(self) -> None:
        """FreeCAD-required function: called when the command is activated."""
        NetworkManager.InitializeNetworkManager()
        firstRunDialog = FirstRunDialog()
        if not firstRunDialog.exec():
            return
        self.connection_checker.start()

    def launch(self) -> None:
        """Shows the Addon Manager UI"""

        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManager.ui")
        )
        self.dialog.setObjectName("AddonManager_Main_Window")
        # self.dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        # cleanup the leftovers from previous runs
        self.macro_repo_dir = FreeCAD.getUserMacroDir(True)
        self.packages_with_updates = set()
        self.startup_sequence = []
        self.cleanup_workers()
        self.update_cache = local_cache_needs_update()

        # restore window geometry from stored state
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        w = pref.GetInt("WindowWidth", 800)
        h = pref.GetInt("WindowHeight", 600)
        self.dialog.resize(w, h)

        # If we are checking for updates automatically, hide the Check for updates button:
        autocheck = pref.GetBool("AutoCheck", False)
        if autocheck:
            self.dialog.buttonCheckForUpdates.hide()
        else:
            self.dialog.buttonUpdateAll.hide()

        # Set up the listing of packages using the model-view-controller architecture
        self.packageList = PackageList(self.dialog)
        self.item_model = PackageListItemModel()
        self.packageList.setModel(self.item_model)
        self.dialog.contentPlaceholder.hide()
        self.dialog.layout().replaceWidget(self.dialog.contentPlaceholder, self.packageList)
        self.packageList.show()

        # Package details start out hidden
        self.packageDetails = PackageDetails(self.dialog)
        self.packageDetails.hide()
        index = self.dialog.layout().indexOf(self.packageList)
        self.dialog.layout().insertWidget(index, self.packageDetails)

        # set nice icons to everything, by theme with fallback to FreeCAD icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
        self.dialog.buttonUpdateAll.setIcon(QtGui.QIcon(":/icons/button_valid.svg"))
        self.dialog.buttonCheckForUpdates.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
        self.dialog.buttonClose.setIcon(
            QtGui.QIcon.fromTheme("close", QtGui.QIcon(":/icons/process-stop.svg"))
        )
        self.dialog.buttonPauseUpdate.setIcon(
            QtGui.QIcon.fromTheme("pause", QtGui.QIcon(":/icons/media-playback-stop.svg"))
        )

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        dev_mode_active = pref.GetBool("developerMode", False)

        # enable/disable stuff
        self.dialog.buttonUpdateAll.setEnabled(False)
        self.hide_progress_widgets()
        self.dialog.buttonUpdateCache.setEnabled(False)
        self.dialog.buttonUpdateCache.setText(translate("AddonsInstaller", "Starting up..."))
        if dev_mode_active:
            self.dialog.buttonDevTools.show()
        else:
            self.dialog.buttonDevTools.hide()

        # connect slots
        self.dialog.rejected.connect(self.reject)
        self.dialog.buttonUpdateAll.clicked.connect(self.update_all)
        self.dialog.buttonClose.clicked.connect(self.dialog.reject)
        self.dialog.buttonUpdateCache.clicked.connect(self.on_buttonUpdateCache_clicked)
        self.dialog.buttonPauseUpdate.clicked.connect(self.stop_update)
        self.dialog.buttonCheckForUpdates.clicked.connect(
            lambda: self.force_check_updates(standalone=True)
        )
        self.dialog.buttonUpdateDependencies.clicked.connect(self.show_python_updates_dialog)
        self.dialog.buttonDevTools.clicked.connect(self.show_developer_tools)
        self.packageList.itemSelected.connect(self.table_row_activated)
        self.packageList.setEnabled(False)
        self.packageDetails.execute.connect(self.executemacro)
        self.packageDetails.install.connect(self.launch_installer_gui)
        self.packageDetails.uninstall.connect(self.remove)
        self.packageDetails.update.connect(self.update)
        self.packageDetails.back.connect(self.on_buttonBack_clicked)
        self.packageDetails.update_status.connect(self.status_updated)

        # center the dialog over the FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(
            mw.frameGeometry().topLeft() + mw.rect().center() - self.dialog.rect().center()
        )

        # set info for the progress bar:
        self.dialog.progressBar.setMaximum(1000)

        # begin populating the table in a set of sub-threads
        self.startup()

        # set the label text to start with
        self.show_information(translate("AddonsInstaller", "Loading addon information"))

        # rock 'n roll!!!
        self.dialog.exec()

    def cleanup_workers(self) -> None:
        """Ensure that no workers are running by explicitly asking them to stop and waiting for
        them until they do"""
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.blockSignals(True)
                        thread.requestInterruption()
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        finished = thread.wait(500)
                        if not finished:
                            FreeCAD.Console.PrintWarning(
                                translate(
                                    "AddonsInstaller",
                                    "Worker process {} is taking a long time to stop...",
                                ).format(worker)
                                + "\n"
                            )

    def reject(self) -> None:
        """called when the window has been closed"""

        # save window geometry for next use
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("WindowWidth", self.dialog.width())
        pref.SetInt("WindowHeight", self.dialog.height())

        # ensure all threads are finished before closing
        oktoclose = True
        worker_killed = False
        self.startup_sequence = []
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.blockSignals(True)
                        thread.requestInterruption()
                        worker_killed = True
                        oktoclose = False
        while not oktoclose:
            oktoclose = True
            for worker in self.workers:
                if hasattr(self, worker):
                    thread = getattr(self, worker)
                    if thread:
                        thread.wait(25)
                        if not thread.isFinished():
                            oktoclose = False
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)

        # Write the cache data if it's safe to do so:
        if not worker_killed:
            for repo in self.item_model.repos:
                if repo.repo_type == Addon.Kind.MACRO:
                    self.cache_macro(repo)
                else:
                    self.cache_package(repo)
            self.write_package_cache()
            self.write_macro_cache()
        else:
            self.write_cache_stopfile()
            FreeCAD.Console.PrintLog(
                "Not writing the cache because a process was forcibly terminated and the state is "
                "unknown.\n"
            )

        if self.restart_required:
            # display restart dialog
            m = QtWidgets.QMessageBox()
            m.setWindowTitle(translate("AddonsInstaller", "Addon manager"))
            m.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
            m.setText(
                translate(
                    "AddonsInstaller",
                    "You must restart FreeCAD for changes to take effect.",
                )
            )
            m.setIcon(m.Warning)
            m.setStandardButtons(m.Ok | m.Cancel)
            m.setDefaultButton(m.Cancel)
            okBtn = m.button(QtWidgets.QMessageBox.StandardButton.Ok)
            cancelBtn = m.button(QtWidgets.QMessageBox.StandardButton.Cancel)
            okBtn.setText(translate("AddonsInstaller", "Restart now"))
            cancelBtn.setText(translate("AddonsInstaller", "Restart later"))
            ret = m.exec_()
            if ret == m.Ok:
                # restart FreeCAD after a delay to give time to this dialog to close
                QtCore.QTimer.singleShot(1000, utils.restart_freecad)

    def startup(self) -> None:
        """Downloads the available packages listings and populates the table

        This proceeds in four stages: first, the main GitHub repository is queried for a list of
        possible addons. Each addon is specified as a git submodule with name and branch
        information. The actual specific commit ID of the submodule (as listed on Github) is
        ignored. Any extra repositories specified by the user are appended to this list.

        Second, the list of macros is downloaded from the FreeCAD/FreeCAD-macros repository and
        the wiki.

        Third, each of these items is queried for a package.xml metadata file. If that file exists
        it is downloaded, cached, and any icons that it references are also downloaded and cached.

        Finally, for workbenches that are not contained within a package (e.g. they provide no
        metadata), an additional git query is made to see if an update is available. Macros are
        checked for file changes.

        Each of these stages is launched in a separate thread to ensure that the UI remains
        responsive, and the operation can be cancelled.

        Each stage is also subject to caching, so may return immediately, if no cache update has
        been requested."""

        # Each function in this list is expected to launch a thread and connect its completion
        # signal to self.do_next_startup_phase, or to shortcut to calling
        # self.do_next_startup_phase if it is not launching a worker
        self.startup_sequence = [
            self.populate_packages_table,
            self.activate_table_widgets,
            self.populate_macros,
            self.update_metadata_cache,
            self.check_updates,
            self.check_python_updates,
        ]
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        if pref.GetBool("DownloadMacros", False):
            self.startup_sequence.append(self.load_macro_metadata)
        selection = pref.GetString("SelectedAddon", "")
        if selection:
            self.startup_sequence.insert(2, functools.partial(self.select_addon, selection))
            pref.SetString("SelectedAddon", "")
        self.current_progress_region = 0
        self.number_of_progress_regions = len(self.startup_sequence)
        self.do_next_startup_phase()

    def do_next_startup_phase(self) -> None:
        """Pop the top item in self.startup_sequence off the list and run it"""

        if len(self.startup_sequence) > 0:
            phase_runner = self.startup_sequence.pop(0)
            self.current_progress_region += 1
            phase_runner()
        else:
            self.hide_progress_widgets()
            self.update_cache = False
            self.dialog.buttonUpdateCache.setEnabled(True)
            self.dialog.buttonUpdateCache.setText(
                translate("AddonsInstaller", "Refresh local cache")
            )
            pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
            pref.SetString("LastCacheUpdate", date.today().isoformat())
            self.packageList.item_filter.invalidateFilter()

    def populate_packages_table(self) -> None:
        self.item_model.clear()

        use_cache = not self.update_cache
        if use_cache:
            if os.path.isfile(utils.get_cache_file_name("package_cache.json")):
                with open(utils.get_cache_file_name("package_cache.json"), encoding="utf-8") as f:
                    data = f.read()
                    try:
                        from_json = json.loads(data)
                        if len(from_json) == 0:
                            use_cache = False
                    except json.JSONDecodeError:
                        use_cache = False
            else:
                use_cache = False

        if not use_cache:
            self.update_cache = True  # Make sure to trigger the other cache updates, if the json
            # file was missing
            self.create_addon_list_worker = CreateAddonListWorker()
            self.create_addon_list_worker.status_message.connect(self.show_information)
            self.create_addon_list_worker.addon_repo.connect(self.add_addon_repo)
            self.update_progress_bar(10, 100)
            self.create_addon_list_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 2
            self.create_addon_list_worker.start()
        else:
            self.create_addon_list_worker = LoadPackagesFromCacheWorker(
                utils.get_cache_file_name("package_cache.json")
            )
            self.create_addon_list_worker.addon_repo.connect(self.add_addon_repo)
            self.update_progress_bar(10, 100)
            self.create_addon_list_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 2
            self.create_addon_list_worker.start()

    def cache_package(self, repo: Addon):
        if not hasattr(self, "package_cache"):
            self.package_cache = {}
        self.package_cache[repo.name] = repo.to_cache()

    def write_package_cache(self):
        if hasattr(self, "package_cache"):
            package_cache_path = utils.get_cache_file_name("package_cache.json")
            with open(package_cache_path, "w", encoding="utf-8") as f:
                f.write(json.dumps(self.package_cache, indent="  "))

    def activate_table_widgets(self) -> None:
        self.packageList.setEnabled(True)
        self.packageList.ui.lineEditFilter.setFocus()
        self.do_next_startup_phase()

    def populate_macros(self) -> None:
        macro_cache_file = utils.get_cache_file_name("macro_cache.json")
        cache_is_bad = True
        if os.path.isfile(macro_cache_file):
            size = os.path.getsize(macro_cache_file)
            if size > 1000:  # Make sure there is actually data in there
                cache_is_bad = False
        if cache_is_bad:
            if not self.update_cache:
                self.update_cache = True  # Make sure to trigger the other cache updates, if the
                # json file was missing
                self.create_addon_list_worker = CreateAddonListWorker()
                self.create_addon_list_worker.status_message.connect(self.show_information)
                self.create_addon_list_worker.addon_repo.connect(self.add_addon_repo)
                self.update_progress_bar(10, 100)
                self.create_addon_list_worker.finished.connect(
                    self.do_next_startup_phase
                )  # Link to step 2
                self.create_addon_list_worker.start()
            else:
                # It's already been done in the previous step (TODO: Refactor to eliminate this
                # step)
                self.do_next_startup_phase()
        else:
            self.macro_worker = LoadMacrosFromCacheWorker(
                utils.get_cache_file_name("macro_cache.json")
            )
            self.macro_worker.add_macro_signal.connect(self.add_addon_repo)
            self.macro_worker.finished.connect(self.do_next_startup_phase)
            self.macro_worker.start()

    def cache_macro(self, repo: Addon):
        if not hasattr(self, "macro_cache"):
            self.macro_cache = []
        if repo.macro is not None:
            self.macro_cache.append(repo.macro.to_cache())
        else:
            FreeCAD.Console.PrintError(
                f"Addon Manager: Internal error, cache_macro called on non-macro {repo.name}\n"
            )

    def write_macro_cache(self):
        if not hasattr(self, "macro_cache"):
            return
        macro_cache_path = utils.get_cache_file_name("macro_cache.json")
        with open(macro_cache_path, "w", encoding="utf-8") as f:
            f.write(json.dumps(self.macro_cache, indent="  "))
            self.macro_cache = []

    def update_metadata_cache(self) -> None:
        if self.update_cache:
            self.update_metadata_cache_worker = UpdateMetadataCacheWorker(self.item_model.repos)
            self.update_metadata_cache_worker.status_message.connect(self.show_information)
            self.update_metadata_cache_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 4
            self.update_metadata_cache_worker.progress_made.connect(self.update_progress_bar)
            self.update_metadata_cache_worker.package_updated.connect(self.on_package_updated)
            self.update_metadata_cache_worker.start()
        else:
            self.do_next_startup_phase()

    def on_buttonUpdateCache_clicked(self) -> None:
        self.update_cache = True
        cache_path = FreeCAD.getUserCachePath()
        am_path = os.path.join(cache_path, "AddonManager")
        utils.rmdir(am_path)
        self.dialog.buttonUpdateCache.setEnabled(False)
        self.dialog.buttonUpdateCache.setText(translate("AddonsInstaller", "Updating cache..."))
        self.startup()

        # Recaching implies checking for updates, regardless of the user's autocheck option
        self.startup_sequence.remove(self.check_updates)
        self.startup_sequence.append(self.force_check_updates)

    def on_package_updated(self, repo: Addon) -> None:
        """Called when the named package has either new metadata or a new icon (or both)"""

        with self.lock:
            repo.icon = self.get_icon(repo, update=True)
            self.item_model.reload_item(repo)

    def load_macro_metadata(self) -> None:
        if self.update_cache:
            self.load_macro_metadata_worker = CacheMacroCodeWorker(self.item_model.repos)
            self.load_macro_metadata_worker.status_message.connect(self.show_information)
            self.load_macro_metadata_worker.update_macro.connect(self.on_package_updated)
            self.load_macro_metadata_worker.progress_made.connect(self.update_progress_bar)
            self.load_macro_metadata_worker.finished.connect(self.do_next_startup_phase)
            self.load_macro_metadata_worker.start()
        else:
            self.do_next_startup_phase()

    def select_addon(self, name: str) -> None:
        found = False
        for addon in self.item_model.repos:
            if addon.name == name:
                self.table_row_activated(addon)
                found = True
                break
        if not found:
            FreeCAD.Console.PrintWarning(
                translate("AddonsInstaller", "Could not find addon '{}' to select\n").format(name)
            )
        self.do_next_startup_phase()

    def check_updates(self) -> None:
        "checks every installed addon for available updates"

        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        autocheck = pref.GetBool("AutoCheck", False)
        if not autocheck:
            FreeCAD.Console.PrintLog(
                "Addon Manager: Skipping update check because AutoCheck user preference is False\n"
            )
            self.do_next_startup_phase()
            return
        if not self.packages_with_updates:
            self.force_check_updates(standalone=False)
        else:
            self.do_next_startup_phase()

    def force_check_updates(self, standalone=False) -> None:
        if hasattr(self, "check_worker"):
            thread = self.check_worker
            if thread:
                if not thread.isFinished():
                    self.do_next_startup_phase()
                    return

        self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller", "Checking for updates..."))
        self.packages_with_updates.clear()
        self.dialog.buttonUpdateAll.show()
        self.dialog.buttonCheckForUpdates.setDisabled(True)
        self.check_worker = CheckWorkbenchesForUpdatesWorker(self.item_model.repos)
        self.check_worker.finished.connect(self.do_next_startup_phase)
        self.check_worker.finished.connect(self.update_check_complete)
        self.check_worker.progress_made.connect(self.update_progress_bar)
        if standalone:
            self.current_progress_region = 1
            self.number_of_progress_regions = 1
        self.check_worker.update_status.connect(self.status_updated)
        self.check_worker.start()
        self.enable_updates(len(self.packages_with_updates))

    def status_updated(self, repo: Addon) -> None:
        self.item_model.reload_item(repo)
        if repo.status() == Addon.Status.UPDATE_AVAILABLE:
            self.packages_with_updates.add(repo)
            self.enable_updates(len(self.packages_with_updates))
        elif repo.status() == Addon.Status.PENDING_RESTART:
            self.restart_required = True

    def enable_updates(self, number_of_updates: int) -> None:
        """enables the update button"""

        if number_of_updates:
            s = translate("AddonsInstaller", "Apply {} update(s)", "", number_of_updates)
            self.dialog.buttonUpdateAll.setText(s.format(number_of_updates))
            self.dialog.buttonUpdateAll.setEnabled(True)
        elif hasattr(self, "check_worker") and self.check_worker.isRunning():
            self.dialog.buttonUpdateAll.setText(
                translate("AddonsInstaller", "Checking for updates...")
            )
        else:
            self.dialog.buttonUpdateAll.setText(
                translate("AddonsInstaller", "No updates available")
            )
            self.dialog.buttonUpdateAll.setEnabled(False)

    def update_check_complete(self) -> None:
        self.enable_updates(len(self.packages_with_updates))
        self.dialog.buttonCheckForUpdates.setEnabled(True)

    def check_python_updates(self) -> None:
        PythonPackageManager.migrate_old_am_installations()  # Migrate 0.20 to 0.21
        self.do_next_startup_phase()

    def show_python_updates_dialog(self) -> None:
        if not hasattr(self, "manage_python_packages_dialog"):
            self.manage_python_packages_dialog = PythonPackageManager(self.item_model.repos)
        self.manage_python_packages_dialog.show()

    def show_developer_tools(self) -> None:
        """Display the developer tools dialog"""
        if not self.developer_mode:
            self.developer_mode = DeveloperMode()
        self.developer_mode.show()

        checker = MetadataValidators()
        checker.validate_all(self.item_model.repos)

    def add_addon_repo(self, addon_repo: Addon) -> None:
        """adds a workbench to the list"""

        if addon_repo.icon is None or addon_repo.icon.isNull():
            addon_repo.icon = self.get_icon(addon_repo)
        for repo in self.item_model.repos:
            if repo.name == addon_repo.name:
                # self.item_model.reload_item(repo) # If we want to have later additions superseded
                # earlier
                return
        self.item_model.append_item(addon_repo)

    def get_icon(self, repo: Addon, update: bool = False) -> QtGui.QIcon:
        """Returns an icon for an Addon. Uses a cached icon if possible, unless update is True,
        in which case the icon is regenerated."""

        if not update and repo.icon and not repo.icon.isNull() and repo.icon.isValid():
            return repo.icon

        path = ":/icons/" + repo.name.replace(" ", "_")
        if repo.repo_type == Addon.Kind.WORKBENCH:
            path += "_workbench_icon.svg"
            default_icon = QtGui.QIcon(":/icons/document-package.svg")
        elif repo.repo_type == Addon.Kind.MACRO:
            if repo.macro and repo.macro.icon:
                if os.path.isabs(repo.macro.icon):
                    path = repo.macro.icon
                    default_icon = QtGui.QIcon(":/icons/document-python.svg")
                else:
                    path = os.path.join(os.path.dirname(repo.macro.src_filename), repo.macro.icon)
                    default_icon = QtGui.QIcon(":/icons/document-python.svg")
            elif repo.macro and repo.macro.xpm:
                cache_path = FreeCAD.getUserCachePath()
                am_path = os.path.join(cache_path, "AddonManager", "MacroIcons")
                os.makedirs(am_path, exist_ok=True)
                path = os.path.join(am_path, repo.name + "_icon.xpm")
                if not os.path.exists(path):
                    with open(path, "w") as f:
                        f.write(repo.macro.xpm)
                default_icon = QtGui.QIcon(repo.macro.xpm)
            else:
                path += "_macro_icon.svg"
                default_icon = QtGui.QIcon(":/icons/document-python.svg")
        elif repo.repo_type == Addon.Kind.PACKAGE:
            # The cache might not have been downloaded yet, check to see if it's there...
            if os.path.isfile(repo.get_cached_icon_filename()):
                path = repo.get_cached_icon_filename()
            elif repo.contains_workbench():
                path += "_workbench_icon.svg"
                default_icon = QtGui.QIcon(":/icons/document-package.svg")
            elif repo.contains_macro():
                path += "_macro_icon.svg"
                default_icon = QtGui.QIcon(":/icons/document-python.svg")
            else:
                default_icon = QtGui.QIcon(":/icons/document-package.svg")

        if QtCore.QFile.exists(path):
            addonicon = QtGui.QIcon(path)
        else:
            addonicon = default_icon
        repo.icon = addonicon

        return addonicon

    def table_row_activated(self, selected_repo: Addon) -> None:
        """a row was activated, show the relevant data"""

        self.packageList.hide()
        self.packageDetails.show()
        self.packageDetails.show_repo(selected_repo)

    def show_information(self, message: str) -> None:
        """shows generic text in the information pane"""

        self.dialog.labelStatusInfo.setText(message)
        self.dialog.labelStatusInfo.repaint()

    def show_workbench(self, repo: Addon) -> None:
        self.packageList.hide()
        self.packageDetails.show()
        self.packageDetails.show_repo(repo)

    def on_buttonBack_clicked(self) -> None:
        self.packageDetails.hide()
        self.packageList.show()

    def append_to_repos_list(self, repo: Addon) -> None:
        """this function allows threads to update the main list of workbenches"""
        self.item_model.append_item(repo)

    def update(self, repo: Addon) -> None:
        self.launch_installer_gui(repo)

    def mark_repo_update_available(self, repo: Addon, available: bool) -> None:
        if available:
            repo.set_status(Addon.Status.UPDATE_AVAILABLE)
        else:
            repo.set_status(Addon.Status.NO_UPDATE_AVAILABLE)
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)

    def launch_installer_gui(self, addon: Addon) -> None:
        if self.installer_gui is not None:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller",
                    "Cannot launch a new installer until the previous one has finished.",
                )
            )
            return
        if addon.macro is not None:
            self.installer_gui = MacroInstallerGUI(addon)
        else:
            self.installer_gui = AddonInstallerGUI(addon, self.item_model.repos)
        self.installer_gui.success.connect(self.on_package_status_changed)
        self.installer_gui.finished.connect(self.cleanup_installer)
        self.installer_gui.run()  # Does not block

    def cleanup_installer(self) -> None:
        QtCore.QTimer.singleShot(500, self.no_really_clean_up_the_installer)

    def no_really_clean_up_the_installer(self) -> None:
        self.installer_gui = None

    def update_all(self) -> None:
        """Asynchronously apply all available updates: individual failures are noted, but do not
        stop other updates"""

        if self.installer_gui is not None:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller",
                    "Cannot launch a new installer until the previous one has finished.",
                )
            )
            return

        self.installer_gui = UpdateAllGUI(self.item_model.repos)
        self.installer_gui.addon_updated.connect(self.on_package_status_changed)
        self.installer_gui.finished.connect(self.cleanup_installer)
        self.installer_gui.run()  # Does not block

    def hide_progress_widgets(self) -> None:
        """hides the progress bar and related widgets"""

        self.dialog.labelStatusInfo.hide()
        self.dialog.progressBar.hide()
        self.dialog.buttonPauseUpdate.hide()
        self.packageList.ui.lineEditFilter.setFocus()

    def show_progress_widgets(self) -> None:
        if self.dialog.progressBar.isHidden():
            self.dialog.progressBar.show()
            self.dialog.buttonPauseUpdate.show()
            self.dialog.labelStatusInfo.show()

    def update_progress_bar(self, current_value: int, max_value: int) -> None:
        """Update the progress bar, showing it if it's hidden"""

        max_value = max_value if max_value > 0 else 1

        if current_value < 0:
            current_value = 0
        elif current_value > max_value:
            current_value = max_value

        self.show_progress_widgets()
        region_size = 100.0 / self.number_of_progress_regions
        completed_region_portion = (self.current_progress_region - 1) * region_size
        current_region_portion = (float(current_value) / float(max_value)) * region_size
        value = completed_region_portion + current_region_portion
        self.dialog.progressBar.setValue(
            value * 10
        )  # Out of 1000 segments, so it moves sort of smoothly
        self.dialog.progressBar.repaint()

    def stop_update(self) -> None:
        self.cleanup_workers()
        self.hide_progress_widgets()
        self.write_cache_stopfile()
        self.dialog.buttonUpdateCache.setEnabled(True)
        self.dialog.buttonUpdateCache.setText(translate("AddonsInstaller", "Refresh local cache"))

    def write_cache_stopfile(self) -> None:
        stopfile = utils.get_cache_file_name("CACHE_UPDATE_INTERRUPTED")
        with open(stopfile, "w", encoding="utf8") as f:
            f.write(
                "This file indicates that a cache operation was interrupted, and "
                "the cache is in an unknown state. It will be deleted next time "
                "AddonManager recaches."
            )

    def on_package_status_changed(self, repo: Addon) -> None:
        if repo.status() == Addon.Status.PENDING_RESTART:
            self.restart_required = True
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)
        if repo in self.packages_with_updates:
            self.packages_with_updates.remove(repo)
            self.enable_updates(len(self.packages_with_updates))

    def executemacro(self, repo: Addon) -> None:
        """executes a selected macro"""

        macro = repo.macro
        if not macro or not macro.code:
            return

        if macro.is_installed():
            macro_path = os.path.join(self.macro_repo_dir, macro.filename)
            FreeCADGui.open(str(macro_path))
            self.dialog.hide()
            FreeCADGui.SendMsgToActiveView("Run")
        else:
            with tempfile.TemporaryDirectory() as dir:
                temp_install_succeeded = macro.install(dir)
                if not temp_install_succeeded:
                    message = translate(
                        "AddonsInstaller",
                        "Execution of macro failed. See console for failure details.",
                    )
                    return
                macro_path = os.path.join(dir, macro.filename)
                FreeCADGui.open(str(macro_path))
                self.dialog.hide()
                FreeCADGui.SendMsgToActiveView("Run")

    def remove(self, addon: Addon) -> None:
        """Remove this addon."""
        if self.installer_gui is not None:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller",
                    "Cannot launch a new installer until the previous one has finished.",
                )
            )
            return
        self.installer_gui = AddonUninstallerGUI(addon)
        self.installer_gui.finished.connect(self.cleanup_installer)
        self.installer_gui.finished.connect(
            functools.partial(self.on_package_status_changed, addon)
        )
        self.installer_gui.run()  # Does not block


# @}
