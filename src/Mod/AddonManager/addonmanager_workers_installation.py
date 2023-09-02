# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

""" Worker thread classes for Addon Manager installation and removal """

# pylint: disable=c-extension-no-member,too-few-public-methods,too-many-instance-attributes

import io
import os
import queue
import shutil
import subprocess
import time
import zipfile
from typing import Dict, List
from enum import Enum, auto

from PySide import QtCore

import FreeCAD
import addonmanager_utilities as utils
from addonmanager_metadata import MetadataReader
from Addon import Addon
import NetworkManager

translate = FreeCAD.Qt.translate

#  @package AddonManager_workers
#  \ingroup ADDONMANAGER
#  \brief Multithread workers for the addon manager
#  @{


class UpdateMetadataCacheWorker(QtCore.QThread):
    """Scan through all available packages and see if our local copy of package.xml needs to be
    updated"""

    status_message = QtCore.Signal(str)
    progress_made = QtCore.Signal(int, int)
    package_updated = QtCore.Signal(Addon)

    class RequestType(Enum):
        """The type of item being downloaded."""

        PACKAGE_XML = auto()
        METADATA_TXT = auto()
        REQUIREMENTS_TXT = auto()
        ICON = auto()

    def __init__(self, repos):

        QtCore.QThread.__init__(self)
        self.repos = repos
        self.requests: Dict[int, (Addon, UpdateMetadataCacheWorker.RequestType)] = {}
        NetworkManager.AM_NETWORK_MANAGER.completed.connect(self.download_completed)
        self.requests_completed = 0
        self.total_requests = 0
        self.store = os.path.join(FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata")
        FreeCAD.Console.PrintLog(f"Storing Addon Manager cache data in {self.store}\n")
        self.updated_repos = set()

    def run(self):
        """Not usually called directly: instead, create an instance and call its
        start() function to spawn a new thread."""

        current_thread = QtCore.QThread.currentThread()

        for repo in self.repos:
            if repo.url and utils.recognized_git_location(repo):
                # package.xml
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "package.xml")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.PACKAGE_XML,
                )
                self.total_requests += 1

                # metadata.txt
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "metadata.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.METADATA_TXT,
                )
                self.total_requests += 1

                # requirements.txt
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
                    utils.construct_git_url(repo, "requirements.txt")
                )
                self.requests[index] = (
                    repo,
                    UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT,
                )
                self.total_requests += 1

        while self.requests:
            if current_thread.isInterruptionRequested():
                NetworkManager.AM_NETWORK_MANAGER.completed.disconnect(self.download_completed)
                for request in self.requests:
                    NetworkManager.AM_NETWORK_MANAGER.abort(request)
                return
            # 50 ms maximum between checks for interruption
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)

        # This set contains one copy of each of the repos that got some kind of data in
        # this process. For those repos, tell the main Addon Manager code that it needs
        # to update its copy of the repo, and redraw its information.
        for repo in self.updated_repos:
            self.package_updated.emit(repo)

    def download_completed(self, index: int, code: int, data: QtCore.QByteArray) -> None:
        """Callback for handling a completed metadata file download."""
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
                elif request[1] == UpdateMetadataCacheWorker.RequestType.REQUIREMENTS_TXT:
                    self.process_requirements_txt(request[0], data)
                elif request[1] == UpdateMetadataCacheWorker.RequestType.ICON:
                    self.process_icon(request[0], data)

    def process_package_xml(self, repo: Addon, data: QtCore.QByteArray):
        """Process the package.xml metadata file"""
        repo.repo_type = Addon.Kind.PACKAGE  # By definition
        package_cache_directory = os.path.join(self.store, repo.name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "package.xml")
        with open(new_xml_file, "wb") as f:
            f.write(data.data())
        metadata = MetadataReader.from_file(new_xml_file)
        repo.set_metadata(metadata)
        FreeCAD.Console.PrintLog(f"Downloaded package.xml for {repo.name}\n")
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded package.xml for {}").format(repo.name)
        )

        # Grab a new copy of the icon as well: we couldn't enqueue this earlier because
        # we didn't know the path to it, which is stored in the package.xml file.
        icon = repo.get_best_icon_relative_path()

        icon_url = utils.construct_git_url(repo, icon)
        index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(icon_url)
        self.requests[index] = (repo, UpdateMetadataCacheWorker.RequestType.ICON)
        self.total_requests += 1

    def _decode_data(self, byte_data, addon_name, file_name) -> str:
        """UTF-8 decode data, and print an error message if that fails"""

        # For review and debugging purposes, store the file locally
        package_cache_directory = os.path.join(self.store, addon_name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, file_name)
        with open(new_xml_file, "wb") as f:
            f.write(byte_data)

        f = ""
        try:
            f = byte_data.decode("utf-8")
        except UnicodeDecodeError as e:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Failed to decode {} file for Addon '{}'",
                ).format(file_name, addon_name)
                + "\n"
            )
            FreeCAD.Console.PrintWarning(str(e) + "\n")
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Any dependency information in this file will be ignored",
                )
                + "\n"
            )
        return f

    def process_metadata_txt(self, repo: Addon, data: QtCore.QByteArray):
        """Process the metadata.txt metadata file"""
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded metadata.txt for {}").format(repo.display_name)
        )

        f = self._decode_data(data.data(), repo.name, "metadata.txt")
        lines = f.splitlines()
        for line in lines:
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
                            f"{repo.display_name} optionally imports python package"
                            + f" '{pl.strip()}'\n"
                        )

    def process_requirements_txt(self, repo: Addon, data: QtCore.QByteArray):
        """Process the requirements.txt metadata file"""
        self.status_message.emit(
            translate(
                "AddonsInstaller",
                "Downloaded requirements.txt for {}",
            ).format(repo.display_name)
        )

        f = self._decode_data(data.data(), repo.name, "requirements.txt")
        lines = f.splitlines()
        for line in lines:
            break_chars = " <>=~!+#"
            package = line
            for n, c in enumerate(line):
                if c in break_chars:
                    package = line[:n].strip()
                    break
            if package:
                repo.python_requires.add(package)

    def process_icon(self, repo: Addon, data: QtCore.QByteArray):
        """Convert icon data into a valid icon file and store it"""
        self.status_message.emit(
            translate("AddonsInstaller", "Downloaded icon for {}").format(repo.display_name)
        )
        cache_file = repo.get_cached_icon_filename()
        with open(cache_file, "wb") as icon_file:
            icon_file.write(data.data())
            repo.cached_icon_filename = cache_file


#  @}
