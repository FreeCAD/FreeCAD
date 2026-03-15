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

from FreeCAD import Base
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
                group="AnalysisType",
                doc="Type of the analysis",
                value=["static", "frequency", "thermomech", "check", "buckling", "electromagnetic"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="GeometricalNonlinearity",
                group="Solver",
                doc="Use geometrical nonlinearity",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="MaterialNonlinearity",
                group="Solver",
                doc="If available, use nonlinear material properties",
                value=True,
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
                name="IncrementsMaximum",
                group="TimeIncrement",
                doc="Maximum Number of increments in each CalculiX step.\n"
                + "Set to 0 to use CalculiX default value",
                value={"value": 2000, "min": 0},
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
                name="TimeInitialIncrement",
                group="TimeIncrement",
                doc="Initial time increment",
                value=1.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimePeriod",
                group="TimeIncrement",
                doc="Time period of the CalculiX step",
                value=1.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeMinimumIncrement",
                group="TimeIncrement",
                doc="Minimum time increment",
                value=0.00001,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyTime",
                name="TimeMaximumIncrement",
                group="TimeIncrement",
                doc="Maximum time increment",
                value=1.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ThermoMechSteadyState",
                group="AnalysisType",
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
                name="AutomaticIncrementation",
                group="TimeIncrement",
                doc="If False, switch off automatic incrementation via `DIRECT`\n"
                + "parameter and ignore minimum and maximum time increments.\n"
                + "Analysis may not converge!",
                value=True,
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
                group="ElementModel",
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
                group="ElementModel",
                doc="Type of model space",
                value=["3D", "plane stress", "plane strain", "axisymmetric"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ThermoMechType",
                group="AnalysisType",
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
                group="AnalysisType",
                doc="Electromagnetic mode",
                value=["electrostatic"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ExcludeBendingStiffness",
                group="ElementModel",
                doc="Exclude bending stiffness to replace shells with membranes or beams with trusses",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="PastixMixedPrecision",
                group="Solver",
                doc="Mixed precision for the PaStiX matrix solver",
                value=False,
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

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            # change GeometricalNonlinearity and MaterialNonlinearity types
            if prop.name == "MaterialNonlinearity":
                prop.handle_change_type(
                    obj, "App::PropertyEnumeration", lambda x: False if x == "linear" else True
                )
            elif prop.name == "GeometricalNonlinearity":
                prop.handle_change_type(
                    obj, "App::PropertyEnumeration", lambda x: False if x == "linear" else True
                )
        # remove old properties
        try:
            obj.AutomaticIncrementation = not obj.getPropertyByName(
                "IterationsUserDefinedIncrementations"
            )
            obj.setPropertyStatus("IterationsUserDefinedIncrementations", "-LockDynamic")
            obj.removeProperty("IterationsUserDefinedIncrementations")
            obj.setPropertyStatus("IterationsUserDefinedTimeStepLength", "-LockDynamic")
            obj.removeProperty("IterationsUserDefinedTimeStepLength")

            obj.TimeInitialIncrement = obj.getPropertyByName("TimeInitialStep")
            obj.setPropertyStatus("TimeInitialStep", "-LockDynamic")
            obj.removeProperty("TimeInitialStep")

            obj.TimePeriod = obj.getPropertyByName("TimeEnd")
            obj.setPropertyStatus("TimeEnd", "-LockDynamic")
            obj.removeProperty("TimeEnd")

            obj.TimeMaximumIncrement = obj.getPropertyByName("TimeMaximumStep")
            obj.setPropertyStatus("TimeMaximumStep", "-LockDynamic")
            obj.removeProperty("TimeMaximumStep")

            obj.TimeMinimumIncrement = obj.getPropertyByName("TimeMinimumStep")
            obj.setPropertyStatus("TimeMinimumStep", "-LockDynamic")
            obj.removeProperty("TimeMinimumStep")

            obj.IncrementsMaximum = obj.getPropertyByName("IterationsMaximum")
            obj.setPropertyStatus("IterationsMaximum", "-LockDynamic")
            obj.removeProperty("IterationsMaximum")

        except Base.PropertyError:
            # do nothing
            pass
