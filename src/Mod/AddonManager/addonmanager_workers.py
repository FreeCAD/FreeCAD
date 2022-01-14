# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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
import re
import shutil
import json
import tempfile
import hashlib
import threading
import queue
import io
import time
import subprocess
import sys
import platform
import itertools
from datetime import datetime
from typing import Union, List, Dict
from enum import Enum, auto


from PySide2 import QtCore

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui

import addonmanager_utilities as utils
from addonmanager_macro import Macro

from AddonManagerRepo import AddonManagerRepo
from NetworkManager import AM_NETWORK_MANAGER

translate = FreeCAD.Qt.translate

have_git = False
try:
    import git

    # some versions of git module have no "Repo" class??  Bug #4072 module
    # 'git' has no attribute 'Repo'
    have_git = hasattr(git, "Repo")
    if not have_git:
        FreeCAD.Console.PrintMessage(
            "'import git' gave strange results (no Repo attribute)..."
        )
except ImportError:
    pass

have_zip = False
try:
    import zipfile

    have_zip = True
except ImportError:
    pass

have_markdown = False
try:
    import markdown

    have_markdown = True
except ImportError:
    pass

#  @package AddonManager_workers
#  \ingroup ADDONMANAGER
#  \brief Multithread workers for the addon manager
#  @{

# reject_listed addons
macros_reject_list = []
mod_reject_list = []

# These addons will print an additional message informing the user
obsolete = []

# These addons will print an additional message informing the user Python2 only
py2only = []

NOGIT = False  # for debugging purposes, set this to True to always use http downloads
NOMARKDOWN = False  # for debugging purposes, set this to True to disable Markdown lib
"""Multithread workers for the Addon Manager"""


class ConnectionChecker(QtCore.QThread):

    success = QtCore.Signal()
    failure = QtCore.Signal(str)

    def __init__(self):
        QtCore.QThread.__init__(self)

    def run(self):
        FreeCAD.Console.PrintLog(
            translate("AddonsInstaller", "Checking network connection...\n")
        )
        url = "https://api.github.com/zen"
        result = AM_NETWORK_MANAGER.blocking_get(url)
        if QtCore.QThread.currentThread().isInterruptionRequested():
            return
        if not result:
            self.failure.emit(
                translate(
                    "AddonsInstaller",
                    "Unable to read data from GitHub: check your internet connection and proxy settings and try again.",
                )
            )
            return

        result = result.data().decode("utf8")
        FreeCAD.Console.PrintLog(f"GitHub's zen message response: {result}\n")
        self.success.emit()


class UpdateWorker(QtCore.QThread):
    """This worker updates the list of available workbenches"""

    status_message = QtCore.Signal(str)
    addon_repo = QtCore.Signal(object)

    def __init__(self):

        QtCore.QThread.__init__(self)

    def run(self):
        "populates the list of addons"

        self.current_thread = QtCore.QThread.currentThread()

        # update info lists
        global obsolete, macros_reject_list, mod_reject_list, py2only
        p = AM_NETWORK_MANAGER.blocking_get(
            "https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/addonflags.json"
        )
        if p:
            p = p.data().decode("utf8")
            j = json.loads(p)
            if "obsolete" in j and "Mod" in j["obsolete"]:
                obsolete = j["obsolete"]["Mod"]

            if "blacklisted" in j and "Macro" in j["blacklisted"]:
                macros_reject_list = j["blacklisted"]["Macro"]

            if "blacklisted" in j and "Mod" in j["blacklisted"]:
                mod_reject_list = j["blacklisted"]["Mod"]

            if "py2only" in j and "Mod" in j["py2only"]:
                py2only = j["py2only"]["Mod"]

            if "deprecated" in j:
                fc_major = int(FreeCAD.Version()[0])
                fc_minor = int(FreeCAD.Version()[1])
                for item in j["deprecated"]:
                    if "as_of" in item and "name" in item:
                        try:
                            version_components = item["as_of"].split(".")
                            major = int(version_components[0])
                            if len(version_components) > 1:
                                minor = int(version_components[1])
                            else:
                                minor = 0
                            if major < fc_major or (
                                major == fc_major and minor <= fc_minor
                            ):
                                if "kind" not in item or item["kind"] == "mod":
                                    obsolete.append(item["name"])
                                elif item["kind"] == "macro":
                                    macros_reject_list.append(item["name"])
                                else:
                                    FreeCAD.Console.PrintMessage(
                                        f'Unrecognized Addon kind {item["kind"]} in deprecation list.'
                                    )
                        except Exception:
                            FreeCAD.Console.PrintMessage(
                                f"Exception caught when parsing deprecated Addon {item['name']}, version {item['as_of']}"
                            )

        else:
            message = translate(
                "AddonsInstaller",
                "Failed to connect to GitHub. Check your connection and proxy settings.",
            )
            FreeCAD.Console.PrintError(message + "\n")
            self.status_message.emit(message)
            return

        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        package_names = []

        # querying custom addons first
        addon_list = (
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
            .GetString("CustomRepositories", "")
            .split("\n")
        )
        custom_addons = []
        for addon in addon_list:
            if " " in addon:
                addon_and_branch = addon.split(" ")
                custom_addons.append(
                    {"url": addon_and_branch[0], "branch": addon_and_branch[1]}
                )
            else:
                custom_addons.append({"url": addon, "branch": "master"})
        for addon in custom_addons:
            if self.current_thread.isInterruptionRequested():
                return
            if addon and addon["url"]:
                if addon["url"][-1] == "/":
                    addon["url"] = addon["url"][0:-1]  # Strip trailing slash
                name = addon["url"].split("/")[-1]
                if name.lower().endswith(".git"):
                    name = name[:-4]
                if name in package_names:
                    # We already have something with this name, skip this one
                    continue
                package_names.append(name)
                addondir = moddir + os.sep + name
                if os.path.exists(addondir) and os.listdir(addondir):
                    state = AddonManagerRepo.UpdateStatus.UNCHECKED
                else:
                    state = AddonManagerRepo.UpdateStatus.NOT_INSTALLED
                repo = AddonManagerRepo(name, addon["url"], state, addon["branch"])
                md_file = os.path.join(addondir, "package.xml")
                if os.path.isfile(md_file):
                    repo.load_metadata_file(md_file)
                    repo.installed_version = repo.metadata.Version
                    repo.updated_timestamp = os.path.getmtime(md_file)
                self.addon_repo.emit(repo)

        # querying official addons
        p = AM_NETWORK_MANAGER.blocking_get(
            "https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/.gitmodules"
        )
        if not p:
            return
        p = p.data().decode("utf8")
        p = re.findall(
            (
                r'(?m)\[submodule\s*"(?P<name>.*)"\]\s*'
                r"path\s*=\s*(?P<path>.+)\s*"
                r"url\s*=\s*(?P<url>https?://.*)\s*"
                r"(branch\s*=\s*(?P<branch>[^\s]*)\s*)?"
            ),
            p,
        )
        for name, _, url, _, branch in p:
            if self.current_thread.isInterruptionRequested():
                return
            if name in package_names:
                # We already have something with this name, skip this one
                continue
            package_names.append(name)
            if branch is None or len(branch) == 0:
                branch = "master"
            url = url.split(".git")[0]
            addondir = moddir + os.sep + name
            if os.path.exists(addondir) and os.listdir(addondir):
                # make sure the folder exists and it contains files!
                state = AddonManagerRepo.UpdateStatus.UNCHECKED
            else:
                state = AddonManagerRepo.UpdateStatus.NOT_INSTALLED
            repo = AddonManagerRepo(name, url, state, branch)
            md_file = os.path.join(addondir, "package.xml")
            if os.path.isfile(md_file):
                repo.load_metadata_file(md_file)
                repo.installed_version = repo.metadata.Version
                repo.updated_timestamp = os.path.getmtime(md_file)
            if name in py2only:
                repo.python2 = True
            if name in mod_reject_list:
                repo.rejected = True
            if name in obsolete:
                repo.obsolete = True
            self.addon_repo.emit(repo)

            self.status_message.emit(
                translate("AddonsInstaller", "Workbenches list was updated.")
            )


