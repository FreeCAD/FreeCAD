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
from enum import Enum, auto
from html.parser import HTMLParser
import re

import addonmanager_freecad_interface as fci
import addonmanager_utilities as utils
import NetworkManager

translate = fci.translate

REGEX_HTML_COMMENT = r"(?s)<!--.*-->"


class ReadmeViewer(QtWidgets.QTextBrowser):

    """A QTextBrowser widget that, when given an Addon, downloads the README data as appropriate
    and renders it with whatever technology is available (usually Qt's Markdown renderer for
    workbenches and its HTML renderer for Macros)."""

    def __init__(self, parent=None):
        super().__init__(parent)
        NetworkManager.InitializeNetworkManager()
        NetworkManager.AM_NETWORK_MANAGER.completed.connect(self._download_completed)
        self.readme_request_index = 0
        self.resource_requests = {}
        self.url = ""
        self.repo: Addon.Addon = None
        self.setOpenExternalLinks(True)
        self.setOpenLinks(True)
        self.image_map = {}
        self.stop = True

    def set_addon(self, repo: Addon):
        """Set which Addon's information is displayed"""

        self.setPlainText(translate("AddonsInstaller", "Loading README data..."))
        self.repo = repo
        self.stop = False
        if self.repo.repo_type == Addon.Addon.Kind.MACRO:
            self.url = self.repo.macro.wiki
            if not self.url:
                self.url = self.repo.macro.url
        else:
            self.url = utils.get_readme_url(repo)

        self.readme_request_index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
            self.url
        )

    def _download_completed(self, index: int, code: int, data: QtCore.QByteArray) -> None:
        """Callback for handling a completed README file download."""
        if index == self.readme_request_index:
            if code == 200:  # HTTP success
                self._process_package_download(data.data().decode("utf-8"))
            else:
                self.setPlainText(
                    translate(
                        "AddonsInstaller",
                        "Failed to download data from {} -- received response code {}.",
                    ).format(self.url, code)
                )
        elif index in self.resource_requests:
            if code == 200:
                self._process_resource_download(self.resource_requests[index], data.data())
            else:
                self.image_map[self.resource_requests[index]] = None
            del self.resource_requests[index]
            if not self.resource_requests:
                self.set_addon(self.repo)  # Trigger a reload of the page now with resources

    def removeHTMLComments(self, string: str):
        """Remove HTML Comments from a string"""
        return re.sub(REGEX_HTML_COMMENT, "", string)

    def cleanMarkdown(self, markdown: str):
        """Clean a string of Markdown"""
        return self.removeHTMLComments(markdown)

    def _process_package_download(self, data: str):
        if self.repo.repo_type == Addon.Addon.Kind.MACRO:
            parser = WikiCleaner()
            parser.feed(data)
            self.setHtml(parser.final_html)
        else:
            # Check for recent Qt (e.g. Qt5.15 or later). Check can be removed when
            # we no longer support Ubuntu 20.04LTS for compiling.
            if hasattr(self, "setMarkdown"):
                self.setMarkdown(self.cleanMarkdown(data))
            else:
                self.setPlainText(data)

    def _process_resource_download(self, resource_name: str, resource_data: bytes):
        image = QtGui.QImage.fromData(resource_data)
        if image:
            self.image_map[resource_name] = self._ensure_appropriate_width(image)
        else:
            self.image_map[resource_name] = None

    def loadResource(self, resource_type: int, name: QtCore.QUrl) -> object:
        """Callback for resource loading. Called automatically by underlying Qt
        code when external resources are needed for rendering. In particular,
        here it is used to download and cache (in RAM) the images needed for the
        README and Wiki pages."""
        if resource_type == QtGui.QTextDocument.ImageResource and not self.stop:
            full_url = self._create_full_url(name.toString())
            if full_url not in self.image_map:
                self.image_map[full_url] = None
                fci.Console.PrintMessage(f"Downloading image from {full_url}...\n")
                index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(full_url)
                self.resource_requests[index] = full_url
            return self.image_map[full_url]
        return super().loadResource(resource_type, name)

    def hideEvent(self, event: QtGui.QHideEvent):
        self.stop = True
        for request in self.resource_requests:
            NetworkManager.AM_NETWORK_MANAGER.abort(request)
        self.resource_requests.clear()

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


