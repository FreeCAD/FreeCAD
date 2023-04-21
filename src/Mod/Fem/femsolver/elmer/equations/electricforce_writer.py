# ***************************************************************************
# *   Copyright (c) 2020 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

__title__ = "FreeCAD FEM Electricforce Elmer writer"
__author__ = "Markus Hovorka, Uwe Stöhr"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

from .. import sifio
from . import electricforce


class EFwriter:

    def __init__(self, writer, solver):
        self.write = writer
        self.solver = solver

    def getElectricforceSolver(self, equation):
        # check if we need to update the equation
        self._updateElectricforceSolver(equation)
        # output the equation parameters
        s = self.write.createEmptySolver()
        s["Equation"] = "Electric Force"  # equation.Name
        s["Procedure"] = sifio.FileAttr("ElectricForce/StatElecForce")
        s["Exec Solver"] = equation.ExecSolver
        s["Stabilize"] = equation.Stabilize
        return s

    def _updateElectricforceSolver(self, equation):
        # updates older Electricforce equations
        if not hasattr(equation, "ExecSolver"):
            equation.addProperty(
                "App::PropertyEnumeration",
                "ExecSolver",
                "Electric Force",
                (
                    "That solver is only executed after solution converged\n"
                    "To execute always, change to 'Always'"
                )
            )
            equation.ExecSolver = electricforce.SOLVER_EXEC_METHODS
            equation.ExecSolver = "After Timestep"

##  @}
