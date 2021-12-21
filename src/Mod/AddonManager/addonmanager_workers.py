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
from datetime import datetime
from typing import Union, List

from PySide2 import QtCore, QtGui, QtNetwork

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui

import addonmanager_utilities as utils
from addonmanager_macro import Macro
from addonmanager_metadata import MetadataDownloadWorker
from AddonManagerRepo import AddonManagerRepo

translate = FreeCAD.Qt.translate

have_git = False
try:
    import git

    # some versions of git module have no "Repo" class??  Bug #4072 module
    # 'git' has no attribute 'Repo'
    have_git = hasattr(git, "Repo")
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

# These addons will print an additional message informing the user
obsolete = []

# These addons will print an additional message informing the user Python2 only
py2only = []

NOGIT = False  # for debugging purposes, set this to True to always use http downloads
NOMARKDOWN = False  # for debugging purposes, set this to True to disable Markdown lib
"""Multithread workers for the Addon Manager"""


class UpdateWorker(QtCore.QThread):
    """This worker updates the list of available workbenches"""

    status_message = QtCore.Signal(str)
    addon_repo = QtCore.Signal(object)
    done = QtCore.Signal()

    def __init__(self):

        QtCore.QThread.__init__(self)

    def run(self):
        "populates the list of addons"

        self.current_thread = QtCore.QThread.currentThread()

        # update info lists
        global obsolete, macros_reject_list, py2only
        u = utils.urlopen(
            "https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/addonflags.json"
        )
        if u:
            p = u.read()
            u.close()
            j = json.loads(p)
            if "obsolete" in j and "Mod" in j["obsolete"]:
                obsolete = j["obsolete"]["Mod"]

            if "blacklisted" in j and "Macro" in j["blacklisted"]:
                macros_reject_list = j["blacklisted"]["Macro"]

            if "py2only" in j and "Mod" in j["py2only"]:
                py2only = j["py2only"]["Mod"]
        else:
            FreeCAD.Console.PrintWarning("Debug: addon_flags.json not found\n")

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
                md_file = os.path.join(addondir,"package.xml")
                if os.path.isfile(md_file):
                    repo.load_metadata_file(md_file)
                    repo.installed_version = repo.metadata.Version
                    repo.updated_timestamp = os.path.getmtime(md_file)
                self.addon_repo.emit(repo)

        # querying official addons
        u = utils.urlopen(
            "https://raw.githubusercontent.com/FreeCAD/FreeCAD-addons/master/.gitmodules"
        )
        if not u:
            self.done.emit()
            self.stop = True
            return
        p = u.read()
        if isinstance(p, bytes):
            p = p.decode("utf-8")
        u.close()
        p = re.findall(
            (
                r'(?m)\[submodule\s*"(?P<name>.*)"\]\s*'
                r"path\s*=\s*(?P<path>.+)\s*"
                r"url\s*=\s*(?P<url>https?://.*)\s*"
                r"(branch\s*=\s*(?P<branch>[^\s]*)\s*)?"
            ),
            p,
        )
        for name, path, url, _, branch in p:
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
            md_file = os.path.join(addondir,"package.xml")
            if os.path.isfile(md_file):
                repo.load_metadata_file(md_file)
                repo.installed_version = repo.metadata.Version
                repo.updated_timestamp = os.path.getmtime(md_file)
            self.addon_repo.emit(repo)

            self.status_message.emit(
                translate("AddonsInstaller", "Workbenches list was updated.")
            )

        if not self.current_thread.isInterruptionRequested():
            self.done.emit()
            self.stop = True


class LoadPackagesFromCacheWorker(QtCore.QThread):
    addon_repo = QtCore.Signal(object)
    done = QtCore.Signal()

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
                            repo.updated_timestamp = os.path.getmtime(repo_metadata_cache_path)
                        except Exception:
                            FreeCAD.Console.PrintWarning(
                                translate("AddonsInstaller","Failed loading") + f"{repo_metadata_cache_path}\n"
                            )
                            pass
                    self.addon_repo.emit(repo)
        self.done.emit()


