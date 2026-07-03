# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM solver Z88 document object"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package solver_z88
#  \ingroup FEM
#  \brief solver Z88 object

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class SolverZ88(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::SolverZ88"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="AnalysisType",
                group="Solver",
                doc="Type of the analysis",
                value=["static", "test"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="DisplaceMesh",
                group="Solver",
                doc="Deform the mesh by the displacement field",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="SolverType",
                group="Solver",
                doc="Type of solver to use",
                value=["choly", "sorcg", "siccg"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ModelSpace",
                group="ElementModel",
                doc="Type of model space",
                value=["3D", "plane stress", "axisymmetric", "plate"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ExcludeBendingStiffness",
                group="ElementModel",
                doc="Exclude bending stiffness to replace beams with trusses",
                read_only=True,
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="IntegrationOrderQuad",
                group="Solver",
                doc="Displacement integration order for quad elements",
                value=[2, 3, 4],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="IntegrationOrderHexa",
                group="Solver",
                doc="Displacement integration order for hexa elements",
                value=[1, 2, 3, 4],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="IntegrationOrderTria",
                group="Solver",
                doc="Displacement integration order for triangle elements",
                value=[1, 7, 13],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="IntegrationOrderTetra",
                group="Solver",
                doc="Displacement integration order for tetra elements",
                value=[1, 4, 5],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloatConstraint",
                name="RelaxationFactor",
                group="Solver",
                doc="ROMEGA value. Convergence acceleration parameter for SOR pre-conditioner",
                value={"value": 1.0, "min": 0.0, "max": 2.0, "step": 0.01},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloatConstraint",
                name="ShiftFactor",
                group="Solver",
                doc="RALPHA value. Convergence acceleration parameter for SIC pre-conditioner",
                value={"value": 0.0001, "min": 0.0, "max": 1.0, "step": 0.0001},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="IterationMaximum",
                group="Solver",
                doc="MAXIT value. Maximum number of iterations for iterative solvers",
                value={"value": 10000, "min": 1},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="ResidualLimit",
                group="Solver",
                doc="EPS value. It is compared to the norm of the residual vector",
                value=1e-7,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="ShellFlag",
                group="ElementModel",
                doc="IHFLAG value. Only has effect if there is shell elements in the structure.\n"
                + "Set to `2` or `3` for thin shells.\n"
                + "set to `4` in case of very thin shells",
                value={"value": 1, "min": 1, "max": 4},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="MatrixMaximum",
                group="Solver",
                doc="MAXGS value. Maximum number of entries in the stiffness matrix",
                value={"value": 100000000, "min": 1},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="VectorMaximum",
                group="Solver",
                doc="MAXKOI value. Maximum number of entries in the coincidence vector",
                value={"value": 2800000, "min": 1},
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            # Migrate group of properties for old projects
            if obj.getGroupOfProperty(prop.name) != prop.group:
                obj.setGroupOfProperty(prop.name, prop.group)
