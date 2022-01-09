# ***************************************************************************
# *                                                                         *
# * Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>      *
# *                                                                         *
# * This program is free software; you can redistribute it and/or modify    *
# * it under the terms of the GNU Lesser General Public License (LGPL)      *
# * as published by the Free Software Foundation; either version 2 of       *
# * the License, or (at your option) any later version.                     *
# * for detail see the LICENCE text file.                                   *
# *                                                                         *
# * This program is distributed in the hope that it will be useful,         *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
# * GNU Library General Public License for more details.                    *
# *                                                                         *
# * You should have received a copy of the GNU Library General Public       *
# * License along with this program; if not, write to the Free Software     *
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307     *
# * USA                                                                     *
# *                                                                         *
# ***************************************************************************

import FreeCAD

import os
import io
import hashlib
from typing import Dict, List

from PySide2 import QtCore, QtNetwork
from PySide2.QtCore import QObject

import addonmanager_utilities as utils
from AddonManagerRepo import AddonManagerRepo

translate = FreeCAD.Qt.translate


class DownloadWorker(QObject):
    updated = QtCore.Signal(AddonManagerRepo)

    def __init__(self, parent, url: str):
        """repo is an AddonManagerRepo object, and index is a dictionary of SHA1 hashes of the package.xml files in the cache"""

        super().__init__(parent)
        self.url = url

    def start_fetch(self, network_manager: QtNetwork.QNetworkAccessManager):
        """Asynchronously begin the network access. Intended as a set-and-forget black box for downloading files."""

        self.request = QtNetwork.QNetworkRequest(QtCore.QUrl(self.url))
        self.request.setAttribute(
            QtNetwork.QNetworkRequest.RedirectPolicyAttribute,
            QtNetwork.QNetworkRequest.UserVerifiedRedirectPolicy,
        )

        self.fetch_task = network_manager.get(self.request)
        self.fetch_task.finished.connect(self.resolve_fetch)
        self.fetch_task.redirected.connect(self.on_redirect)
        self.fetch_task.sslErrors.connect(self.on_ssl_error)

    def abort(self):
        if not self.fetch_task.isFinished():
            self.fetch_task.abort()

    def on_redirect(self, _):
        # For now just blindly follow all redirects
        self.fetch_task.redirectAllowed.emit()

    def on_ssl_error(self, reply: str, errors: List[str]):
        FreeCAD.Console.PrintWarning(
            translate("AddonsInstaller", "Error with encrypted connection") + "\n:"
        )
        FreeCAD.Console.PrintWarning(reply)
        for error in errors:
            FreeCAD.Console.PrintWarning(error)