class LoadMacrosFromCacheWorker(QtCore.QThread):
    add_macro_signal = QtCore.Signal(object)
    done = QtCore.Signal()

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
                self.add_macro_signal.emit(AddonManagerRepo.from_macro(new_macro))
        self.done.emit()


class CheckWorkbenchesForUpdatesWorker(QtCore.QThread):
    """This worker checks for available updates for all workbenches"""

    update_status = QtCore.Signal(AddonManagerRepo)
    progress_made = QtCore.Signal(int, int)
    done = QtCore.Signal()

    def __init__(self, repos: List[AddonManagerRepo]):

        QtCore.QThread.__init__(self)
        self.repos = repos

    def run(self):

        if NOGIT or not have_git:
            self.done.emit()
            self.stop = True
            return
        self.current_thread = QtCore.QThread.currentThread()
        self.basedir = FreeCAD.getUserAppDataDir()
        self.moddir = self.basedir + os.sep + "Mod"
        upds = []
        gitpython_warning = False
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

        self.stop = True
        self.done.emit()

    def check_workbench(self, wb):
        if not have_git or NOGIT:
            return
        clonedir = self.moddir + os.sep + wb.name
        if os.path.exists(clonedir):
            # mark as already installed AND already checked for updates
            if not os.path.exists(clonedir + os.sep + ".git"):
                # Repair addon installed with raw download
                bare_repo = git.Repo.clone_from(
                    wb.url, clonedir + os.sep + ".git", bare=True
                )
                try:
                    with bare_repo.config_writer() as cw:
                        cw.set("core", "bare", False)
                except AttributeError:
                    if not gitpython_warning:
                        FreeCAD.Console.PrintWarning(
                            translate(
                                "AddonsInstaller",
                                "Outdated GitPython detected, "
                                "consider upgrading with pip.",
                            )
                            + "\n"
                        )
                        gitpython_warning = True
                    cw = bare_repo.config_writer()
                    cw.set("core", "bare", False)
                    del cw
                repo = git.Repo(clonedir)
                repo.head.reset("--hard")
            gitrepo = git.Git(clonedir)
            try:
                gitrepo.fetch()
            except Exception:
                FreeCAD.Console.PrintWarning(
                    "AddonManager: "
                    + translate(
                        "AddonsInstaller", "Unable to fetch git updates for workbench"
                    )
                    + " "
                    + wb.name
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
                        "AddonManager - " + wb.name + " git status"
                        " fatal: this operation must be run in a work tree \n"
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
                package.set_metadata(installed_metadata)
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
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    translate("AddonsInstaller", "Failed to read metadata from")
                    + f" {installed_metadata_file}"
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
                f"Failed to fetch code for macro '{macro_wrapper.macro.name}'\n"
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
    done = QtCore.Signal()

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
                    "Retrieving macros from FreeCAD/FreeCAD-Macros Git repository",
                )
            )
            self.retrieve_macros_from_wiki()

        if self.current_thread.isInterruptionRequested():
            return

        self.status_message_signal.emit(
            translate("AddonsInstaller", "Done locating macros.")
        )
        self.stop = True
        self.done.emit()

    def retrieve_macros_from_git(self):
        """Retrieve macros from FreeCAD-macros.git

        Emits a signal for each macro in
        https://github.com/FreeCAD/FreeCAD-macros.git
        """

        if not have_git or NOGIT:
            self.status_message_signal.emit(
                "GitPython not installed! Cannot retrieve macros from Git"
            )
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "GitPython not installed! Cannot retrieve macros from git",
                )
                + "\n"
            )
            return

        try:
            # TODO: someday see if the directory exists, and do a pull instead
            # of a clone
            git.Repo.clone_from(
                "https://github.com/FreeCAD/FreeCAD-macros.git", self.repo_dir
            )
        except Exception:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Something went wrong with the Git Macro Retrieval, "
                    "possibly the Git executable is not in the path",
                )
                + "\n"
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
                    self.add_macro_signal.emit(repo)

    def retrieve_macros_from_wiki(self):
        """Retrieve macros from the wiki

        Read the wiki and emit a signal for each found macro.
        Reads only the page https://wiki.freecad.org/Macros_recipes
        """

        u = utils.urlopen("https://wiki.freecad.org/Macros_recipes")
        if not u:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "There appears to be an issue connecting to the Wiki, "
                    "therefore FreeCAD cannot retrieve the Wiki macro list at this time",
                )
                + "\n"
            )
            return
        p = u.read()
        u.close()
        if isinstance(p, bytes):
            p = p.decode("utf-8")
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
                self.add_macro_signal.emit(repo)


