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
__url__ = "https://www.freecad.org"


import FreeCAD


def write_step_equation(f, ccxwriter):

    f.write("\n{}\n".format(59 * "*"))
    f.write("** At least one step is needed to run an CalculiX analysis of FreeCAD\n")

    # build STEP line
    step = "*STEP"
    if ccxwriter.solver_obj.GeometricalNonlinearity:
        if ccxwriter.analysis_type in ["static", "thermomech"]:
            # https://www.comsol.com/blogs/what-is-geometric-nonlinearity
            step += ", NLGEOM"
        elif ccxwriter.analysis_type == "frequency":
            FreeCAD.Console.PrintMessage(
                "Analysis type frequency and geometrical nonlinear "
                "analysis are not allowed together, linear is used instead!\n"
            )

    if ccxwriter.solver_obj.IncrementsMaximum:
        if ccxwriter.analysis_type in ["static", "thermomech", "electromagnetic"]:
            step += f", INC={ccxwriter.solver_obj.IncrementsMaximum}"

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
    analysis_type = ""
    if ccxwriter.analysis_type == "static":
        analysis_type = "*STATIC"
    elif ccxwriter.analysis_type == "frequency":
        analysis_type = "*FREQUENCY"
    elif ccxwriter.analysis_type == "thermomech":
        if ccxwriter.solver_obj.ThermoMechType == "coupled":
            analysis_type = "*COUPLED TEMPERATURE-DISPLACEMENT"
        elif ccxwriter.solver_obj.ThermoMechType == "uncoupled":
            analysis_type = "*UNCOUPLED TEMPERATURE-DISPLACEMENT"
        elif ccxwriter.solver_obj.ThermoMechType == "pure heat transfer":
            analysis_type = "*HEAT TRANSFER"
        if ccxwriter.solver_obj.ThermoMechSteadyState:
            analysis_type += ", STEADY STATE"
    elif ccxwriter.analysis_type == "check":
        analysis_type = "*NO ANALYSIS"
    elif ccxwriter.analysis_type == "buckling":
        analysis_type = "*BUCKLE"
    elif ccxwriter.analysis_type == "electromagnetic":
        if ccxwriter.solver_obj.ElectromagneticMode == "electrostatic":
            analysis_type = "*HEAT TRANSFER, STEADY STATE"

    # analysis line --> solver type
    # https://forum.freecad.org/viewtopic.php?f=18&t=43178
    if ccxwriter.solver_obj.MatrixSolverType == "default":
        pass
    elif ccxwriter.solver_obj.MatrixSolverType == "pastix":
        analysis_type += ", SOLVER=PASTIX"
    elif ccxwriter.solver_obj.MatrixSolverType == "pardiso":
        analysis_type += ", SOLVER=PARDISO"
    elif ccxwriter.solver_obj.MatrixSolverType == "spooles":
        analysis_type += ", SOLVER=SPOOLES"
    elif ccxwriter.solver_obj.MatrixSolverType == "iterativescaling":
        analysis_type += ", SOLVER=ITERATIVE SCALING"
    elif ccxwriter.solver_obj.MatrixSolverType == "iterativecholesky":
        analysis_type += ", SOLVER=ITERATIVE CHOLESKY"

    # analysis line --> automatic incrementation --> parameter DIRECT
    # completely switch off ccx automatic incrementation
    if not ccxwriter.solver_obj.AutomaticIncrementation:
        if ccxwriter.analysis_type in ["static", "thermomech", "electromagnetic"]:
            analysis_type += ", DIRECT"

    # ANALYSIS parameter line
    analysis_parameter = ""
    if ccxwriter.analysis_type in ["static", "thermomech", "electromagnetic"]:
        analysis_parameter = "{},{},{},{}".format(
            ccxwriter.solver_obj.TimeInitialIncrement.getValueAs("s").Value,
            ccxwriter.solver_obj.TimePeriod.getValueAs("s").Value,
            ccxwriter.solver_obj.TimeMinimumIncrement.getValueAs("s").Value,
            ccxwriter.solver_obj.TimeMaximumIncrement.getValueAs("s").Value,
        )
    elif ccxwriter.analysis_type == "frequency":
        if (
            ccxwriter.solver_obj.EigenmodeLowLimit == 0.0
            and ccxwriter.solver_obj.EigenmodeHighLimit == 0.0
        ):
            analysis_parameter = f"{ccxwriter.solver_obj.EigenmodesCount}\n"
        else:
            analysis_parameter = "{},{},{}\n".format(
                ccxwriter.solver_obj.EigenmodesCount,
                ccxwriter.solver_obj.EigenmodeLowLimit.getValueAs("Hz").Value,
                ccxwriter.solver_obj.EigenmodeHighLimit.getValueAs("Hz").Value,
            )
    elif ccxwriter.analysis_type == "buckling":
        analysis_parameter = "{},{}".format(
            ccxwriter.solver_obj.BucklingFactors,
            ccxwriter.solver_obj.BucklingAccuracy,
        )
    elif ccxwriter.analysis_type == "check":
        analysis_parameter = ""

    # write analysis type line, analysis parameter line
    f.write(analysis_type + "\n")
    f.write(analysis_parameter + "\n")


def write_step_end(f, ccxwriter):
    f.write("\n{}\n".format(59 * "*"))
    f.write("*END STEP\n")
