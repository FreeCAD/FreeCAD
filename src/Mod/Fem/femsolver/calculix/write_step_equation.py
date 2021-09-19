# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM calculix write inpfile step equation"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


import FreeCAD


def write_step_equation(f, ccxwriter):

    f.write("\n{}\n".format(59 * "*"))
    f.write("** At least one step is needed to run an CalculiX analysis of FreeCAD\n")

    # build STEP line
    step = "*STEP"
    if ccxwriter.solver_obj.GeometricalNonlinearity == "nonlinear":
        if ccxwriter.analysis_type == "static" or ccxwriter.analysis_type == "thermomech":
            # https://www.comsol.com/blogs/what-is-geometric-nonlinearity
            step += ", NLGEOM"
        elif ccxwriter.analysis_type == "frequency":
            FreeCAD.Console.PrintMessage(
                "Analysis type frequency and geometrical nonlinear "
                "analysis are not allowed together, linear is used instead!\n"
            )
    if ccxwriter.solver_obj.IterationsThermoMechMaximum:
        if ccxwriter.analysis_type == "thermomech":
            step += ", INC={}".format(ccxwriter.solver_obj.IterationsThermoMechMaximum)
        elif (
            ccxwriter.analysis_type == "static"
            or ccxwriter.analysis_type == "frequency"
            or ccxwriter.analysis_type == "buckling"
        ):
            # parameter is for thermomechanical analysis only, see ccx manual *STEP
            pass
    # write STEP line
    f.write(step + "\n")

    # CONTROLS line
    # all analysis types, ... really in frequency too?!?
    if ccxwriter.solver_obj.IterationsControlParameterTimeUse:
        f.write("*CONTROLS, PARAMETERS=TIME INCREMENTATION\n")
        f.write(ccxwriter.solver_obj.IterationsControlParameterIter + "\n")
        f.write(ccxwriter.solver_obj.IterationsControlParameterCutb + "\n")

    # ANALYSIS type line
    # analysis line --> analysis type
    if ccxwriter.analysis_type == "static":
        analysis_type = "*STATIC"
    elif ccxwriter.analysis_type == "frequency":
        analysis_type = "*FREQUENCY"
    elif ccxwriter.analysis_type == "thermomech":
        analysis_type = "*COUPLED TEMPERATURE-DISPLACEMENT"
    elif ccxwriter.analysis_type == "check":
        analysis_type = "*NO ANALYSIS"
    elif ccxwriter.analysis_type == "buckling":
        analysis_type = "*BUCKLE"
    # analysis line --> solver type
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=43178
    if ccxwriter.solver_obj.MatrixSolverType == "default":
        pass
    elif ccxwriter.solver_obj.MatrixSolverType == "spooles":
        analysis_type += ", SOLVER=SPOOLES"
    elif ccxwriter.solver_obj.MatrixSolverType == "iterativescaling":
        analysis_type += ", SOLVER=ITERATIVE SCALING"
    elif ccxwriter.solver_obj.MatrixSolverType == "iterativecholesky":
        analysis_type += ", SOLVER=ITERATIVE CHOLESKY"
    # analysis line --> user defined incrementations --> parameter DIRECT
    # --> completely switch off ccx automatic incrementation
    if ccxwriter.solver_obj.IterationsUserDefinedIncrementations:
        if ccxwriter.analysis_type == "static":
            analysis_type += ", DIRECT"
        elif ccxwriter.analysis_type == "thermomech":
            analysis_type += ", DIRECT"
        elif ccxwriter.analysis_type == "frequency":
            FreeCAD.Console.PrintMessage(
                "Analysis type frequency and IterationsUserDefinedIncrementations "
                "are not allowed together, it is ignored\n"
            )
    # analysis line --> steadystate --> thermomech only
    if ccxwriter.solver_obj.ThermoMechSteadyState:
        # bernd: I do not know if STEADY STATE is allowed with DIRECT
        # but since time steps are 1.0 it makes no sense IMHO
        if ccxwriter.analysis_type == "thermomech":
            analysis_type += ", STEADY STATE"
            # Set time to 1 and ignore user inputs for steady state
            ccxwriter.solver_obj.TimeInitialStep = 1.0
            ccxwriter.solver_obj.TimeEnd = 1.0
        elif (
            ccxwriter.analysis_type == "static"
            or ccxwriter.analysis_type == "frequency"
            or ccxwriter.analysis_type == "buckling"
        ):
            pass  # not supported for static and frequency!

    # ANALYSIS parameter line
    analysis_parameter = ""
    if ccxwriter.analysis_type == "static" or ccxwriter.analysis_type == "check":
        if ccxwriter.solver_obj.IterationsUserDefinedIncrementations is True \
                or ccxwriter.solver_obj.IterationsUserDefinedTimeStepLength is True:
            analysis_parameter = "{},{}".format(
                ccxwriter.solver_obj.TimeInitialStep,
                ccxwriter.solver_obj.TimeEnd
            )
    elif ccxwriter.analysis_type == "frequency":
        if ccxwriter.solver_obj.EigenmodeLowLimit == 0.0 \
                and ccxwriter.solver_obj.EigenmodeHighLimit == 0.0:
            analysis_parameter = "{}\n".format(ccxwriter.solver_obj.EigenmodesCount)
        else:
            analysis_parameter = "{},{},{}\n".format(
                ccxwriter.solver_obj.EigenmodesCount,
                ccxwriter.solver_obj.EigenmodeLowLimit,
                ccxwriter.solver_obj.EigenmodeHighLimit
            )
    elif ccxwriter.analysis_type == "thermomech":
        # OvG: 1.0 increment, total time 1 for steady state will cut back automatically
        analysis_parameter = "{},{}".format(
            ccxwriter.solver_obj.TimeInitialStep,
            ccxwriter.solver_obj.TimeEnd
        )
    elif ccxwriter.analysis_type == "buckling":
        analysis_parameter = "{}\n".format(ccxwriter.solver_obj.BucklingFactors)

    # write analysis type line, analysis parameter line
    f.write(analysis_type + "\n")
    f.write(analysis_parameter + "\n")


def write_step_end(f, ccxwriter):
    f.write("\n{}\n".format(59 * "*"))
    f.write("*END STEP \n")