class ShowWorker(QtCore.QThread):
    """This worker retrieves info of a given workbench"""

    status_message = QtCore.Signal(str)
    readme_updated = QtCore.Signal(str)
    update_status = QtCore.Signal(AddonManagerRepo)
    done = QtCore.Signal()

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
                FreeCAD.Console.PrintWarning(f"Debug: README not found for {url}\n")
            u = utils.urlopen(readmeurl)
            if not u:
                FreeCAD.Console.PrintWarning(
                    f"Debug: README not found at {readmeurl}\n"
                )
            u = utils.urlopen(readmeurl)
            if u:
                p = u.read()
                if isinstance(p, bytes):
                    p = p.decode("utf-8")
                u.close()
                readme = re.findall(regex, p, flags=re.MULTILINE | re.DOTALL)
                if readme:
                    desc = readme[0]
            else:
                FreeCAD.Console.PrintWarning(
                    f"Debug: README not found at {readmeurl}\n"
                )
        else:
            # convert raw markdown using lib
            readmeurl = utils.get_readme_url(self.repo)
            if not readmeurl:
                FreeCAD.Console.PrintWarning(f"Debug: README not found for {url}\n")
            u = utils.urlopen(readmeurl)
            if u:
                p = u.read()
                if isinstance(p, bytes):
                    p = p.decode("utf-8")
                u.close()
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
                FreeCAD.Console.PrintWarning("Debug: README not found at {readmeurl}\n")
            if desc == "":
                # fall back to the description text
                u = utils.urlopen(url)
                if not u:
                    self.stop = True
                    return
                p = u.read()
                if isinstance(p, bytes):
                    p = p.decode("utf-8")
                u.close()
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
            # Addon is installed but we haven't checked it yet, so lets check if it has an update
            upd = False
            # checking for updates
            if not NOGIT and have_git:
                repo = self.repo
                clonedir = (
                    FreeCAD.getUserAppDataDir() + os.sep + "Mod" + os.sep + repo.name
                )
                if os.path.exists(clonedir):
                    if not os.path.exists(clonedir + os.sep + ".git"):
                        # Repair addon installed with raw download
                        bare_repo = git.Repo.clone_from(
                            repo.url, clonedir + os.sep + ".git", bare=True
                        )
                        try:
                            with bare_repo.config_writer() as cw:
                                cw.set("core", "bare", False)
                        except AttributeError:
                            FreeCAD.Console.PrintWarning(
                                translate(
                                    "AddonsInstaller",
                                    "Outdated GitPython detected, "
                                    "consider upgrading with pip.",
                                )
                                + "\n"
                            )
                            cw = bare_repo.config_writer()
                            cw.set("core", "bare", False)
                            del cw
                        repo = git.Repo(clonedir)
                        repo.head.reset("--hard")
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

        # If the Addon is obsolete, let the user know through the Addon UI
        if self.repo.name in obsolete:
            message = """
<div style="width: 100%; text-align:center; background: #FFB3B3;">
    <strong style="color: #FFFFFF; background: #FF0000;">
"""
            message += (
                translate("AddonsInstaller", "This addon is marked as obsolete")
                + "</strong><br/><br/>"
            )
            message += (
                translate(
                    "AddonsInstaller",
                    "This usually means it is no longer maintained, "
                    "and some more advanced addon in this list "
                    "provides the same functionality.",
                )
                + "<br/></div><hr/>"
                + desc
            )

        # If the Addon is Python 2 only, let the user know through the Addon UI
        if self.repo.name in py2only:
            message = """
<div style="width: 100%; text-align:center; background: #ffe9b3;">
    <strong style="color: #FFFFFF; background: #ff8000;">
"""
            message += (
                translate("AddonsInstaller", "This addon is marked as Python 2 Only")
                + "</strong><br/><br/>"
            )
            message += translate(
                "AddonsInstaller",
                "This workbench may no longer be maintained and "
                "installing it on a Python 3 system will more than "
                "likely result in errors at startup or while in use.",
            )
            message += "<br/></div><hr/>" + desc

        if QtCore.QThread.currentThread().isInterruptionRequested():
            return
        self.readme_updated.emit(message)
        self.mustLoadImages = True
        label = self.loadImages(message, self.repo.url, self.repo.name)
        if label:
            self.readme_updated.emit(label)
        if QtCore.QThread.currentThread().isInterruptionRequested():
            return
        self.done.emit()
        self.stop = True

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
            storedimages = []
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
                            u = utils.urlopen(path)
                            imagedata = u.read()
                            u.close()
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
                            f.write(imagedata)
                            f.close()
                            # resize the image to 300x300px if needed
                            img = QtGui.QImage(storename)
                            if (img.width() > 300) or (img.height() > 300):
                                pix = QtGui.QPixmap()
                                pix = pix.fromImage(
                                    img.scaled(
                                        300,
                                        300,
                                        QtCore.Qt.KeepAspectRatio,
                                        QtCore.Qt.FastTransformation,
                                    )
                                )
                                pix.save(storename, "jpeg", 100)
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
    done = QtCore.Signal()

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
        if self.macro.is_installed():
            already_installed_msg = (
                '<strong style="background: #00B629;">'
                + translate("AddonsInstaller", "This macro is already installed.")
                + "</strong><br>"
            )
        else:
            already_installed_msg = ""
        message = (
            already_installed_msg
            + "<h1>"
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
        self.done.emit()
        self.stop = True


class InstallWorkbenchWorker(QtCore.QThread):
    "This worker installs a workbench"

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    success = QtCore.Signal(AddonManagerRepo, str)
    failure = QtCore.Signal(AddonManagerRepo, str)

    def __init__(self, repo: AddonManagerRepo):

        QtCore.QThread.__init__(self)
        self.repo = repo
        if have_git and not NOGIT:
            self.git_progress = GitProgressMonitor()
            # TODO: What is wrong with these?
            # self.git_progress.progress_made.connect(self.progress_made.emit)
            # self.git_progress.info_message.connect(self.status_message.emit)

    def run(self):
        "installs or updates the selected addon"

        if not self.repo:
            return

        if not have_git or NOGIT:
            FreeCAD.Console.PrintWarning(
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
                        "Your version of python doesn't appear to support ZIP "
                        "files. Unable to proceed.",
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
            self.run_git(target_dir)
        else:
            self.run_zip(target_dir)

        self.stop = True

    def run_git(self, clonedir: str) -> None:

        if NOGIT or not have_git:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "No Git Python installed, skipping git operations",
                )
                + "\n"
            )
            return

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
                    "User requested updating a Python 2 workbench on "
                    "a system running Python 3 - ",
                )
                + str(self.repo.name)
                + "\n"
            )
        if not os.path.exists(clonedir + os.sep + ".git"):
            # Repair addon installed with raw download by adding the .git
            # directory to it
            bare_repo = git.Repo.clone_from(
                self.repo.url, clonedir + os.sep + ".git", bare=True
            )
            try:
                with bare_repo.config_writer() as cw:
                    cw.set("core", "bare", False)
            except AttributeError:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        "Outdated GitPython detected, consider " "upgrading with pip.",
                    )
                    + "\n"
                )
                cw = bare_repo.config_writer()
                cw.set("core", "bare", False)
                del cw
            repo = git.Repo(clonedir)
            repo.head.reset("--hard")
        repo = git.Git(clonedir)
        try:
            repo.pull()
            answer = translate(
                "AddonsInstaller",
                "Workbench successfully updated. "
                "Please restart FreeCAD to apply the changes.",
            )
        except Exception:
            answer = (
                translate("AddonsInstaller", "Error updating module ")
                + self.repo.name
                + " - "
                + translate("AddonsInstaller", "Please fix manually")
            )
            answer += repo.status()
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
        depsok, answer = self.check_python_dependencies(self.repo.url)
        if depsok:
            if str(self.repo.name) in py2only:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "AddonsInstaller",
                        "User requested installing a Python 2 "
                        "workbench on a system running Python 3 - ",
                    )
                    + str(self.repo.name)
                    + "\n"
                )
            self.status_message.emit("Cloning module...")
            repo = git.Repo.clone_from(self.repo.url, clonedir)

            # Make sure to clone all the submodules as well
            if repo.submodules:
                repo.submodule_update(recursive=True)

            if self.repo.branch in repo.heads:
                repo.heads[self.repo.branch].checkout()

            answer = translate(
                "AddonsInstaller",
                "Workbench successfully installed. Please restart "
                "FreeCAD to apply the changes.",
            )
        else:
            self.failure.emit(self.repo, answer)
            return

        if self.repo.repo_type == AddonManagerRepo.RepoType.WORKBENCH:
            # symlink any macro contained in the module to the macros folder
            macro_dir = FreeCAD.getUserMacroDir(True)
            if not os.path.exists(macro_dir):
                os.makedirs(macro_dir)
            if os.path.exists(clonedir):
                for f in os.listdir(clonedir):
                    if f.lower().endswith(".fcmacro"):
                        utils.symlink(
                            os.path.join(clonedir, f), os.path.join(macro_dir, f)
                        )
                        FreeCAD.ParamGet(
                            "User parameter:Plugins/" + self.repo.name
                        ).SetString("destination", clonedir)
                        answer += "\n\n" + translate(
                            "AddonsInstaller",
                            "A macro has been installed and is available "
                            "under Macro -> Macros menu",
                        )
                        answer += ":\n<b>" + f + "</b>"
        self.update_metadata()
        self.success.emit(self.repo, answer)

    def check_python_dependencies(self, baseurl: str) -> Union[bool, str]:
        """checks if the repo contains a metadata.txt and check its contents"""

        ok = True
        message = ""
        depsurl = baseurl.replace("github.com", "raw.githubusercontent.com")
        if not depsurl.endswith("/"):
            depsurl += "/"
        depsurl += "master/metadata.txt"
        try:
            mu = utils.urlopen(depsurl)
        except Exception:
            return True, translate(
                "AddonsInstaller",
                "No metadata.txt found, cannot evaluate Python dependencies",
            )
        if mu:
            # metadata.txt found
            depsfile = mu.read()
            mu.close()

            # urllib2 gives us a bytelike object instead of a string.  Have to
            # consider that
            try:
                depsfile = depsfile.decode("utf-8")
            except AttributeError:
                pass

            deps = depsfile.split("\n")
            for line in deps:
                if line.startswith("workbenches="):
                    depswb = line.split("=")[1].split(",")
                    for wb in depswb:
                        if wb.strip():
                            if not wb.strip() in FreeCADGui.listWorkbenches().keys():
                                if (
                                    not wb.strip() + "Workbench"
                                    in FreeCADGui.listWorkbenches().keys()
                                ):
                                    ok = False
                                    message += (
                                        translate(
                                            "AddonsInstaller", "Missing workbench"
                                        )
                                        + ": "
                                        + wb
                                        + ", "
                                    )
                elif line.startswith("pylibs="):
                    depspy = line.split("=")[1].split(",")
                    for pl in depspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except ImportError:
                                ok = False
                                message += (
                                    translate(
                                        "AddonsInstaller", "Missing python module"
                                    )
                                    + ": "
                                    + pl
                                    + ", "
                                )
                elif line.startswith("optionalpylibs="):
                    opspy = line.split("=")[1].split(",")
                    for pl in opspy:
                        if pl.strip():
                            try:
                                __import__(pl.strip())
                            except ImportError:
                                message += translate(
                                    "AddonsInstaller",
                                    "Missing optional python module (doesn't prevent installing)",
                                )
                                message += ": " + pl + ", "
        if message and (not ok):
            final_message = translate(
                "AddonsInstaller",
                "Some errors were found that prevent installation of this workbench",
            )
            final_message += ": <b>" + message + "</b>. "
            final_message += translate(
                "AddonsInstaller", "Please install the missing components first."
            )
            message = final_message
        return ok, message

    def check_package_dependencies(self):
        # TODO: Use the dependencies set in the package.xml metadata
        pass

    def run_zip(self, zipdir: str) -> None:
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
                translate("AddonsInstaller", "Error: Unable to locate zip from")
                + " "
                + self.repo.name,
            )
            return
        try:
            u = utils.urlopen(zipurl)
        except Exception:
            self.failure.emit(
                self.repo,
                translate("AddonsInstaller", "Error: Unable to download")
                + " "
                + zipurl,
            )
            return
        if not u:
            self.failure.emit(
                self.repo,
                translate("AddonsInstaller", "Error: Unable to download")
                + " "
                + zipurl,
            )
            return

        data_size = 0
        if "content-length" in u.headers:
            try:
                data_size = int(u.headers["content-length"])
            except Exception:
                pass

        current_thread = QtCore.QThread.currentThread()
        if data_size and data_size > 5 * 1024 * 1024:
            # Use an on-disk file and track download progress, if the zipfile
            # is over 5mb
            with tempfile.NamedTemporaryFile(delete=False) as temp:
                bytes_to_read = 16 * 1024
                bytes_read = 0
                while True:
                    if current_thread.isInterruptionRequested():
                        return
                    chunk = u.read(bytes_to_read)
                    if not chunk:
                        break
                    bytes_read += bytes_to_read
                    temp.write(chunk)
                    self.progress_made.emit(bytes_read, data_size)
            zfile = zipfile.ZipFile(temp)
        else:
            zfile = io.BytesIO()
            zfile.write(u.read())
            zfile = zipfile.ZipFile(zfile)
        master = zfile.namelist()[0]  # github will put everything in a subfolder
        zfile.extractall(zipdir)
        u.close()
        zfile.close()
        for filename in os.listdir(zipdir + os.sep + master):
            shutil.move(
                zipdir + os.sep + master + os.sep + filename, zipdir + os.sep + filename
            )
        os.rmdir(zipdir + os.sep + master)
        if bakdir:
            shutil.rmtree(bakdir)
        self.update_metadata()
        self.success.emit(
            self.repo,
            translate("AddonsInstaller", "Successfully installed") + " " + zipurl,
        )

    def update_metadata(self):
        basedir = FreeCAD.getUserAppDataDir()
        package_xml = os.path.join(basedir, "Mod", self.repo.name, "package.xml")
        if os.path.isfile(package_xml):
            self.repo.load_metadata_file(package_xml)
            self.repo.installed_version = self.repo.metadata.Version
            self.repo.updated_timestamp = datetime.now().timestamp()


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
    done = QtCore.Signal()
    package_updated = QtCore.Signal(AddonManagerRepo)

    class AtomicCounter(object):
        def __init__(self, start=0):
            self.lock = threading.Lock()
            self.count = start

        def set(self, new_value):
            with self.lock:
                self.count = new_value

        def get(self):
            with self.lock:
                return self.count

        def increment(self):
            with self.lock:
                self.count += 1

        def decrement(self):
            with self.lock:
                self.count -= 1

    def __init__(self, repos):

        QtCore.QThread.__init__(self)
        self.repos = repos
        self.counter = UpdateMetadataCacheWorker.AtomicCounter()

    def run(self):
        current_thread = QtCore.QThread.currentThread()
        self.num_downloads_required = len(self.repos)
        self.progress_made.emit(0, self.num_downloads_required)
        self.status_message.emit(
            translate("AddonsInstaller", "Retrieving package metadata...")
        )
        store = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        index_file = os.path.join(store, "index.json")
        self.index = {}
        if os.path.isfile(index_file):
            with open(index_file, "r") as f:
                index_string = f.read()
                self.index = json.loads(index_string)

        download_queue = (
            QtNetwork.QNetworkAccessManager()
        )  # Must be created on this thread
        download_queue.finished.connect(self.on_finished)

        self.downloaders = []
        for repo in self.repos:
            if repo.metadata_url:
                downloader = MetadataDownloadWorker(None, repo, self.index)
                downloader.start_fetch(download_queue)
                downloader.updated.connect(self.on_updated)
                self.downloaders.append(downloader)

        # Run a local event loop until we've processed all of the downloads:
        # this is local to this thread, and does not affect the main event loop
        ui_updater = QtCore.QTimer()
        ui_updater.timeout.connect(self.send_ui_update)
        ui_updater.start(100)  # Send an update back to the main thread every 100ms
        self.num_downloads_required = len(self.downloaders)
        self.num_downloads_completed = UpdateMetadataCacheWorker.AtomicCounter()
        aborted = False
        while True:
            if current_thread.isInterruptionRequested() and not aborted:
                for downloader in self.downloaders:
                    downloader.abort()
                aborted = True
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents)
            if self.num_downloads_completed.get() >= self.num_downloads_required:
                break

        if aborted:
            FreeCAD.Console.PrintMessage("Metadata update cancelled\n")
            return

        # Update and serialize the updated index, overwriting whatever was
        # there before
        for downloader in self.downloaders:
            self.index[downloader.repo.name] = downloader.last_sha1
        if not os.path.exists(store):
            os.makedirs(store)
        with open(index_file, "w") as f:
            json.dump(self.index, f, indent="  ")

        # Signal completion to our parent thread
        self.done.emit()
        self.stop = True

    def on_finished(self, reply):
        # Called by the QNetworkAccessManager's sub-threads when a fetch
        # process completed (in any state)
        self.num_downloads_completed.increment()
        reply.deleteLater()

    def on_updated(self, repo):
        # Called if this repo got new metadata and/or a new icon
        self.package_updated.emit(repo)

    def send_ui_update(self):
        self.progress_made.emit(
            self.num_downloads_completed.get(), self.num_downloads_required
        )