class LoadPackagesFromCacheWorker(QtCore.QThread):
    addon_repo = QtCore.Signal(object)

    def __init__(self, cache_file: str):
        QtCore.QThread.__init__(self)
        self.cache_file = cache_file

    def run(self):
        metadata_cache_path = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        with open(self.cache_file, "r") as f:
            data = f.read()
            if data:
                dict_data = json.loads(data)
                for item in dict_data.values():
                    if QtCore.QThread.currentThread().isInterruptionRequested():
                        return
                    repo = AddonManagerRepo.from_cache(item)
                    repo_metadata_cache_path = os.path.join(
                        metadata_cache_path, repo.name, "package.xml"
                    )
                    if os.path.isfile(repo_metadata_cache_path):
                        try:
                            repo.load_metadata_file(repo_metadata_cache_path)
                            repo.installed_version = repo.metadata.Version
                            repo.updated_timestamp = os.path.getmtime(
                                repo_metadata_cache_path
                            )
                        except Exception:
                            FreeCAD.Console.PrintLog(
                                f"Failed loading {repo_metadata_cache_path}\n"
                            )
                            pass
                    self.addon_repo.emit(repo)


class LoadMacrosFromCacheWorker(QtCore.QThread):
    add_macro_signal = QtCore.Signal(object)

    def __init__(self, cache_file: str):
        QtCore.QThread.__init__(self)
        self.cache_file = cache_file

    def run(self):
        with open(self.cache_file, "r") as f:
            data = f.read()
            dict_data = json.loads(data)
            for item in dict_data:
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    return
                new_macro = Macro.from_cache(item)
                repo = AddonManagerRepo.from_macro(new_macro)
                utils.update_macro_installation_details(repo)
                self.add_macro_signal.emit(repo)


class CheckWorkbenchesForUpdatesWorker(QtCore.QThread):
    """This worker checks for available updates for all workbenches"""

    update_status = QtCore.Signal(AddonManagerRepo)
    progress_made = QtCore.Signal(int, int)

    def __init__(self, repos: List[AddonManagerRepo]):

        QtCore.QThread.__init__(self)
        self.repos = repos

    def run(self):

        if NOGIT or not have_git:
            return
        self.current_thread = QtCore.QThread.currentThread()
        self.basedir = FreeCAD.getUserAppDataDir()
        self.moddir = self.basedir + os.sep + "Mod"
        count = 1
        for repo in self.repos:
            if self.current_thread.isInterruptionRequested():
                return
            self.progress_made.emit(count, len(self.repos))
            count += 1
            if repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
                if repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
                    self.check_workbench(repo)
                elif repo.repo_type == AddonManagerRepo.RepoType.MACRO:
                    self.check_macro(repo)
                elif repo.repo_type == AddonManagerRepo.RepoType.PACKAGE:
                    self.check_package(repo)

    def check_workbench(self, wb):
        if not have_git or NOGIT:
            return
        clonedir = self.moddir + os.sep + wb.name
        if os.path.exists(clonedir):
            # mark as already installed AND already checked for updates
            if not os.path.exists(clonedir + os.sep + ".git"):
                utils.repair_git_repo(wb.url, clonedir)
            gitrepo = git.Git(clonedir)
            try:
                gitrepo.fetch()
            except Exception:
                FreeCAD.Console.PrintWarning(
                    "AddonManager: "
                    + translate(
                        "AddonsInstaller",
                        f"Unable to fetch git updates for workbench {wb.name}",
                    )
                )
            else:
                try:
                    if "git pull" in gitrepo.status():
                        wb.update_status = (
                            AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
                        )
                    else:
                        wb.update_status = (
                            AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
                        )
                    self.update_status.emit(wb)
                except Exception:
                    FreeCAD.Console.PrintWarning(
                        translate("AddonsInstaller", "git fetch failed for {wb.name}")
                    )

    def check_package(self, package: AddonManagerRepo) -> None:
        clonedir = self.moddir + os.sep + package.name
        if os.path.exists(clonedir):
            installed_metadata_file = os.path.join(clonedir, "package.xml")
            if not os.path.isfile(installed_metadata_file):
                # If there is no package.xml file, then it's because the package author added it after the last time
                # the local installation was updated. By definition, then, there is an update available, if only to
                # download the new XML file.
                package.update_status = AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
                package.installed_version = None
                self.update_status.emit(package)
                return
            else:
                package.updated_timestamp = os.path.getmtime(installed_metadata_file)
            try:
                installed_metadata = FreeCAD.Metadata(installed_metadata_file)
                package.installed_version = installed_metadata.Version
                # Packages are considered up-to-date if the metadata version matches. Authors should update
                # their version string when they want the addon manager to alert users of a new version.
                if package.metadata.Version != installed_metadata.Version:
                    package.update_status = (
                        AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
                    )
                else:
                    package.update_status = (
                        AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
                    )
                self.update_status.emit(package)
            except Exception:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        f"Failed to read metadata from {installed_metadata_file}",
                    )
                    + "\n"
                )

    def check_macro(self, macro_wrapper: AddonManagerRepo) -> None:
        # Make sure this macro has its code downloaded:
        try:
            if not macro_wrapper.macro.parsed and macro_wrapper.macro.on_git:
                macro_wrapper.macro.fill_details_from_file(
                    macro_wrapper.macro.src_filename
                )
            if not macro_wrapper.macro.parsed and macro_wrapper.macro.on_wiki:
                mac = macro_wrapper.macro.name.replace(" ", "_")
                mac = mac.replace("&", "%26")
                mac = mac.replace("+", "%2B")
                url = "https://wiki.freecad.org/Macro_" + mac
                macro_wrapper.macro.fill_details_from_wiki(url)
        except Exception:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    f"Failed to fetch code for macro '{macro_wrapper.macro.name}'",
                )
                + "\n"
            )
            return

        hasher1 = hashlib.sha1()
        hasher2 = hashlib.sha1()
        hasher1.update(macro_wrapper.macro.code.encode("utf-8"))
        new_sha1 = hasher1.hexdigest()
        test_file_one = os.path.join(
            FreeCAD.getUserMacroDir(True), macro_wrapper.macro.filename
        )
        test_file_two = os.path.join(
            FreeCAD.getUserMacroDir(True), "Macro_" + macro_wrapper.macro.filename
        )
        if os.path.exists(test_file_one):
            with open(test_file_one, "rb") as f:
                contents = f.read()
                hasher2.update(contents)
                old_sha1 = hasher2.hexdigest()
        elif os.path.exists(test_file_two):
            with open(test_file_two, "rb") as f:
                contents = f.read()
                hasher2.update(contents)
                old_sha1 = hasher2.hexdigest()
        else:
            return
        if new_sha1 == old_sha1:
            macro_wrapper.update_status = (
                AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
            )
        else:
            macro_wrapper.update_status = AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
        self.update_status.emit(macro_wrapper)


