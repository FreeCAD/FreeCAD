# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM solver Elmer document object"
__author__ = "Markus Hovorka, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package solver_elmer
#  \ingroup FEM
#  \brief solver Elmer object

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class SolverElmer(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::SolverElmer"

    def __init__(self, obj):
        super().__init__(obj)
        obj.addExtension("App::GroupExtensionPython")

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="CoordinateSystem",
                group="Solver",
                doc="Type of the analysis",
                value=[
                    "Cartesian",
                    "Cartesian 1D",
                    "Cartesian 2D",
                    "Cartesian 3D",
                    "Polar 2D",
                    "Polar 3D",
                    "Cylindric",
                    "Cylindric Symmetric",
                    "Axi Symmetric",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="BDFOrder",
                group="Timestepping",
                doc="Order of time stepping method 'BDF'",
                value={"value": 2, "min": 1, "max": 5},
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerList",
                name="OutputIntervals",
                group="Timestepping",
                doc="After how many time steps a result file is output",
                value=[1],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerList",
                name="TimestepIntervals",
                group="Timestepping",
                doc="List of times if Simulation Type\n" + "is either `Scanning` or `Transient`",
                value=[100],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloatList",
                name="TimestepSizes",
                group="Timestepping",
                doc="List of times steps if Simulation Type\n"
                + "is either `Scanning` or `Transient`",
                value=[0.1],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="SimulationType",
                group="Solver",
                doc="Simulation type",
                value=["Scanning", "Steady State", "Transient"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="SteadyStateMaxIterations",
                group="Solver",
                doc="Maximal steady state iterations",
                value=1,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="SteadyStateMinIterations",
                group="Solver",
                doc="Minimal steady state iterations",
                value=0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="BinaryOutput",
                group="Solver",
                doc="Save result in binary format",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="SaveGeometryIndex",
                group="Solver",
                doc="Save geometry IDs",
                value=False,
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