if have_git and not NOGIT:

    class GitProgressMonitor(git.RemoteProgress):
        """An object that receives git progress updates and transforms them into Qt signals"""

        progress_made = QtCore.Signal(int, int)
        info_message = QtCore.Signal(str)

        def __init__(self):
            super().__init__()

        def update(
            self,
            op_code: int,
            cur_count: Union[str, float],
            max_count: Union[str, float, None] = None,
            message: str = "",
        ) -> None:
            if max_count:
                self.progress_made.emit(int(cur_count), int(max_count))
            if message:
                self.info_message.emit(message)


class UpdateAllWorker(QtCore.QThread):
    """Update all listed packages, of any kind"""

    progress_made = QtCore.Signal(int, int)
    status_message = QtCore.Signal(str)
    success = QtCore.Signal(AddonManagerRepo)
    failure = QtCore.Signal(AddonManagerRepo)
    done = QtCore.Signal()

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
                    worker.requestInterruption()
                    worker.wait()
                return
            # Ensure our signals propagate out by running an internal thread-local event loop
            QtCore.QCoreApplication.processEvents()

        self.repo_queue.join()

        # Make sure all of our child threads have fully exited:
        for i, worker in enumerate(workers):
            worker.wait()

        self.done.emit()

    def on_success(self, repo: AddonManagerRepo) -> None:
        self.progress_made.emit(self.repo_queue.qsize(), len(self.repos))
        self.success.emit(repo)

    def on_failure(self, repo: AddonManagerRepo) -> None:
        self.progress_made.emit(self.repo_queue.qsize(), len(self.repos))
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
        install_succeeded, errors = repo.macro.install(cache_path)

        if install_succeeded:
            install_succeeded, errors = repo.macro.install(FreeCAD.getUserMacroDir())

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
