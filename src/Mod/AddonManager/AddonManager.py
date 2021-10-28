#!/usr/bin/env python
# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import os
import shutil
import stat
import tempfile
from typing import Dict, Union

from PySide2 import QtGui, QtCore, QtWidgets
import FreeCADGui

from addonmanager_utilities import translate  # this needs to be as is for pylupdate
from addonmanager_workers import *
import addonmanager_utilities as utils
import AddonManager_rc
from AddonManagerRepo import AddonManagerRepo

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

    workers = ["update_worker", "check_worker", "show_worker",
               "showmacro_worker", "macro_worker", "install_worker",
               "update_metadata_cache_worker", "update_all_worker"]

    lock = threading.Lock()

    def GetResources(self) -> Dict[str,str]:
        return {"Pixmap": "AddonManager",
                "MenuText": QT_TRANSLATE_NOOP("Std_AddonMgr", "&Addon manager"),
                "ToolTip": QT_TRANSLATE_NOOP("Std_AddonMgr", "Manage external workbenches, macros, and preference packs"),
                "Group": "Tools"}

    def Activated(self) -> None:

        # display first use dialog if needed
        readWarningParameter = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        readWarning = readWarningParameter.GetBool("readWarning", False)
        newReadWarningParameter = FreeCAD.ParamGet("User parameter:Plugins/addonsRepository")
        readWarning |= newReadWarningParameter.GetBool("readWarning", False)
        if not readWarning:
            if (QtWidgets.QMessageBox.warning(None,
                                          "FreeCAD",
                                          translate("AddonsInstaller",
                                                    "The addons that can be installed here are not "
                                                    "officially part of FreeCAD, and are not reviewed "
                                                    "by the FreeCAD team. Make sure you know what you "
                                                    "are installing!"),
                                          QtWidgets.QMessageBox.Cancel |
                                          QtWidgets.QMessageBox.Ok) !=
                    QtWidgets.QMessageBox.StandardButton.Cancel):
                readWarningParameter.SetBool("readWarning", True)
                readWarning = True

        if readWarning:
            self.launch()

    def launch(self) -> None:
        """Shows the Addon Manager UI"""

        # create the dialog
        self.dialog = FreeCADGui.PySideUic.loadUi(os.path.join(os.path.dirname(__file__),
                                                               "AddonManager.ui"))

        # cleanup the leftovers from previous runs
        self.macro_repo_dir = tempfile.mkdtemp()
        self.packages_with_updates = []
        self.startup_sequence = []
        self.addon_removed = False
        self.cleanup_workers()

        # restore window geometry and splitter state from stored state
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        w = pref.GetInt("WindowWidth", 600)
        h = pref.GetInt("WindowHeight", 480)
        self.dialog.resize(w, h)
        sl = pref.GetInt("SplitterLeft", 298)
        sr = pref.GetInt("SplitterRight", 274)
        self.dialog.splitter.setSizes([sl, sr])

        # Set up the listing of packages using the model-view-controller architecture
        self.item_model = PackageListItemModel()
        self.item_delegate = PackageListIconDelegate()
        self.item_filter = PackageListFilter()
        self.item_filter.setSourceModel(self.item_model)
        self.dialog.tablePackages.setModel (self.item_filter)
        self.dialog.tablePackages.setItemDelegate(self.item_delegate)
        header = self.dialog.tablePackages.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Fixed)
        header.resizeSection(0,20)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        self.dialog.tablePackages.sortByColumn(1, QtCore.Qt.AscendingOrder) # Default to sorting alphabetically by name

        # set nice icons to everything, by theme with fallback to FreeCAD icons
        self.dialog.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
        self.dialog.buttonUninstall.setIcon(QtGui.QIcon.fromTheme("cancel", QtGui.QIcon(":/icons/edit_Cancel.svg")))
        self.dialog.buttonInstall.setIcon(QtGui.QIcon.fromTheme("download", QtGui.QIcon(":/icons/edit_OK.svg")))
        self.dialog.buttonUpdateAll.setIcon(QtGui.QIcon(":/icons/button_valid.svg"))
        self.dialog.buttonConfigure.setIcon(QtGui.QIcon(":/icons/preferences-system.svg"))
        self.dialog.buttonClose.setIcon(QtGui.QIcon.fromTheme("close", QtGui.QIcon(":/icons/process-stop.svg")))

        # enable/disable stuff
        self.dialog.buttonUninstall.setEnabled(False)
        self.dialog.buttonInstall.setEnabled(False)
        self.dialog.buttonUpdateAll.setEnabled(False)
        self.dialog.buttonExecute.hide()
        self.dialog.labelFilterValidity.hide()

        # Hide package-related interface elements until needed
        self.dialog.labelPackageName.hide()
        self.dialog.labelVersion.hide()
        self.dialog.labelMaintainer.hide()
        self.dialog.labelIcon.hide()
        self.dialog.labelDescription.hide()
        self.dialog.labelUrl.hide()
        self.dialog.labelUrlType.hide()
        self.dialog.labelContents.hide()

        # connect slots
        self.dialog.rejected.connect(self.reject)
        self.dialog.buttonInstall.clicked.connect(self.install)
        self.dialog.buttonUninstall.clicked.connect(self.remove)
        self.dialog.buttonUpdateAll.clicked.connect(self.update_all)
        self.dialog.comboPackageType.currentIndexChanged.connect(self.update_type_filter)
        self.dialog.lineEditFilter.textChanged.connect(self.update_text_filter)
        self.dialog.buttonConfigure.clicked.connect(self.show_config)
        self.dialog.buttonClose.clicked.connect(self.dialog.reject)
        self.dialog.buttonExecute.clicked.connect(self.executemacro)
        self.dialog.tablePackages.selectionModel().currentRowChanged.connect(self.table_row_selected)
        self.dialog.tablePackages.setEnabled(False)

        # Show "Workbenches" to start with
        self.dialog.comboPackageType.setCurrentIndex(1) 

        # allow links to open in browser
        self.dialog.description.setOpenLinks(True)
        self.dialog.description.setOpenExternalLinks(True)

        # center the dialog over the FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.dialog.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.dialog.rect().center())

        # set info for the progress bar:
        self.dialog.progressBar.setMaximum (100)

        # begin populating the table in a set of sub-threads
        self.startup()

        # set the label text to start with
        self.show_information(translate("AddonsInstaller", "Loading addon information"))

        # rock 'n roll!!!
        self.dialog.exec_()

    def cleanup_workers(self, wait=False) -> None:
        """ Ensure that no workers are running by explicitly asking them to stop and waiting for them until they do """
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.requestInterruption()
                        thread.wait()

    def wait_on_other_workers(self) -> None:
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.wait()

    def reject(self) -> None:
        """called when the window has been closed"""

        # save window geometry and splitter state for next use
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        pref.SetInt("WindowWidth", self.dialog.width())
        pref.SetInt("WindowHeight", self.dialog.height())
        pref.SetInt("SplitterLeft", self.dialog.splitter.sizes()[0])
        pref.SetInt("SplitterRight", self.dialog.splitter.sizes()[1])

        # ensure all threads are finished before closing
        oktoclose = True
        self.startup_sequence = []
        for worker in self.workers:
            if hasattr(self, worker):
                thread = getattr(self, worker)
                if thread:
                    if not thread.isFinished():
                        thread.requestInterruption()
                        oktoclose = False
        if not oktoclose:
            oktoclose = True
            for worker in self.workers:
                if hasattr(self, worker):
                    thread = getattr(self, worker)
                    if thread:
                        thread.wait()

        # all threads have finished
        if oktoclose:
            if ((hasattr(self, "install_worker") and self.install_worker) or
                    (hasattr(self, "addon_removed") and self.addon_removed)):
                # display restart dialog
                m = QtWidgets.QMessageBox()
                m.setWindowTitle(translate("AddonsInstaller", "Addon manager"))
                m.setWindowIcon(QtGui.QIcon(":/icons/AddonManager.svg"))
                m.setText(translate("AddonsInstaller",
                                    "You must restart FreeCAD for changes to take "
                                    "effect."))
                m.setIcon(m.Warning)
                m.setStandardButtons(m.Ok | m.Cancel)
                m.setDefaultButton(m.Cancel)
                okBtn = m.button(QtWidgets.QMessageBox.StandardButton.Ok)
                cancelBtn = m.button(QtWidgets.QMessageBox.StandardButton.Cancel)
                okBtn.setText(translate("AddonsInstaller","Restart now"))
                cancelBtn.setText(translate("AddonsInstaller","Restart later"))
                ret = m.exec_()
                if ret == m.Ok:
                    shutil.rmtree(self.macro_repo_dir, onerror=self.remove_readonly)
                    # restart FreeCAD after a delay to give time to this dialog to close
                    QtCore.QTimer.singleShot(1000, utils.restart_freecad)
            try:
                shutil.rmtree(self.macro_repo_dir, onerror=self.remove_readonly)
            except Exception:
                pass
        else:
            FreeCAD.Console.PrintWarning("Could not terminate sub-threads in Addon Manager.\n")
            self.cleanup_workers()

    def startup(self) -> None:
        """ Downloads the available packages listings and populates the table

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

        """

        # Each function in this list is expected to launch a thread and connect its completion signal 
        # to self.do_next_startup_phase
        self.startup_sequence = [self.populate_packages_table, 
                                 self.populate_macros, 
                                 self.update_metadata_cache, 
                                 self.check_updates]
        self.current_progress_region = 0
        self.number_of_progress_regions = len(self.startup_sequence)
        self.do_next_startup_phase()

    def do_next_startup_phase(self) -> None:
        """ Pop the top item in self.startup_sequence off the list and run it """

        if (len(self.startup_sequence) > 0):
            phase_runner = self.startup_sequence.pop(0)
            self.current_progress_region += 1
            phase_runner()
        else:
            self.hide_progress_widgets()
            self.dialog.tablePackages.setEnabled(True)
            self.dialog.lineEditFilter.setFocus()

    def populate_packages_table(self) -> None:
        self.item_model.clear()
        self.current_progress_region += 1
        self.update_worker = UpdateWorker()
        self.update_worker.status_message.connect(self.show_information)
        self.update_worker.addon_repo.connect(self.add_addon_repo)
        self.update_progress_bar(10,100)
        self.update_worker.done.connect(self.do_next_startup_phase) # Link to step 2
        self.update_worker.start()

    def populate_macros(self) -> None:
        self.current_progress_region += 1
        self.macro_worker = FillMacroListWorker(self.macro_repo_dir)
        self.macro_worker.status_message_signal.connect(self.show_information)
        self.macro_worker.progress_made.connect(self.update_progress_bar)
        self.macro_worker.add_macro_signal.connect(self.add_addon_repo)
        self.macro_worker.done.connect(self.do_next_startup_phase) # Link to step 3
        self.macro_worker.start()
        
    def update_metadata_cache(self) -> None:
        self.current_progress_region += 1
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        if pref.GetBool("AutoFetchMetadata", True):
            self.update_metadata_cache_worker = UpdateMetadataCacheWorker(self.item_model.repos)
            self.update_metadata_cache_worker.status_message.connect(self.show_information)
            self.update_metadata_cache_worker.done.connect(self.do_next_startup_phase) # Link to step 4
            self.update_metadata_cache_worker.progress_made.connect(self.update_progress_bar)
            self.update_metadata_cache_worker.package_updated.connect(self.on_package_updated)
            self.update_metadata_cache_worker.start()
        else:
            self.do_next_startup_phase()

    def on_package_updated(self, repo:AddonManagerRepo) -> None:
        """Called when the named package has either new metadata or a new icon (or both)"""

        with self.lock:
            cache_path = os.path.join(FreeCAD.getUserAppDataDir(), "AddonManager", "PackageMetadata", repo.name)
            icon_filename = repo.metadata.Icon
            icon_path = os.path.join(cache_path, icon_filename)
            if os.path.isfile(icon_path):
                addonicon = QtGui.QIcon(icon_path)
                repo.icon = addonicon
            self.item_model.reload_item(repo)
            

    def check_updates(self) -> None:
        "checks every installed addon for available updates"
        
        self.current_progress_region += 1
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        autocheck = pref.GetBool("AutoCheck", False)
        if not autocheck:
            self.do_next_startup_phase()
            return
        if not self.packages_with_updates:
            if hasattr(self, "check_worker"):
                thread = self.check_worker
                if thread:
                    if not thread.isFinished():
                        self.do_next_startup_phase()
                        return
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller", "Checking for updates..."))
            self.check_worker = CheckWorkbenchesForUpdatesWorker(self.item_model.repos)
            self.check_worker.done.connect(self.do_next_startup_phase)
            self.check_worker.progress_made.connect(self.update_progress_bar)
            self.check_worker.update_status.connect(self.status_updated)
            self.check_worker.start()
            self.enable_updates(len(self.packages_with_updates))

    def status_updated(self, repo:str, status:AddonManagerRepo.UpdateStatus) -> None:
        self.item_model.update_item_status(repo.name, status)
        if status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.packages_with_updates.append(repo)
            self.enable_updates(len(self.packages_with_updates))

    def enable_updates(self, number_of_updates:int) -> None:
        """enables the update button"""

        if number_of_updates:
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller", "Apply") +
                                                " " + str(number_of_updates) + " " +
                                                translate("AddonsInstaller", "update(s)"))
            self.dialog.buttonUpdateAll.setEnabled(True)
        else:
            self.dialog.buttonUpdateAll.setText(translate("AddonsInstaller", "No updates available"))
            self.dialog.buttonUpdateAll.setEnabled(False)

    def add_addon_repo(self, addon_repo:AddonManagerRepo) -> None:
        """adds a workbench to the list"""
        
        if addon_repo.icon is None or  addon_repo.icon.isNull():
            addon_repo.icon = self.get_icon(addon_repo)
        for repo in self.item_model.repos:
            if repo.name == addon_repo.name:
                FreeCAD.Console.PrintLog(f"Possible duplicate addon: ignoring second addition of {addon_repo.name}\n")
                return
        self.item_model.append_item(addon_repo)

    def get_icon(self, repo:AddonManagerRepo, update:bool=False) -> QtGui.QIcon:
        """returns an icon for a repo"""

        if not update and repo.icon and not repo.icon.isNull():
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
            if repo.cached_icon_filename and os.path.isfile(repo.cached_icon_filename):
                path = repo.cached_icon_filename
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

    def table_row_selected(self, current:QtCore.QModelIndex, previous:QtCore.QModelIndex) -> None:
        """a new row was selected, show the relevant data"""
 
        if not current.isValid():
            self.selected_repo = None
            return
        source_selection = self.item_filter.mapToSource (current)
        self.selected_repo = self.item_model.repos[source_selection.row()]
        self.dialog.description.clear()
        if self.selected_repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            self.show_macro(self.selected_repo)
            self.dialog.buttonExecute.show()
        elif self.selected_repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            self.show_workbench(self.selected_repo)
            self.dialog.buttonExecute.hide()
        elif self.selected_repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            self.show_package(self.selected_repo)
            self.dialog.buttonExecute.hide()
        if self.selected_repo.update_status == AddonManagerRepo.UpdateStatus.NOT_INSTALLED:
            self.dialog.buttonInstall.setEnabled(True)
            self.dialog.buttonUninstall.setEnabled(False)
            self.dialog.buttonInstall.setText(translate("AddonsInstaller", "Install selected"))
        elif self.selected_repo.update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
            self.dialog.buttonInstall.setEnabled(False)
            self.dialog.buttonUninstall.setEnabled(True)
            self.dialog.buttonInstall.setText(translate("AddonsInstaller", "Already updated"))
        elif self.selected_repo.update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
            self.dialog.buttonInstall.setEnabled(True)
            self.dialog.buttonUninstall.setEnabled(True)
            self.dialog.buttonInstall.setText(translate("AddonsInstaller", "Update selected"))
        elif self.selected_repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            self.dialog.buttonInstall.setEnabled(False)
            self.dialog.buttonUninstall.setEnabled(True)
            self.dialog.buttonInstall.setText(translate("AddonsInstaller", "Checking status..."))

    def show_package_widgets(self, show:bool) -> None:
        """ Show or hide the widgets related to packages with a package.xml metadata file """
        
        # Always rebuild the urlGrid, remove all previous items
        for i in reversed(range(self.dialog.urlGrid.rowCount())):
            if self.dialog.urlGrid.itemAtPosition(i,0):
                self.dialog.urlGrid.itemAtPosition(i,0).widget().setParent(None)
            if self.dialog.urlGrid.itemAtPosition(i,1):
                self.dialog.urlGrid.itemAtPosition(i,1).widget().setParent(None)

        if show:
            # Show all the package-related widgets:
            self.dialog.labelPackageName.show()
            self.dialog.labelVersion.show()
            self.dialog.labelMaintainer.show()
            self.dialog.labelIcon.show()
            self.dialog.labelDescription.show()
            self.dialog.labelContents.show()        
        else:
            # Hide all the package-related widgets:
            self.dialog.labelPackageName.hide()
            self.dialog.labelVersion.hide()
            self.dialog.labelMaintainer.hide()
            self.dialog.labelIcon.hide()
            self.dialog.labelDescription.hide()
            self.dialog.labelContents.hide()

    def show_information(self, message:str) -> None:
        """shows generic text in the information pane"""

        self.dialog.labelStatusInfo.show()
        self.dialog.labelStatusInfo.setText(message)

    def show_workbench(self, repo:AddonManagerRepo) -> None:
        """loads information of a given workbench"""

        self.cleanup_workers()
        self.show_package_widgets(False)
        self.show_worker = ShowWorker(repo)
        self.show_worker.status_message.connect(self.show_information)
        self.show_worker.description_updated.connect(lambda desc: self.dialog.description.setText(desc))
        self.show_worker.addon_repos.connect(self.append_to_repos_list)
        self.show_worker.done.connect(lambda : self.dialog.labelStatusInfo.hide())
        self.show_worker.start()

    def show_package(self, repo:AddonManagerRepo) -> None:
        """ Show the details for a package (a repo with a package.xml metadata file) """

        self.cleanup_workers()
        self.show_package_widgets(True)

        # Name
        self.dialog.labelPackageName.setText(f"<h1>{repo.metadata.Name}</h1>")

        # Description
        self.dialog.labelDescription.setText(repo.metadata.Description)

        # Version
        self.dialog.labelVersion.setText(f"<h3>v{repo.metadata.Version}</h3>")

        # Maintainers and authors
        maintainers = ""
        for maintainer in repo.metadata.Maintainer:
            maintainers += translate("AddonsInstaller","Maintainer") + f": {maintainer['name']} <{maintainer['email']}>\n"
        if len(repo.metadata.Author) > 0:
            for author in repo.metadata.Author:
                maintainers += translate("AddonsInstaller","Author") + f": {author['name']} <{author['email']}>\n"
        self.dialog.labelMaintainer.setText(maintainers)

        # Main package icon
        if not repo.icon or repo.icon.isNull():
            icon = self.get_icon(repo, update=True)
            self.item_model.update_item_icon(repo.name, icon)
        self.dialog.labelIcon.setPixmap(repo.icon.pixmap(QtCore.QSize(64,64)))

        # Urls
        urls = repo.metadata.Urls
        ui = FreeCADGui.UiLoader()
        for row, url in enumerate(urls):
            location = url["location"]
            url_type = url["type"]
            url_type_string = translate("AddonsInstaller","Other URL")
            if url_type == "website":
                url_type_string = translate("AddonsInstaller", "Website")
            elif url_type == "repository":
                url_type_string = translate("AddonsInstaller", "Repository")
            elif url_type == "bugtracker":
                url_type_string = translate("AddonsInstaller", "Bug tracker")
            elif url_type == "readme":
                url_type_string = translate("AddonsInstaller", "Readme")
            elif url_type == "documentation":
                url_type_string = translate("AddonsInstaller", "Documentation")
            self.dialog.urlGrid.addWidget(QtWidgets.QLabel(url_type_string), row, 0)
            ui=FreeCADGui.UiLoader()
            url_label=ui.createWidget("Gui::UrlLabel")
            url_label.setText(location)
            url_label.setUrl(location)
            self.dialog.urlGrid.addWidget(url_label, row, 1)

        # Package contents:
        content_string = ""
        for name,item_list in repo.metadata.Content.items():
            if name == "preferencepack":
                content_type = translate("AddonsInstaller","Preference Packs")
            elif name == "workbench":
                content_type = translate("AddonsInstaller","Workbenches")
            elif name == "macro":
                content_type = translate("AddonsInstaller","Macros")
            else:
                content_type = translate("AddonsInstaller","Other content") + ": " + name
            content_string += f"<h2>{content_type}</h2>"
            content_string += "<ul>"
            for item in item_list:
                content_string += f"<li><b>{item.Name}</b> &ndash; {item.Description}</li>"
            content_string += "</ul>"
        self.dialog.description.setText(content_string)

    def show_macro(self, repo:AddonManagerRepo) -> None:
        """loads information of a given macro"""

        self.cleanup_workers()
        self.show_package_widgets(False)
        self.showmacro_worker = GetMacroDetailsWorker(repo)
        self.showmacro_worker.status_message.connect(self.show_information)
        self.showmacro_worker.description_updated.connect(lambda desc: self.dialog.description.setText(desc))
        self.showmacro_worker.done.connect(lambda : self.dialog.labelStatusInfo.hide())
        self.showmacro_worker.start()

    def append_to_repos_list(self, repo:AddonManagerRepo) -> None:
        """this function allows threads to update the main list of workbenches"""

        self.item_model.append_item(repo)

    def install(self) -> None:
        """installs or updates a workbench, macro, or package"""

        if hasattr(self, "install_worker") and self.install_worker:
            if self.install_worker.isRunning():
                return

        if not hasattr(self, "selected_repo"):
            FreeCAD.Console.PrintWarning ("Internal error: no selected repo\n")
            return

        repo = self.selected_repo

        if not repo:
            return

        if repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH or repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
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
            with tempfile.TemporaryDirectory() as dir:
                temp_install_succeeded = macro.install(dir)
                if not temp_install_succeeded:
                    failed = True

            if not failed:
                failed = macro.install(self.macro_repo_dir)

            if not failed:
                message = translate("AddonsInstaller",
                                    "Macro successfully installed. The macro is "
                                    "now available from the Macros dialog.")
                self.on_package_installed (repo, message)
            else:
                message = translate("AddonsInstaller", "Installation of macro failed. See console for failure details.")
                self.on_installation_failed (repo, message)

    def update_all(self) -> None:
        """ Asynchronously apply all available updates: individual failures are noted, but do not stop other updates """

        if hasattr(self, "update_all_worker") and self.update_all_worker:
            if self.update_all_worker.isRunning():
                return

        self.subupdates_succeeded = []
        self.subupdates_failed = []
        
        self.current_progress_region = 1
        self.number_of_progress_regions = 1
        self.update_all_worker = UpdateAllWorker(self.packages_with_updates)
        self.update_all_worker.progress_made.connect(self.update_progress_bar)
        self.update_all_worker.status_message.connect(self.show_information)
        self.update_all_worker.success.connect(lambda repo : self.subupdates_succeeded.append(repo))
        self.update_all_worker.failure.connect(lambda repo : self.subupdates_failed.append(repo))
        self.update_all_worker.done.connect(self.on_update_all_completed)
        self.update_all_worker.start()

    def on_update_all_completed(self) -> None:
        #self.show_progress_bar(False)
        if not self.subupdates_failed:
            message = translate ("AddonsInstaller", "All packages were successfully updated. Packages:") + "\n"
            message += ''.join([repo.name + "\n" for repo in self.subupdates_succeeded])
        elif not self.subupdates_succeeded:
            message = translate ("AddonsInstaller", "All packages updates failed. Packages:") + "\n"
            message += ''.join([repo.name + "\n" for repo in self.subupdates_failed])
        else:
            message = translate ("AddonsInstaller", "Some packages updates failed. Successful packages:") + "\n"
            message += ''.join([repo.name + "\n" for repo in self.subupdates_succeeded])
            message += translate ("AddonsInstaller", "Failed packages:") + "\n"
            message += ''.join([repo.name + "\n" for repo in self.subupdates_failed])

        for installed_repo in self.subupdates_succeeded:
            for requested_repo in self.packages_with_updates:
                if installed_repo.name == requested_repo.name:
                    self.packages_with_updates.remove(installed_repo)
                    break
        self.enable_updates(len(self.packages_with_updates))
        QtWidgets.QMessageBox.information(None,
                                        translate("AddonsInstaller", "Update report"),
                                        message,
                                        QtWidgets.QMessageBox.Close)

    def hide_progress_widgets(self) -> None:
        """ hides the progress bar and related widgets"""

        self.dialog.labelStatusInfo.hide()
        self.dialog.progressBar.hide()
        self.dialog.lineEditFilter.setFocus()

    def update_progress_bar(self, current_value:int, max_value:int) -> None:
        """ Update the progress bar, showing it if it's hidden """

        self.dialog.progressBar.show()
        region_size = 100 / self.number_of_progress_regions
        value = (self.current_progress_region-1)*region_size + (current_value / max_value / self.number_of_progress_regions)*region_size
        self.dialog.progressBar.setValue(value)

    def on_package_installed(self, repo:AddonManagerRepo, message:str) -> None:
        QtWidgets.QMessageBox.information(None,
                                      translate("AddonsInstaller", "Installation succeeded"),
                                      message,
                                      QtWidgets.QMessageBox.Close)
        self.dialog.progressBar.hide()
        self.table_row_selected(self.dialog.tablePackages.selectionModel().selectedIndexes()[0], QtCore.QModelIndex())
        if repo.contains_workbench():
            self.item_model.update_item_status(repo.name, AddonManagerRepo.UpdateStatus.PENDING_RESTART)
        else:
            self.item_model.update_item_status(repo.name, AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE)

    def on_installation_failed(self, _:AddonManagerRepo, message:str) -> None:
        QtWidgets.QMessageBox.warning(None,
                                      translate("AddonsInstaller", "Installation failed"),
                                      message,
                                      QtWidgets.QMessageBox.Close) 
        self.dialog.progressBar.hide()

    def executemacro(self) -> None:
        """executes a selected macro"""

        macro = self.selected_repo.macro
        if not macro or not macro.code:
            return

        if macro.is_installed():
            macro_path = os.path.join(self.macro_repo_dir,macro.filename)
            FreeCADGui.open(str(macro_path))
            self.dialog.hide()
            FreeCADGui.SendMsgToActiveView("Run")        
        else:
            with tempfile.TemporaryDirectory() as dir:
                temp_install_succeeded = macro.install(dir)
                if not temp_install_succeeded:
                    message = translate("AddonsInstaller", "Execution of macro failed. See console for failure details.")
                    self.on_installation_failed (self.selected_repo, message)
                    return
                else:
                    macro_path = os.path.join(dir,macro.filename)
                    FreeCADGui.open(str(macro_path))
                    self.dialog.hide()
                    FreeCADGui.SendMsgToActiveView("Run")        

    def remove_readonly(self, func, path, _) -> None:
        """Remove a read-only file."""

        os.chmod(path, stat.S_IWRITE)
        func(path)

    def remove(self) -> None:
        """uninstalls a macro or workbench"""

        if self.selected_repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH or \
           self.selected_repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
            basedir = FreeCAD.getUserAppDataDir()
            moddir = basedir + os.sep + "Mod"
            clonedir = moddir + os.sep + self.selected_repo.name
            if os.path.exists(clonedir):
                shutil.rmtree(clonedir, onerror=self.remove_readonly)
                self.dialog.description.setText(translate("AddonsInstaller",
                                                          "Addon successfully removed. Please restart FreeCAD."))
                self.item_model.update_item_status(self.selected_repo.name, AddonManagerRepo.UpdateStatus.NOT_INSTALLED)
                self.addon_removed = True  # A value to trigger the restart message on dialog close
            else:
                self.dialog.description.setText(translate("AddonsInstaller", "Unable to remove this addon with the Addon Manager."))

        elif self.selected_repo.repo_type == AddonManagerRepo.RepoType.MACRO:
            macro = self.selected_repo.macro
            if macro.remove():
                self.dialog.description.setText(translate("AddonsInstaller", "Macro successfully removed."))
                self.item_model.update_item_status(self.selected_repo.name, AddonManagerRepo.UpdateStatus.NOT_INSTALLED)
            else:
                self.dialog.description.setText(translate("AddonsInstaller", "Macro could not be removed."))

    def show_config(self) -> None:
        """shows the configuration dialog"""

        self.config = FreeCADGui.PySideUic.loadUi(os.path.join(os.path.dirname(__file__), "AddonManagerOptions.ui"))

        # restore stored values
        pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        self.config.checkUpdates.setChecked(pref.GetBool("AutoCheck", False))
        self.config.customRepositories.setPlainText(pref.GetString("CustomRepositories", ""))
        self.config.radioButtonNoProxy.setChecked(pref.GetBool("NoProxyCheck", True))
        self.config.radioButtonSystemProxy.setChecked(pref.GetBool("SystemProxyCheck", False))
        self.config.radioButtonUserProxy.setChecked(pref.GetBool("UserProxyCheck", False))
        self.config.userProxy.setPlainText(pref.GetString("ProxyUrl", ""))

        # center the dialog over the Addon Manager
        self.config.move(self.dialog.frameGeometry().topLeft() +
                         self.dialog.rect().center() -
                         self.config.rect().center())

        ret = self.config.exec_()

        if ret:
            # OK button has been pressed
            pref.SetBool("AutoCheck", self.config.checkUpdates.isChecked())
            pref.SetString("CustomRepositories", self.config.customRepositories.toPlainText())
            pref.SetBool("NoProxyCheck", self.config.radioButtonNoProxy.isChecked())
            pref.SetBool("SystemProxyCheck", self.config.radioButtonSystemProxy.isChecked())
            pref.SetBool("UserProxyCheck", self.config.radioButtonUserProxy.isChecked())
            pref.SetString("ProxyUrl", self.config.userProxy.toPlainText())

    

    def update_type_filter(self, type_filter:int) -> None:
        """hide/show rows corresponding to the type filter
       
        type_filter is an integer: 0 for all, 1 for workbenches, 2 for macros, and 3 for preference packs
        
        """
        self.item_filter.setPackageFilter(type_filter)
            

    def update_text_filter(self, text_filter:str) -> None:
        """filter name and description by the regex specified by text_filter"""

        if text_filter:
            test_regex = QtCore.QRegularExpression(text_filter)
            if test_regex.isValid():
                self.dialog.labelFilterValidity.setToolTip(translate("AddonsInstaller","Filter is valid"))
                icon = QtGui.QIcon.fromTheme("ok", QtGui.QIcon(":/icons/edit_OK.svg"))
                self.dialog.labelFilterValidity.setPixmap(icon.pixmap(16,16))
            else:
                self.dialog.labelFilterValidity.setToolTip(translate("AddonsInstaller","Filter regular expression is invalid"))
                icon = QtGui.QIcon.fromTheme("cancel", QtGui.QIcon(":/icons/edit_Cancel.svg"))
                self.dialog.labelFilterValidity.setPixmap(icon.pixmap(16,16))
            self.dialog.labelFilterValidity.show()
        else:
            self.dialog.labelFilterValidity.hide()
        self.item_filter.setFilterRegularExpression(text_filter)


