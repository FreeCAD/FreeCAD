# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import femtools.femutils as femutils

from .. import run
from .. import solverbase
from . import tasks

from .equations import heat
from .equations import elasticity
from .equations import electrostatic
from .equations import fluxsolver
from .equations import flow


def create(doc, name="ElmerSolver"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """Proxy for FemSolverElmers Document Object."""

    Type = "Fem::FemSolverObjectElmer"

    _EQUATIONS = {
        "Heat": heat,
        "Elasticity": elasticity,
        "Electrostatic": electrostatic,
        "Fluxsolver": fluxsolver,
        "Flow": flow,
    }

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.addProperty(
            "App::PropertyInteger", "SteadyStateMaxIterations",
            "Steady State", "")
        obj.addProperty(
            "App::PropertyInteger", "SteadyStateMinIterations",
            "Steady State", "")
        obj.addProperty(
            "App::PropertyLink", "ElmerResult",
            "Base", "", 4 | 8)
        obj.addProperty(
            "App::PropertyLink", "ElmerOutput",
            "Base", "", 4 | 8)

        obj.SteadyStateMaxIterations = 1
        obj.SteadyStateMinIterations = 0

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


class ViewProxy(solverbase.ViewProxy):
    """Proxy for FemSolverElmers View Provider."""

    def getIcon(self):
        return ":/icons/fem-solver-elmer.svg"

##  @}
