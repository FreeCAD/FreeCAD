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
    else:
        f.write("U\n")
    if not ccxwriter.member.geos_fluidsection:
        f.write("*EL FILE\n")
        if ccxwriter.solver_obj.MaterialNonlinearity == "nonlinear":
            f.write("S, E, PEEQ\n")
        else:
            f.write("S, E\n")

        # dat file
        # reaction forces: freecad.org/tracker/view.php?id=2934
        # some hint can be found in this topic:
        # https://forum.freecad.org/viewtopic.php?f=18&t=20664&start=10#p520642
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
        if ccxwriter.member.cons_fixed or ccxwriter.member.cons_displacement:
            f.write("\n")
        f.write("*OUTPUT, FREQUENCY={}".format(ccxwriter.solver_obj.OutputFrequency))

        # there is no need to write all integration point results
        # as long as there is no reader for them
        # see https://forum.freecad.org/viewtopic.php?f=18&t=29060
        # f.write("*NODE PRINT , NSET=" + ccxwriter.ccx_nall + "\n")
        # f.write("U \n")
        # f.write("*EL PRINT , ELSET=" + ccxwriter.ccx_eall + "\n")
        # f.write("S \n")
