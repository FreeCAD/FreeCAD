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

__title__ = "FreeCAD FEM calculix write inpfile step output"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def write_step_output(f, ccxwriter):

    f.write("\n{}\n".format(59 * "*"))
    f.write("** Outputs --> frd file\n")
    if (
        ccxwriter.member.geos_beamsection
        or ccxwriter.member.geos_shellthickness
        or ccxwriter.member.geos_fluidsection
    ):
        if ccxwriter.solver_obj.BeamShellResultOutput3D is False:
            f.write("*NODE FILE, OUTPUT=2d\n")
        else:
            f.write("*NODE FILE, OUTPUT=3d\n")
    else:
        f.write("*NODE FILE\n")
    # MPH write out nodal temperatures if thermomechanical
    if ccxwriter.analysis_type == "thermomech":
        if not ccxwriter.member.geos_fluidsection:
            f.write("U, NT\n")
        else:
            f.write("MF, PS\n")
    elif ccxwriter.analysis_type == "electromagnetic":
        f.write("NT\n")
    else:
        f.write("U\n")
    if not ccxwriter.member.geos_fluidsection:
        f.write("*EL FILE\n")
        variables = "S, E"
        if ccxwriter.analysis_type == "thermomech":
            variables += ", HFL"

        # plastic strain only if some material has nonlinear properties
        if ccxwriter.solver_obj.MaterialNonlinearity:
            for mat in ccxwriter.member.mats_linear:
                mat_nonlin = mat["Object"].Nonlinear
                if mat_nonlin and not mat_nonlin.Suppressed:
                    variables += ", PEEQ"
                    break

        if ccxwriter.analysis_type == "electromagnetic":
            variables = "HFL"

        f.write(variables + "\n")

        # dat file
        if ccxwriter.member.cons_fixed or ccxwriter.member.cons_displacement:
            f.write("** outputs --> dat file\n")
        if ccxwriter.member.cons_fixed:
            # reaction forces for all Constraint fixed
            f.write("** reaction forces for Constraint fixed\n")
            for femobj in ccxwriter.member.cons_fixed:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                f.write("*NODE PRINT, NSET={}, TOTALS=ONLY\n".format(femobj["Object"].Name))
                f.write("RF\n")
        if ccxwriter.member.cons_displacement:
            # reaction forces for Constraint displacement constraining translation
            f.write("** reaction forces for Constraint displacement constraining translation\n")
            for femobj in ccxwriter.member.cons_displacement:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                if (
                    not femobj["Object"].xFree
                    or not femobj["Object"].yFree
                    or not femobj["Object"].zFree
                ):
                    f.write("*NODE PRINT, NSET={}, TOTALS=ONLY\n".format(femobj["Object"].Name))
                    f.write("RF\n")
        if ccxwriter.member.cons_rigidbody:
            # displacements and reaction forces/moments for Constraint rigid body
            f.write("** displacements and reaction forces/moments for Constraint rigid body\n")
            for femobj in ccxwriter.member.cons_rigidbody:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                f.write("*NODE PRINT, NSET={}_RefNode\n".format(femobj["Object"].Name))
                f.write("U\n")
                f.write("*NODE PRINT, NSET={}_RotNode\n".format(femobj["Object"].Name))
                f.write("U\n")
                if (
                    femobj["Object"].TranslationalModeX != "Free"
                    or femobj["Object"].TranslationalModeY != "Free"
                    or femobj["Object"].TranslationalModeZ != "Free"
                ):
                    f.write(
                        "*NODE PRINT, NSET={}_RefNode, TOTALS=ONLY\n".format(femobj["Object"].Name)
                    )
                    f.write("RF\n")
                if (
                    femobj["Object"].RotationalModeX != "Free"
                    or femobj["Object"].RotationalModeY != "Free"
                    or femobj["Object"].RotationalModeZ != "Free"
                ):
                    f.write(
                        "*NODE PRINT, NSET={}_RotNode, TOTALS=ONLY\n".format(femobj["Object"].Name)
                    )
                    f.write("RF\n")
        if ccxwriter.member.cons_contact:
            # contact forces for all Constraint contact (only available for face-to-face penalty contact)
            f.write("** contact forces for Constraint contact\n")
            for femobj in ccxwriter.member.cons_contact:
                # femobj --> dict, FreeCAD document object is femobj["Object"]
                f.write(
                    "*CONTACT PRINT, MASTER={}, SLAVE={}\n".format(
                        "IND" + femobj["Object"].Name, "DEP" + femobj["Object"].Name
                    )
                )
                f.write("CF, CFN, CFS\n")
        if any(
            vars(ccxwriter.member).get(f"cons_{key}")
            for key in ["fixed", "displacement", "rigidbody", "contact"]
        ):
            f.write("\n")
        f.write(f"*OUTPUT, FREQUENCY={ccxwriter.solver_obj.OutputFrequency}")

        # there is no need to write all integration point results
        # as long as there is no reader for them
        # see https://forum.freecad.org/viewtopic.php?f=18&t=29060
        # f.write("*NODE PRINT , NSET=" + ccxwriter.ccx_nall + "\n")
        # f.write("U \n")
        # f.write("*EL PRINT , ELSET=" + ccxwriter.ccx_eall + "\n")
        # f.write("S \n")
