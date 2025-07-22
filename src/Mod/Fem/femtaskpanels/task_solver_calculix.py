# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD task panel for CalculiX solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_solver_calculix
#  \ingroup FEM
#  \brief task panel for CalculiX solver

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

import FemGui

from femsolver.calculix import calculixtools

from . import base_femlogtaskpanel


class _TaskPanel(base_femlogtaskpanel._BaseLogTaskPanel):
    """
    The TaskPanel for run CalculiX solver
    """

    def __init__(self, obj):
        super().__init__(obj, calculixtools.CalculiXTools(obj))

        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/SolverCalculiX.ui"
        )

        self.text_log = self.form.te_output
        self.text_time = self.form.l_time
        self.prepared = False
        self.run_complete = False

        self.setup_connections()

    def setup_connections(self):
        super().setup_connections()

        QtCore.QObject.connect(
            self.form.ckb_working_directory,
            QtCore.SIGNAL("toggled(bool)"),
            self.working_directory_toggled,
        )
        QtCore.QObject.connect(
            self.form.cb_analysis_type,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.analysis_type_changed,
        )
        QtCore.QObject.connect(
            self.form.pb_write_input, QtCore.SIGNAL("clicked()"), self.write_input_clicked
        )
        QtCore.QObject.connect(
            self.form.pb_edit_input, QtCore.SIGNAL("clicked()"), self.edit_input_clicked
        )
        QtCore.QObject.connect(
            self.form.pb_working_directory,
            QtCore.SIGNAL("clicked()"),
            self.working_directory_clicked,
        )
        QtCore.QObject.connect(
            self.form.pb_solver_version, QtCore.SIGNAL("clicked()"), self.get_version
        )
        QtCore.QObject.connect(
            self.form.let_working_directory,
            QtCore.SIGNAL("editingFinished()"),
            self.working_directory_edited,
        )

        self.get_object_params()
        self.set_widgets()

    def preparation_finished(self):
        # override base class method to not auto compute
        self.prepared = True
        if not self.run_complete:
            self.timer.stop()
            self.form.pb_edit_input.setEnabled(True)
        else:
            super().preparation_finished()

    def apply(self):
        self.text_log.clear()
        self.elapsed.restart()
        if self.prepared:
            self.timer.start(100)
            self.tool.compute()
        else:
            # run complete process if 'Apply' is pressed without
            # previously write the input files
            self.run_complete = True
            super().apply()

    def get_object_params(self):
        self.analysis_type = self.obj.AnalysisType

    def set_object_params(self):
        self.obj.AnalysisType = self.analysis_type

    def set_widgets(self):
        "fills the widgets"
        self.analysis_type_enum = self.obj.getEnumerationsOfProperty("AnalysisType")
        index = self.analysis_type_enum.index(self.analysis_type)
        self.form.cb_analysis_type.addItems(self.analysis_type_enum)
        self.form.cb_analysis_type.setCurrentIndex(index)

        self.form.let_working_directory.setText(self.obj.WorkingDirectory)
        self.form.ckb_working_directory.setChecked(False)
        self.form.gpb_working_directory.setVisible(False)

    def analysis_type_changed(self, index):
        self.analysis_type = self.analysis_type_enum[index]
        self.obj.AnalysisType = self.analysis_type

    def working_directory_clicked(self):
        directory = QtGui.QFileDialog.getExistingDirectory(dir=self.obj.WorkingDirectory)
        if directory:
            self.form.let_working_directory.setText(directory)
            self.form.let_working_directory.editingFinished.emit()

    def working_directory_edited(self):
        self.obj.WorkingDirectory = self.form.let_working_directory.text()

    def write_input_clicked(self):
        self.prepared = False
        self.run_complete = False
        self.run_process()

    def edit_input_clicked(self):
        ccx_param = self.tool.fem_param.GetGroup("Ccx")
        internal = ccx_param.GetBool("UseInternalEditor", True)
        ext_editor_path = ccx_param.GetString("ExternalEditorPath", "")
        if internal or not ext_editor_path:
            FemGui.open(self.tool.model_file)
        else:
            ext_editor_process = QtCore.QProcess()
            ext_editor_process.start(ext_editor_path, [self.tool.model_file])
            ext_editor_process.waitForFinished()

    def working_directory_toggled(self, bool_value):
        self.form.gpb_working_directory.setVisible(bool_value)