class FillMacroListWorker(QtCore.QThread):
    """This worker populates the list of macros"""

    add_macro_signal = QtCore.Signal(object)
    status_message_signal = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)

    def __init__(self, repo_dir):

        QtCore.QThread.__init__(self)
        self.repo_dir = repo_dir
        self.repo_names = []

    def run(self):
        """Populates the list of macros"""

        self.current_thread = QtCore.QThread.currentThread()

        if not self.current_thread.isInterruptionRequested():
            self.status_message_signal.emit(
                translate(
                    "AddonInstaller",
                    "Retrieving macros from FreeCAD/FreeCAD-Macros Git repository",
                )
            )
            self.retrieve_macros_from_git()

        if not self.current_thread.isInterruptionRequested():
            self.status_message_signal.emit(
                translate(
                    "AddonInstaller",
                    "Retrieving macros from FreeCAD wiki",
                )
            )
            self.retrieve_macros_from_wiki()

        if self.current_thread.isInterruptionRequested():
            return

        self.status_message_signal.emit(
            translate("AddonsInstaller", "Done locating macros.")
        )

    def retrieve_macros_from_git(self):
        """Retrieve macros from FreeCAD-macros.git

        Emits a signal for each macro in
        https://github.com/FreeCAD/FreeCAD-macros.git
        """

        if not have_git or NOGIT:
            message = translate(
                "AddonsInstaller",
                "Failed to execute Git Python command: check installation of GitPython and/or git",
            )
            self.status_message_signal.emit(message)
            FreeCAD.Console.PrintWarning(message + "\n")
            return

        try:
            if os.path.exists(self.repo_dir):
                if not os.path.exists(os.path.join(self.repo_dir, ".git")):
                    utils.repair_git_repo(
                        "https://github.com/FreeCAD/FreeCAD-macros.git", self.repo_dir
                    )
                gitrepo = git.Git(self.repo_dir)
                gitrepo.pull()
            else:
                git.Repo.clone_from(
                    "https://github.com/FreeCAD/FreeCAD-macros.git", self.repo_dir
                )
        except Exception as e:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller", "An error occurred fetching macros from GitHub"
                )
                + f":\n{e}\n"
            )
        n_files = 0
        for _, _, filenames in os.walk(self.repo_dir):
            n_files += len(filenames)
        counter = 0
        for dirpath, _, filenames in os.walk(self.repo_dir):
            self.progress_made.emit(counter, n_files)
            counter += 1
            if self.current_thread.isInterruptionRequested():
                return
            if ".git" in dirpath:
                continue
            for filename in filenames:
                if self.current_thread.isInterruptionRequested():
                    return
                if filename.lower().endswith(".fcmacro"):
                    macro = Macro(filename[:-8])  # Remove ".FCMacro".
                    macro.on_git = True
                    macro.src_filename = os.path.join(dirpath, filename)
                    repo = AddonManagerRepo.from_macro(macro)
                    repo.url = "https://github.com/FreeCAD/FreeCAD-macros.git"
                    utils.update_macro_installation_details(repo)
                    self.add_macro_signal.emit(repo)

    def retrieve_macros_from_wiki(self):
        """Retrieve macros from the wiki

        Read the wiki and emit a signal for each found macro.
        Reads only the page https://wiki.freecad.org/Macros_recipes
        """

        p = AM_NETWORK_MANAGER.blocking_get("https://wiki.freecad.org/Macros_recipes")
        if not p:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Error connecting to the Wiki, FreeCAD cannot retrieve the Wiki macro list at this time",
                )
                + "\n"
            )
            return
        p = p.data().decode("utf8")
        macros = re.findall('title="(Macro.*?)"', p)
        macros = [mac for mac in macros if ("translated" not in mac)]
        macro_names = []
        for i, mac in enumerate(macros):
            self.progress_made.emit(i, len(macros))
            if self.current_thread.isInterruptionRequested():
                return
            macname = mac[6:]  # Remove "Macro ".
            macname = macname.replace("&amp;", "&")
            if not macname:
                continue
            if (
                (macname not in macros_reject_list)
                and ("recipes" not in macname.lower())
                and (macname not in macro_names)
            ):
                macro_names.append(macname)
                macro = Macro(macname)
                macro.on_wiki = True
                repo = AddonManagerRepo.from_macro(macro)
                repo.url = "https://wiki.freecad.org/Macros_recipes"
                utils.update_macro_installation_details(repo)
                self.add_macro_signal.emit(repo)