class WikiCleaner(HTMLParser):
    """This HTML parser cleans up FreeCAD Macro Wiki Page for display in a
    QTextBrowser widget (which does not deal will with tables used as formatting,
    etc.) It strips out any tables, and extracts the mw-parser-output div as the only
    thing that actually gets displayed. It also discards anything inside the [edit]
    spans that litter wiki output."""

    class State(Enum):
        BeforeMacroContent = auto()
        InMacroContent = auto()
        InTable = auto()
        InEditSpan = auto()
        AfterMacroContent = auto()

    def __init__(self):
        super().__init__()
        self.depth_in_div = 0
        self.depth_in_span = 0
        self.depth_in_table = 0
        self.final_html = "<html><body>"
        self.previous_state = WikiCleaner.State.BeforeMacroContent
        self.state = WikiCleaner.State.BeforeMacroContent

    def handle_starttag(self, tag: str, attrs):
        if tag == "div":
            self.handle_div_start(attrs)
        elif tag == "span":
            self.handle_span_start(attrs)
        elif tag == "table":
            self.handle_table_start(attrs)
        else:
            if self.state == WikiCleaner.State.InMacroContent:
                self.add_tag_to_html(tag, attrs)

    def handle_div_start(self, attrs):
        for name, value in attrs:
            if name == "class" and value == "mw-parser-output":
                self.previous_state = self.state
                self.state = WikiCleaner.State.InMacroContent
        if self.state == WikiCleaner.State.InMacroContent:
            self.depth_in_div += 1
            self.add_tag_to_html("div", attrs)

    def handle_span_start(self, attrs):
        for name, value in attrs:
            if name == "class" and value == "mw-editsection":
                self.previous_state = self.state
                self.state = WikiCleaner.State.InEditSpan
                break
        if self.state == WikiCleaner.State.InEditSpan:
            self.depth_in_span += 1
        elif WikiCleaner.State.InMacroContent:
            self.add_tag_to_html("span", attrs)

    def handle_table_start(self, unused):
        if self.state != WikiCleaner.State.InTable:
            self.previous_state = self.state
            self.state = WikiCleaner.State.InTable
        self.depth_in_table += 1

    def add_tag_to_html(self, tag, attrs=None):
        self.final_html += f"<{tag}"
        if attrs:
            self.final_html += " "
            for attr, value in attrs:
                self.final_html += f"{attr}='{value}'"
        self.final_html += ">\n"

    def handle_endtag(self, tag):
        if tag == "table":
            self.handle_table_end()
        elif tag == "span":
            self.handle_span_end()
        elif tag == "div":
            self.handle_div_end()
        else:
            if self.state == WikiCleaner.State.InMacroContent:
                self.add_tag_to_html(f"/{tag}")

    def handle_span_end(self):
        if self.state == WikiCleaner.State.InEditSpan:
            self.depth_in_span -= 1
            if self.depth_in_span <= 0:
                self.depth_in_span = 0
                self.state = self.previous_state
        else:
            self.add_tag_to_html(f"/span")

    def handle_div_end(self):
        if self.state == WikiCleaner.State.InMacroContent:
            self.depth_in_div -= 1
            if self.depth_in_div <= 0:
                self.depth_in_div = 0
                self.state = WikiCleaner.State.AfterMacroContent
                self.final_html += "</body></html>"
        else:
            self.add_tag_to_html(f"/div")

    def handle_table_end(self):
        if self.state == WikiCleaner.State.InTable:
            self.depth_in_table -= 1
            if self.depth_in_table <= 0:
                self.depth_in_table = 0
                self.state = self.previous_state

    def handle_data(self, data):
        if self.state == WikiCleaner.State.InMacroContent:
            self.final_html += data
