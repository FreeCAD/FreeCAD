# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 The FreeCAD Project Association AISBL              *
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

""" A Qt Widget for displaying Addon README information """

import Addon
from PySide import QtCore, QtGui, QtWidgets

import addonmanager_freecad_interface as fci
import addonmanager_utilities as utils
import NetworkManager

translate = fci.translate


class ReadmeViewer(QtWidgets.QTextBrowser):

    """A QTextBrowser widget that, when given an Addon, downloads the README data as appropriate
    and renders it with whatever technology is available (usually Qt's Markdown renderer for
    workbenches and its HTML renderer for Macros)."""

    def __init__(self, parent=None):
        super().__init__(parent)
        NetworkManager.InitializeNetworkManager()
        NetworkManager.AM_NETWORK_MANAGER.completed.connect(self._download_completed)
        self.request_index = 0
        self.url = ""
        self.repo: Addon.Addon = None
        self.setOpenExternalLinks(True)
        self.setOpenLinks(True)
        self.image_map = {}

    def set_addon(self, repo: Addon):
        """Set which Addon's information is displayed"""

        self.setPlainText(translate("AddonsInstaller", "Loading README data..."))
        self.repo = repo
        if self.repo.repo_type == Addon.Addon.Kind.MACRO:
            self.url = self.repo.macro.wiki
            if not self.url:
                self.url = self.repo.macro.url
        else:
            self.url = utils.get_readme_url(repo)

        self.request_index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(self.url)

    def _download_completed(self, index: int, code: int, data: QtCore.QByteArray) -> None:
        """Callback for handling a completed README file download."""
        if index == self.request_index:
            if code == 200:  # HTTP success
                self._process_package_download(data.data().decode("utf-8"))
            else:
                self.setPlainText(
                    translate(
                        "AddonsInstaller",
                        "Failed to download data from {} -- received response code {}.",
                    ).format(self.url, code)
                )

    def _process_package_download(self, data: str):
        if self.repo.repo_type == Addon.Addon.Kind.MACRO:
            self.setHtml(data)
        else:
            if hasattr(self, "setMarkdown"):
                self.setMarkdown(data)
            else:
                self.setPlainText(data)

    def loadResource(self, resource_type: int, name: QtCore.QUrl) -> object:
        """Callback for resource loading. Called automatically by underlying Qt
        code when external resources are needed for rendering. In particular,
        here it is used to download and cache (in RAM) the images needed for the
        README and Wiki pages."""
        if resource_type == QtGui.QTextDocument.ImageResource:
            full_url = self._create_full_url(name.toString())
            if full_url not in self.image_map:
                self.image_map[full_url] = None
                fci.Console.PrintMessage(f"Downloading image from {full_url}...\n")
                data = NetworkManager.AM_NETWORK_MANAGER.blocking_get(full_url)
                if data and data.data():
                    image = QtGui.QImage.fromData(data.data())
                    if image:
                        self.image_map[full_url] = self._ensure_appropriate_width(image)
            return self.image_map[full_url]
        return super().loadResource(resource_type, name)

    def _create_full_url(self, url: str) -> str:
        if url.startswith("http"):
            return url
        if not self.url:
            return url
        lhs, slash, _ = self.url.rpartition("/")
        return lhs + slash + url

    def _ensure_appropriate_width(self, image: QtGui.QImage) -> QtGui.QImage:
        ninety_seven_percent = self.width() * 0.97
        if image.width() < ninety_seven_percent:
            return image
        return image.scaledToWidth(ninety_seven_percent)
