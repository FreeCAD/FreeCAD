# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "Netgen preference page class"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui


class DlgSettingsNetgen(QtGui.QWidget):

    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":ui/DlgSettingsNetgen.ui")
        self.grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Netgen")

        QtCore.QObject.connect(
            self.form.fc_netgen_python_path,
            QtCore.SIGNAL("fileNameSelected(QString)"),
            self.netgen_python_path_selected,
        )

    def loadSettings(self):
        self.form.ckb_legacy.setChecked(self.grp.GetBool("UseLegacyNetgen", True))
        self.form.sb_threads.setValue(
            self.grp.GetInt("NumOfThreads", QtCore.QThread.idealThreadCount())
        )
        self.form.fc_netgen_python_path.setProperty(
            "fileName", self.grp.GetString("NetgenPythonPath", "")
        )

        self.populate_log_verbosity()

    def saveSettings(self):
        self.grp.SetBool("UseLegacyNetgen", self.form.ckb_legacy.isChecked())
        self.grp.SetInt("LogVerbosity", self.form.cb_log_verbosity.currentData())
        self.grp.SetInt("NumOfThreads", self.form.sb_threads.value())
        self.grp.SetString("NetgenPythonPath", self.form.fc_netgen_python_path.property("fileName"))

    def populate_log_verbosity(self):
        values = {
            "None": 0,
            "Least": 1,
            "Little": 2,
            "Moderate": 3,
            "Much": 4,
            "Most": 5,
        }

        for v in values:
            self.form.cb_log_verbosity.addItem(v, values[v])

        current = self.grp.GetInt("LogVerbosity", 2)
        index = self.form.cb_log_verbosity.findData(current)
        self.form.cb_log_verbosity.setCurrentIndex(index)

    def netgen_python_path_selected(self, path):
        if path and not QtCore.QStandardPaths.findExecutable(path):
            QtGui.QMessageBox.critical(
                None, "Netgen", self.tr("Executable '{}' not found").format(path)
            )
