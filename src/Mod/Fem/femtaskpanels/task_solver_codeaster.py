# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Tim Swait <t.swait@sheffield.ac.uk>                *
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

__title__ = "FreeCAD task panel for Code Aster solver"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## @package task_solver_codeaster
#  \ingroup FEM
#  \brief task panel for Code Aster solver

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui

from femsolver.codeaster import codeastertools

# from femtools.femutils import getOutputWinColor

from . import base_femlogtaskpanel


class _TaskPanel(base_femlogtaskpanel._BaseWorkerTaskPanel):
    """
    The TaskPanel to run Code Aster solver
    """

    def __init__(self, obj):
        # set Tool, form and observer before init base class
        codeastertools.CodeAsterTools(obj)
        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/SolverCodeAster.ui"
        )
        self.observer = _Observer(self)

        super().__init__(obj)

        self.setup_connections()

    def setup_connections(self):
        super().setup_connections()

        QtCore.QObject.connect(
            self.form.pb_solver_version, QtCore.SIGNAL("clicked()"), self.get_version
        )

        self.get_object_params()
        self.set_widgets()

    def get_object_params(self):
        self.solver_precision = self.obj.SolverPrecision

    def set_object_params(self):
        self.obj.SolverPrecision = self.solver_precision

    def set_widgets(self):
        "fills the widgets"
        super().set_widgets()

    #        self.solver_type_enum = self.obj.getEnumerationsOfProperty("SolverType")
    #        index = self.solver_type_enum.index(self.solver_type)
    #        self.form.cb_solver_type.addItems(self.solver_type_enum)
    #        self.form.cb_solver_type.setCurrentIndex(index)

    def edit_input_clicked(self):
        file_filter = QtGui.QFileDialog.getOpenFileName(
            None, "Code Aster input files", self.obj.WorkingDirectory
        )
        self.tool.model_file = file_filter[0]
        if self.tool.model_file:
            super().edit_input_clicked()


class _Observer(base_femlogtaskpanel._WorkerObserver):
    def __init__(self, task):
        super().__init__(task)
        # define property groups to be observed
        self.groups = ["Solver", "ElementModel"]