class CacheMacroCode(QtCore.QThread):
    """Download and cache the macro code, and parse its internal metadata"""

    status_message = QtCore.Signal(str)
    update_macro = QtCore.Signal(AddonManagerRepo)
    progress_made = QtCore.Signal(int, int)

    def __init__(self, repos: List[AddonManagerRepo]) -> None:
        QtCore.QThread.__init__(self)
        self.repos = repos
        self.workers = []
        self.terminators = []
        self.lock = threading.Lock()
        self.failed = []
        self.counter = 0

    def run(self):
        self.status_message.emit(translate("AddonsInstaller", "Caching macro code..."))

        self.repo_queue = queue.Queue()
        current_thread = QtCore.QThread.currentThread()
        num_macros = 0
        for repo in self.repos:
            if repo.macro is not None:
                self.repo_queue.put(repo)
                num_macros += 1

        # Emulate QNetworkAccessManager and spool up six connections:
        for _ in range(6):
            self.update_and_advance(None)

        while True:
            if current_thread.isInterruptionRequested():
                for worker in self.workers:
                    worker.blockSignals(True)
                    worker.requestInterruption()
                    if not worker.wait(100):
                        FreeCAD.Console.PrintWarning(
                            f"Addon Manager: a worker process failed to halt ({worker.macro.name})"
                        )
                return
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()
            with self.lock:
                if self.counter >= num_macros:
                    break
            time.sleep(0.1)

        # Make sure all of our child threads have fully exited:
        for worker in self.workers:
            worker.wait(50)
            if not worker.isFinished():
                FreeCAD.Console.PrintError(
                    f"Addon Manager: a worker process failed to complete while fetching {worker.macro.name}\n"
                )

        self.repo_queue.join()
        for terminator in self.terminators:
            if terminator and terminator.isActive():
                terminator.stop()

        if len(self.failed) > 0:
            num_failed = len(self.failed)
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    f"Out of {num_macros} macros, {num_failed} timed out while processing",
                )
            )

    def update_and_advance(self, repo: AddonManagerRepo) -> None:
        if repo is not None:
            if repo.macro.name not in self.failed:
                self.update_macro.emit(repo)
            self.repo_queue.task_done()
            with self.lock:
                self.counter += 1

        if QtCore.QThread.currentThread().isInterruptionRequested():
            return

        self.progress_made.emit(
            len(self.repos) - self.repo_queue.qsize(), len(self.repos)
        )

        try:
            next_repo = self.repo_queue.get_nowait()
            worker = GetMacroDetailsWorker(next_repo)
            worker.finished.connect(lambda: self.update_and_advance(next_repo))
            with self.lock:
                self.workers.append(worker)
                self.terminators.append(
                    QtCore.QTimer.singleShot(10000, lambda: self.terminate(worker))
                )
            self.status_message.emit(
                translate(
                    "AddonsInstaller",
                    f"Getting metadata from macro {next_repo.macro.name}",
                )
            )
            worker.start()
        except queue.Empty:
            pass

    def terminate(self, worker) -> None:
        if not worker.isFinished():
            macro_name = worker.macro.name
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    f"Timeout while fetching metadata for macro {macro_name}",
                )
                + "\n"
            )
            worker.blockSignals(True)
            worker.requestInterruption()
            worker.wait(100)
            if worker.isRunning():
                FreeCAD.Console.PrintError(
                    f"Failed to kill process for macro {macro_name}!\n"
                )
            with self.lock:
                self.failed.append(macro_name)


class ShowWorker(QtCore.QThread):
    """This worker retrieves info of a given workbench"""

    status_message = QtCore.Signal(str)
    readme_updated = QtCore.Signal(str)
    update_status = QtCore.Signal(AddonManagerRepo)

    def __init__(self, repo, cache_path):

        QtCore.QThread.__init__(self)
        self.repo = repo
        self.cache_path = cache_path

    def run(self):
        self.status_message.emit(
            translate("AddonsInstaller", "Retrieving description...")
        )
        u = None
        url = self.repo.url
        self.status_message.emit(
            translate("AddonsInstaller", "Retrieving info from") + " " + str(url)
        )
        desc = ""
        regex = utils.get_readme_regex(self.repo)
        if regex:
            # extract readme from html via regex
            readmeurl = utils.get_readme_html_url(self.repo)
            if not readmeurl:
                FreeCAD.Console.PrintLog(f"README not found for {url}\n")
            p = AM_NETWORK_MANAGER.blocking_get(readmeurl)
            if not p:
                FreeCAD.Console.PrintLog(f"Debug: README not found at {readmeurl}\n")
            p = p.data().decode("utf8")
            readme = re.findall(regex, p, flags=re.MULTILINE | re.DOTALL)
            if readme:
                desc = readme[0]
            else:
                FreeCAD.Console.PrintLog(f"Debug: README not found at {readmeurl}\n")
        else:
            # convert raw markdown using lib
            readmeurl = utils.get_readme_url(self.repo)
            if not readmeurl:
                FreeCAD.Console.PrintLog(f"Debug: README not found for {url}\n")
            p = AM_NETWORK_MANAGER.blocking_get(readmeurl)
            if p:
                p = p.data().decode("utf8")
                desc = utils.fix_relative_links(p, readmeurl.rsplit("/README.md")[0])
                if not NOMARKDOWN and have_markdown:
                    desc = markdown.markdown(desc, extensions=["md_in_html"])
                else:
                    message = """
<div style="width: 100%; text-align:center;background: #91bbe0;">
<strong style="color: #FFFFFF;">
"""
                    message += translate("AddonsInstaller", "Raw markdown displayed")
                    message += "</strong><br/><br/>"
                    message += translate(
                        "AddonsInstaller", "Python Markdown library is missing."
                    )
                    message += "<br/></div><hr/><pre>" + desc + "</pre>"
                    desc = message
            else:
                FreeCAD.Console.PrintLog("Debug: README not found at {readmeurl}\n")
                if desc == "":
                    # fall back to the description text
                    p = AM_NETWORK_MANAGER.blocking_get(url)
                    if not p:
                        return
                    p = p.data().decode("utf8")
                    descregex = utils.get_desc_regex(self.repo)
                    if descregex:
                        desc = re.findall(descregex, p)
                        if desc:
                            desc = desc[0]
                if not desc:
                    desc = "Unable to retrieve addon description"
                self.repo.description = desc
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    return
        message = desc
        if self.repo.update_status == AddonManagerRepo.UpdateStatus.UNCHECKED:
            # Addon is installed but we haven't checked it yet, so let's check if it has an update
            upd = False
            # checking for updates
            if not NOGIT and have_git:
                repo = self.repo
                clonedir = (
                    FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + repo.name
                )
                if os.path.exists(clonedir):
                    if not os.path.exists(clonedir + os.sep + ".git"):
                        utils.repair_git_repo(self.repo.url, clonedir)
                    gitrepo = git.Git(clonedir)
                    gitrepo.fetch()
                    if "git pull" in gitrepo.status():
                        upd = True
            if upd:
                self.repo.update_status = AddonManagerRepo.UpdateStatus.UPDATE_AVAILABLE
            else:
                self.repo.update_status = (
                    AddonManagerRepo.UpdateStatus.NO_UPDATE_AVAILABLE
                )
            self.update_status.emit(self.repo)

        if QtCore.QThread.currentThread().isInterruptionRequested():
            return
        self.readme_updated.emit(message)
        self.mustLoadImages = True
        label = self.loadImages(message, self.repo.url, self.repo.name)
        if label:
            self.readme_updated.emit(label)
        if QtCore.QThread.currentThread().isInterruptionRequested():
            return

    def stopImageLoading(self):
        "this stops the image loading process and allow the thread to terminate earlier"

        self.mustLoadImages = False

    def loadImages(self, message, url, wbName):
        "checks if the given page contains images and downloads them"

        # QTextBrowser cannot display online images.  So we download them
        # here, and replace the image link in the html code with the
        # downloaded version

        imagepaths = re.findall('<img.*?src="(.*?)"', message)
        if imagepaths:
            store = os.path.join(self.cache_path, "Images")
            if not os.path.exists(store):
                os.makedirs(store)
            with open(os.path.join(store, "download_in_progress"), "w") as f:
                f.write(
                    "If this file still exists, it's because a download was interrupted. It can be safely ignored."
                )
            for path in imagepaths:
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    return message
                if not self.mustLoadImages:
                    return message
                origpath = path
                if "?" in path:
                    # remove everything after the ?
                    path = path.split("?")[0]
                if not path.startswith("http"):
                    path = utils.getserver(url) + path
                name = path.split("/")[-1]
                if name and path.startswith("http"):
                    storename = os.path.join(store, name)
                    if len(storename) >= 260:
                        remainChars = 259 - (len(store) + len(wbName) + 1)
                        storename = os.path.join(store, wbName + name[-remainChars:])
                    if not os.path.exists(storename):
                        try:
                            imagedata = AM_NETWORK_MANAGER.blocking_get(path)
                            if not imagedata:
                                raise Exception
                        except Exception:
                            FreeCAD.Console.PrintLog(
                                "AddonManager: Debug: Error retrieving image from "
                                + path
                            )
                        else:
                            try:
                                f = open(storename, "wb")
                            except OSError:
                                # ecryptfs (and probably not only ecryptfs) has
                                # lower length limit for path
                                storename = storename[-140:]
                                f = open(storename, "wb")
                            f.write(imagedata.data())
                            f.close()
                    message = message.replace(
                        'src="' + origpath,
                        'src="file:///' + storename.replace("\\", "/"),
                    )
            os.remove(os.path.join(store, "download_in_progress"))
            return message
        return None


