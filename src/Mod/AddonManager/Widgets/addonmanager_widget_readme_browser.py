# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 The FreeCAD Project Association AISBL         *
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
import re

import FreeCAD

# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6  # Outside FreeCAD, try Qt6 first

        PySide = PySide6
    except ImportError:
        import PySide2  # Fall back to Qt5 (if this fails, Python will kill this module's import)

        PySide = PySide2

from PySide import QtCore, QtGui, QtWidgets

from typing import Optional


class WidgetReadmeBrowser(QtWidgets.QTextBrowser):
    """A QTextBrowser widget that emits signals for each requested image resource, allowing an external controller
    to load and re-deliver those images. Once all resources have been re-delivered, the original data is redisplayed
    with the images in-line. Call setUrl prior to calling setMarkdown or setHtml to ensure URLs are resolved
    correctly."""

    load_resource = QtCore.Signal(str)  # Str is a URL to a resource
    follow_link = QtCore.Signal(str)  # Str is a URL to another page

    def __init__(self, parent: QtWidgets.QWidget = None):
        super().__init__(parent)
        self.image_map = {}
        self.url = ""
        self.stop = False
        self.setOpenExternalLinks(True)

    def setUrl(self, url: str):
        """Set the base URL of the page. Used to resolve relative URLs in the page source."""
        self.url = url

    def setMarkdown(self, md: str):
        """Provides an optional fallback to the markdown library for older versions of Qt (prior to 5.15) that did not
        have native markdown support. Lacking that, plaintext is displayed."""
        geometry = self.geometry()
        if hasattr(super(), "setMarkdown"):

            super().setMarkdown(self._clean_markdown(md))
        else:
            try:
                import markdown

                html = markdown.markdown(md)
                self.setHtml(html)
            except ImportError:
                self.setText(md)
                FreeCAD.Console.Warning(
                    "Qt < 5.15 and no `import markdown` -- falling back to plain text display\n"
                )
        self.setGeometry(geometry)

    def _clean_markdown(self, md: str):
        # Remove some HTML tags (for now just img and br, which are the most common offenders that break rendering)
        br_re = re.compile(r"<br\s*/?>")
        img_re = re.compile(r"<img\s.*?src=(?:'|\")([^'\">]+)(?:'|\").*?\/?>")

        cleaned = br_re.sub("\n", md)
        cleaned = img_re.sub("[html tag removed]", cleaned)

        return cleaned

    def set_resource(self, resource_url: str, image: Optional[QtGui.QImage]):
        """Once a resource has been fetched (or the fetch has failed), this method should be used to inform the widget
        that the resource has been loaded. Note that the incoming image is scaled to 97% of the widget width if it is
        larger than that."""
        self.image_map[resource_url] = self._ensure_appropriate_width(image)

    def loadResource(self, resource_type: int, name: QtCore.QUrl) -> object:
        """Callback for resource loading. Called automatically by underlying Qt
        code when external resources are needed for rendering. In particular,
        here it is used to download and cache (in RAM) the images needed for the
        README and Wiki pages."""
        if resource_type == QtGui.QTextDocument.ImageResource and not self.stop:
            full_url = self._create_full_url(name.toString())
            if full_url not in self.image_map:
                self.load_resource.emit(full_url)
                self.image_map[full_url] = None
            return self.image_map[full_url]
        elif resource_type == QtGui.QTextDocument.MarkdownResource:
            self.follow_link.emit(name.toString())
            return self.toMarkdown()
        elif resource_type == QtGui.QTextDocument.HtmlResource:
            self.follow_link.emit(name.toString())
            return self.toHtml()
        return super().loadResource(resource_type, name)

    def _ensure_appropriate_width(self, image: QtGui.QImage) -> QtGui.QImage:
        ninety_seven_percent = self.width() * 0.97
        if image.width() < ninety_seven_percent:
            return image
        return image.scaledToWidth(ninety_seven_percent)

    def _create_full_url(self, url: str) -> str:
        if url.startswith("http"):
            return url
        if not self.url:
            return url
        lhs, slash, _ = self.url.rpartition("/")
        return lhs + slash + url