class MetadataDownloadWorker(DownloadWorker):
    """A worker for downloading package.xml and associated icon(s)

    To use, instantiate an object of this class and call the start_fetch() function
    with a QNetworkAccessManager. It is expected that many of these objects will all
    be created and associated with the same QNAM, which will then handle the actual
    asynchronous downloads in some Qt-defined number of threads. To monitor progress
    you should connect to the QNAM's "finished" signal, and ensure it is called the
    number of times you expect based on how many workers you have enqueued.

    """

    def __init__(self, parent, repo: AddonManagerRepo, index: Dict[str, str]):
        """repo is an AddonManagerRepo object, and index is a dictionary of SHA1 hashes of the package.xml files in the cache"""

        super().__init__(parent, repo.metadata_url)
        self.repo = repo
        self.index = index
        self.store = os.path.join(
            FreeCAD.getUserCachePath(), "AddonManager", "PackageMetadata"
        )
        self.last_sha1 = ""

    def resolve_fetch(self):
        """Called when the data fetch completed, either with an error, or if it found the metadata file"""

        if self.fetch_task.error() == QtNetwork.QNetworkReply.NetworkError.NoError:
            FreeCAD.Console.PrintLog(f"Found a package.xml file for {self.repo.name}\n")
            self.repo.repo_type = AddonManagerRepo.RepoType.PACKAGE
            new_xml = self.fetch_task.readAll()
            hasher = hashlib.sha1()
            hasher.update(new_xml.data())
            new_sha1 = hasher.hexdigest()
            self.last_sha1 = new_sha1
            # Determine if we need to download the icon: only do that if the
            # package.xml file changed (since
            # a change in the version number will show up as a change in the
            # SHA1, without having to actually
            # read the metadata)
            if self.repo.name in self.index:
                cached_sha1 = self.index[self.repo.name]
                if cached_sha1 != new_sha1:
                    self.update_local_copy(new_xml)
                else:
                    # Assume that if the package.xml file didn't change,
                    # neither did the icon, so don't waste
                    # resources downloading it
                    xml_file = os.path.join(self.store, self.repo.name, "package.xml")
                    self.repo.metadata = FreeCAD.Metadata(xml_file)
            else:
                # There is no local copy yet, so we definitely have to update
                # the cache
                self.update_local_copy(new_xml)
        elif (
            self.fetch_task.error()
            == QtNetwork.QNetworkReply.NetworkError.ContentNotFoundError
        ):
            pass
        elif (
            self.fetch_task.error()
            == QtNetwork.QNetworkReply.NetworkError.OperationCanceledError
        ):
            pass
        else:
            FreeCAD.Console.PrintWarning(
                translate("AddonsInstaller", "Failed to connect to URL")
                + f":\n{self.url}\n {self.fetch_task.error()}\n"
            )

    def update_local_copy(self, new_xml):
        # We have to update the local copy of the metadata file and re-download
        # the icon file

        name = self.repo.name
        repo_url = self.repo.url
        package_cache_directory = os.path.join(self.store, name)
        if not os.path.exists(package_cache_directory):
            os.makedirs(package_cache_directory)
        new_xml_file = os.path.join(package_cache_directory, "package.xml")
        with open(new_xml_file, "wb") as f:
            f.write(new_xml.data())
        metadata = FreeCAD.Metadata(new_xml_file)
        self.repo.metadata = metadata
        self.repo.repo_type = AddonManagerRepo.RepoType.PACKAGE
        icon = metadata.Icon

        if not icon:
            # If there is no icon set for the entire package, see if there are
            # any workbenches, which are required to have icons, and grab the first
            # one we find:
            content = self.repo.metadata.Content
            if "workbench" in content:
                wb = content["workbench"][0]
                if wb.Icon:
                    if wb.Subdirectory:
                        subdir = wb.Subdirectory
                    else:
                        subdir = wb.Name
                    self.repo.Icon = subdir + wb.Icon
                    icon = self.repo.Icon

        icon_url = utils.construct_git_url(self.repo, icon)
        icon_stream = utils.urlopen(icon_url)
        if icon and icon_stream and icon_url:
            icon_data = icon_stream.read()
            cache_file = self.repo.get_cached_icon_filename()
            with open(cache_file, "wb") as icon_file:
                icon_file.write(icon_data)
                self.repo.cached_icon_filename = cache_file
        self.updated.emit(self.repo)


class DependencyDownloadWorker(DownloadWorker):
    """A worker for downloading metadata.txt"""

    def __init__(self, parent, repo: AddonManagerRepo):
        super().__init__(parent, utils.construct_git_url(repo, "metadata.txt"))
        self.repo = repo

    def resolve_fetch(self):
        """Called when the data fetch completed, either with an error, or if it found the metadata file"""

        if self.fetch_task.error() == QtNetwork.QNetworkReply.NetworkError.NoError:
            FreeCAD.Console.PrintLog(
                f"Found a metadata.txt file for {self.repo.name}\n"
            )
            new_deps = self.fetch_task.readAll()
            self.parse_file(new_deps.data().decode("utf8"))
        elif (
            self.fetch_task.error()
            == QtNetwork.QNetworkReply.NetworkError.ContentNotFoundError
        ):
            pass
        elif (
            self.fetch_task.error()
            == QtNetwork.QNetworkReply.NetworkError.OperationCanceledError
        ):
            pass
        else:
            FreeCAD.Console.PrintWarning(
                translate("AddonsInstaller", "Failed to connect to URL")
                + f":\n{self.url}\n {self.fetch_task.error()}\n"
            )

    def parse_file(self, data: str) -> None:
        f = io.StringIO(data)
        while True:
            line = f.readline()
            if not line:
                break
            if line.startswith("workbenches="):
                depswb = line.split("=")[1].split(",")
                for wb in depswb:
                    wb_name = wb.strip()
                    if wb_name:
                        self.repo.requires.add(wb_name)
                        FreeCAD.Console.PrintLog(
                            f"{self.repo.display_name} requires FreeCAD Addon '{wb_name}'\n"
                        )

            elif line.startswith("pylibs="):
                depspy = line.split("=")[1].split(",")
                for pl in depspy:
                    if pl.strip():
                        self.repo.python_requires.add(pl.strip())
                        FreeCAD.Console.PrintLog(
                            f"{self.repo.display_name} requires python package '{pl.strip()}'\n"
                        )
            elif line.startswith("optionalpylibs="):
                opspy = line.split("=")[1].split(",")
                for pl in opspy:
                    if pl.strip():
                        self.repo.python_optional.add(pl.strip())
                        FreeCAD.Console.PrintLog(
                            f"{self.repo.display_name} optionally imports python package '{pl.strip()}'\n"
                        )
        self.updated.emit(self.repo)