class GetMacroDetailsWorker(QtCore.QThread):
    """Retrieve the macro details for a macro"""

    status_message = QtCore.Signal(str)
    readme_updated = QtCore.Signal(str)

    def __init__(self, repo):

        QtCore.QThread.__init__(self)
        self.macro = repo.macro

    def run(self):

        self.status_message.emit(
            translate("AddonsInstaller", "Retrieving macro description...")
        )
        if not self.macro.parsed and self.macro.on_git:
            self.status_message.emit(
                translate("AddonsInstaller", "Retrieving info from git")
            )
            self.macro.fill_details_from_file(self.macro.src_filename)
        if not self.macro.parsed and self.macro.on_wiki:
            self.status_message.emit(
                translate("AddonsInstaller", "Retrieving info from wiki")
            )
            mac = self.macro.name.replace(" ", "_")
            mac = mac.replace("&", "%26")
            mac = mac.replace("+", "%2B")
            url = "https://wiki.freecad.org/Macro_" + mac
            self.macro.fill_details_from_wiki(url)
        message = (
            "<h1>"
            + self.macro.name
            + "</h1>"
            + self.macro.desc
            + '<br/><br/>Macro location: <a href="'
            + self.macro.url
            + '">'
            + self.macro.url
            + "</a>"
        )
        if QtCore.QThread.currentThread().isInterruptionRequested():
            return
        self.readme_updated.emit(message)


