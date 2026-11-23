# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.               #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses      #
#                                                                              #
################################################################################

"""Dialog for importing a Machine from an MTConnect agent.

Accepts the URL of a machine's MTConnect agent (the /probe endpoint) or the
path of a saved probe XML file, fetches and parses the document, and returns
the resulting in-memory Machine together with the ImportReport.  Saving is
left to the Machine Editor, which the caller opens with the imported machine.
"""

import os
import urllib.error
import urllib.parse
import urllib.request

import FreeCAD
from PySide import QtGui

from Machine.models.mtconnect_import import (
    ProbeParseError,
    list_devices,
    machine_from_probe,
)

translate = FreeCAD.Qt.translate

FETCH_TIMEOUT_SECONDS = 5
# Probe documents are tens of kilobytes; anything near this size is not one.
MAX_DOCUMENT_BYTES = 4 * 1024 * 1024


class FetchError(Exception):
    """The probe document could not be retrieved."""


def _fetch_url(url: str) -> str:
    try:
        with urllib.request.urlopen(url, timeout=FETCH_TIMEOUT_SECONDS) as response:
            return response.read(MAX_DOCUMENT_BYTES).decode("utf-8", errors="replace")
    except (urllib.error.URLError, OSError, ValueError) as e:
        raise FetchError(str(e))


def fetch_probe(source: str) -> str:
    """Return probe XML from a URL or local file path.

    A URL without a path (e.g. http://machine:5000) is retried with /probe
    appended, matching the MTConnect convention.
    """
    source = source.strip()
    if os.path.isfile(source):
        with open(source, "r", encoding="utf-8", errors="replace") as f:
            return f.read(MAX_DOCUMENT_BYTES)

    url = source if "://" in source else "http://" + source
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https"):
        raise FetchError(translate("CAM_MachineImport", "Only http and https URLs are supported."))
    if parsed.path in ("", "/"):
        return _fetch_url(url.rstrip("/") + "/probe")
    return _fetch_url(url)


class MTConnectImportDialog(QtGui.QDialog):
    """Prompt for a probe source and build a Machine from it."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_MachineImport", "Import Machine from MTConnect"))
        self.machine = None
        self.report = None

        layout = QtGui.QVBoxLayout(self)
        label = QtGui.QLabel(
            translate(
                "CAM_MachineImport",
                "Enter the URL of the machine's MTConnect agent "
                "(for example http://machine:5000/probe) or select a saved "
                "probe XML file.",
            )
        )
        label.setWordWrap(True)
        layout.addWidget(label)

        source_layout = QtGui.QHBoxLayout()
        self.source_edit = QtGui.QLineEdit()
        self.source_edit.setPlaceholderText("http://machine:5000/probe")
        source_layout.addWidget(self.source_edit)
        browse_button = QtGui.QToolButton()
        browse_button.setIcon(QtGui.QIcon.fromTheme("folder-open"))
        browse_button.setToolTip(translate("CAM_MachineImport", "Select a probe XML file"))
        browse_button.clicked.connect(self._browse)
        source_layout.addWidget(browse_button)
        layout.addLayout(source_layout)

        buttons = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel)
        buttons.button(QtGui.QDialogButtonBox.Ok).setText(translate("CAM_MachineImport", "Import"))
        buttons.accepted.connect(self._import)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        self.source_edit.setFocus()

    def _browse(self):
        filename, _ = QtGui.QFileDialog.getOpenFileName(
            self,
            translate("CAM_MachineImport", "Select Probe XML File"),
            "",
            translate("CAM_MachineImport", "XML files (*.xml);;All files (*)"),
        )
        if filename:
            self.source_edit.setText(filename)

    def _import(self):
        source = self.source_edit.text().strip()
        if not source:
            return
        try:
            xml_text = fetch_probe(source)
            devices = list_devices(xml_text)
            device_name = None
            if len(devices) > 1:
                names = [d["name"] for d in devices]
                device_name, ok = QtGui.QInputDialog.getItem(
                    self,
                    translate("CAM_MachineImport", "Select Device"),
                    translate("CAM_MachineImport", "This agent describes several machines:"),
                    names,
                    0,
                    False,
                )
                if not ok:
                    return
            self.machine, self.report = machine_from_probe(
                xml_text, device_name=device_name, source=source
            )
        except (FetchError, ProbeParseError) as e:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM_MachineImport", "Import Failed"),
                str(e),
            )
            return
        self.accept()

    @classmethod
    def get_machine(cls, parent=None):
        """Run the dialog; return (machine, report) or None if cancelled."""
        dialog = cls(parent)
        if dialog.exec_() == QtGui.QDialog.Accepted and dialog.machine is not None:
            summary = dialog.report.summary() if dialog.report else ""
            if summary:
                QtGui.QMessageBox.information(
                    parent,
                    translate("CAM_MachineImport", "Machine Imported"),
                    translate(
                        "CAM_MachineImport",
                        "The machine was imported. Review it in the editor.\n\n{summary}",
                    ).format(summary=summary),
                )
            return dialog.machine, dialog.report
        return None
