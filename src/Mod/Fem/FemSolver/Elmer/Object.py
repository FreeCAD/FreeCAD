# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "Elmer"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


import FemSolver.SolverBase
import FemMisc
import FemRun

import FemSolver.Elmer.Tasks as tasks
import Equations.Heat
import Equations.Elasticity
import Equations.Electrostatic
import Equations.Flow


def create(doc, name="ElmerSolver"):
    return FemMisc.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(FemSolver.SolverBase.Proxy):
    """Proxy for FemSolverElmers Document Object."""

    Type = "Fem::FemSolverObjectElmer"

    _EQUATIONS = {
        "Heat": Equations.Heat,
        "Elasticity": Equations.Elasticity,
        "Electrostatic": Equations.Electrostatic,
        "Flow": Equations.Flow,
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

    def createMachine(self, obj, directory):
        return FemRun.Machine(
            solver=obj, directory=directory,
            check=tasks.Check(),
            prepare=tasks.Prepare(),
            solve=tasks.Solve(),
            results=tasks.Results())

    def createEquation(self, doc, eqId):
        return self._EQUATIONS[eqId].create(doc)

    def isSupported(self, eqId):
        return eqId in self._EQUATIONS


class ViewProxy(FemSolver.SolverBase.ViewProxy):
    """Proxy for FemSolverElmers View Provider."""

    def getIcon(self):
        return ":/icons/fem-elmer.png"
