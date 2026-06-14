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

from femsolver.elmer import elmertools

from . import base_femlogtaskpanel


class _TaskPanel(base_femlogtaskpanel._BaseWorkerTaskPanel):
    """
    The TaskPanel for run Elmer solver
    """

    def __init__(self, obj):
        # set Tool, form and observer before init base class
        elmertools.ElmerTools(obj)
        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/SolverElmer.ui"
        )
        self.observer = _Observer(self)

        super().__init__(obj)

        self.setup_connections()

    def setup_connections(self):
        super().setup_connections()

        QtCore.QObject.connect(
            self.form.cb_simulation_type,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.simulation_type_changed,
        )
        QtCore.QObject.connect(
            self.form.pb_solver_version, QtCore.SIGNAL("clicked()"), self.get_version
        )

        self.get_object_params()
        self.set_widgets()

    def get_object_params(self):
        self.simulation_type = self.obj.SimulationType

    def set_object_params(self):
        self.obj.SimulationType = self.simulation_type

    def set_widgets(self):
        "fill the widgets"
        super().set_widgets()

        self.simulation_type_enum = self.obj.getEnumerationsOfProperty("SimulationType")
        index = self.simulation_type_enum.index(self.simulation_type)
        self.form.cb_simulation_type.addItems(self.simulation_type_enum)
        self.form.cb_simulation_type.setCurrentIndex(index)

    def simulation_type_changed(self, index):
        self.simulation_type = self.simulation_type_enum[index]
        self.obj.SimulationType = self.simulation_type


class _Observer(base_femlogtaskpanel._WorkerObserver):
    def __init__(self, task):
        super().__init__(task)
        # define property groups to be observed
        self.groups = ["Solver", "Timestepping"]

    def slotChangedObject(self, observed, prop):
        super().slotChangedObject(observed, prop)
        # check if equation changes
        if observed in self.task.obj.Group:
            self.task.prepared = False
