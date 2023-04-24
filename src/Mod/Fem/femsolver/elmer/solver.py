# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM solver object Elmer"
__author__ = "Markus Hovorka"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import glob
import os

import FreeCAD

from . import tasks
from .equations import deformation
from .equations import elasticity
from .equations import electricforce
from .equations import electrostatic
from .equations import flow
from .equations import flux
from .equations import heat
from .equations import magnetodynamic
from .equations import magnetodynamic2D
from .. import run
from .. import solverbase
from femtools import femutils

if FreeCAD.GuiUp:
    import FemGui

COORDINATE_SYSTEM = ["Cartesian", "Cartesian 1D", "Cartesian 2D", "Cartesian 3D",
                     "Polar 2D", "Polar 3D",
                     "Cylindric", "Cylindric Symmetric",
                     "Axi Symmetric"]
SIMULATION_TYPE = ["Scanning", "Steady State", "Transient"]


def create(doc, name="ElmerSolver"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """Proxy for FemSolverElmers Document Object."""

    Type = "Fem::SolverElmer"

    _EQUATIONS = {
        "Deformation": deformation,
        "Elasticity": elasticity,
        "Electrostatic": electrostatic,
        "Flux": flux,
        "Electricforce": electricforce,
        "Flow": flow,
        "Heat": heat,
        "Magnetodynamic": magnetodynamic,
        "Magnetodynamic2D": magnetodynamic2D,
    }

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

        obj.addProperty(
            "App::PropertyEnumeration",
            "CoordinateSystem",
            "Coordinate System",
            ""
        )
        obj.CoordinateSystem = COORDINATE_SYSTEM
        obj.CoordinateSystem = "Cartesian"

        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "BDFOrder",
            "Timestepping",
            "Order of time stepping method 'BDF'"
        )
        # according to the Elmer manual recommended is order 2
        # possible ranage is 1 - 5
        obj.BDFOrder = (2, 1, 5, 1)

        obj.addProperty(
            "App::PropertyIntegerList",
            "OutputIntervals",
            "Timestepping",
            "After how many time steps a result file is output"
        )
        obj.OutputIntervals = [1]

        obj.addProperty(
            "App::PropertyIntegerList",
            "TimestepIntervals",
            "Timestepping",
            (
                "List of times if 'Simulation Type'\n"
                "is either 'Scanning' or 'Transient'"
            )
        )
        obj.addProperty(
            "App::PropertyFloatList",
            "TimestepSizes",
            "Timestepping",
            (
                "List of time steps sizes if 'Simulation Type'\n"
                "is either 'Scanning' or 'Transient'"
            )
        )
        # there is no universal default, it all depends on the analysis, however
        # we have to set something and set 10 seconds in steps of 0.1s
        obj.TimestepIntervals = [100]
        obj.TimestepSizes = [0.1]

        obj.addProperty(
            "App::PropertyEnumeration",
            "SimulationType",
            "Type",
            ""
        )
        obj.SimulationType = SIMULATION_TYPE
        obj.SimulationType = "Steady State"

        obj.addProperty(
            "App::PropertyInteger",
            "SteadyStateMaxIterations",
            "Type",
            "Maximal steady state iterations"
        )
        obj.SteadyStateMaxIterations = 1

        obj.addProperty(
            "App::PropertyInteger",
            "SteadyStateMinIterations",
            "Type",
            "Minimal steady state iterations"
        )
        obj.SteadyStateMinIterations = 0

        obj.addProperty(
            "App::PropertyLink",
            "ElmerResult",
            "Base",
            "",
            4 | 8
        )

        obj.addProperty(
            "App::PropertyLinkList",
            "ElmerTimeResults",
            "Base",
            "",
            4 | 8
        )

        obj.addProperty(
            "App::PropertyLink",
            "ElmerOutput",
            "Base",
            "",
            4 | 8
        )

    def createMachine(self, obj, directory, testmode=False):
        return run.Machine(
            solver=obj, directory=directory,
            check=tasks.Check(),
            prepare=tasks.Prepare(),
            solve=tasks.Solve(),
            results=tasks.Results(),
            testmode=testmode)

    def createEquation(self, doc, eqId):
        return self._EQUATIONS[eqId].create(doc)

    def isSupported(self, eqId):
        return eqId in self._EQUATIONS

    def editSupported(self):
        return True

    def edit(self, directory):
        pattern = os.path.join(directory, "case.sif")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)


class ViewProxy(solverbase.ViewProxy):
    """Proxy for FemSolverElmers View Provider."""

    def getIcon(self):
        return ":/icons/FEM_SolverElmer.svg"

##  @}
