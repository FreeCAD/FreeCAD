# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver CalculiX document object"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package solver_calculix
#  \ingroup FEM
#  \brief solver CalculiX object

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class SolverCalculiX(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::SolverCalculiX"

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
                value=["static", "frequency", "thermomech", "check", "buckling", "electromagnetic"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="GeometricalNonlinearity",
                group="Solver",
                doc="Set geometrical nonlinearity",
                value=["linear", "nonlinear"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="MaterialNonlinearity",
                group="Solver",
                doc="Set material nonlinearity",
                value=["linear", "nonlinear"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="EigenmodesCount",
                group="Solver",
                doc="Number of modes for frequency calculations",
                value=(10, 1, 100, 1),
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFrequency",
                name="EigenmodeLowLimit",
                group="Solver",
                doc="Low frequency limit for eigenmode calculations",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFrequency",
                name="EigenmodeHighLimit",
                group="Solver",
                doc="High frequency limit for eigenmode calculations",
                value=1000000.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="IterationsMaximum",
                group="Solver",
                doc="Maximum Number of iterations in each time step before stopping jobs",
                value=2000,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="BucklingFactors",
                group="Solver",
                doc="Calculates the lowest buckling modes to the corresponding buckling factors",
                value=1,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeInitialStep",
                group="Solver",
                doc="Initial time steps",
                value=0.01,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeEnd",
                group="Solver",
                doc="End time analysis",
                value=1.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeMinimumStep",
                group="Solver",
                doc="Minimum time step",
                value=0.00001,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeMaximumStep",
                group="Solver",
                doc="Maximum time step",
                value=1.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ThermoMechSteadyState",
                group="Solver",
                doc="Choose between steady state thermo mech or transient thermo mech analysis",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="IterationsControlParameterTimeUse",
                group="Solver",
                doc="Use the user defined time incrementation control parameter",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="SplitInputWriter",
                group="Solver",
                doc="Split writing of ccx input file",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="IterationsControlParameterIter",
                group="Solver",
                doc="User defined time incrementation iterations control parameter",
                value="{I_0},{I_R},{I_P},{I_C},{I_L},{I_G},{I_S},{I_A},{I_J},{I_T}".format(
                    I_0=4,
                    I_R=8,
                    I_P=9,
                    I_C=200,  # ccx default = 16
                    I_L=10,
                    I_G=400,  # ccx default = 4
                    I_S="",
                    I_A=200,  # ccx default = 5
                    I_J="",
                    I_T="",
                ),
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="IterationsControlParameterCutb",
                group="Solver",
                doc="User defined time incrementation cutbacks control parameter",
                value="{D_f},{D_C},{D_B},{D_A},{D_S},{D_H},{D_D},{W_G}".format(
                    D_f=0.25,
                    D_C=0.5,
                    D_B=0.75,
                    D_A=0.85,
                    D_S="",
                    D_H="",
                    D_D=1.5,
                    W_G="",
                ),
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="IterationsUserDefinedIncrementations",
                group="Solver",
                doc="Set to True to switch off the ccx automatic incrementation completely\n"
                + "(ccx parameter DIRECT). Use with care. Analysis may not converge!",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="IterationsUserDefinedTimeStepLength",
                group="Solver",
                doc="Set to True to use the user defined time steps.\n"
                + "They are set with TimeInitialStep, TimeEnd, TimeMinimum and TimeMaximum",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="MatrixSolverType",
                group="Solver",
                doc="Type of solver to use",
                value=[
                    "default",
                    "pastix",
                    "pardiso",
                    "spooles",
                    "iterativescaling",
                    "iterativecholesky",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="BeamShellResultOutput3D",
                group="Solver",
                doc="Output 3D results for 1D and 2D analysis ",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="BeamReducedIntegration",
                group="Solver",
                doc="Set to True to use beam elements with reduced integration",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="OutputFrequency",
                group="Solver",
                doc="Set the output frequency in increments",
                value=1,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ModelSpace",
                group="Solver",
                doc="Type of model space",
                value=["3D", "plane stress", "plane strain", "axisymmetric"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ThermoMechType",
                group="Solver",
                doc="Type of thermomechanical analysis",
                value=["coupled", "uncoupled", "pure heat transfer"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloatConstraint",
                name="BucklingAccuracy",
                group="Solver",
                doc="Accuracy for buckling analysis",
                value=0.01,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ElectromagneticMode",
                group="Solver",
                doc="Electromagnetic mode",
                value=["electrostatic"],
            )
        )

        return prop
