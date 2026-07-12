# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
GUI for the Feeds & Speeds library -> job Presets sync (see
Roadmap/Epics/FeedsAndSpeeds.md, "Library -> Job preset sync"). ``sync_job_from_gui`` runs
it on demand from a Job's context menu; the document-load observer at the
bottom prompts automatically when a Job has stale presets.
"""

from PySide import QtCore, QtGui, QtWidgets
import FreeCAD
import FreeCADGui
import Path
from Path.Tool import cam_assets
from Path.Tool.FeedsSpeeds.library_sync import sync_job_tools

__title__ = "Feeds & Speeds Library Sync"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Sync ToolBit presets from the library into a Job's tools."

translate = FreeCAD.Qt.translate


def _is_job(obj) -> bool:
    return hasattr(obj, "Proxy") and obj.Proxy.__class__.__name__ == "ObjectJob"


def _format_report(report) -> str:
    """Formats a report with nothing to sync or reconcile (``updated`` and
    ``orphaned`` are both empty here - see ``sync_job_from_gui``, which routes
    anything worth showing through the dialog instead)."""
    if report.unchanged:
        return translate("CAM_Job", "{} tool(s) already up to date.").format(len(report.unchanged))
    return translate("CAM_Job", "This job has no tools to sync.")


def sync_job_from_gui(job, parent=None) -> None:
    """Manual entry point from the Job's context menu: dry-run, then reuse
    the same stale-presets dialog as the load-time prompt."""
    report = sync_job_tools(job, cam_assets, apply=False)
    if not report.updated and not report.orphaned:
        QtWidgets.QMessageBox.information(
            parent or FreeCADGui.getMainWindow(),
            translate("CAM_Job", "Sync Tool Presets from Library"),
            _format_report(report),
        )
        return
    _notify_and_prompt_stale_jobs(job.Document, [(job, report)])


class _StalePresetsDialog(QtWidgets.QDialog):
    """Scrollable table of updated + orphaned tools. Yes/No (default No) if
    anything is actually updatable; otherwise just a Close button, since
    orphaned-only means nothing can be applied."""

    def __init__(self, doc_label: str, stale, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_Job", "Tool Presets Available"))
        self.setWindowIcon(QtGui.QIcon(":/icons/CAM_SyncTools.svg"))
        self.resize(520, 320)

        any_updates = any(report.updated for _, report in stale)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        header = QtWidgets.QHBoxLayout()
        header.setSpacing(12)
        icon_label = QtWidgets.QLabel()
        icon_label.setPixmap(QtGui.QIcon(":/icons/CAM_SyncTools.svg").pixmap(64, 64))
        header.addWidget(icon_label)
        if any_updates:
            intro_text = translate(
                "CAM_Job",
                "'{}' has tools with newer presets available in the library. Sync "
                "them now? Presets only - feed/speed/spindle values on existing "
                "operations are never changed.",
            ).format(doc_label)
        else:
            intro_text = translate(
                "CAM_Job",
                "'{}' has tools that couldn't be matched to anything in the "
                "library by ID. Nothing will be changed.",
            ).format(doc_label)
        intro = QtWidgets.QLabel(intro_text)
        intro.setWordWrap(True)
        header.addWidget(intro, 1)
        layout.addLayout(header)

        table = QtWidgets.QTableWidget(0, 4, self)
        table.setHorizontalHeaderLabels(
            [
                translate("CAM_Job", "Job"),
                translate("CAM_Job", "Tool #"),
                translate("CAM_Job", "Tool"),
                translate("CAM_Job", "Status"),
            ]
        )
        table.horizontalHeaderItem(1).setTextAlignment(QtCore.Qt.AlignCenter)
        table.verticalHeader().setVisible(False)
        table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        table.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        # Tool (the variable-length toolbit description) gets the stretch,
        # not Status - a short fixed-width label doesn't need it.
        table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

        rows = []
        for job, report in stale:
            for detail in report.updated:
                rows.append(
                    (
                        job.Label,
                        detail.tool_number,
                        detail.label,
                        translate("CAM_Job", "Sync"),
                        False,
                    )
                )
            for detail in report.orphaned:
                rows.append(
                    (
                        job.Label,
                        detail.tool_number,
                        detail.label,
                        translate("CAM_Job", "No match"),
                        True,
                    )
                )
        status_icon = {
            False: QtGui.QIcon(":/icons/button_valid.svg"),
            True: QtGui.QIcon(":/icons/button_invalid.svg"),
        }
        table.setRowCount(len(rows))
        for row, (job_label, tool_number, tool_label, status_text, is_orphaned) in enumerate(rows):
            job_item = QtWidgets.QTableWidgetItem(job_label)
            number_item = QtWidgets.QTableWidgetItem(
                "" if tool_number is None else str(tool_number)
            )
            number_item.setTextAlignment(QtCore.Qt.AlignCenter)
            tool_item = QtWidgets.QTableWidgetItem(tool_label)
            status_item = QtWidgets.QTableWidgetItem(status_icon[is_orphaned], status_text)
            if is_orphaned:
                gray = QtGui.QColor(QtCore.Qt.gray)
                for item in (job_item, number_item, tool_item, status_item):
                    item.setForeground(gray)
            table.setItem(row, 0, job_item)
            table.setItem(row, 1, number_item)
            table.setItem(row, 2, tool_item)
            table.setItem(row, 3, status_item)
        table.resizeColumnToContents(0)
        table.resizeColumnToContents(1)
        table.resizeColumnToContents(3)
        layout.addWidget(table, 1)

        if any_updates:
            buttons = QtWidgets.QDialogButtonBox(
                QtWidgets.QDialogButtonBox.Yes | QtWidgets.QDialogButtonBox.No
            )
            buttons.button(QtWidgets.QDialogButtonBox.No).setDefault(True)
            buttons.button(QtWidgets.QDialogButtonBox.No).setFocus()
            buttons.accepted.connect(self.accept)
            buttons.rejected.connect(self.reject)
        else:
            buttons = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Close)
            buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)


def _notify_and_prompt_stale_jobs(doc, stale) -> None:
    """``stale`` is a list of (job, report) pairs with at least one updatable
    or orphaned tool."""
    dialog = _StalePresetsDialog(doc.Label, stale, parent=FreeCADGui.getMainWindow())
    if dialog.exec_() == QtWidgets.QDialog.Accepted:
        for job, _ in stale:
            sync_job_tools(job, cam_assets)


_IDLE_POLL_MS = 100


def _is_document_idle(doc) -> bool:
    """True once restore/recompute is done and the GUI isn't busy (cursor, modal dialog)."""
    if getattr(doc, "Restoring", False) or getattr(doc, "Recomputing", False):
        return False
    if QtWidgets.QApplication.overrideCursor() is not None:
        return False
    return QtWidgets.QApplication.activeModalWidget() is None