class InstallWorkbenchWorker(QtCore.QThread):
    "This worker installs a workbench"

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    success = QtCore.Signal(AddonManagerRepo, str)
    failure = QtCore.Signal(AddonManagerRepo, str)

    def __init__(self, repo: AddonManagerRepo):

        QtCore.QThread.__init__(self)
        self.repo = repo
        self.update_timer = QtCore.QTimer()
        self.update_timer.setInterval(100)
        self.update_timer.timeout.connect(self.update_status)
        self.update_timer.start()

    def run(self):
        "installs or updates the selected addon"

        if not self.repo:
            return

        if not have_git or NOGIT:
            FreeCAD.Console.PrintLog(
                translate(
                    "AddonsInstaller",
                    "GitPython not found. Using ZIP file download instead.",
                )
                + "\n"
            )
            if not have_zip:
                FreeCAD.Console.PrintError(
                    translate(
                        "AddonsInstaller",
                        "Your version of Python doesn't appear to support ZIP files. Unable to proceed.",
                    )
                    + "\n"
                )
                return

        basedir = FreeCAD.getUserAppDataDir()
        moddir = basedir + os.sep + "Mod"
        if not os.path.exists(moddir):
            os.makedirs(moddir)
        target_dir = moddir + os.sep + self.repo.name

        if have_git and not NOGIT:
            # Do the git process...
            self.run_git(target_dir)
        else:

            # The zip process uses an event loop, since the download can potentially be quite large
            self.launch_zip(target_dir)
            self.zip_complete = False
            current_thread = QtCore.QThread.currentThread()
            while not self.zip_complete:
                if current_thread.isInterruptionRequested():
                    return
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

    def update_status(self) -> None:
        if hasattr(self, "git_progress") and self.isRunning():
            self.progress_made.emit(self.git_progress.current, self.git_progress.total)
            self.status_message.emit(self.git_progress.message)

    def run_git(self, clonedir: str) -> None:

        if NOGIT or not have_git:
            FreeCAD.Console.PrintLog(
                translate(
                    "AddonsInstaller",
                    "No Git Python installed, skipping git operations",
                )
                + "\n"
            )
            return

        self.git_progress = GitProgressMonitor()

        if os.path.exists(clonedir):
            self.run_git_update(clonedir)
        else:
            self.run_git_clone(clonedir)

    def run_git_update(self, clonedir: str) -> None:
        self.status_message.emit("Updating module...")
        if str(self.repo.name) in py2only:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "You are installing a Python 2 workbench on a system running Python 3 - ",
                )
                + str(self.repo.name)
                + "\n"
            )
        if not os.path.exists(clonedir + os.sep + ".git"):
            utils.repair_git_repo(self.repo.url, clonedir)
        repo = git.Git(clonedir)
        try:
            repo.pull()  # Refuses to take a progress object?
            answer = translate(
                "AddonsInstaller",
                "Workbench successfully updated. Please restart FreeCAD to apply the changes.",
            )
        except Exception as e:
            answer = (
                translate("AddonsInstaller", "Error updating module ")
                + self.repo.name
                + " - "
                + translate("AddonsInstaller", "Please fix manually")
                + " -- \n"
            )
            answer += str(e)
            self.failure.emit(self.repo, answer)
        else:
            # Update the submodules for this repository
            repo_sms = git.Repo(clonedir)
            self.status_message.emit("Updating submodules...")
            for submodule in repo_sms.submodules:
                submodule.update(init=True, recursive=True)
            self.update_metadata()
            self.success.emit(self.repo, answer)

    def run_git_clone(self, clonedir: str) -> None:
        self.status_message.emit("Checking module dependencies...")
        if str(self.repo.name) in py2only:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "You are installing a Python 2 workbench on a system running Python 3 - ",
                )
                + str(self.repo.name)
                + "\n"
            )
        self.status_message.emit("Cloning module...")
        current_thread = QtCore.QThread.currentThread()

        # NOTE: There is no way to interrupt this process in GitPython: someday we should
        # support pygit2/libgit2 so we can actually interrupt this properly.
        repo = git.Repo.clone_from(self.repo.url, clonedir, progress=self.git_progress)
        if current_thread.isInterruptionRequested():
            return

        # Make sure to clone all the submodules as well
        if repo.submodules:
            repo.submodule_update(recursive=True)

        if current_thread.isInterruptionRequested():
            return

        if self.repo.branch in repo.heads:
            repo.heads[self.repo.branch].checkout()

        answer = translate(
            "AddonsInstaller",
            "Workbench successfully installed. Please restart FreeCAD to apply the changes.",
        )

        if self.repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            # symlink any macro contained in the module to the macros folder
            macro_dir = FreeCAD.getUserMacroDir(True)
            if not os.path.exists(macro_dir):
                os.makedirs(macro_dir)
            if os.path.exists(clonedir):
                for f in os.listdir(clonedir):
                    if f.lower().endswith(".fcmacro"):
                        try:
                            utils.symlink(
                                os.path.join(clonedir, f), os.path.join(macro_dir, f)
                            )
                        except OSError:
                            # If the symlink failed (e.g. for a non-admin user on Windows), copy the macro instead
                            shutil.copy(
                                os.path.join(clonedir, f), os.path.join(macro_dir, f)
                            )
                        FreeCAD.ParamGet(
                            "User parameter:Plugins/" + self.repo.name
                        ).SetString("destination", clonedir)
                        answer += "\n\n" + translate(
                            "AddonsInstaller",
                            "A macro has been installed and is available under Macro -> Macros menu",
                        )
                        answer += ":\n<b>" + f + "</b>"
        self.update_metadata()
        self.success.emit(self.repo, answer)

    def launch_zip(self, zipdir: str) -> None:
        "downloads and unzip a zip version from a git repo"

        bakdir = None
        if os.path.exists(zipdir):
            bakdir = zipdir + ".bak"
            if os.path.exists(bakdir):
                shutil.rmtree(bakdir)
            os.rename(zipdir, bakdir)
        os.makedirs(zipdir)
        zipurl = utils.get_zip_url(self.repo)
        if not zipurl:
            self.failure.emit(
                self.repo,
                translate("AddonsInstaller", "Error: Unable to locate ZIP from")
                + " "
                + self.repo.name,
            )
            return

        self.zipdir = zipdir
        self.bakdir = bakdir

        AM_NETWORK_MANAGER.progress_made.connect(self.update_zip_status)
        AM_NETWORK_MANAGER.progress_complete.connect(self.finish_zip)
        self.zip_download_index = AM_NETWORK_MANAGER.submit_monitored_get(zipurl)

    def update_zip_status(self, index: int, bytes_read: int, data_size: int):
        if index == self.zip_download_index:
            locale = QtCore.QLocale()
            if data_size > 10 * 1024 * 1024: # To avoid overflows, show MB instead
                MB_read = bytes_read / 1024 / 1024
                MB_total = data_size / 1024 / 1024
                self.progress_made.emit(MB_read, MB_total)
                mbytes_str = locale.toString(MB_read)
                mbytes_total_str = locale.toString(MB_total)
                percent = int(100 * float(MB_read / MB_total))
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        f"Downloading: {mbytes_str}MB of {mbytes_total_str}MB ({percent}%)",
                    )
                )
            elif data_size > 0:
                self.progress_made.emit(bytes_read, data_size)
                bytes_str = locale.toString(bytes_read)
                bytes_total_str = locale.toString(data_size)
                percent = int(100 * float(bytes_read / data_size))
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        f"Downloading: {bytes_str} of {bytes_total_str} bytes ({percent}%)",
                    )
                )
            else:
                MB_read = bytes_read / 1024 / 1024
                bytes_str = locale.toString(MB_read)
                self.status_message.emit(
                    translate(
                        "AddonsInstaller",
                        f"Downloading: {bytes_str}MB of unknown total",
                    )
                )

    def finish_zip(self, index: int, response_code: int, filename: os.PathLike):
        self.zip_complete = True
        if response_code != 200:
            self.failure.emit(
                self.repo,
                translate(
                    "AddonsInstaller",
                    f"Error: Error while downloading ZIP file for {self.repo.display_name}",
                ),
            )
            return

        with zipfile.ZipFile(filename, "r") as zfile:
            master = zfile.namelist()[0]  # github will put everything in a subfolder
            self.status_message.emit(
                translate("AddonsInstaller", f"Download complete. Unzipping file...")
            )
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
            zfile.extractall(self.zipdir)
        for filename in os.listdir(self.zipdir + os.sep + master):
            shutil.move(
                self.zipdir + os.sep + master + os.sep + filename,
                self.zipdir + os.sep + filename,
            )
        os.rmdir(self.zipdir + os.sep + master)
        if self.bakdir:
            shutil.rmtree(self.bakdir)
        self.update_metadata()
        self.success.emit(
            self.repo,
            translate(
                "AddonsInstaller",
                f"Successfully installed {self.repo.display_name} from ZIP file",
            ),
        )

    def update_metadata(self):
        basedir = FreeCAD.getUserAppDataDir()
        package_xml = os.path.join(basedir, "Mod", self.repo.name, "package.xml")
        if os.path.isfile(package_xml):
            self.repo.load_metadata_file(package_xml)
            self.repo.installed_version = self.repo.metadata.Version
            self.repo.updated_timestamp = datetime.now().timestamp()


