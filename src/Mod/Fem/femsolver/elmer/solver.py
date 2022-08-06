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
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import glob
import os

import FreeCAD

from . import tasks
from .equations import elasticity
from .equations import electrostatic
from .equations import flow
from .equations import flux
from .equations import electricforce
from .equations import heat
from .. import run
from .. import solverbase
from femtools import femutils

if FreeCAD.GuiUp:
    import FemGui

SIMULATION_TYPE = ["Scanning", "Steady State", "Transient"]

def create(doc, name="ElmerSolver"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """Proxy for FemSolverElmers Document Object."""

    Type = "Fem::SolverElmer"

    _EQUATIONS = {
        "Heat": heat,
        "Elasticity": elasticity,
        "Electrostatic": electrostatic,
        "Flux": flux,
        "Electricforce": electricforce,
        "Flow": flow,
    }

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)

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
            "App::PropertyIntegerConstraint",
            "TimestepIntervals",
            "Timestepping",
            "Maximum optimization rounds if 'Simulation Type'\nis either 'Scanning' or 'Transient'"
        )
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "TimestepSizes",
            "Timestepping",
            "Time step of optimization if 'Simulation Type'\nis either 'Scanning' or 'Transient'"
        )
        # there is no universal default, it all depends on the analysis, however
        # we have to set something and set 10 seconds in steps of 0.1s
        # since the Emler manual lacks proper info, here a link to a forum thread:
        # http://www.elmerfem.org/forum/viewtopic.php?p=18057&sid=73169c4ec544fd7f181f85178bbc8ffe#p18057
        # -----
        # Set maximum to 1e8 because on Win the max int is always 32bit (4.29e9)
        # for TimestepSizes also 1e8 just to set something
        obj.TimestepIntervals = (100, 1, int(1e8), 10)
        obj.TimestepSizes = (0.1, 1e-8, 1e8, 0.1)

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
