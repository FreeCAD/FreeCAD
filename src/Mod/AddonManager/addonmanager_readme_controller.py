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

import FreeCAD
from Addon import Addon
import addonmanager_utilities as utils

from enum import IntEnum, Enum, auto
from html.parser import HTMLParser
from typing import Optional

import NetworkManager

translate = FreeCAD.Qt.translate

from PySide import QtCore, QtGui


class ReadmeDataType(IntEnum):
    PlainText = 0
    Markdown = 1
    Html = 2


class ReadmeController(QtCore.QObject):

    """A class that can provide README data from an Addon, possibly loading external resources such
    as images"""

    def __init__(self, widget):
        super().__init__()
        NetworkManager.InitializeNetworkManager()
        NetworkManager.AM_NETWORK_MANAGER.completed.connect(self._download_completed)
        self.readme_request_index = 0
        self.resource_requests = {}
        self.resource_failures = []
        self.url = ""
        self.readme_data = None
        self.readme_data_type = None
        self.addon: Optional[Addon] = None
        self.stop = True
        self.widget = widget
        self.widget.load_resource.connect(self.loadResource)
        self.widget.follow_link.connect(self.follow_link)

    def set_addon(self, repo: Addon):
        """Set which Addon's information is displayed"""

        self.addon = repo
        self.stop = False
        self.readme_data = None
        if self.addon.repo_type == Addon.Kind.MACRO:
            self.url = self.addon.macro.wiki
            if not self.url:
                self.url = self.addon.macro.url
            if not self.url:
                self.widget.setText(
                    translate(
                        "AddonsInstaller",
                        "Loading info for {} from the FreeCAD Macro Recipes wiki...",
                    ).format(self.addon.display_name, self.url)
                )
                return
        else:
            self.url = utils.get_readme_url(repo)
        self.widget.setUrl(self.url)

        self.widget.setText(
            translate("AddonsInstaller", "Loading page for {} from {}...").format(
                self.addon.display_name, self.url
            )
        )
        self.readme_request_index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(
            self.url
        )

    def _download_completed(self, index: int, code: int, data: QtCore.QByteArray) -> None:
        """Callback for handling a completed README file download."""
        if index == self.readme_request_index:
            if code == 200:  # HTTP success
                self._process_package_download(data.data().decode("utf-8"))
            else:
                self.widget.setText(
                    translate(
                        "AddonsInstaller",
                        "Failed to download data from {} -- received response code {}.",
                    ).format(self.url, code)
                )
        elif index in self.resource_requests:
            if code == 200:
                self._process_resource_download(self.resource_requests[index], data.data())
            else:
                FreeCAD.Console.PrintLog(f"Failed to load {self.resource_requests[index]}\n")
                self.resource_failures.append(self.resource_requests[index])
            del self.resource_requests[index]
            if not self.resource_requests:
                if self.readme_data:
                    if self.readme_data_type == ReadmeDataType.Html:
                        self.widget.setHtml(self.readme_data)
                    elif self.readme_data_type == ReadmeDataType.Markdown:
                        self.widget.setMarkdown(self.readme_data)
                    else:
                        self.widget.setText(self.readme_data)
                else:
                    self.set_addon(self.addon)  # Trigger a reload of the page now with resources

    def _process_package_download(self, data: str):
        if self.addon.repo_type == Addon.Kind.MACRO:
            parser = WikiCleaner()
            parser.feed(data)
            self.readme_data = parser.final_html
            self.readme_data_type = ReadmeDataType.Html
            self.widget.setHtml(parser.final_html)
        else:
            self.readme_data = data
            self.readme_data_type = ReadmeDataType.Markdown
            self.widget.setMarkdown(data)

    def _process_resource_download(self, resource_name: str, resource_data: bytes):
        image = QtGui.QImage.fromData(resource_data)
        self.widget.set_resource(resource_name, image)

    def loadResource(self, full_url: str):
        if full_url not in self.resource_failures:
            index = NetworkManager.AM_NETWORK_MANAGER.submit_unmonitored_get(full_url)
            self.resource_requests[index] = full_url

    def cancel_resource_loading(self):
        self.stop = True
        for request in self.resource_requests:
            NetworkManager.AM_NETWORK_MANAGER.abort(request)
        self.resource_requests.clear()

    def follow_link(self, url: str) -> None:
        final_url = url
        if not url.startswith("http"):
            if url.endswith(".md"):
                final_url = self._create_markdown_url(url)
            else:
                final_url = self._create_full_url(url)
        FreeCAD.Console.PrintLog(f"Loading {final_url} in the system browser")
        QtGui.QDesktopServices.openUrl(final_url)

    def _create_full_url(self, url: str) -> str:
        if url.startswith("http"):
            return url
        if not self.url:
            return url
        lhs, slash, _ = self.url.rpartition("/")
        return lhs + slash + url

    def _create_markdown_url(self, file: str) -> str:
        base_url = utils.get_readme_html_url(self.addon)
        lhs, slash, _ = base_url.rpartition("/")
        return lhs + slash + file


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