class DependencyInstallationWorker(QtCore.QThread):
    """Install dependencies using Addonmanager for FreeCAD, and pip for python"""

    no_python_exe = QtCore.Signal()
    no_pip = QtCore.Signal(str)  # Attempted command
    failure = QtCore.Signal(str, str)  # Short message, detailed message
    success = QtCore.Signal()

    def __init__(self, addons, python_required, python_optional):
        QtCore.QThread.__init__(self)
        self.addons = addons
        self.python_required = python_required
        self.python_optional = python_optional

    def run(self):

        for repo in self.addons:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            worker = InstallWorkbenchWorker(repo)
            worker.start()
            while worker.isRunning():
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    worker.requestInterruption()
                    worker.wait()
                    return
                time.sleep(0.1)
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        if self.python_required or self.python_optional:

            # Find Python. In preference order
            #   A) The value of the PythonExecutableForPip user preference
            #   B) The executable located in the same bin directory as FreeCAD and called "python3"
            #   C) The executable located in the same bin directory as FreeCAD and called "python"
            #   D) The result of an shutil search for your system's "python3" executable
            #   E) The result of an shutil search for your system's "python" executable
            prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
            python_exe = prefs.GetString("PythonExecutableForPip", "Not set")
            if (
                not python_exe
                or python_exe == "Not set"
                or not os.path.exists(python_exe)
            ):
                fc_dir = FreeCAD.getHomePath()
                python_exe = os.path.join(fc_dir, "bin", "python3")
                if "Windows" in platform.system():
                    python_exe += ".exe"

            if not python_exe or not os.path.exists(python_exe):
                python_exe = os.path.join(fc_dir, "bin", "python")
                if "Windows" in platform.system():
                    python_exe += ".exe"

            if not python_exe or not os.path.exists(python_exe):
                python_exe = shutil.which("python3")

            if not python_exe or not os.path.exists(python_exe):
                python_exe = shutil.which("python")

            if not python_exe or not os.path.exists(python_exe):
                self.no_python_exe.emit()
                return

            prefs.SetString("PythonExecutableForPip", python_exe)

            pip_failed = False
            try:
                proc = subprocess.run(
                    [python_exe, "-m", "pip", "--version"], stdout=subprocess.PIPE
                )
            except subprocess.CalledProcessError as e:
                pip_failed = True
            if proc.returncode != 0:
                pip_failed = True
            if pip_failed:
                self.no_pip.emit(f"{python_exe} -m pip --version")
                return
            FreeCAD.Console.PrintMessage(proc.stdout)
            FreeCAD.Console.PrintWarning(proc.stderr)
            result = proc.stdout
            FreeCAD.Console.PrintMessage(result.decode())
            vendor_path = os.path.join(
                FreeCAD.getUserAppDataDir(), "AdditionalPythonPackages"
            )
            if not os.path.exists(vendor_path):
                os.makedirs(vendor_path)

        for pymod in self.python_required:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            proc = subprocess.run(
                [python_exe, "-m", "pip", "install", "--target", vendor_path, pymod],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            FreeCAD.Console.PrintMessage(proc.stdout.decode())
            if proc.returncode != 0:
                self.emit.failure(
                    translate(
                        "AddonsInstaller",
                        f"Installation of Python package {pymod} failed",
                    ),
                    proc.stderr,
                )
                return

        for pymod in self.python_optional:
            if QtCore.QThread.currentThread().isInterruptionRequested():
                return
            proc = subprocess.run(
                [python_exe, "-m", "pip", "install", "--target", vendor_path, pymod],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            FreeCAD.Console.PrintMessage(proc.stdout.decode())
            if proc.returncode != 0:
                self.emit.failure(
                    translate(
                        "AddonsInstaller",
                        f"Installation of Python package {pymod} failed",
                    ),
                    proc.stderr,
                )
                return

        self.success.emit()


class CheckSingleWorker(QtCore.QThread):
    """Worker to check for updates for a single addon"""

    updateAvailable = QtCore.Signal(bool)

    def __init__(self, name):

        QtCore.QThread.__init__(self)
        self.name = name

    def run(self):

        if not have_git or NOGIT:
            return
        FreeCAD.Console.PrintLog(
            "Checking for available updates of the " + self.name + " addon\n"
        )
        addondir = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", self.name)
        if os.path.exists(addondir):
            if os.path.exists(addondir + os.sep + ".git"):
                gitrepo = git.Git(addondir)
                try:
                    gitrepo.fetch()
                    if "git pull" in gitrepo.status():
                        self.updateAvailable.emit(True)
                        return
                except Exception:
                    # can fail for any number of reasons, ex.  not being online
                    pass
        self.updateAvailable.emit(False)


class UpdateMetadataCacheWorker(QtCore.QThread):
    "Scan through all available packages and see if our local copy of package.xml needs to be updated"

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    package_updated = QtCore.Signal(AddonManagerRepo)

    class RequestType(Enum):
        PACKAGE_XML = auto()
        METADATA_TXT = auto()
        REQUIREMENTS_TXT = auto()
        ICON = auto()

    def __init__(self, repos):

        QtCore.QThread.__init__(self)
        self.repos = repos
        self.requests: Dict[
            int, (AddonManagerRepo, UpdateMetadataCacheWorker.RequestType)
        ] = {}
        AM_NETWORK_MANAGER.completed.connect(self.download_completed)
        self.requests_completed = 0
        self.total_requests = 0
        self.store = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        self.updated_repos = set()

    def run(self):
        current_thread = QtCore.QThread.currentThread()

        for repo in self.repos:
            if repo.url and utils.recognized_git_location(repo):
                # package.xml
                index = AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "package.xml")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.PACKAGE_XML,
                )
                self.total_requests += 1

                # metadata.txt
                index = AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "metadata.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.METADATA_TXT,
                )
                self.total_requests += 1

                # requirements.txt
                index = AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "requirements.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT,
                )
                self.total_requests += 1

        while self.requests:
            if current_thread.isInterruptionRequested():
                AM_NETWORK_MANAGER.completed.disconnect(self.download_completed)
                for request in self.requests.keys():
                    AM_NETWORK_MANAGER.abort(request)
                return
            # 50 ms maximum between checks for interruption
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        # This set contains one copy of each of the repos that got some kind of data in
        # this process. For those repos, tell the main Addon Manager code that it needs
        # to update its copy of the repo, and redraw its information.
        for repo in self.updated_repos:
            self.package_updated.emit(repo)

    def download_completed(
        self, index: int, code: int, data: QtCore.QByteArray
    ) -> None:
        if index in self.requests:
            self.requests_completed += 1
            self.progress_made.emit(self.requests_completed, self.total_requests)
            request = self.requests.pop(index)
            if code == 200:  # HTTP success
                self.updated_repos.add(request[0])  # mark this repo as updated
                if request[1] == UpdateMetadataCacheWorker.RequestType.PACKAGE_XML:
                    self.process_package_xml(request[0], data)
                elif request[1] == UpdateMetadataCacheWorker.RequestType.METADATA_TXT:
                    self.process_metadata_txt(request[0], data)
                elif (
                    request[1] == UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT
                ):
                    self.process_requirements_txt(request[0], data)
                elif request[1] == UpdateMetadataCacheWorker.RequestType.ICON:
                    self.process_icon(request[0], data)

    def process_package_xml(self, repo: AddonManagerRepo, data: QtCore.QByteArray):
        repo.repo_type = AddonManagerRepo.RepoType.PACKAGE  # By definition
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "package.xml")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())
        metadata = FreeCAD.Metadata(new_xml_file)
        repo.metadata = metadata
        self.status_message.emit(
            translate("AddonsInstaller", f"Downloaded package.xml for {repo.name}")
        )

        # Grab a new copy of the icon as well: we couldn't enqueue this earlier because
        # we didn't know the path to it, which is stored in the package.xml file.
        icon = metadata.Icon
        if not icon:
            # If there is no icon set for the entire package, see if there are
            # any workbenches, which are required to have icons, and grab the first
            # one we find:
            content = repo.metadata.Content
            if "workbench" in content:
                wb = content["workbench"][0]
                if wb.Icon:
                    if wb.Subdirectory:
                        subdir = wb.Subdirectory
                    else:
                        subdir = wb.Name
                    repo.Icon = subdir + wb.Icon
                    icon = repo.Icon

        icon_url = utils.construct_git_url(repo, icon)
        index = AM_NETWORK_MANAGER.submit_unmonitored_get(icon_url)
        self.requests[index] = (repo, UpdateMetadataCacheWorker.RequestType.ICON)
        self.total_requests += 1

    def process_metadata_txt(self, repo: AddonManagerRepo, data: QtCore.QByteArray):
        self.status_message.emit(
            translate(
                "AddonsInstaller", f"Downloaded metadata.txt for {repo.display_name}"
            )
        )
        f = io.StringIO(data.data().decode("utf8"))
        while True:
            line = f.readline()
            if not line:
                break
            if line.startswith("workbenches="):
                depswb = line.split("=")[1].split(",")
                for wb in depswb:
                    wb_name = wb.strip()
                    if wb_name:
                        repo.requires.add(wb_name)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} requires FreeCAD Addon '{wb_name}'\n"
                        )

            elif line.startswith("pylibs="):
                depspy = line.split("=")[1].split(",")
                for pl in depspy:
                    dep = pl.strip()
                    if dep:
                        repo.python_requires.add(dep)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} requires python package '{dep}'\n"
                        )

            elif line.startswith("optionalpylibs="):
                opspy = line.split("=")[1].split(",")
                for pl in opspy:
                    dep = pl.strip()
                    if dep:
                        repo.python_optional.add(dep)
                        FreeCAD.Console.PrintLog(
                            f"{repo.display_name} optionally imports python package '{pl.strip()}'\n"
                        )
        # For review and debugging purposes, store the file locally
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "metadata.txt")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())

    def process_requirements_txt(self, repo: AddonManagerRepo, data: QtCore.QByteArray):
        self.status_message.emit(
            translate(
                "AddonsInstaller",
                f"Downloaded requirements.txt for {repo.display_name}",
            )
        )
        f = io.StringIO(data.data().decode("utf8"))
        lines = f.readlines()
        for line in lines:
            break_chars = " <>=~!+#"
            package = line
            for n, c in enumerate(line):
                if c in break_chars:
                    package = line[:n].strip()
                    break
            if package:
                repo.python_requires.add(package)
        # For review and debugging purposes, store the file locally
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "requirements.txt")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())

    def process_icon(self, repo: AddonManagerRepo, data: QtCore.QByteArray):
        self.status_message.emit(
            translate("AddonsInstaller", f"Downloaded icon for {repo.display_name}")
        )
        cache_file = repo.get_cached_icon_filename()
        with open(cache_file, "wb") as icon_file:
            icon_file.write(data.data())
            repo.cached_icon_filename = cache_file


