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

__title__ = "FreeCAD FEM solver object OpenRadioss"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

## @package SolverOpenRadioss
#  \ingroup FEM

import glob
import os

import FreeCAD

from . import tasks
from .. import run
from .. import solverbase
from femtools import femutils

if FreeCAD.GuiUp:
    import FemGui

ANALYSIS_TYPES = ["explicit"]
LOAD_CURVES = ["Ramp", "Constant"]


def create(doc, name="SolverOpenRadioss"):
    return femutils.createObject(doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """Explicit dynamic analysis with OpenRadioss."""

    Type = "Fem::SolverOpenRadioss"

    def __init__(self, obj):
        super().__init__(obj)
        obj.Proxy = self
        self.add_properties(obj)

    def onDocumentRestored(self, obj):
        self.add_properties(obj)  # documents saved by older versions get new properties

    @staticmethod
    def add_properties(obj):
        def add(prop_type, name, doc, value):
            if not hasattr(obj, name):
                obj.addProperty(prop_type, name, "Solver", doc)
                setattr(obj, name, value)

        if not hasattr(obj, "AnalysisType"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "AnalysisType",
                "Fem",
                "Type of the analysis",
                locked=True,
            )
            obj.AnalysisType = ANALYSIS_TYPES
            obj.AnalysisType = ANALYSIS_TYPES[0]
        add(
            "App::PropertyTime",
            "EndTime",
            "Simulated time. Explicit solvers need small time steps: "
            "keep it short or use mass scaling",
            "0.01 s",
        )
        add(
            "App::PropertyIntegerConstraint",
            "OutputFrames",
            "Number of result frames written over the simulated time",
            (20, 1, 10000, 1),
        )
        if not hasattr(obj, "LoadCurve"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "LoadCurve",
                "Solver",
                "Ramp: loads grow linearly from 0 to full over the end time (quasi-static); "
                "Constant: loads act fully from the start (impact-like)",
            )
            obj.LoadCurve = LOAD_CURVES
            obj.LoadCurve = LOAD_CURVES[0]
        add(
            "App::PropertyFloat",
            "MassScalingMicroseconds",
            "Mass scaling time step in microseconds (0 = off). Mass is added where needed "
            "so the time step stays at least this value: much faster runs, check the "
            "added mass in the listing",
            0.0,
        )
        cpus = os.cpu_count() or 1
        add(
            "App::PropertyIntegerConstraint",
            "Threads",
            "Threads used by starter and engine",
            (min(4, cpus), 1, cpus, 1),
        )
        add(
            "App::PropertyLink",
            "AdditionalKeywords",
            "Text document with extra LS-DYNA keywords appended to the deck",
            None,
        )

    def createMachine(self, obj, directory, testmode=False):
        return run.Machine(
            solver=obj,
            directory=directory,
            check=tasks.Check(),
            prepare=tasks.Prepare(),
            solve=tasks.Solve(),
            results=tasks.Results(),
            testmode=testmode,
        )

    def editSupported(self):
        return True

    def edit(self, directory):
        decks = glob.glob(os.path.join(directory, "*.k"))
        if not decks:
            FreeCAD.Console.PrintError(f"No deck written yet in {directory}\n")
            return
        FemGui.open(decks[0])

    def execute(self, obj):
        return


class ViewProxy(solverbase.ViewProxy):

    def getIcon(self):
        return ":/icons/FEM_SolverOpenRadioss.svg"
