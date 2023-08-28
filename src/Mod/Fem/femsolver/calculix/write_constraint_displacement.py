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

__title__ = "FreeCAD FEM calculix constraint displacement"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import FreeCAD


def get_analysis_types():
    return "all"    # write for all analysis types


def get_sets_name():
    return "constraints_displacement_node_sets"


def get_constraint_title():
    return "Displacement constraint applied"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return "\n"


def write_meshdata_constraint(f, femobj, disp_obj, ccxwriter):
    f.write("*NSET,NSET={}\n".format(disp_obj.Name))
    for n in femobj["Nodes"]:
        f.write("{},\n".format(n))


def write_constraint(f, femobj, disp_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    f.write("*BOUNDARY\n")
    if disp_obj.xFix:
        f.write("{},1\n".format(disp_obj.Name))
    elif not disp_obj.xFree:
        f.write(
            "{},1,1,{}\n".format(
                disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.xDisplacement.getValueAs("mm"))
            )
        )
    if disp_obj.yFix:
        f.write("{},2\n".format(disp_obj.Name))
    elif not disp_obj.yFree:
        f.write(
            "{},2,2,{}\n".format(
                disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.yDisplacement.getValueAs("mm"))
            )
        )
    if disp_obj.zFix:
        f.write("{},3\n".format(disp_obj.Name))
    elif not disp_obj.zFree:
        f.write(
            "{},3,3,{}\n".format(
                disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.zDisplacement.getValueAs("mm"))
            )
        )

    if ccxwriter.member.geos_beamsection or ccxwriter.member.geos_shellthickness:
        if disp_obj.rotxFix:
            f.write("{},4\n".format(disp_obj.Name))
        elif not disp_obj.rotxFree:
            f.write(
                "{},4,4,{}\n".format(
                    disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.xRotation.getValueAs("deg"))
                )
            )
        if disp_obj.rotyFix:
            f.write("{},5\n".format(disp_obj.Name))
        elif not disp_obj.rotyFree:
            f.write(
                "{},5,5,{}\n".format(
                    disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.yRotation.getValueAs("deg"))
                )
            )
        if disp_obj.rotzFix:
            f.write("{},6\n".format(disp_obj.Name))
        elif not disp_obj.rotzFree:
            f.write(
                "{},6,6,{}\n".format(
                    disp_obj.Name, FreeCAD.Units.Quantity(disp_obj.zRotation.getValueAs("deg"))
                )
            )