class PackageListItemModel(QtCore.QAbstractTableModel):

    repos = []
    write_lock = threading.Lock()

    DataAccessRole = QtCore.Qt.UserRole
    StatusUpdateRole = QtCore.Qt.UserRole + 1
    IconUpdateRole = QtCore.Qt.UserRole + 2

    def __init__(self) -> None:
        QtCore.QAbstractTableModel.__init__(self)

    def rowCount(self, parent:QtCore.QModelIndex=QtCore.QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return len(self.repos)

    def columnCount(self, parent:QtCore.QModelIndex=QtCore.QModelIndex()) -> int:
        if parent.isValid():
            return 0
        return 3 # Icon, Name, Status

    def data(self, index:QtCore.QModelIndex, role:int=QtCore.Qt.DisplayRole) -> Union[QtGui.QIcon,str]:
        if not index.isValid():
            return None
        row = index.row()
        column = index.column()
        if role == QtCore.Qt.DisplayRole:
            if row >= len(self.repos):
                return None
            if column == 1:
                return self.repos[row].name if self.repos[row].metadata is None else self.repos[row].metadata.Name
            elif column == 2:
                if self.repos[row].update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
                    return translate("AddonsInstaller","Installed")
                elif self.repos[row].update_status == AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE:
                    return translate("AddonsInstaller","Up-to-date")
                elif self.repos[row].update_status == AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE:
                    return translate("AddonsInstaller","Update available")
                elif self.repos[row].update_status == AddonManagerRepo.UpdateStatus.PENDING_RESTART:
                    return translate("AddonsInstaller","Restart required")
                else:
                    return None
            else:
                return None
        elif role == QtCore.Qt.DecorationRole:
            if column == 0:
                return self.repos[row].icon
        elif role == QtCore.Qt.ToolTipRole:
            tooltip = ""
            if self.repos[row].repo_type == AddonManagerRepo.RepoType.PACKAGE:
                tooltip = f"Package '{self.repos[row].name}'"
                # TODO add more info from Metadata
            elif self.repos[row].repo_type == AddonManagerRepo.RepoType.WORKBENCH:
                tooltip = f"Workbench '{self.repos[row].name}'"
            elif self.repos[row].repo_type == AddonManagerRepo.RepoType.MACRO:
                tooltip = f"Macro '{self.repos[row].name}'"
            return tooltip
        elif role == QtCore.Qt.TextAlignmentRole:
            return QtCore.Qt.AlignLeft | QtCore.Qt.AlignTop
        elif role == PackageListItemModel.DataAccessRole:
            return self.repos[row]

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                if section == 0:
                    return None
                elif section == 1:
                    return translate("AddonsInstaller", "Name")
                elif section == 2:
                    return translate("AddonsInstaller", "Status")
            else:
                return None

    def setData(self, index:QtCore.QModelIndex, value, role=QtCore.Qt.EditRole) -> None:
        """ Set the data for this row. The column of the index is ignored. """

        row = index.row()
        self.write_lock.acquire()
        if role == PackageListItemModel.StatusUpdateRole:
            self.repos[row].update_status = value
            self.dataChanged.emit(self.index(row,2), self.index(row,2), [PackageListItemModel.StatusUpdateRole])
        elif role == PackageListItemModel.IconUpdateRole:
            self.repos[row].icon = value
            self.dataChanged.emit(self.index(row,0), self.index(row,0), [PackageListItemModel.IconUpdateRole]) 
        self.write_lock.release()

    def append_item(self, repo:AddonManagerRepo) -> None:
        if repo in self.repos:
            # Cowardly refuse to insert the same repo a second time
            return
        self.write_lock.acquire()
        self.beginInsertRows(QtCore.QModelIndex(), self.rowCount(), self.rowCount())
        self.repos.append(repo)
        self.endInsertRows()
        self.write_lock.release()

    def clear(self) -> None:
        if self.rowCount() > 0:
            self.write_lock.acquire()
            self.beginRemoveRows(QtCore.QModelIndex(), 0, self.rowCount()-1)
            self.repos = []
            self.endRemoveRows()
            self.write_lock.release()

    def update_item_status(self, name:str, status:AddonManagerRepo.UpdateStatus) -> None:
        for row,item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row,0), status, PackageListItemModel.StatusUpdateRole)
                return

    def update_item_icon(self, name:str, icon:QtGui.QIcon) -> None:
        for row,item in enumerate(self.repos):
            if item.name == name:
                self.setData(self.index(row,0), icon, PackageListItemModel.IconUpdateRole)
                return

    def reload_item(self,repo:AddonManagerRepo) -> None:
        for index,item in enumerate(self.repos):
            if item.name == repo.name:
                self.write_lock.acquire()
                self.repos[index] = repo
                self.write_lock.release()
                return


