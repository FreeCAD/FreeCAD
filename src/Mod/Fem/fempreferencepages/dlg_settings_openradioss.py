# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
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

"""Edit > Preferences > FEM > OpenRadioss: solver folder, requirements check, download."""

__title__ = "FreeCAD FEM OpenRadioss preference page"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

import html
import os
import tempfile

from PySide import QtCore
from PySide import QtGui

import FreeCAD

from femsolver.openradioss import runner

translate = FreeCAD.Qt.translate

OK, BAD = "#3c9a3c", "#d9534f"


def _tr(text):
    return translate("DlgSettingsOpenRadioss", text)


class DlgSettingsOpenRadioss:
    """Preference page, registered with FreeCADGui.addPreferencePage in InitGui.py."""

    def __init__(self, parent=None):
        self.grp = FreeCAD.ParamGet(runner.PARAM_PATH)
        self.form = QtGui.QWidget(parent)
        self.form.setWindowTitle(_tr("OpenRadioss"))
        layout = QtGui.QVBoxLayout(self.form)

        solver = QtGui.QGroupBox(_tr("Solver: OpenCourant / OpenRadioss"))
        grid = QtGui.QGridLayout(solver)
        intro = QtGui.QLabel(
            _tr(
                "The OpenRadioss FEM solver runs the starter, engine and anim_to_vtk of an "
                "unpacked <a href='{}'>OpenCourant</a> release (the community continuation "
                "of OpenRadioss; OpenRadioss releases work too)."
            ).format(runner.RELEASES_PAGE)
        )
        intro.setWordWrap(True)
        intro.setOpenExternalLinks(True)
        grid.addWidget(intro, 0, 0, 1, 4)

        grid.addWidget(QtGui.QLabel(_tr("Solver folder:")), 1, 0)
        self.path_edit = QtGui.QLineEdit()
        self.path_edit.setPlaceholderText(_tr("folder that contains exec/ and hm_cfg_files/"))
        self.path_edit.textChanged.connect(self.refresh)
        grid.addWidget(self.path_edit, 1, 1)
        browse = QtGui.QPushButton(_tr("Browse..."))
        browse.clicked.connect(self.browse)
        grid.addWidget(browse, 1, 2)
        detect = QtGui.QPushButton(_tr("Auto-detect"))
        detect.setToolTip(
            _tr("Looks in: {}").format(", ".join(runner.candidate_paths(environ={})))
            + ", $OPENCOURANT_PATH, $OPENRADIOSS_PATH"
        )
        detect.clicked.connect(self.auto_detect)
        grid.addWidget(detect, 1, 3)

        self.requisites_label = QtGui.QLabel()
        self.requisites_label.setTextFormat(QtCore.Qt.RichText)
        self.requisites_label.setWordWrap(True)
        self.requisites_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        grid.addWidget(self.requisites_label, 2, 0, 1, 4)

        buttons = QtGui.QHBoxLayout()
        self.test_button = QtGui.QPushButton(_tr("Test installation"))
        self.test_button.setToolTip(
            _tr(
                "Runs the starter on a small deck (a few seconds). This also finds reader "
                "configuration files that do not match the executables."
            )
        )
        self.test_button.clicked.connect(self.test_installation)
        buttons.addWidget(self.test_button)
        download = QtGui.QPushButton(_tr("Download OpenCourant..."))
        download.setToolTip(
            _tr("Downloads and unpacks the newest release from {}").format(runner.RELEASES_PAGE)
        )
        download.clicked.connect(self.download)
        buttons.addWidget(download)
        buttons.addStretch()
        grid.addLayout(buttons, 3, 0, 1, 4)
        self.test_label = QtGui.QLabel()
        self.test_label.setWordWrap(True)
        self.test_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        grid.addWidget(self.test_label, 4, 0, 1, 4)
        grid.setColumnStretch(1, 1)
        layout.addWidget(solver)
        layout.addStretch()

    # --- FreeCAD preference page interface ---------------------------
    def loadSettings(self):
        path = self.grp.GetString("SolverPath", "")
        self.path_edit.setText(path or runner.get_installation().path)
        self.refresh()

    def saveSettings(self):
        self.grp.SetString("SolverPath", self.path_edit.text().strip())

    # --- actions -------------------------------------------------------
    def installation(self):
        return runner.Installation(self.path_edit.text().strip() or runner.DEFAULT_INSTALL)

    def refresh(self):
        rows = []
        for name, ok, detail in self.installation().requisites():
            mark = (
                f"<span style='color:{OK if ok else BAD}'>{'&#10004;' if ok else '&#10008;'}</span>"
            )
            rows.append(
                f"<tr><td>{mark}</td><td>{html.escape(name)}</td>"
                f"<td><small>{html.escape(detail)}</small></td></tr>"
            )
        self.requisites_label.setText("<table cellspacing='4'>" + "".join(rows) + "</table>")
        self.test_label.clear()

    def browse(self):
        start = self.path_edit.text() or os.path.expanduser("~")
        path = QtGui.QFileDialog.getExistingDirectory(
            self.form, _tr("Solver folder (contains exec/)"), start
        )
        if path:
            self.path_edit.setText(path)

    def auto_detect(self):
        found = runner.detect_installation()
        if found is None:
            text = _tr("No installation found in: {}").format(", ".join(runner.candidate_paths()))
            self.test_label.setText(f"<span style='color:{BAD}'>{html.escape(text)}</span>")
            return
        self.path_edit.setText(found.path)

    def test_installation(self):
        self.test_label.setText(_tr("Running the starter..."))
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        try:
            QtGui.QApplication.processEvents()
            ok, message = self.installation().test_run()
        finally:
            QtGui.QApplication.restoreOverrideCursor()
        self.refresh()  # the Flatpak libraries may have been copied
        self.test_label.setText(
            f"<span style='color:{OK if ok else BAD}'>{html.escape(message)}</span>"
        )
        return ok

    def download(self):
        title = _tr("Download OpenCourant")
        parent = QtGui.QFileDialog.getExistingDirectory(
            self.form, _tr("Folder to unpack OpenCourant into"), os.path.expanduser("~")
        )
        if not parent:
            return
        try:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            try:
                tag, url, size = runner.latest_release()
            finally:
                QtGui.QApplication.restoreOverrideCursor()
        except Exception as e:
            QtGui.QMessageBox.warning(
                self.form,
                title,
                _tr("Cannot download OpenCourant:\n{}\n\nReleases: {}").format(
                    e, runner.RELEASES_PAGE
                ),
            )
            return
        target = os.path.join(parent, f"OpenCourant_{tag}")
        answer = QtGui.QMessageBox.question(
            self.form,
            title,
            _tr(
                "Download OpenCourant {} ({} MB, AGPL-3.0) from GitHub and unpack it to\n{}?"
            ).format(tag, round(size / 1e6), target),
        )
        if answer != QtGui.QMessageBox.Yes:
            return

        progress = QtGui.QProgressDialog(
            _tr("Downloading OpenCourant {}...").format(tag), _tr("Cancel"), 0, 100, self.form
        )
        progress.setWindowModality(QtCore.Qt.WindowModal)
        progress.setMinimumDuration(0)

        def report(done, total):
            if total:
                progress.setValue(int(100 * done / total))
                progress.setLabelText(
                    _tr("Downloading OpenCourant {}: {} of {} MB").format(
                        tag, round(done / 1e6), round(total / 1e6)
                    )
                )
            QtGui.QApplication.processEvents()
            return not progress.wasCanceled()

        try:
            with tempfile.TemporaryDirectory(prefix="fem_openradioss_download_") as tmp:
                archive = runner.download(url, os.path.join(tmp, os.path.basename(url)), report)
                progress.setLabelText(_tr("Unpacking..."))
                progress.setRange(0, 0)
                QtGui.QApplication.processEvents()
                path = runner.extract_release(archive, target)
        except Exception as e:
            progress.close()
            QtGui.QMessageBox.warning(self.form, title, _tr("Download failed:\n{}").format(e))
            return
        progress.close()
        FreeCAD.Console.PrintMessage(f"OpenRadioss: OpenCourant {tag} unpacked to {path}\n")
        self.path_edit.setText(path)
        self.test_installation()