def _run_when_idle(doc_name: str) -> None:
    try:
        doc = FreeCAD.getDocument(doc_name)
    except Exception:
        return  # closed before we got to it
    if not _is_document_idle(doc):
        QtCore.QTimer.singleShot(_IDLE_POLL_MS, lambda: _run_when_idle(doc_name))
        return
    _check_and_prompt(doc)


def _check_and_prompt(doc) -> None:
    try:
        jobs = [obj for obj in doc.Objects if _is_job(obj)]
    except Exception:
        return
    stale = []
    for job in jobs:
        try:
            report = sync_job_tools(job, cam_assets, apply=False)
        except Exception as e:
            Path.Log.debug(f"library_sync: stale-presets check failed for '{job.Label}': {e}")
            continue
        if report.updated:
            stale.append((job, report))
    if stale:
        _notify_and_prompt_stale_jobs(doc, stale)


class _JobLoadObserver:
    """Prompts once per document load if a Job has stale tool presets.

    ``slotActivateDocument`` fires reliably on open (no "document loaded"
    signal exists), but can fire before the document settles, so the real
    check is deferred via ``_run_when_idle``. ``_checked_docs`` limits this
    to once per load; ``slotDeletedDocument`` clears it so a reopen re-checks.
    """

    def __init__(self):
        self._checked_docs = set()

    def slotActivateDocument(self, doc):
        if doc.Name in self._checked_docs:
            return
        self._checked_docs.add(doc.Name)
        # No "does it even have a Job" shortcut here: doc.Objects at signal-fire
        # time isn't trustworthy (the document may not be fully populated yet).
        # _check_and_prompt re-derives the job list after the idle-wait, when
        # it actually is.
        _run_when_idle(doc.Name)

    def slotDeletedDocument(self, doc):
        self._checked_docs.discard(doc.Name)


_observer = None


def _ensure_observer_registered():
    global _observer
    if _observer is None:
        _observer = _JobLoadObserver()
        FreeCAD.addDocumentObserver(_observer)


if FreeCAD.GuiUp:
    _ensure_observer_registered()
