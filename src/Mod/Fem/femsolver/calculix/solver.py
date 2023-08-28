# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver object CalculiX"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package SolverCalculix
#  \ingroup FEM

import glob
import os

import FreeCAD

from . import tasks
from .. import run
from .. import solverbase
from femtools import femutils

if FreeCAD.GuiUp:
    import FemGui

ANALYSIS_TYPES = ["static", "frequency", "thermomech", "check", "buckling"]


def create(doc, name="SolverCalculiX"):
    return femutils.createObject(
        doc, name, Proxy, ViewProxy)


class Proxy(solverbase.Proxy):
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """

    Type = "Fem::SolverCalculix"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.Proxy = self
        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
        add_attributes(obj, ccx_prefs)

    def onDocumentRestored(self, obj):
        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
        # since it is needed for the ccxtools solver too
        # the method is implemented outside of the class
        # thus we need to pass the prefs
        on_restore_of_document(obj, ccx_prefs)

    def createMachine(self, obj, directory, testmode=False):
        return run.Machine(
            solver=obj, directory=directory,
            check=tasks.Check(),
            prepare=tasks.Prepare(),
            solve=tasks.Solve(),
            results=tasks.Results(),
            testmode=testmode)

    def editSupported(self):
        return True

    def edit(self, directory):
        pattern = os.path.join(directory, "*.inp")
        FreeCAD.Console.PrintMessage("{}\n".format(pattern))
        f = glob.glob(pattern)[0]
        FemGui.open(f)

    def execute(self, obj):
        return


class ViewProxy(solverbase.ViewProxy):
    pass


# ************************************************************************************************
# helper
# these methods are outside of the class to be able
# to use them from framework solver and ccxtools solver


def on_restore_of_document(obj, ccx_prefs):

    # ANALYSIS_TYPES
    # They have been extended. If file was saved with a old FC version
    # not all enum types are available, because they are saved in the FC file
    # thus refresh the list of known ANALYSIS_TYPES
    # print("onRestoredFromSuperClass")
    # print(obj.AnalysisType)
    # print(obj.getEnumerationsOfProperty("AnalysisType"))

    temp_analysis_type = obj.AnalysisType
    # self.add_properties(obj)
    obj.AnalysisType = ANALYSIS_TYPES
    if temp_analysis_type in ANALYSIS_TYPES:
        obj.AnalysisType = temp_analysis_type
    else:
        FreeCAD.Console.PrintWarning(
            "Analysis type {} not found. Standard is used.\n"
            .format(temp_analysis_type)
        )
        analysis_type = ccx_prefs.GetInt("AnalysisType", 0)
        obj.AnalysisType = ANALYSIS_TYPES[analysis_type]

    # add missing properties
    # for example BucklingFactors will be added
    # for all files created before buckle analysis was introduced
    add_attributes(obj, ccx_prefs)


def add_attributes(obj, ccx_prefs):

    if not hasattr(obj, "AnalysisType"):
        obj.addProperty(
            "App::PropertyEnumeration",
            "AnalysisType",
            "Fem",
            "Type of the analysis"
        )
        obj.AnalysisType = ANALYSIS_TYPES
        analysis_type = ccx_prefs.GetInt("AnalysisType", 0)
        obj.AnalysisType = ANALYSIS_TYPES[analysis_type]

    if not hasattr(obj, "GeometricalNonlinearity"):
        choices_geom_nonlinear = ["linear", "nonlinear"]
        obj.addProperty(
            "App::PropertyEnumeration",
            "GeometricalNonlinearity",
            "Fem",
            "Set geometrical nonlinearity"
        )
        obj.GeometricalNonlinearity = choices_geom_nonlinear
        nonlinear_geom = ccx_prefs.GetBool("NonlinearGeometry", False)
        if nonlinear_geom is True:
            obj.GeometricalNonlinearity = choices_geom_nonlinear[1]  # nonlinear
        else:
            obj.GeometricalNonlinearity = choices_geom_nonlinear[0]  # linear

    if not hasattr(obj, "MaterialNonlinearity"):
        choices_material_nonlinear = ["linear", "nonlinear"]
        obj.addProperty(
            "App::PropertyEnumeration",
            "MaterialNonlinearity",
            "Fem",
            "Set material nonlinearity (needs geometrical nonlinearity)"
        )
        obj.MaterialNonlinearity = choices_material_nonlinear
        obj.MaterialNonlinearity = choices_material_nonlinear[0]

    if not hasattr(obj, "EigenmodesCount"):
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "EigenmodesCount",
            "Fem",
            "Number of modes for frequency calculations"
        )
        noem = ccx_prefs.GetInt("EigenmodesCount", 10)
        obj.EigenmodesCount = (noem, 1, 100, 1)

    if not hasattr(obj, "EigenmodeLowLimit"):
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "EigenmodeLowLimit",
            "Fem",
            "Low frequency limit for eigenmode calculations"
        )
        ell = ccx_prefs.GetFloat("EigenmodeLowLimit", 0.0)
        obj.EigenmodeLowLimit = (ell, 0.0, 1000000.0, 10000.0)

    if not hasattr(obj, "EigenmodeHighLimit"):
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "EigenmodeHighLimit",
            "Fem",
            "High frequency limit for eigenmode calculations"
        )
        ehl = ccx_prefs.GetFloat("EigenmodeHighLimit", 1000000.0)
        obj.EigenmodeHighLimit = (ehl, 0.0, 1000000.0, 10000.0)

    if not hasattr(obj, "IterationsThermoMechMaximum"):
        help_string_IterationsThermoMechMaximum = (
            "Maximum Number of thermo mechanical iterations "
            "in each time step before stopping jobs"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "IterationsThermoMechMaximum",
            "Fem",
            help_string_IterationsThermoMechMaximum
        )
        niter = ccx_prefs.GetInt("AnalysisMaxIterations", 200)
        obj.IterationsThermoMechMaximum = niter

    if not hasattr(obj, "BucklingFactors"):
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "BucklingFactors",
            "Fem",
            "Calculates the lowest buckling modes to the corresponding buckling factors"
        )
        bckl = ccx_prefs.GetInt("BucklingFactors", 1)
        obj.BucklingFactors = bckl

    if not hasattr(obj, "TimeInitialStep"):
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "TimeInitialStep",
            "Fem",
            "Initial time steps"
        )
        ini = ccx_prefs.GetFloat("AnalysisTimeInitialStep", 1.0)
        obj.TimeInitialStep = ini

    if not hasattr(obj, "TimeEnd"):
        obj.addProperty(
            "App::PropertyFloatConstraint",
            "TimeEnd",
            "Fem",
            "End time analysis"
        )
        eni = ccx_prefs.GetFloat("AnalysisTime", 1.0)
        obj.TimeEnd = eni

    if not hasattr(obj, "ThermoMechSteadyState"):
        obj.addProperty(
            "App::PropertyBool",
            "ThermoMechSteadyState",
            "Fem",
            "Choose between steady state thermo mech or transient thermo mech analysis"
        )
        sted = ccx_prefs.GetBool("StaticAnalysis", True)
        obj.ThermoMechSteadyState = sted

    if not hasattr(obj, "IterationsControlParameterTimeUse"):
        obj.addProperty(
            "App::PropertyBool",
            "IterationsControlParameterTimeUse",
            "Fem",
            "Use the user defined time incrementation control parameter"
        )
        use_non_ccx_iterations_param = ccx_prefs.GetInt("UseNonCcxIterationParam", False)
        obj.IterationsControlParameterTimeUse = use_non_ccx_iterations_param

    if not hasattr(obj, "SplitInputWriter"):
        obj.addProperty(
            "App::PropertyBool",
            "SplitInputWriter",
            "Fem",
            "Split writing of ccx input file"
        )
        split = ccx_prefs.GetBool("SplitInputWriter", False)
        obj.SplitInputWriter = split

    if not hasattr(obj, "IterationsControlParameterIter"):
        control_parameter_iterations = (
            "{I_0},{I_R},{I_P},{I_C},{I_L},{I_G},{I_S},{I_A},{I_J},{I_T}".format(
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
            )
        )
        obj.addProperty(
            "App::PropertyString",
            "IterationsControlParameterIter",
            "Fem",
            "User defined time incrementation iterations control parameter"
        )
        obj.IterationsControlParameterIter = control_parameter_iterations

    if not hasattr(obj, "IterationsControlParameterCutb"):
        control_parameter_cutback = (
            "{D_f},{D_C},{D_B},{D_A},{D_S},{D_H},{D_D},{W_G}".format(
                D_f=0.25,
                D_C=0.5,
                D_B=0.75,
                D_A=0.85,
                D_S="",
                D_H="",
                D_D=1.5,
                W_G="",
            )
        )
        obj.addProperty(
            "App::PropertyString",
            "IterationsControlParameterCutb",
            "Fem",
            "User defined time incrementation cutbacks control parameter"
        )
        obj.IterationsControlParameterCutb = control_parameter_cutback

    if not hasattr(obj, "IterationsUserDefinedIncrementations"):
        stringIterationsUserDefinedIncrementations = (
            "Set to True to switch off the ccx automatic incrementation completely "
            "(ccx parameter DIRECT). Use with care. Analysis may not converge!"
        )
        obj.addProperty(
            "App::PropertyBool",
            "IterationsUserDefinedIncrementations",
            "Fem",
            stringIterationsUserDefinedIncrementations
        )
        obj.IterationsUserDefinedIncrementations = False

    if not hasattr(obj, "IterationsUserDefinedTimeStepLength"):
        help_string_IterationsUserDefinedTimeStepLength = (
            "Set to True to use the user defined time steps. "
            "The time steps are set with TimeInitialStep and TimeEnd"
        )
        obj.addProperty(
            "App::PropertyBool",
            "IterationsUserDefinedTimeStepLength",
            "Fem",
            help_string_IterationsUserDefinedTimeStepLength
        )
        obj.IterationsUserDefinedTimeStepLength = False

    if not hasattr(obj, "MatrixSolverType"):
        known_ccx_solver_types = [
            "default",
            "spooles",
            "iterativescaling",
            "iterativecholesky"
        ]
        obj.addProperty(
            "App::PropertyEnumeration",
            "MatrixSolverType",
            "Fem",
            "Type of solver to use"
        )
        obj.MatrixSolverType = known_ccx_solver_types
        solver_type = ccx_prefs.GetInt("Solver", 0)
        obj.MatrixSolverType = known_ccx_solver_types[solver_type]

    if not hasattr(obj, "BeamShellResultOutput3D"):
        obj.addProperty(
            "App::PropertyBool",
            "BeamShellResultOutput3D",
            "Fem",
            "Output 3D results for 1D and 2D analysis "
        )
        dimout = ccx_prefs.GetBool("BeamShellOutput", False)
        obj.BeamShellResultOutput3D = dimout


"""
Should there be some equation object for Calculix too?

Necessarily yes! The properties GeometricalNonlinearity,
MaterialNonlinearity, ThermoMechSteadyState might be moved
to the appropriate equation.

Furthermore the material Category should not be used in writer.
See common material object for more information. The equation
should used instead to get this information needed in writer.
"""
