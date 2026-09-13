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
Update a Job's tools from the library - a full replace (presets,
geometry, everything), via Path.Tool.UpdateDocumentTools.

Two entry points share one dialog: ``update_tools_from_gui`` (manual,
from a Job's context menu) and the document-load observer at the bottom
(prompts automatically when a Job has a stale tool). A tool is either
updated completely or not at all - there is no partial apply - so nothing
needs to be held back from the automatic path.
"""

from typing import Optional

from PySide import QtCore, QtGui, QtWidgets
import FreeCAD
import FreeCADGui
import Path
from Path.Tool import cam_assets
from Path.Tool.UpdateDocumentTools import job_stale_tools, replace_tool_from_library
from Path.Tool.toolbit.util import format_value
from Path.Tool.docobject.ui.docobject import _get_label_text
from Path.Preferences import tool_update_on_load_enabled

__title__ = "Update Tools From Library"
__author__ = "Connor (Billy Huddleston <billy@ivdc.com>)"
__url__ = "https://www.freecad.org"
__doc__ = "Update a Job's tools (presets and geometry) from the library."

translate = FreeCAD.Qt.translate


def _summary_text(info) -> str:
    """Tag list for the collapsed row - what kind of thing changed, not
    the specifics."""
    parts = []
    if info.presets_differ:
        parts.append(translate("CAM_Job", "Presets"))
    if info.geometry_changes:
        parts.append(translate("CAM_Job", "Geometry"))
    return ", ".join(parts)


def _detail_lines(info) -> list:
    """One line per difference: a bare "Presets" marker (it's a merged
    blob, not worth itemizing) plus one "Label: old -> new" line per
    differing geometry property. Both sides are formatted in the
    library tool's Units schema (Metric/Imperial), not the embedded
    tool's - apply fully replaces the tool, Units included, so that's
    the schema the value will actually end up in, not whichever unit
    each Quantity happens to carry. Otherwise a re-authored library tool
    could show up as "1.1250 in -> 30.00 mm" for what's really the same
    dimension.
    """
    units = info.units
    lines = []
    if info.presets_differ:
        lines.append(translate("CAM_Job", "Presets"))
    for change in info.geometry_changes:
        label = _get_label_text(change.name)
        old = format_value(change.old_value, precision=3, units=units) or "?"
        new = format_value(change.new_value, precision=3, units=units) or "?"
        lines.append(f"{label}: {old} → {new}")
    return lines


_CHECK_COLUMN = 0
_JOB_COLUMN = 1
_TOOL_NUMBER_COLUMN = 2
_TOOL_COLUMN = 3
_STATUS_COLUMN = 4


class _Row:
    def __init__(self, job_label, info):
        self.job_label = job_label
        self.info = info

    def sort_key(self):
        number = self.info.tool_number if self.info.tool_number is not None else float("inf")
        return (self.job_label, number)


def _job_data_to_rows(job_data) -> list:
    rows = []
    for job_label, stale_infos in job_data:
        rows += [_Row(job_label, info) for info in stale_infos]
    rows.sort(key=_Row.sort_key)
    return rows


class _UpdateToolsDialog(QtWidgets.QDialog):
    """One row per stale tool. Each row has a checkbox (default checked)
    and an expandable Changes tree. Update fully replaces every checked
    tool with its current library copy - not a per-field merge.
    """

    def __init__(self, doc_label: str, job_data, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_Job", "Tool Updates Available"))
        self.setWindowIcon(QtGui.QIcon(":/icons/CAM_UpdateDocumentTools.svg"))
        self.resize(750, 350)

        self._checkboxes = []  # [(QCheckBox, _Row), ...]
        rows = _job_data_to_rows(job_data)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        header = QtWidgets.QHBoxLayout()
        header.setSpacing(12)
        icon_label = QtWidgets.QLabel()
        icon_label.setPixmap(QtGui.QIcon(":/icons/CAM_UpdateDocumentTools.svg").pixmap(64, 64))
        header.addWidget(icon_label)
        intro = QtWidgets.QLabel(
            translate(
                "CAM_Job",
                "'{}' has tools that differ from the library (newer presets, "
                "changed dimensions - e.g. a regrind - or both). Updating "
                "replaces the whole tool with the current library version and "
                "recomputes the document - expand a row to review before "
                "applying.",
            ).format(doc_label)
        )
        intro.setWordWrap(True)
        header.addWidget(intro, 1)
        layout.addLayout(header)

        table = QtWidgets.QTableWidget(0, 5, self)
        self._table = table
        table.setHorizontalHeaderLabels(
            [
                "",
                translate("CAM_Job", "Job"),
                translate("CAM_Job", "Tool #"),
                translate("CAM_Job", "Tool"),
                translate("CAM_Job", "Changes"),
            ]
        )
        table.horizontalHeaderItem(_TOOL_NUMBER_COLUMN).setTextAlignment(QtCore.Qt.AlignCenter)
        table.verticalHeader().setVisible(False)
        table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        table.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        table.horizontalHeader().setSectionResizeMode(_STATUS_COLUMN, QtWidgets.QHeaderView.Stretch)

        table.setRowCount(len(rows))
        for i, row in enumerate(rows):
            checkbox = QtWidgets.QCheckBox()
            checkbox.setChecked(True)
            checkbox.clicked.connect(self._on_row_checkbox_clicked)
            self._checkboxes.append((checkbox, row))
            cell = QtWidgets.QWidget()
            cell_layout = QtWidgets.QHBoxLayout(cell)
            cell_layout.setContentsMargins(0, 0, 0, 0)
            cell_layout.addStretch(1)
            cell_layout.addWidget(checkbox)
            cell_layout.addStretch(1)
            table.setCellWidget(i, _CHECK_COLUMN, cell)

            number_item = QtWidgets.QTableWidgetItem(
                "" if row.info.tool_number is None else str(row.info.tool_number)
            )
            number_item.setTextAlignment(QtCore.Qt.AlignCenter)
            table.setItem(i, _JOB_COLUMN, QtWidgets.QTableWidgetItem(row.job_label))
            table.setItem(i, _TOOL_NUMBER_COLUMN, number_item)
            table.setItem(i, _TOOL_COLUMN, QtWidgets.QTableWidgetItem(row.info.label))
            status_cell = self._make_status_tree(row.info, i)
            if status_cell is None:
                table.setItem(
                    i, _STATUS_COLUMN, QtWidgets.QTableWidgetItem(_summary_text(row.info))
                )
            else:
                table.setCellWidget(i, _STATUS_COLUMN, status_cell)

        for column in (_CHECK_COLUMN, _JOB_COLUMN, _TOOL_NUMBER_COLUMN, _TOOL_COLUMN):
            table.resizeColumnToContents(column)
        table.setColumnWidth(_CHECK_COLUMN, max(table.columnWidth(_CHECK_COLUMN), 40))
        table.resizeRowsToContents()
        layout.addWidget(table, 1)

        header = table.horizontalHeader()
        self._select_all = QtWidgets.QCheckBox(header)
        self._select_all.setTristate(True)
        self._select_all.setCheckState(QtCore.Qt.Checked)
        self._select_all.setToolTip(translate("CAM_Job", "Select all"))
        self._select_all.clicked.connect(self._on_select_all_clicked)

        def _position_select_all():
            x = header.sectionViewportPosition(_CHECK_COLUMN)
            width = header.sectionSize(_CHECK_COLUMN)
            size = self._select_all.sizeHint()
            self._select_all.move(
                x + (width - size.width()) // 2, (header.height() - size.height()) // 2
            )

        header.sectionResized.connect(lambda *_args: _position_select_all())
        _position_select_all()
        self._select_all.show()

        self._buttons = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        self._update_button = self._buttons.button(QtWidgets.QDialogButtonBox.Ok)
        self._update_button.setText(translate("CAM_Job", "Update"))
        self._buttons.button(QtWidgets.QDialogButtonBox.Cancel).setDefault(True)
        self._buttons.button(QtWidgets.QDialogButtonBox.Cancel).setFocus()
        self._buttons.accepted.connect(self.accept)
        self._buttons.rejected.connect(self.reject)
        layout.addWidget(self._buttons)

    def _make_status_tree(self, info, row_index: int) -> Optional[QtWidgets.QWidget]:
        """Collapsed summary that expands into itemized change lines,
        confined to one table cell. Resizes itself and the table row on
        expand/collapse, since QTableWidget doesn't track a cell widget's
        size on its own.

        Presets-only rows have nothing to itemize (the one detail line
        just repeats the summary), so they get no tree at all - returns
        None and the caller falls back to a plain table item.
        """
        if not info.geometry_changes:
            return None

        tree = QtWidgets.QTreeWidget()
        tree.setHeaderHidden(True)
        tree.setColumnCount(1)
        tree.setFrameShape(QtWidgets.QFrame.NoFrame)
        tree.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        tree.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        tree.setFocusPolicy(QtCore.Qt.NoFocus)
        tree.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        tree.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        tree.setStyleSheet("QTreeWidget { background: transparent; border: none; }")

        top = QtWidgets.QTreeWidgetItem(tree, [_summary_text(info)])
        for line in _detail_lines(info):
            child = QtWidgets.QTreeWidgetItem(top, [line])
            child.setToolTip(0, line)

        def resize():
            tree.setFixedHeight(self._tree_content_height(tree))
            self._table.resizeRowToContents(row_index)

        tree.itemExpanded.connect(lambda _item: resize())
        tree.itemCollapsed.connect(lambda _item: resize())
        resize()

        container = QtWidgets.QWidget()
        container.setStyleSheet("background: transparent;")
        container_layout = QtWidgets.QVBoxLayout(container)
        container_layout.setContentsMargins(0, 0, 0, 0)
        container_layout.addStretch(1)
        container_layout.addWidget(tree)
        container_layout.addStretch(1)
        return container

    @staticmethod
    def _tree_content_height(tree: QtWidgets.QTreeWidget) -> int:
        row_height = tree.sizeHintForRow(0) or 20

        def visible_count(item) -> int:
            n = 1
            if item.isExpanded():
                for i in range(item.childCount()):
                    n += visible_count(item.child(i))
            return n

        total = sum(visible_count(tree.topLevelItem(i)) for i in range(tree.topLevelItemCount()))
        return total * row_height + 4

    def _on_row_checkbox_clicked(self, _checked) -> None:
        checked_count = sum(1 for cb, _ in self._checkboxes if cb.isChecked())
        if checked_count == 0:
            self._select_all.setCheckState(QtCore.Qt.Unchecked)
        elif checked_count == len(self._checkboxes):
            self._select_all.setCheckState(QtCore.Qt.Checked)
        else:
            self._select_all.setCheckState(QtCore.Qt.PartiallyChecked)
        self._update_button.setEnabled(checked_count > 0)

    def _on_select_all_clicked(self, _checked) -> None:
        checked = self._select_all.checkState() != QtCore.Qt.Unchecked
        self._select_all.setCheckState(QtCore.Qt.Checked if checked else QtCore.Qt.Unchecked)
        for checkbox, _ in self._checkboxes:
            checkbox.setChecked(checked)
        self._update_button.setEnabled(checked and bool(self._checkboxes))

    def checked_infos(self) -> list:
        return [row.info for checkbox, row in self._checkboxes if checkbox.isChecked()]


def _prompt_and_apply(doc, doc_label, job_data, parent=None) -> None:
    """Shared by the manual command and the load-time prompt: show the
    dialog, fully replace whatever's checked, recompute once at the end
    if anything was applied. RecomputesFrozen avoids a recompute pass
    per tool while relinking a batch of them."""
    dialog = _UpdateToolsDialog(doc_label, job_data, parent=parent or FreeCADGui.getMainWindow())
    if dialog.exec_() != QtWidgets.QDialog.Accepted:
        return

    doc.RecomputesFrozen = True
    try:
        applied = False
        for info in dialog.checked_infos():
            if not replace_tool_from_library(info.tc, cam_assets):
                continue
            applied = True
            new_tool = info.tc.Tool
            if getattr(new_tool, "ViewObject", None) and new_tool.ViewObject.Visibility:
                new_tool.ViewObject.Visibility = False
    finally:
        doc.RecomputesFrozen = False

    if applied:
        doc.recompute()


def update_tools_from_gui(job, parent=None) -> None:
    """Manual entry point from the Job's context menu: dry-run this one
    job, show the dialog if there's anything stale."""
    stale_infos = job_stale_tools(job, cam_assets)
    if not stale_infos:
        QtWidgets.QMessageBox.information(
            parent or FreeCADGui.getMainWindow(),
            translate("CAM_Job", "Update Tools from Library"),
            translate("CAM_Job", "This job has no tool updates available."),
        )
        return
    _prompt_and_apply(job.Document, job.Label, [(job.Label, stale_infos)], parent=parent)


def _is_job(obj) -> bool:
    import Path.Main.Job

    return hasattr(obj, "Proxy") and isinstance(obj.Proxy, Path.Main.Job.ObjectJob)


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
    job_data = []
    for job in jobs:
        try:
            stale_infos = job_stale_tools(job, cam_assets)
        except Exception as e:
            Path.Log.debug(
                f"UpdateDocumentToolsDlg: stale-tools check failed for '{job.Label}': {e}"
            )
            continue
        if stale_infos:
            job_data.append((job.Label, stale_infos))
    if job_data:
        _prompt_and_apply(doc, doc.Label, job_data)


class _JobLoadObserver:
    """Prompts once per document load if a Job has a stale tool.

    ``slotActivateDocument`` fires reliably on open (no "document loaded"
    signal exists), but can fire before the document settles, so the real
    check is deferred via ``_run_when_idle``. ``_checked_docs`` limits this
    to once per load; ``slotDeletedDocument`` clears it so a reopen re-checks.
    """

    def __init__(self):
        self._checked_docs = set()

    def slotActivateDocument(self, doc):
        if not tool_update_on_load_enabled():
            return
        if doc.Name in self._checked_docs:
            return
        self._checked_docs.add(doc.Name)
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
