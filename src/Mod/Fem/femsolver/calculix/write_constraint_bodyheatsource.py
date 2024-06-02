# SPDX-License-Identifier: LGPL-2.1-or-later

#/***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
# **************************************************************************

__title__ = "FreeCAD FEM calculix constraint body heat source"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


import FreeCAD


def get_analysis_types():
    return ["thermomech"]


def get_sets_name():
    return "constraints_bodyheatsource_element_sets"


def get_constraint_title():
    return "Body Heat Source Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, bodyheatsource_obj, ccxwriter):
    f.write("*ELSET,ELSET={}\n".format(bodyheatsource_obj.Name))
    if isinstance(femobj["FEMElements"], str):
        f.write("{}\n".format(femobj["FEMElements"]))
    else:
        for e in femobj["FEMElements"]:
            f.write("{},\n".format(e))


def write_constraint(f, femobj, bodyheatsource_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    # search referenced material
    ref = bodyheatsource_obj.References
    density = None
    for mat in ccxwriter.member.mats_linear:
        for mat_ref in mat["Object"].References:
            if mat_ref[0] == ref[0][0]:
                density = FreeCAD.Units.Quantity(mat["Object"].Material["Density"])
                break

    if not density:
        # search material without references
        for mat in ccxwriter.member.mats_linear:
            if not mat["Object"].References:
                density = FreeCAD.Units.Quantity(mat["Object"].Material["Density"])

    # get some data from the bodyheatsource_obj (is in power per unit mass)
    heat = FreeCAD.Units.Quantity(bodyheatsource_obj.HeatSource, "m^2/s^3") * density
    # write to file
    f.write("*DFLUX\n")
    f.write(
        "{},BF,{:.13G}\n".format(
            bodyheatsource_obj.Name, heat.getValueAs("t/(mm*s^3)").Value
        )
    )
    f.write("\n")