class PackageListIconDelegate(QtWidgets.QStyledItemDelegate):
    """ A delegate to ensure proper alignment of the icon in the table cells """

    def paint(self, painter, option, index):
        if (index.column() == 0):
            option.decorationAlignment = QtCore.Qt.AlignTop | QtCore.Qt.AlignHCenter
        super().paint(painter, option, index)
        

class PackageListFilter(QtCore.QSortFilterProxyModel):
    """ Handle filtering the item list on various criteria """

    def __init__(self):
        super().__init__()
        self.package_type = 0 # Default to showing everything
        self.setSortCaseSensitivity(QtCore.Qt.CaseInsensitive)

    def setPackageFilter(self, type:int) -> None: # 0=All, 1=Workbenches, 2=Macros, 3=Preference Packs
        self.package_type = type
        self.invalidateFilter()
        
    def lessThan(self, left, right) -> bool:
        l = self.sourceModel().data(left,PackageListItemModel.DataAccessRole)
        r = self.sourceModel().data(right,PackageListItemModel.DataAccessRole)

        if left.column() == 0: # Icon
            return False
        elif left.column() == 1: # Name
            lname = l.name if l.metadata is None else l.metadata.Name
            rname = r.name if r.metadata is None else r.metadata.Name
            return lname.lower() < rname.lower()
        elif left.column() == 2: # Status
            return l.update_status < r.update_status

    def filterAcceptsRow(self, row, parent=QtCore.QModelIndex()):
        index = self.sourceModel().createIndex(row, 0)
        data = self.sourceModel().data(index,PackageListItemModel.DataAccessRole)
        if self.package_type == 1:
            if not data.contains_workbench():
                return False
        elif self.package_type == 2:
           if  not data.contains_macro():
               return False
        elif self.package_type == 3:
           if not data.contains_preference_pack():
               return False
        
        name = data.name if data.metadata is None else data.metadata.Name
        desc = data.description if not data.metadata else data.metadata.Description
        re = self.filterRegularExpression()
        if re.isValid():
            re.setPatternOptions(QtCore.QRegularExpression.CaseInsensitiveOption)
            if re.match(name).hasMatch():
                return True
            if re.match(desc).hasMatch():
                return True
            return False
        else:
            return False

    def sort(self, column, order):
        if column == 0: # Icons
          return
        else:
          super().sort(column, order)
# @}
