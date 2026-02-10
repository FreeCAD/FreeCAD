# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver calculix ccx tools document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package solver_ccxtools
#  \ingroup FEM
#  \brief solver calculix ccx tools object

from .base_fempythonobject import _PropHelper
from .solver_calculix import SolverCalculiX


class SolverCcxTools(SolverCalculiX):
    """The Fem::FemSolver's Proxy python type, add solver specific properties"""

    Type = "Fem::SolverCcxTools"

    def __init__(self, obj):
        super().__init__(obj)

    def _get_properties(self):
        prop = super()._get_properties()

        # set analysis types supported by CcxTools solver
        for p in prop:
            if p.name == "AnalysisType":
                p.value = ["static", "frequency", "thermomech", "check", "buckling"]

        # remove unused properties
        prop = list(filter(lambda p: p.name != "ElectromagneticMode", prop))

        prop.append(
            _PropHelper(
                type="App::PropertyPath",
                name="WorkingDir",
                group="Solver",
                doc="Working directory for calculations.\n"
                + "Will only be used it is left blank in preferences",
                value="",
            )
        )

        return prop