if have_git and not NOGIT:

    class GitProgressMonitor(git.RemoteProgress):
        """An object that receives git progress updates and stores them for later display"""

        def __init__(self):
            super().__init__()
            self.current = 0
            self.total = 100
            self.message = ""

        def update(
            self,
            _: int,
            cur_count: Union[str, float],
            max_count: Union[str, float, None] = None,
            message: str = "",
        ) -> None:
            if max_count:
                self.current = int(cur_count)
                self.total = int(max_count)
            if message:
                self.message = message


class UpdateAllWorker(QtCore.QThread):
    """Update all listed packages, of any kind"""

    progress_made = QtCore.Signal(int, int)
    status_message = QtCore.Signal(str)
    success = QtCore.Signal(AddonManagerRepo)
    failure = QtCore.Signal(AddonManagerRepo)

    def __init__(self, repos):
        super().__init__()
        self.repos = repos

    def run(self):
        self.progress_made.emit(0, len(self.repos))
        self.repo_queue = queue.Queue()
        current_thread = QtCore.QThread.currentThread()
        for repo in self.repos:
            self.repo_queue.put(repo)

        # Following the QNetworkAccessManager model, we'll spawn six threads to process these requests in parallel:
        workers = []
        for _ in range(6):
            worker = UpdateSingleWorker(self.repo_queue)
            worker.success.connect(self.on_success)
            worker.failure.connect(self.on_failure)
            worker.start()
            workers.append(worker)

        while not self.repo_queue.empty():
            if current_thread.isInterruptionRequested():
                for worker in workers:
                    worker.blockSignals(True)
                    worker.requestInterruption()
                    worker.wait()
                return
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()

        self.repo_queue.join()

        # Make sure all of our child threads have fully exited:
        for worker in workers:
            worker.wait()

    def on_success(self, repo: AddonManagerRepo) -> None:
        self.progress_made.emit(
            len(self.repos) - self.repo_queue.qsize(), len(self.repos)
        )
        self.success.emit(repo)

    def on_failure(self, repo: AddonManagerRepo) -> None:
        self.progress_made.emit(
            len(self.repos) - self.repo_queue.qsize(), len(self.repos)
        )
        self.failure.emit(repo)


class UpdateSingleWorker(QtCore.QThread):
    success = QtCore.Signal(AddonManagerRepo)
    failure = QtCore.Signal(AddonManagerRepo)

    def __init__(self, repo_queue: queue.Queue):
        super().__init__()
        self.repo_queue = repo_queue

    def run(self):
        current_thread = QtCore.QThread.currentThread()
        while True:
            if current_thread.isInterruptionRequested():
                return
            try:
                repo = self.repo_queue.get_nowait()
            except queue.Empty:
                return
            if repo.repo_type == AddonManagerRepo.RepoType.MACRO:
                self.update_macro(repo)
            else:
                self.update_package(repo)
            self.repo_queue.task_done()

    def update_macro(self, repo: AddonManagerRepo):
        """Updating a macro happens in this function, in the current thread"""

        cache_path = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "MacroCache"
        )
        os.makedirs(cache_path, exist_ok=True)
        install_succeeded, _ = repo.macro.install(cache_path)

        if install_succeeded:
            install_succeeded, _ = repo.macro.install(FreeCAD.getUserMacroDir(True))
            utils.update_macro_installation_details(repo)

        if install_succeeded:
            self.success.emit(repo)
        else:
            self.failure.emit(repo)

    def update_package(self, repo: AddonManagerRepo):
        """Updating a package re-uses the package installation worker, so actually spawns another thread that we block on"""

        worker = InstallWorkbenchWorker(repo)
        worker.success.connect(lambda repo, _: self.success.emit(repo))
        worker.failure.connect(lambda repo, _: self.failure.emit(repo))
        worker.start()
        while True:
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()
            if not worker.isRunning():
                break


#  @}
