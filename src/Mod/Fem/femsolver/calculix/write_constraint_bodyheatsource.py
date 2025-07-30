# SPDX-License-Identifier: LGPL-2.1-or-later

# /***************************************************************************
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
import itertools


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
    f.write(f"*ELSET,ELSET={bodyheatsource_obj.Name}\n")
    if isinstance(femobj["FEMElements"], str):
        f.write("{}\n".format(femobj["FEMElements"]))
    else:
        for e in femobj["FEMElements"]:
            f.write(f"{e},\n")


def write_constraint(f, femobj, bodyheatsource_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    # search referenced material
    ref = bodyheatsource_obj.References[0]
    ref_feat = ref[0]
    ref_sub_obj = ref[1][0]
    density = None
    for mat in ccxwriter.member.mats_linear:
        mat_ref = [
            *itertools.chain(*[itertools.product([i[0]], i[1]) for i in mat["Object"].References])
        ]
        if (ref_feat, ref_sub_obj) in mat_ref:
            density = FreeCAD.Units.Quantity(mat["Object"].Material["Density"])
            break

    if not density:
        # search material without references
        for mat in ccxwriter.member.mats_linear:
            if not mat["Object"].References:
                density = FreeCAD.Units.Quantity(mat["Object"].Material["Density"])
                break

    # get data from the bodyheatsource_obj (DissipationRate is in power per unit mass)
    if bodyheatsource_obj.Mode == "Dissipation Rate":
        heat = bodyheatsource_obj.DissipationRate * density
    elif bodyheatsource_obj.Mode == "Total Power":
        volume = ref_feat.getSubObject(ref_sub_obj).Volume
        heat = bodyheatsource_obj.TotalPower / FreeCAD.Units.Quantity(volume, "mm^3")
    # write to file
    if bodyheatsource_obj.EnableAmplitude:
        f.write(f"*DFLUX, AMPLITUDE={bodyheatsource_obj.Name}\n")
    else:
        f.write("*DFLUX\n")
    f.write("{},BF,{:.13G}\n".format(bodyheatsource_obj.Name, heat.getValueAs("t/(mm*s^3)").Value))
    f.write("\n")
