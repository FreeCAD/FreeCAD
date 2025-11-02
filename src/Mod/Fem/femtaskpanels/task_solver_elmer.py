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

__title__ = "FreeCAD task panel for Elmer solver"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_solver_elmer
#  \ingroup FEM
#  \brief task panel for Elmer solver

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

import FemGui

from femsolver.elmer import elmertools

from . import base_femlogtaskpanel


class _TaskPanel(base_femlogtaskpanel._BaseLogTaskPanel):
    """
    The TaskPanel for run Elmer solver
    """

    def __init__(self, obj):
        super().__init__(obj, elmertools.ElmerTools(obj))

        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/SolverElmer.ui"
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
            self.form.cb_simulation_type,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.simulation_type_changed,
        )
        QtCore.QObject.connect(
            self.form.pb_write_input, QtCore.SIGNAL("clicked()"), self.write_input_clicked
        )
        QtCore.QObject.connect(
            self.form.pb_edit_input, QtCore.SIGNAL("clicked()"), self.edit_input_clicked
        )
        QtCore.QObject.connect(
            self.form.fc_working_directory,
            QtCore.SIGNAL("fileNameSelected(QString)"),
            self.working_directory_selected,
        )
        QtCore.QObject.connect(
            self.form.pb_solver_version, QtCore.SIGNAL("clicked()"), self.get_version
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
        self.simulation_type = self.obj.SimulationType

    def set_object_params(self):
        self.obj.SimulationType = self.simulation_type

    def set_widgets(self):
        "fill the widgets"
        self.simulation_type_enum = self.obj.getEnumerationsOfProperty("SimulationType")
        index = self.simulation_type_enum.index(self.simulation_type)
        self.form.cb_simulation_type.addItems(self.simulation_type_enum)
        self.form.cb_simulation_type.setCurrentIndex(index)

        self.form.fc_working_directory.setProperty("fileName", self.obj.WorkingDirectory)
        self.form.ckb_working_directory.setChecked(True)
        self.form.gpb_working_directory.setVisible(True)

    def simulation_type_changed(self, index):
        self.simulation_type = self.simulation_type_enum[index]
        self.obj.SimulationType = self.simulation_type

    def working_directory_selected(self):
        self.obj.WorkingDirectory = self.form.fc_working_directory.property("fileName")

    def write_input_clicked(self):
        self.prepared = False
        self.run_complete = False
        self.run_process()

    def edit_input_clicked(self):
        gen_param = self.tool.fem_param.GetGroup("General")
        internal = gen_param.GetBool("UseInternalEditor", True)
        ext_editor_path = gen_param.GetString("ExternalEditorPath", "")
        if internal or not ext_editor_path:
            FemGui.open(self.tool.model_file)
        else:
            ext_editor_process = QtCore.QProcess()
            ext_editor_process.start(ext_editor_path, [self.tool.model_file])
            ext_editor_process.waitForFinished()

    def working_directory_toggled(self, bool_value):
        self.form.gpb_working_directory.setVisible(bool_value)
