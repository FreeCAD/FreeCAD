#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import os
import shutil
import stat
import tempfile
import hashlib
from datetime import date, timedelta
from typing import Dict, List

from PySide2 import QtGui, QtCore, QtWidgets
import FreeCADGui

from addonmanager_workers import *
import addonmanager_utilities as utils
import AddonManager_rc
from package_list import PackageList, PackageListItemModel
from package_details import PackageDetails
from Addon import Addon
from install_to_toolbar import (
    ask_to_install_toolbar_button,
    remove_custom_toolbar_button,
)

from NetworkManager import HAVE_QTNETWORK, InitializeNetworkManager

translate = FreeCAD.Qt.translate

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

You need a working internet connection, and optionally the GitPython package
installed.
"""

#  \defgroup ADDONMANAGER AddonManager
#  \ingroup ADDONMANAGER
#  \brief The Addon Manager allows users to install workbenches and macros made by other users
#  @{


def QT_TRANSLATE_NOOP(ctx, txt):
    return txt

ADDON_MANAGER_DEVELOPER_MODE = False


class CommandAddonManager:
    """The main Addon Manager class and FreeCAD command"""

    workers = [
        "connection_checker",
        "update_worker",
        "check_worker",
        "show_worker",
        "showmacro_worker",
        "macro_worker",
        "install_worker",
        "update_metadata_cache_worker",
        "load_macro_metadata_worker",
        "update_all_worker",
        "dependency_installation_worker",
    ]

    lock = threading.Lock()
    restart_required = False

    def __init__(self):
        FreeCADGui.addPreferencePage(
            os.path.join(os.path.dirname(__file__), "AddonManagerOptions.ui"),
            translate("AddonsInstaller","Addon Manager"),
        )

        self.allowed_packages = set()
        allow_file = os.path.join(
            os.path.dirname(__file__), "ALLOWED_PYTHON_PACKAGES.txt"
        )
        if os.path.exists(allow_file):
            with open(allow_file, "r", encoding="utf8") as f:
                lines = f.readlines()
                for line in lines:
                    if line and len(line) > 0 and line[0] != "#":
                        self.allowed_packages.add(line.strip())
        else:
            FreeCAD.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Addon Manager installation problem: could not locate ALLOWED_PYTHON_PACKAGES.txt",
                )
                + "\n"
            )

    def GetResources(self) -> Dict[str, str]:
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

        InitializeNetworkManager()

        # display first use dialog if needed
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        readWarning = pref.GetBool("readWarning2022", False)

        global ADDON_MANAGER_DEVELOPER_MODE
        ADDON_MANAGER_DEVELOPER_MODE = pref.GetBool("developerMode", False)

        if not readWarning:
            warning_dialog = FreeCADGui.PySideUic.loadUi(
                os.path.join(os.path.dirname(__file__), "first_run.ui")
            )
            warning_dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
            autocheck = pref.GetBool("AutoCheck", False)
            download_macros = pref.GetBool("DownloadMacros", False)
            proxy_string = pref.GetString("ProxyUrl", "")
            if pref.GetBool("NoProxyCheck", True):
                proxy_option = 0
            elif pref.GetBool("SystemProxyCheck", False):
                proxy_option = 1
            elif pref.GetBool("UserProxyCheck", False):
                proxy_option = 2

            def toggle_proxy_list(option: int):
                if option == 2:
                    warning_dialog.lineEditProxy.show()
                else:
                    warning_dialog.lineEditProxy.hide()

            warning_dialog.checkBoxAutoCheck.setChecked(autocheck)
            warning_dialog.checkBoxDownloadMacroMetadata.setChecked(download_macros)
            warning_dialog.comboBoxProxy.setCurrentIndex(proxy_option)
            toggle_proxy_list(proxy_option)
            if proxy_option == 2:
                warning_dialog.lineEditProxy.setText(proxy_string)

            warning_dialog.comboBoxProxy.currentIndexChanged.connect(toggle_proxy_list)

            warning_dialog.labelWarning.setStyleSheet(
                f"color:{utils.warning_color_string()};font-weight:bold;"
            )

            if warning_dialog.exec() == QtWidgets.QDialog.Accepted:
                readWarning = True
                pref.SetBool("readWarning2022", True)
                pref.SetBool("AutoCheck", warning_dialog.checkBoxAutoCheck.isChecked())
                pref.SetBool(
                    "DownloadMacros",
                    warning_dialog.checkBoxDownloadMacroMetadata.isChecked(),
                )
                if warning_dialog.checkBoxDownloadMacroMetadata.isChecked():
                    self.trigger_recache = True
                selected_proxy_option = warning_dialog.comboBoxProxy.currentIndex()
                if selected_proxy_option == 0:
                    pref.SetBool("NoProxyCheck", True)
                    pref.SetBool("SystemProxyCheck", False)
                    pref.SetBool("UserProxyCheck", False)
                elif selected_proxy_option == 1:
                    pref.SetBool("NoProxyCheck", False)
                    pref.SetBool("SystemProxyCheck", True)
                    pref.SetBool("UserProxyCheck", False)
                else:
                    pref.SetBool("NoProxyCheck", False)
                    pref.SetBool("SystemProxyCheck", False)
                    pref.SetBool("UserProxyCheck", True)
                    pref.SetString("ProxyUrl", warning_dialog.lineEditProxy.text())

        if readWarning:
            # Check the connection in a new thread, so FreeCAD stays responsive
            self.connection_checker = ConnectionChecker()
            self.connection_checker.success.connect(self.launch)
            self.connection_checker.failure.connect(self.network_connection_failed)
            self.connection_checker.start()

            # If it takes longer than a half second to check the connection, show a message:
            self.connection_message_timer = QtCore.QTimer.singleShot(
                500, self.show_connection_check_message
            )

    def show_connection_check_message(self):
        if not self.connection_checker.isFinished():
            self.connection_check_message = QtWidgets.QMessageBox(
                QtWidgets.QMessageBox.Information,
                translate("AddonsInstaller", "Checking connection"),
                translate("AddonsInstaller", "Checking for connection to GitHub..."),
                QtWidgets.QMessageBox.Cancel,
            )
            self.connection_check_message.buttonClicked.connect(
                self.cancel_network_check
            )
            self.connection_check_message.show()

    def cancel_network_check(self, button):
        if not self.connection_checker.isFinished():
            self.connection_checker.success.disconnect(self.launch)
            self.connection_checker.failure.disconnect(self.network_connection_failed)
            self.connection_checker.requestInterruption()
            self.connection_checker.wait(500)
            self.connection_check_message.close()

    def network_connection_failed(self, message: str) -> None:
        # This must run on the main GUI thread
        if hasattr(self, "connection_check_message") and self.connection_check_message:
            self.connection_check_message.close()
        if HAVE_QTNETWORK:
            QtWidgets.QMessageBox.critical(
                None, translate("AddonsInstaller", "Connection failed"), message
            )
        else:
            QtWidgets.QMessageBox.critical(
                None,
                translate("AddonsInstaller", "Missing dependency"),
                translate(
                    "AddonsInstaller",
                    "Could not import QtNetwork -- see Report View for details. Addon Manager unavailable.",
                ),
            )

    def launch(self) -> None:
        """Shows the Addon Manager UI"""

        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManager.ui")
        )
        # self.dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        # cleanup the leftovers from previous runs
        self.macro_repo_dir = FreeCAD.getUserMacroDir(True)
        self.packages_with_updates = set()
        self.startup_sequence = []
        self.cleanup_workers()
        self.determine_cache_update_status()

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
        self.dialog.layout().replaceWidget(
            self.dialog.contentPlaceholder, self.packageList
        )
        self.packageList.show()

        # Package details start out hidden
        self.packageDetails = PackageDetails(self.dialog)
        self.packageDetails.hide()
        index = self.dialog.layout().indexOf(self.packageList)
        self.dialog.layout().insertWidget(index, self.packageDetails)

        # set nice icons to everything, by theme with fallback to FreeCAD icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
        self.dialog.buttonUpdateAll.setIcon(QtGui.QIcon(":/icons/button_valid.svg"))
        self.dialog.buttonCheckForUpdates.setIcon(
            QtGui.QIcon(":/icons/view-refresh.svg")
        )
        self.dialog.buttonClose.setIcon(
            QtGui.QIcon.fromTheme("close", QtGui.QIcon(":/icons/process-stop.svg"))
        )
        self.dialog.buttonPauseUpdate.setIcon(
            QtGui.QIcon.fromTheme(
                "pause", QtGui.QIcon(":/icons/media-playback-stop.svg")
            )
        )

        # enable/disable stuff
        self.dialog.buttonUpdateAll.setEnabled(False)
        self.hide_progress_widgets()
        self.dialog.buttonUpdateCache.setEnabled(False)
        self.dialog.buttonUpdateCache.setText(
            translate("AddonsInstaller", "Starting up...")
        )

        # connect slots
        self.dialog.rejected.connect(self.reject)
        self.dialog.buttonUpdateAll.clicked.connect(self.update_all)
        self.dialog.buttonClose.clicked.connect(self.dialog.reject)
        self.dialog.buttonUpdateCache.clicked.connect(self.on_buttonUpdateCache_clicked)
        self.dialog.buttonPauseUpdate.clicked.connect(self.stop_update)
        self.dialog.buttonCheckForUpdates.clicked.connect(
            lambda: self.force_check_updates(standalone=True)
        )
        self.packageList.itemSelected.connect(self.table_row_activated)
        self.packageList.setEnabled(False)
        self.packageDetails.execute.connect(self.executemacro)
        self.packageDetails.install.connect(self.resolve_dependencies)
        self.packageDetails.uninstall.connect(self.remove)
        self.packageDetails.update.connect(self.update)
        self.packageDetails.back.connect(self.on_buttonBack_clicked)
        self.packageDetails.update_status.connect(self.status_updated)

        # center the dialog over the FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.dialog.rect().center()
        )

        # set info for the progress bar:
        self.dialog.progressBar.setMaximum(1000)

        # begin populating the table in a set of sub-threads
        self.startup()

        # set the label text to start with
        self.show_information(translate("AddonsInstaller", "Loading addon information"))

        if hasattr(self, "connection_check_message") and self.connection_check_message:
            self.connection_check_message.close()

        # rock 'n roll!!!
        self.dialog.exec_()

    def cleanup_workers(self, wait=False) -> None:
        """Ensure that no workers are running by explicitly asking them to stop and waiting for them until they do"""
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
                                    "Worker process {} is taking a long time to stop...\n",
                                ).format(worker)
                            )

    def determine_cache_update_status(self) -> None:
        """Determine whether we need to update the cache, based on user preference, and previous
        cache update status. Sets self.update_cache to either True or False."""

        # Figure out our cache update frequency: there is a combo box in the preferences dialog with three
        # options: never, daily, and weekly. Check that first, but allow it to be overridden by a more specific
        # DaysBetweenUpdates selection, if the user has provided it. For that parameter we use:
        # -1: Only manual updates (default)
        #  0: Update every launch
        # >0: Update every n days
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.update_cache = False
        if hasattr(self, "trigger_recache") and self.trigger_recache:
            self.update_cache = True
        update_frequency = pref.GetInt("UpdateFrequencyComboEntry", 0)
        if update_frequency == 0:
            days_between_updates = -1
        elif update_frequency == 1:
            days_between_updates = 1
        elif update_frequency == 2:
            days_between_updates = 7
        days_between_updates = pref.GetInt("DaysBetweenUpdates", days_between_updates)
        last_cache_update_string = pref.GetString("LastCacheUpdate", "never")
        cache_path = FreeCAD.getUserCachePath()
        am_path = os.path.join(cache_path, "AddonManager")
        if last_cache_update_string == "never":
            self.update_cache = True
        elif days_between_updates > 0:
            if hasattr(date, "fromisoformat"):
                last_cache_update = date.fromisoformat(last_cache_update_string)
            else:
                # Python 3.6 and earlier don't have date.fromisoformat
                date_re = re.compile(
                    "([0-9]{4})-?(1[0-2]|0[1-9])-?(3[01]|0[1-9]|[12][0-9])"
                )
                matches = date_re.match(last_cache_update_string)
                last_cache_update = date(
                    int(matches.group(1)), int(matches.group(2)), int(matches.group(3))
                )
            delta_update = timedelta(days=days_between_updates)
            if date.today() >= last_cache_update + delta_update:
                self.update_cache = True
        elif days_between_updates == 0:
            self.update_cache = True
        elif not os.path.isdir(am_path):
            self.update_cache = True
        stopfile = self.get_cache_file_name("CACHE_UPDATE_INTERRUPTED")
        if os.path.exists(stopfile):
            self.update_cache = True
            os.remove(stopfile)
            FreeCAD.Console.PrintMessage(
                translate(
                    "AddonsInstaller",
                    "Previous cache process was interrupted, restarting...\n",
                )
            )

        # See if the user has changed the custom repos list since our last re-cache:
        stored_hash = pref.GetString("CustomRepoHash", "")
        custom_repos = pref.GetString("CustomRepositories", "")
        if custom_repos:
            hasher = hashlib.sha1()
            hasher.update(custom_repos.encode("utf-8"))
            new_hash = hasher.hexdigest()
        else:
            new_hash = ""
        if new_hash != stored_hash:
            stored_hash = pref.SetString("CustomRepoHash", new_hash)
            self.update_cache = True
            FreeCAD.Console.PrintMessage(
                translate(
                    "AddonsInstaller",
                    "Custom repo list changed, forcing recache...\n",
                )
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
                "Not writing the cache because a process was forcibly terminated and the state is unknown.\n"
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

        This proceeds in four stages: first, the main GitHub repository is queried for a list of possible
        addons. Each addon is specified as a git submodule with name and branch information. The actual specific
        commit ID of the submodule (as listed on Github) is ignored. Any extra repositories specified by the
        user are appended to this list.

        Second, the list of macros is downloaded from the FreeCAD/FreeCAD-macros repository and the wiki

        Third, each of these items is queried for a package.xml metadata file. If that file exists it is
        downloaded, cached, and any icons that it references are also downloaded and cached.

        Finally, for workbenches that are not contained within a package (e.g. they provide no metadata), an
        additional git query is made to see if an update is available. Macros are checked for file changes.

        Each of these stages is launched in a separate thread to ensure that the UI remains responsive, and
        the operation can be cancelled.

        Each stage is also subject to caching, so may return immediately, if no cache update has been requested.

        """

        # Each function in this list is expected to launch a thread and connect its completion signal
        # to self.do_next_startup_phase, or to shortcut to calling self.do_next_startup_phase if it
        # is not launching a worker
        self.startup_sequence = [
            self.populate_packages_table,
            self.activate_table_widgets,
            self.populate_macros,
            self.update_metadata_cache,
            self.check_updates,
        ]
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        if pref.GetBool("DownloadMacros", False):
            self.startup_sequence.append(self.load_macro_metadata)
        selection = pref.GetString("SelectedAddon", "")
        if selection:
            self.startup_sequence.insert(2, lambda: self.select_addon(selection))
            pref.SetString("SelectedAddon", "")
        if ADDON_MANAGER_DEVELOPER_MODE:
            self.startup_sequence.append(self.validate)
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

    def get_cache_file_name(self, file: str) -> str:
        cache_path = FreeCAD.getUserCachePath()
        am_path = os.path.join(cache_path, "AddonManager")
        os.makedirs(am_path, exist_ok=True)
        return os.path.join(am_path, file)

    def populate_packages_table(self) -> None:
        self.item_model.clear()

        use_cache = not self.update_cache
        if use_cache:
            if os.path.isfile(self.get_cache_file_name("package_cache.json")):
                with open(self.get_cache_file_name("package_cache.json"), "r") as f:
                    data = f.read()
                    try:
                        from_json = json.loads(data)
                        if len(from_json) == 0:
                            use_cache = False
                    except Exception as e:
                        use_cache = False
            else:
                use_cache = False

        if not use_cache:
            self.update_cache = True  # Make sure to trigger the other cache updates, if the json file was missing
            self.update_worker = UpdateWorker()
            self.update_worker.status_message.connect(self.show_information)
            self.update_worker.addon_repo.connect(self.add_addon_repo)
            self.update_progress_bar(10, 100)
            self.update_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 2
            self.update_worker.start()
        else:
            self.update_worker = LoadPackagesFromCacheWorker(
                self.get_cache_file_name("package_cache.json")
            )
            self.update_worker.addon_repo.connect(self.add_addon_repo)
            self.update_progress_bar(10, 100)
            self.update_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 2
            self.update_worker.start()

    def cache_package(self, repo: Addon):
        if not hasattr(self, "package_cache"):
            self.package_cache = {}
        self.package_cache[repo.name] = repo.to_cache()

    def write_package_cache(self):
        if hasattr(self, "package_cache"):
            package_cache_path = self.get_cache_file_name("package_cache.json")
            with open(package_cache_path, "w") as f:
                f.write(json.dumps(self.package_cache, indent="  "))

    def activate_table_widgets(self) -> None:
        self.packageList.setEnabled(True)
        self.packageList.ui.lineEditFilter.setFocus()
        self.do_next_startup_phase()

    def populate_macros(self) -> None:
        macro_cache_file = self.get_cache_file_name("macro_cache.json")
        cache_is_bad = True
        if os.path.isfile(macro_cache_file):
            size = os.path.getsize(macro_cache_file)
            if size > 1000:  # Make sure there is actually data in there
                cache_is_bad = False
        if self.update_cache or cache_is_bad:
            self.update_cache = True
            self.macro_worker = FillMacroListWorker(self.get_cache_file_name("Macros"))
            self.macro_worker.status_message_signal.connect(self.show_information)
            self.macro_worker.progress_made.connect(self.update_progress_bar)
            self.macro_worker.add_macro_signal.connect(self.add_addon_repo)
            self.macro_worker.finished.connect(self.do_next_startup_phase)
            self.macro_worker.start()
        else:
            self.macro_worker = LoadMacrosFromCacheWorker(
                self.get_cache_file_name("macro_cache.json")
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
        macro_cache_path = self.get_cache_file_name("macro_cache.json")
        with open(macro_cache_path, "w") as f:
            f.write(json.dumps(self.macro_cache, indent="  "))
            self.macro_cache = []

    def update_metadata_cache(self) -> None:
        if self.update_cache:
            self.update_metadata_cache_worker = UpdateMetadataCacheWorker(
                self.item_model.repos
            )
            self.update_metadata_cache_worker.status_message.connect(
                self.show_information
            )
            self.update_metadata_cache_worker.finished.connect(
                self.do_next_startup_phase
            )  # Link to step 4
            self.update_metadata_cache_worker.progress_made.connect(
                self.update_progress_bar
            )
            self.update_metadata_cache_worker.package_updated.connect(
                self.on_package_updated
            )
            self.update_metadata_cache_worker.start()
        else:
            self.do_next_startup_phase()

    def on_buttonUpdateCache_clicked(self) -> None:
        self.update_cache = True
        cache_path = FreeCAD.getUserCachePath()
        am_path = os.path.join(cache_path, "AddonManager")
        try:
            shutil.rmtree(am_path, onerror=self.remove_readonly)
        except Exception:
            pass
        self.dialog.buttonUpdateCache.setEnabled(False)
        self.dialog.buttonUpdateCache.setText(
            translate("AddonsInstaller", "Updating cache...")
        )
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
            self.load_macro_metadata_worker = CacheMacroCode(self.item_model.repos)
            self.load_macro_metadata_worker.status_message.connect(
                self.show_information
            )
            self.load_macro_metadata_worker.update_macro.connect(
                self.on_package_updated
            )
            self.load_macro_metadata_worker.progress_made.connect(
                self.update_progress_bar
            )
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
                translate(
                    "AddonsInstaller", "Could not find addon '{}' to select\n"
                ).format(name)
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

        self.dialog.buttonUpdateAll.setText(
            translate("AddonsInstaller", "Checking for updates...")
        )
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
            s = translate(
                "AddonsInstaller", "Apply {} update(s)", "", number_of_updates
            )
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

    def add_addon_repo(self, addon_repo: Addon) -> None:
        """adds a workbench to the list"""

        if addon_repo.icon is None or addon_repo.icon.isNull():
            addon_repo.icon = self.get_icon(addon_repo)
        for repo in self.item_model.repos:
            if repo.name == addon_repo.name:
                #self.item_model.reload_item(repo) # If we want to have later additions supersede earlier
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
                    path = os.path.join(
                        os.path.dirname(repo.macro.src_filename), repo.macro.icon
                    )
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

    # @dataclass(frozen)
    class MissingDependencies:
        """Encapsulates a group of four types of dependencies:
        * Internal workbenches -> wbs
        * External addons -> external_addons
        * Required Python packages -> python_required
        * Optional Python packages -> python_optional
        """

        def __init__(self, repo: Addon, all_repos: List[Addon]):

            deps = Addon.Dependencies()
            repo_name_dict = dict()
            for r in all_repos:
                repo_name_dict[repo.name] = r
                repo_name_dict[repo.display_name] = r
            repo.walk_dependency_tree(repo_name_dict, deps)

            self.external_addons = []
            for dep in deps.required_external_addons:
                if dep.status() == Addon.Status.NOT_INSTALLED:
                    self.external_addons.append(dep)

            # Now check the loaded addons to see if we are missing an internal workbench:
            wbs = [wb.lower() for wb in FreeCADGui.listWorkbenches()]

            self.wbs = []
            for dep in deps.internal_workbenches:
                if dep.lower() + "workbench" not in wbs:
                    if dep.lower() == "plot":
                        # Special case for plot, which is no longer a full workbench:
                        try:
                            __import__("Plot")
                        except ImportError:
                            # Plot might fail for a number of reasons
                            self.wbs.append(dep)
                            FreeCAD.Console.PrintLog("Failed to import Plot module")
                    else:
                        self.wbs.append(dep)

            # Check the Python dependencies:
            self.python_required = []
            for py_dep in deps.python_required:
                if py_dep not in self.python_required:
                    try:
                        __import__(py_dep)
                    except ImportError:
                        self.python_required.append(py_dep)

            self.python_optional = []
            for py_dep in deps.python_optional:
                try:
                    __import__(py_dep)
                except ImportError:
                    self.python_optional.append(py_dep)

            self.wbs.sort()
            self.external_addons.sort()
            self.python_required.sort()
            self.python_optional.sort()
            self.python_optional = [
                option
                for option in self.python_optional
                if option not in self.python_required
            ]

    def update_allowed_packages_list(self) -> None:
        FreeCAD.Console.PrintLog("Attempting to fetch remote copy of ALLOWED_PYTHON_PACKAGES.txt...\n")
        p = NetworkManager.AM_NETWORK_MANAGER.blocking_get(
            "https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/ALLOWED_PYTHON_PACKAGES.txt"
        )
        if p:
            FreeCAD.Console.PrintLog("Remote ALLOWED_PYTHON_PACKAGES.txt file located, overriding locally-installed copy\n")
            p = p.data().decode("utf8")
            lines = p.split("\n")
            self.allowed_packages.clear() # Unset the locally-defined list
            for line in lines:
                if line and len(line) > 0 and line[0] != "#":
                    self.allowed_packages.add(line.strip())
        else:
            FreeCAD.Console.PrintLog("Could not fetch remote ALLOWED_PYTHON_PACKAGES.txt, using local copy\n")

    def handle_disallowed_python(self, python_required: List[str]) -> bool:
        """Determine if we are missing any required Python packages that are not in the allowed
        packages list. If so, display a message to the user, and return True. Otherwise return
        False."""

        bad_packages = []
        self.update_allowed_packages_list()
        for dep in python_required:
            if dep not in self.allowed_packages:
                bad_packages.append(dep)

        for dep in bad_packages:
            python_required.remove(dep)

        if bad_packages:
            message = (
                "<p>"
                + translate(
                    "AddonsInstaller",
                    "This addon requires Python packages that are not installed, and cannot be installed automatically. To use this workbench you must install the following Python packages manually:",
                )
                + "</p><ul>"
            )
            if len(bad_packages) < 15:
                for dep in bad_packages:
                    message += f"<li>{dep}</li>"
            else:
                message += (
                    "<li>("
                    + translate("AddonsInstaller", "Too many to list")
                    + ")</li>"
                )
            message += "</ul>"
            message += "To ignore this error and install anyway, press OK."
            r = QtWidgets.QMessageBox.critical(
                self.dialog,
                translate("AddonsInstaller", "Missing Requirement"),
                message,
                QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel,
            )
            FreeCAD.Console.PrintMessage(
                translate(
                    "AddonsInstaller",
                    "The following Python packages are allowed to be automatically installed",
                )
                + ":\n"
            )
            for package in self.allowed_packages:
                FreeCAD.Console.PrintMessage(f"  * {package}\n")

            if r == QtWidgets.QMessageBox.Ok:
                # Force the installation to proceed
                return False
            else:
                return True
        else:
            return False

    def report_missing_workbenches(self, addon_name: str, wbs) -> bool:
        if len(wbs) == 1:
            name = wbs[0]
            message = translate(
                "AddonsInstaller",
                "Addon '{}' requires '{}', which is not available in your copy of FreeCAD.",
            ).format(addon_name, name)
        else:
            message = (
                "<p>"
                + translate(
                    "AddonsInstaller",
                    "Addon '{}' requires the following workbenches, which are not available in your copy of FreeCAD:",
                ).format(addon_name)
                + "</p><ul>"
            )
            for wb in wbs:
                message += "<li>" + wb + "</li>"
            message += "</ul>"
            message += translate("AddonsInstaller", "Press OK to install anyway.")
        r = QtWidgets.QMessageBox.critical(
            self.dialog,
            translate("AddonsInstaller", "Missing Requirement"),
            message,
            QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel,
        )
        if r == QtWidgets.QMessageBox.Ok:
            return True
        else:
            return False

    def display_dep_resolution_dialog(self, missing, repo: Addon) -> None:
        self.dependency_dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "dependency_resolution_dialog.ui")
        )
        self.dependency_dialog.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        for addon in missing.external_addons:
            self.dependency_dialog.listWidgetAddons.addItem(addon)
        for mod in missing.python_required:
            self.dependency_dialog.listWidgetPythonRequired.addItem(mod)
        for mod in missing.python_optional:
            item = QtWidgets.QListWidgetItem(mod)
            item.setFlags(item.flags() | QtCore.Qt.ItemIsUserCheckable)
            item.setCheckState(QtCore.Qt.Unchecked)
            self.dependency_dialog.listWidgetPythonOptional.addItem(item)

        self.dependency_dialog.buttonBox.button(
            QtWidgets.QDialogButtonBox.Yes
        ).clicked.connect(lambda: self.dependency_dialog_yes_clicked(repo))
        self.dependency_dialog.buttonBox.button(
            QtWidgets.QDialogButtonBox.Ignore
        ).clicked.connect(lambda: self.dependency_dialog_ignore_clicked(repo))
        self.dependency_dialog.buttonBox.button(
            QtWidgets.QDialogButtonBox.Cancel
        ).setDefault(True)
        self.dependency_dialog.exec()

    def resolve_dependencies(self, repo: Addon) -> None:
        if not repo:
            return

        missing = CommandAddonManager.MissingDependencies(repo, self.item_model.repos)
        if self.handle_disallowed_python(missing.python_required):
            return

        good_packages = []
        for dep in missing.python_optional:
            if dep in self.allowed_packages:
                good_packages.append(dep)
            else:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        "Optional dependency on {} ignored because it is not in the allow-list\n",
                    ).format(dep)
                )
        missing.python_optional = good_packages

        if missing.wbs:
            r = self.report_missing_workbenches(repo.display_name, missing.wbs)
            if r == False:
                return
        if (
            missing.external_addons
            or missing.python_required
            or missing.python_optional
        ):
            # Recoverable: ask the user if they want to install the missing deps
            self.display_dep_resolution_dialog(missing, repo)
        else:
            # No missing deps, just install
            self.install(repo)

    def dependency_dialog_yes_clicked(self, repo: Addon) -> None:
        # Get the lists out of the dialog:
        addons = []
        for row in range(self.dependency_dialog.listWidgetAddons.count()):
            item = self.dependency_dialog.listWidgetAddons.item(row)
            name = item.text()
            for repo in self.item_model.repos:
                if repo.name == name or repo.display_name == name:
                    addons.append(repo)

        python_required = []
        for row in range(self.dependency_dialog.listWidgetPythonRequired.count()):
            item = self.dependency_dialog.listWidgetPythonRequired.item(row)
            python_required.append(item.text())

        python_optional = []
        for row in range(self.dependency_dialog.listWidgetPythonOptional.count()):
            item = self.dependency_dialog.listWidgetPythonOptional.item(row)
            if item.checkState() == QtCore.Qt.Checked:
                python_optional.append(item.text())

        self.dependency_installation_worker = DependencyInstallationWorker(
            addons, python_required, python_optional
        )
        self.dependency_installation_worker.no_python_exe.connect(
            lambda: self.no_python_exe(repo)
        )
        self.dependency_installation_worker.no_pip.connect(
            lambda command: self.no_pip(command, repo)
        )
        self.dependency_installation_worker.failure.connect(
            self.dependency_installation_failure
        )
        self.dependency_installation_worker.success.connect(lambda: self.install(repo))
        self.dependency_installation_dialog = QtWidgets.QMessageBox(
            QtWidgets.QMessageBox.Information,
            translate("AddonsInstaller", "Installing dependencies"),
            translate("AddonsInstaller", "Installing dependencies") + "...",
            QtWidgets.QMessageBox.Cancel,
            self.dialog,
        )
        self.dependency_installation_dialog.rejected.connect(
            self.cancel_dependency_installation
        )
        self.dependency_installation_dialog.show()
        self.dependency_installation_worker.start()

    def no_python_exe(self, repo: Addon) -> None:
        if hasattr(self, "dependency_installation_dialog"):
            self.dependency_installation_dialog.hide()
        result = QtWidgets.QMessageBox.critical(
            self.dialog,
            translate("AddonsInstaller", "Cannot execute Python"),
            translate(
                "AddonsInstaller",
                "Failed to automatically locate your Python executable, or the path is set incorrectly. Please check the Addon Manager preferences setting for the path to Python.",
            )
            + "\n\n"
            + translate(
                "AddonsInstaller",
                "Dependencies could not be installed. Continue with installation of {} anyway?",
            ).format(repo.name),
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
        )
        if result == QtWidgets.QMessageBox.Yes:
            self.install(repo)

    def no_pip(self, command: str, repo: Addon) -> None:
        if hasattr(self, "dependency_installation_dialog"):
            self.dependency_installation_dialog.hide()
        result = QtWidgets.QMessageBox.critical(
            self.dialog,
            translate("AddonsInstaller", "Cannot execute pip"),
            translate(
                "AddonsInstaller",
                "Failed to execute pip, which may be missing from your Python installation. Please ensure your system has pip installed and try again. The failed command was: ",
            )
            + f"\n\n{command}\n\n"
            + translate(
                "AddonsInstaller",
                "Continue with installation of {} anyway?",
            ).format(repo.name),
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
        )
        if result == QtWidgets.QMessageBox.Yes:
            self.install(repo)

    def dependency_installation_failure(self, short_message: str, details: str) -> None:
        if hasattr(self, "dependency_installation_dialog"):
            self.dependency_installation_dialog.hide()
        FreeCAD.Console.PrintError(details)
        QtWidgets.QMessageBox.critical(
            self.dialog,
            translate("AddonsInstaller", "Package installation failed"),
            short_message
            + "\n\n"
            + translate("AddonsInstaller", "See Report View for detailed failure log."),
            QtWidgets.QMessageBox.Cancel,
        )

    def dependency_dialog_ignore_clicked(self, repo: Addon) -> None:
        self.install(repo)

    def cancel_dependency_installation(self) -> None:
        self.dependency_installation_worker.blockSignals(True)
        self.dependency_installation_worker.requestInterruption()
        self.dependency_installation_dialog.hide()

    def install(self, repo: Addon) -> None:
        """installs or updates a workbench, macro, or package"""

        if hasattr(self, "install_worker") and self.install_worker:
            if self.install_worker.isRunning():
                return

        if hasattr(self, "dependency_installation_dialog"):
            self.dependency_installation_dialog.hide()

        if not repo:
            return

        if (
            repo.repo_type == Addon.Kind.WORKBENCH
            or repo.repo_type == Addon.Kind.PACKAGE
        ):
            self.show_progress_widgets()
            self.install_worker = InstallWorkbenchWorker(repo)
            self.install_worker.status_message.connect(self.show_information)
            self.current_progress_region = 1
            self.number_of_progress_regions = 1
            self.install_worker.progress_made.connect(self.update_progress_bar)
            self.install_worker.success.connect(self.on_package_installed)
            self.install_worker.failure.connect(self.on_installation_failed)
            self.install_worker.start()
        elif repo.repo_type == Addon.Kind.MACRO:
            macro = repo.macro

            # To try to ensure atomicity, test the installation into a temp directory first,
            # and assume if that worked we have good odds of the real installation working
            failed = False
            errors = []
            with tempfile.TemporaryDirectory() as dir:
                temp_install_succeeded, error_list = macro.install(dir)
                if not temp_install_succeeded:
                    failed = True
                    errors = error_list

            if not failed:
                real_install_succeeded, errors = macro.install(self.macro_repo_dir)
                if not real_install_succeeded:
                    failed = True
                else:
                    utils.update_macro_installation_details(repo)

            if not failed:
                message = translate(
                    "AddonsInstaller",
                    "Macro successfully installed. The macro is now available from the Macros dialog.",
                )
                self.on_package_installed(repo, message)
            else:
                message = (
                    translate("AddonsInstaller", "Installation of macro failed") + ":"
                )
                for error in errors:
                    message += "\n  * "
                    message += error
                self.on_installation_failed(repo, message)

    def update(self, repo: Addon) -> None:
        self.install(repo)

    def mark_repo_update_available(self, repo: Addon, available: bool) -> None:
        if available:
            repo.set_status(Addon.Status.UPDATE_AVAILABLE)
        else:
            repo.set_status(Addon.Status.NO_UPDATE_AVAILABLE)
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)

    def update_all(self) -> None:
        """Asynchronously apply all available updates: individual failures are noted, but do not stop other updates"""

        if hasattr(self, "update_all_worker") and self.update_all_worker:
            if self.update_all_worker.isRunning():
                return

        self.subupdates_succeeded = []
        self.subupdates_failed = []

        self.show_progress_widgets()
        self.current_progress_region = 1
        self.number_of_progress_regions = 1
        self.update_all_worker = UpdateAllWorker(self.packages_with_updates)
        self.update_all_worker.progress_made.connect(self.update_progress_bar)
        self.update_all_worker.status_message.connect(self.show_information)
        self.update_all_worker.success.connect(
            lambda repo: self.subupdates_succeeded.append(repo)
        )
        self.update_all_worker.failure.connect(
            lambda repo: self.subupdates_failed.append(repo)
        )
        self.update_all_worker.finished.connect(self.on_update_all_completed)
        self.update_all_worker.start()

    def on_update_all_completed(self) -> None:
        self.hide_progress_widgets()

        def get_package_list(message: str, repos: List[Addon], threshold: int):
            """To ensure that the list doesn't get too long for the dialog, cut it off at some threshold"""
            num_updates = len(repos)
            if num_updates < threshold:
                result = "".join([repo.name + "\n" for repo in repos])
            else:
                result = translate(
                    "AddonsInstaller",
                    "{} total, see Report view for list",
                    "Describes the number of updates that were completed ('{}' is replaced by the number of updates)",
                ).format(num_updates)
                for repo in repos:
                    FreeCAD.Console.PrintMessage(f"{message}: {repo.name}\n")
            return result

        if not self.subupdates_failed:
            message = (
                translate(
                    "AddonsInstaller",
                    "All packages were successfully updated",
                )
                + ": \n"
            )
            message += get_package_list(
                translate("AddonsInstaller", "Succeeded"), self.subupdates_succeeded, 15
            )
        elif not self.subupdates_succeeded:
            message = (
                translate("AddonsInstaller", "All packages updates failed:") + "\n"
            )
            message += get_package_list(
                translate("AddonsInstaller", "Failed"), self.subupdates_failed, 15
            )
        else:
            message = (
                translate(
                    "AddonsInstaller",
                    "Some packages updates failed.",
                )
                + "\n\n"
                + translate(
                    "AddonsInstaller",
                    "Succeeded",
                )
                + ":\n"
            )
            message += get_package_list(
                translate("AddonsInstaller", "Succeeded"), self.subupdates_succeeded, 8
            )
            message += "\n\n"
            message += translate("AddonsInstaller", "Failed") + ":\n"
            message += get_package_list(
                translate("AddonsInstaller", "Failed"), self.subupdates_failed, 8
            )

        for installed_repo in self.subupdates_succeeded:
            if installed_repo.contains_workbench():
                self.restart_required = True
                installed_repo.set_status(Addon.Status.PENDING_RESTART)
            else:
                installed_repo.set_status(Addon.Status.NO_UPDATE_AVAILABLE)
            self.item_model.reload_item(installed_repo)
            for requested_repo in self.packages_with_updates:
                if installed_repo.name == requested_repo.name:
                    self.packages_with_updates.remove(installed_repo)
                    break
        self.enable_updates(len(self.packages_with_updates))
        QtWidgets.QMessageBox.information(
            self.dialog,
            translate("AddonsInstaller", "Update report"),
            message,
            QtWidgets.QMessageBox.Close,
        )

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
        self.dialog.buttonUpdateCache.setText(
            translate("AddonsInstaller", "Refresh local cache")
        )

    def write_cache_stopfile(self) -> None:
        stopfile = self.get_cache_file_name("CACHE_UPDATE_INTERRUPTED")
        with open(stopfile, "w", encoding="utf8") as f:
            f.write(
                "This file indicates that a cache operation was interrupted, and "
                "the cache is in an unknown state. It will be deleted next time "
                "AddonManager recaches."
            )

    def on_package_installed(self, repo: Addon, message: str) -> None:
        self.hide_progress_widgets()
        QtWidgets.QMessageBox.information(
            self.dialog,
            translate("AddonsInstaller", "Installation succeeded"),
            message,
            QtWidgets.QMessageBox.Close,
        )
        if repo.contains_workbench():
            repo.set_status(Addon.Status.PENDING_RESTART)
            self.restart_required = True
        else:
            repo.set_status(Addon.Status.NO_UPDATE_AVAILABLE)
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)
        if repo.repo_type == Addon.Kind.MACRO:
            ask_to_install_toolbar_button(repo)
        if repo in self.packages_with_updates:
            self.packages_with_updates.remove(repo)
            self.enable_updates(len(self.packages_with_updates))

    def on_installation_failed(self, _: Addon, message: str) -> None:
        self.hide_progress_widgets()
        QtWidgets.QMessageBox.warning(
            self.dialog,
            translate("AddonsInstaller", "Installation failed"),
            message,
            QtWidgets.QMessageBox.Close,
        )

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
                    self.on_installation_failed(repo, message)
                    return
                else:
                    macro_path = os.path.join(dir, macro.filename)
                    FreeCADGui.open(str(macro_path))
                    self.dialog.hide()
                    FreeCADGui.SendMsgToActiveView("Run")

    def remove_readonly(self, func, path, _) -> None:
        """Remove a read-only file."""

        os.chmod(path, stat.S_IWRITE)
        func(path)

    def remove(self, repo: Addon) -> None:
        """uninstalls a macro or workbench"""

        confirm = QtWidgets.QMessageBox.question(
            self.dialog,
            translate("AddonsInstaller", "Confirm remove"),
            translate(
                "AddonsInstaller", "Are you sure you want to uninstall this Addon?"
            ),
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.Cancel,
        )
        if confirm == QtWidgets.QMessageBox.Cancel:
            return

        if (
            repo.repo_type == Addon.Kind.WORKBENCH
            or repo.repo_type == Addon.Kind.PACKAGE
        ):
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            clonedir = moddir + os.sep + repo.name

            # First remove any macros that were copied or symlinked in, as long as they have not been modified
            macro_dir = FreeCAD.getUserMacroDir(True)
            if os.path.exists(macro_dir) and os.path.exists(clonedir):
                for macro_filename in os.listdir(clonedir):
                    if macro_filename.lower().endswith(".fcmacro"):
                        mod_macro_path = os.path.join(clonedir, macro_filename)
                        macro_path = os.path.join(macro_dir, macro_filename)

                        if not os.path.isfile(macro_path):
                            continue

                        # Load both files (one may be a symlink of the other, this will still work in that case)
                        with open(mod_macro_path) as f1:
                            f1_contents = f1.read()
                        with open(macro_path) as f2:
                            f2_contents = f2.read()

                        if f1_contents == f2_contents:
                            os.remove(macro_path)
                        else:
                            FreeCAD.Console.PrintMessage(
                                translate(
                                    "AddonsInstaller",
                                    "Macro {} has local changes in the macros directory, so is not being removed by this uninstall process.\n",
                                ).format(macro_filename)
                            )

            # Second, run the Addon's "uninstall.py" script, if it exists
            uninstall_script = os.path.join(clonedir, "uninstall.py")
            if os.path.exists(uninstall_script):
                try:
                    with open(uninstall_script, "r") as f:
                        exec(f.read())
                except Exception:
                    FreeCAD.Console.PrintError(
                        translate(
                            "AddonsInstaller",
                            "Execution of Addon's uninstall.py script failed. Proceeding with uninstall...",
                        )
                        + "\n"
                    )

            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.item_model.update_item_status(
                    repo.name, Addon.Status.NOT_INSTALLED
                )
                if repo.contains_workbench():
                    self.restart_required = True
                self.packageDetails.show_repo(repo)
            else:
                self.dialog.textBrowserReadMe.setText(
                    translate(
                        "AddonsInstaller",
                        "Unable to remove this addon with the Addon Manager.",
                    )
                )

        elif repo.repo_type == Addon.Kind.MACRO:
            macro = repo.macro
            if macro.remove():
                remove_custom_toolbar_button(repo)
                FreeCAD.Console.PrintMessage(
                    translate("AddonsInstaller", "Successfully uninstalled {}").format(
                        repo.name
                    )
                    + "\n"
                )
                self.item_model.update_item_status(
                    repo.name, Addon.Status.NOT_INSTALLED
                )
                self.packageDetails.show_repo(repo)
            else:
                FreeCAD.Console.PrintMessage(
                    translate(
                        "AddonsInstaller",
                        "Failed to uninstall {}. Please remove manually.",
                    ).format(repo.name)
                    + "\n"
                )

    def validate(self):
        """Developer tool: check all repos for validity and print report"""

        FreeCAD.Console.PrintLog(f"\n\nADDON MANAGER DEVELOPER MODE CHECKS\n")
        FreeCAD.Console.PrintLog(f"-----------------------------------\n")

        counter = 0
        for addon in self.item_model.repos:
            counter += 1
            self.update_progress_bar(counter, len(self.item_model.repos))
            if addon.metadata is not None:
                self.validate_package_xml(addon)
            elif addon.repo_type == Addon.Kind.MACRO:
                if addon.macro.parsed:
                    if len(addon.macro.icon) == 0 and len(addon.macro.xpm) == 0:
                        FreeCAD.Console.PrintLog(f"Macro '{addon.name}' does not have an icon\n")
            else:
                FreeCAD.Console.PrintLog(f"Addon '{addon.name}' does not have a package.xml file\n")
        
        FreeCAD.Console.PrintLog(f"-----------------------------------\n\n")
        self.do_next_startup_phase()

    def validate_package_xml(self, addon:Addon):
        if addon.metadata is None:
            return

        # The package.xml standard has some required elements that the basic XML reader is not actually checking
        # for. In developer mode, actually make sure that all of the rules are being followed for each element.

        errors = []

        # Top-level required elements

        if not addon.metadata.Name or len(addon.metadata.Name) == 0:
            errors.append(f"No top-level <name> element found, or <name> element is empty")
        if not addon.metadata.Version or addon.metadata.Version == "0.0.0":
            errors.append(f"No top-level <version> element found, or <version> element is invalid")
        #if not addon.metadata.Date or len(addon.metadata.Date) == 0:
        #    errors.append(f"No top-level <date> element found, or <date> element is invalid")
        if not addon.metadata.Description or len(addon.metadata.Description) == 0:
            errors.append(f"No top-level <description> element found, or <description> element is invalid")

        maintainers = addon.metadata.Maintainer
        if len(maintainers) == 0:
            errors.append(f"No top-level <maintainers> found, at least one is required")
        for maintainer in maintainers:
            if len(maintainer['email']) == 0:
                errors.append(f"No email address specified for maintainer '{maintainer['name']}'")

        licenses = addon.metadata.License
        if len(licenses) == 0:
            errors.append(f"No top-level <license> found, at least one is required")

        urls = addon.metadata.Urls
        if len(urls) == 0:
            errors.append(f"No <url> elements found, at least a repo url must be provided")
        else:
            found_repo = False
            found_readme = False
            for url in urls:
                if url["type"] == "repository":
                    found_repo = True
                    if len(url["branch"]) == 0:
                        errors.append("<repository> element is missing the 'branch' attribute")
                elif url["type"] == "readme":
                    found_readme = True
                    location = url["location"]
                    p = NetworkManager.AM_NETWORK_MANAGER.blocking_get(location)
                    if not p:
                        errors.append(f"Could not access specified readme at {location}")
                    else:
                        p = p.data().decode("utf8")
                        if "<html" in p or "<!DOCTYPE html>" in p:
                            pass
                        else:
                            errors.append(f"Readme data found at {location} does not appear to be rendered HTML")
            if not found_repo:
                errors.append("No repo url specified")
            if not found_readme:
                errors.append("No readme url specified (not required, but highly recommended)")

        contents = addon.metadata.Content
        if not contents or len(contents) == 0:
            errors.append("No content items found")

        missing_icon = True
        if addon.metadata.Icon and len(addon.metadata.Icon) > 0:
            missing_icon = False
        else:
            if "workbench" in contents:
                wb = contents["workbench"][0]
                if wb.Icon:
                   missing_icon = False
        if missing_icon:
            errors.append(f"No <icon> element found, or <icon> element is invalid")

        if "workbench" in contents:
            for wb in contents["workbench"]:
                errors.extend (self.validate_workbench_metadata(wb))

        if "preferencepack" in contents:
            for wb in contents["preferencepack"]:
                errors.extend (self.validate_preference_pack_metadata(wb))

        if len(errors) > 0:
            FreeCAD.Console.PrintLog(f"Errors found in package.xml file for '{addon.name}'\n")
            for error in errors:
                FreeCAD.Console.PrintLog(f"   * {error}\n")

    def validate_workbench_metadata(self, workbench) -> List[str]:
        errors = []
        if not workbench.Classname or len(workbench.Classname) == 0:
            errors.append("No <classname> specified for workbench")
        return errors

    def validate_preference_pack_metadata(self, pack) -> List[str]:
        errors = []
        if not pack.Name or len(pack.Name) == 0:
            errors.append("No <name> specified for preference pack")
        return errors

# @}
