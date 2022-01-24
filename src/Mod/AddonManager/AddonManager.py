#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
from datetime import date, timedelta
from typing import Dict

from PySide2 import QtGui, QtCore, QtWidgets
import FreeCADGui

from addonmanager_utilities import translate  # this needs to be as is for pylupdate
from addonmanager_workers import *
import addonmanager_utilities as utils
import AddonManager_rc
from package_list import PackageList, PackageListItemModel
from package_details import PackageDetails
from AddonManagerRepo import AddonManagerRepo

from NetworkManager import HAVE_QTNETWORK

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
        "update_check_single_worker",
        "dependency_installation_worker",
    ]

    lock = threading.Lock()
    restart_required = False

    def __init__(self):
        FreeCADGui.addPreferencePage(
            os.path.join(os.path.dirname(__file__), "AddonManagerOptions.ui"),
            "Addon Manager",
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

        # display first use dialog if needed
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        readWarning = pref.GetBool("readWarning2022", False)

        if not readWarning:
            warning_dialog = FreeCADGui.PySideUic.loadUi(
                os.path.join(os.path.dirname(__file__), "first_run.ui")
            )
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
                translate("AddonsInstaller", "Could not import QtNetwork -- see Report View for details. Addon Manager unavailable."),
            )

    def launch(self) -> None:
        """Shows the Addon Manager UI"""

        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(
            os.path.join(os.path.dirname(__file__), "AddonManager.ui")
        )
        self.dialog.setWindowFlag(QtCore.Qt.Tool)

        # cleanup the leftovers from previous runs
        self.macro_repo_dir = FreeCAD.getUserMacroDir(True)
        self.packages_with_updates = []
        self.startup_sequence = []
        self.addon_removed = False
        self.cleanup_workers()

        # restore window geometry from stored state
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        w = pref.GetInt("WindowWidth", 800)
        h = pref.GetInt("WindowHeight", 600)
        self.dialog.resize(w, h)

        # figure out our cache update frequency: there is a combo box in the preferences dialog with three
        # options: never, daily, and weekly. Check that first, but allow it to be overridden by a more specific
        # DaysBetweenUpdates selection, if the user has provided it. For that parameter we use:
        # -1: Only manual updates (default)
        #  0: Update every launch
        # >0: Update every n days
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
        self.dialog.buttonCheckForUpdates.clicked.connect(
            self.manually_check_for_updates
        )
        self.dialog.buttonClose.clicked.connect(self.dialog.reject)
        self.dialog.buttonUpdateCache.clicked.connect(self.on_buttonUpdateCache_clicked)
        self.dialog.buttonPauseUpdate.clicked.connect(self.stop_update)
        self.packageList.itemSelected.connect(self.table_row_activated)
        self.packageList.setEnabled(False)
        self.packageDetails.execute.connect(self.executemacro)
        self.packageDetails.install.connect(self.resolve_dependencies)
        self.packageDetails.uninstall.connect(self.remove)
        self.packageDetails.update.connect(self.update)
        self.packageDetails.back.connect(self.on_buttonBack_clicked)
        self.packageDetails.update_status.connect(self.status_updated)
        self.packageDetails.check_for_update.connect(self.check_for_update)

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
                        finished = thread.wait(QtCore.QDeadlineTimer(500))
                        if not finished:
                            FreeCAD.Console.PrintWarning(
                                translate(
                                    "AddonsInstaller",
                                    f"Worker process {worker} is taking a long time to stop...\n",
                                )
                            )

    def wait_on_other_workers(self) -> None:
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.wait()

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
                if repo.repo_type == AddonManagerRepo.RepoType.MACRO:
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

    def cache_package(self, repo: AddonManagerRepo):
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

    def cache_macro(self, repo: AddonManagerRepo):
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

    def on_package_updated(self, repo: AddonManagerRepo) -> None:
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
            if hasattr(self, "check_worker"):
                thread = self.check_worker
                if thread:
                    if not thread.isFinished():
                        self.do_next_startup_phase()
                        return
            self.dialog.buttonUpdateAll.setText(
                translate("AddonsInstaller", "Checking for updates...")
            )
            self.check_worker = CheckWorkbenchesForUpdatesWorker(self.item_model.repos)
            self.check_worker.finished.connect(self.do_next_startup_phase)
            self.check_worker.progress_made.connect(self.update_progress_bar)
            self.check_worker.update_status.connect(self.status_updated)
            self.check_worker.start()
            self.enable_updates(len(self.packages_with_updates))
        else:
            self.do_next_startup_phase()

    def status_updated(self, repo: AddonManagerRepo) -> None:
        self.item_model.reload_item(repo)
        if repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.packages_with_updates.append(repo)
            self.enable_updates(len(self.packages_with_updates))

    def enable_updates(self, number_of_updates: int) -> None:
        """enables the update button"""

        if number_of_updates:
            self.dialog.buttonUpdateAll.setText(
                translate("AddonsInstaller", "Apply")
                + " "
                + str(number_of_updates)
                + " "
                + translate("AddonsInstaller", "update(s)")
            )
            self.dialog.buttonUpdateAll.setEnabled(True)
        else:
            self.dialog.buttonUpdateAll.setText(
                translate("AddonsInstaller", "No updates available")
            )
            self.dialog.buttonUpdateAll.setEnabled(False)

    def add_addon_repo(self, addon_repo: AddonManagerRepo) -> None:
        """adds a workbench to the list"""

        if addon_repo.icon is None or addon_repo.icon.isNull():
            addon_repo.icon = self.get_icon(addon_repo)
        for repo in self.item_model.repos:
            if repo.name == addon_repo.name:
                FreeCAD.Console.PrintLog(
                    f"Possible duplicate addon: ignoring second addition of {addon_repo.name}\n"
                )
                return
        self.item_model.append_item(addon_repo)

    def get_icon(self, repo: AddonManagerRepo, update: bool = False) -> QtGui.QIcon:
        """returns an icon for a repo"""

        if not update and repo.icon and not repo.icon.isNull() and repo.icon.isValid():
            return repo.icon

        path = ":/icons/" + repo.name.replace(" ", "_")
        if repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            path += "_workbench_icon.svg"
            default_icon = QtGui.QIcon(":/icons/document-package.svg")
        elif repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            path += "_macro_icon.svg"
            default_icon = QtGui.QIcon(":/icons/document-python.svg")
        elif repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
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

    def table_row_activated(self, selected_repo: AddonManagerRepo) -> None:
        """a row was activated, show the relevant data"""

        self.packageList.hide()
        self.packageDetails.show()
        self.packageDetails.show_repo(selected_repo)

    def show_information(self, message: str) -> None:
        """shows generic text in the information pane"""

        self.dialog.labelStatusInfo.setText(message)
        self.dialog.labelStatusInfo.repaint()

    def show_workbench(self, repo: AddonManagerRepo) -> None:
        self.packageList.hide()
        self.packageDetails.show()
        self.packageDetails.show_repo(repo)

    def on_buttonBack_clicked(self) -> None:
        self.packageDetails.hide()
        self.packageList.show()

    def append_to_repos_list(self, repo: AddonManagerRepo) -> None:
        """this function allows threads to update the main list of workbenches"""

        self.item_model.append_item(repo)

    def resolve_dependencies(self, repo: AddonManagerRepo) -> None:
        if not repo:
            return

        deps = AddonManagerRepo.Dependencies()
        repo_name_dict = dict()
        for r in self.item_model.repos:
            repo_name_dict[repo.name] = r
            repo_name_dict[repo.display_name] = r
        repo.walk_dependency_tree(repo_name_dict, deps)

        FreeCAD.Console.PrintLog("The following Workbenches are required:\n")
        for addon in deps.unrecognized_addons:
            FreeCAD.Console.PrintLog(addon + "\n")

        FreeCAD.Console.PrintLog("The following addons are required:\n")
        for addon in deps.required_external_addons:
            FreeCAD.Console.PrintLog(addon + "\n")

        FreeCAD.Console.PrintLog("The following Python modules are required:\n")
        for pyreq in deps.python_required:
            FreeCAD.Console.PrintLog(pyreq + "\n")

        FreeCAD.Console.PrintLog("The following Python modules are optional:\n")
        for pyreq in deps.python_optional:
            FreeCAD.Console.PrintLog(pyreq + "\n")

        missing_external_addons = []
        for dep in deps.required_external_addons:
            if dep.update_status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
                missing_external_addons.append(dep)

        # Now check the loaded addons to see if we are missing an internal workbench:
        wbs = FreeCADGui.listWorkbenches()
        missing_wbs = []
        for dep in deps.unrecognized_addons:
            if dep not in wbs and dep + "Workbench" not in wbs:
                missing_wbs.append(dep)

        # Check the Python dependencies:
        missing_python_requirements = []
        for py_dep in deps.python_required:
            if py_dep not in missing_python_requirements:
                try:
                    __import__(py_dep)
                except ImportError:
                    missing_python_requirements.append(py_dep)

        bad_packages = []
        for dep in missing_python_requirements:
            if dep not in self.allowed_packages:
                bad_packages.append(dep)

        if bad_packages:
            message = translate(
                "AddonsInstaller",
                "The Addon {repo.name} requires Python packages that are not installed, and cannot be installed automatically. To use this workbench you must install the following Python packages manually:",
            )
            if len(bad_packages) < 15:
                for dep in bad_packages:
                    message += f"\n  * {dep}"
            else:
                message += (
                    "\n  * (" + translate("AddonsInstaller", "Too many to list") + ")"
                )
            QtWidgets.QMessageBox.critical(
                None, translate("AddonsInstaller", "Connection failed"), message
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
            return

        missing_python_optionals = []
        for py_dep in deps.python_optional:
            try:
                __import__(py_dep)
            except ImportError:
                missing_python_optionals.append(py_dep)

        # Possible cases
        # 1) Missing required FreeCAD workbenches. Unrecoverable failure, needs a new version of FreeCAD installation.
        # 2) Missing required external AddOn(s). List for the user and ask for permission to install them.
        # 3) Missing required Python modules. List for the user and ask for permission to attempt installation.
        # 4) Missing optional Python modules. User can choose from the list to attempt to install any or all.
        # Option 1 is standalone, and simply causes failure to install. Other options can be combined and are
        # presented through a dialog box with options.

        addon = repo.display_name if repo.display_name else repo.name
        if missing_wbs:
            if len(missing_wbs) == 1:
                name = missing_wbs[0]
                message = translate(
                    "AddonsInstaller",
                    f"Installing {addon} requires '{name}', which is not installed in your copy of FreeCAD.",
                )
            else:
                message = translate(
                    "AddonsInstaller",
                    f"Installing {addon} requires the following workbenches, which are not installed in your copy of FreeCAD:\n",
                )
                for wb in missing_wbs:
                    message += "  - " + wb + "\n"
            QtWidgets.QMessageBox.critical(
                self.dialog,
                translate("AddonsInstaller", "Missing Requirement"),
                message,
                QtWidgets.QMessageBox.Cancel,
            )
        elif (
            missing_external_addons
            or missing_python_requirements
            or missing_python_optionals
        ):
            self.dependency_dialog = FreeCADGui.PySideUic.loadUi(
                os.path.join(
                    os.path.dirname(__file__), "dependency_resolution_dialog.ui"
                )
            )
            missing_external_addons.sort()
            missing_python_requirements.sort()
            missing_python_optionals.sort()
            missing_python_optionals = [
                option
                for option in missing_python_optionals
                if option not in missing_python_requirements
            ]

            for addon in missing_external_addons:
                self.dependency_dialog.listWidgetAddons.addItem(addon)
            for mod in missing_python_requirements:
                self.dependency_dialog.listWidgetPythonRequired.addItem(mod)
            for mod in missing_python_optionals:
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
        else:
            self.install(repo)

    def dependency_dialog_yes_clicked(self, repo: AddonManagerRepo) -> None:
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

    def no_python_exe(self, repo: AddonManagerRepo) -> None:
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
                f"Dependencies could not be installed. Continue with installation of {repo.name} anyway?",
            ),
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
        )
        if result == QtWidgets.QMessageBox.Yes:
            self.install(repo)

    def no_pip(self, command: str, repo: AddonManagerRepo) -> None:
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
                f"Continue with installation of {repo.name} anyway?",
            ),
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
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
        )

    def dependency_dialog_ignore_clicked(self, repo: AddonManagerRepo) -> None:
        self.install(repo)

    def cancel_dependency_installation(self) -> None:
        self.dependency_installation_worker.blockSignals(True)
        self.dependency_installation_worker.requestInterruption()
        self.dependency_installation_dialog.hide()

    def install(self, repo: AddonManagerRepo) -> None:
        """installs or updates a workbench, macro, or package"""

        if hasattr(self, "install_worker") and self.install_worker:
            if self.install_worker.isRunning():
                return

        if hasattr(self, "dependency_installation_dialog"):
            self.dependency_installation_dialog.hide()

        if not repo:
            return

        if (
            repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH
            or repo.repo_type == AddonManagerRepo.RepoType.PACKAGE
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
        elif repo.repo_type == AddonManagerRepo.RepoType.MACRO:
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

    def update(self, repo: AddonManagerRepo) -> None:
        self.install(repo)

    def check_for_update(self, repo: AddonManagerRepo) -> None:
        """Check a single repo for available updates asynchronously"""

        if (
            hasattr(self, "update_check_single_worker")
            and self.update_check_single_worker
        ):
            if self.update_check_single_worker.isRunning():
                self.update_check_single_worker.blockSignals(True)
                self.update_check_single_worker.requestInterrupt()
                self.update_check_single_worker.wait()

        self.update_check_single_worker = CheckSingleWorker(repo.name)
        self.update_check_single_worker.updateAvailable.connect(
            lambda update_available: self.mark_repo_update_available(
                repo, update_available
            )
        )
        self.update_check_single_worker.start()

    def mark_repo_update_available(
        self, repo: AddonManagerRepo, available: bool
    ) -> None:
        if available:
            repo.update_status = AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
        else:
            repo.update_status = AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)

    def manually_check_for_updates(self) -> None:
        if hasattr(self, "check_worker"):
            thread = self.check_worker
            if thread:
                if not thread.isFinished():
                    self.do_next_startup_phase()
                    return
        self.dialog.buttonCheckForUpdates.setText(
            translate("AddonsInstaller", "Checking for updates...")
        )
        self.dialog.buttonCheckForUpdates.setEnabled(False)
        self.show_progress_widgets()
        self.current_progress_region = 1
        self.number_of_progress_regions = 1
        self.check_worker = CheckWorkbenchesForUpdatesWorker(self.item_model.repos)
        self.check_worker.finished.connect(self.manual_update_check_complete)
        self.check_worker.progress_made.connect(self.update_progress_bar)
        self.check_worker.update_status.connect(self.status_updated)
        self.check_worker.start()

    def manual_update_check_complete(self) -> None:
        self.dialog.buttonUpdateAll.show()
        self.dialog.buttonCheckForUpdates.hide()
        self.enable_updates(len(self.packages_with_updates))
        self.hide_progress_widgets()

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

        def get_package_list(
            message: str, repos: List[AddonManagerRepo], threshold: int
        ):
            """To ensure that the list doesn't get too long for the dialog, cut it off at some threshold"""
            num_updates = len(repos)
            if num_updates < threshold:
                result = "".join([repo.name + "\n" for repo in repos])
            else:
                result = translate(
                    "AddonsInstaller", f"{num_updates} total, see Report view for list"
                )
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
            if not installed_repo.repo_type == AddonManagerRepo.RepoType.MACRO:
                self.restart_required = True
                installed_repo.update_status = (
                    AddonManagerRepo.UpdateStatus.PENDING_RESTART
                )
            else:
                installed_repo.update_status = (
                    AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
                )
            self.item_model.reload_item(installed_repo)
            for requested_repo in self.packages_with_updates:
                if installed_repo.name == requested_repo.name:
                    self.packages_with_updates.remove(installed_repo)
                    break
        self.enable_updates(len(self.packages_with_updates))
        QtWidgets.QMessageBox.information(
            None,
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

        if current_value < 0:
            FreeCAD.Console.PrintWarning(
                f"Addon Manager: Internal error, current progress value is negative in region {self.current_progress_region}"
            )

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

    def on_package_installed(self, repo: AddonManagerRepo, message: str) -> None:
        self.hide_progress_widgets()
        QtWidgets.QMessageBox.information(
            None,
            translate("AddonsInstaller", "Installation succeeded"),
            message,
            QtWidgets.QMessageBox.Close,
        )
        if repo.repo_type != AddonManagerRepo.RepoType.MACRO:
            repo.update_status = AddonManagerRepo.UpdateStatus.PENDING_RESTART
            self.restart_required = True
        else:
            repo.update_status = AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
        self.item_model.reload_item(repo)
        self.packageDetails.show_repo(repo)

    def on_installation_failed(self, _: AddonManagerRepo, message: str) -> None:
        self.hide_progress_widgets()
        QtWidgets.QMessageBox.warning(
            None,
            translate("AddonsInstaller", "Installation failed"),
            message,
            QtWidgets.QMessageBox.Close,
        )

    def executemacro(self, repo: AddonManagerRepo) -> None:
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

    def remove(self, repo: AddonManagerRepo) -> None:
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
            repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH
            or repo.repo_type == AddonManagerRepo.RepoType.PACKAGE
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
                                    f"Macro {macro_filename} has local changes in the macros directory, so is not being removed by this uninstall process.\n",
                                )
                            )

            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.item_model.update_item_status(
                    repo.name, AddonManagerRepo.UpdateStatus.NOT_INSTALLED
                )
                self.addon_removed = (
                    True  # A value to trigger the restart message on dialog close
                )
                self.packageDetails.show_repo(repo)
                self.restart_required = True
            else:
                self.dialog.textBrowserReadMe.setText(
                    translate(
                        "AddonsInstaller",
                        "Unable to remove this addon with the Addon Manager.",
                    )
                )

        elif repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            macro = repo.macro
            if macro.remove():
                self.item_model.update_item_status(
                    repo.name, AddonManagerRepo.UpdateStatus.NOT_INSTALLED
                )
                self.packageDetails.show_repo(repo)
            else:
                self.dialog.textBrowserReadMe.setText(
                    translate("AddonsInstaller", "Macro could not be removed.")
                )


# @}
