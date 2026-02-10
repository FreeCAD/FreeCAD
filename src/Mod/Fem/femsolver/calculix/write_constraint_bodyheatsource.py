# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# /***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

    ref_iter = itertools.chain(
        *[itertools.product([i[0]], i[1]) for i in bodyheatsource_obj.References]
    )
    ref_iter = tuple(ref_iter)
    for refs, surf, is_sub_el in femobj["BodyHeatSourceElements"]:
        feat, (sub,) = refs
        index = ref_iter.index((feat, sub))
        f.write(f"** {bodyheatsource_obj.Name}.{feat.Name}.{sub}\n")
        f.write(f"*ELSET,ELSET={bodyheatsource_obj.Name}_{index}\n")
        if not is_sub_el:
            for elem in surf:
                f.write(f"{elem},\n")
        f.write("\n")


def write_constraint(f, femobj, bodyheatsource_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    ref_iter = itertools.chain(
        *[itertools.product([i[0]], i[1]) for i in bodyheatsource_obj.References]
    )
    for index, (ref_feat, ref_sub_obj) in enumerate(ref_iter):

        # search referenced material
        mat = _search_member_property(
            ccxwriter.member.mats_linear, (ref_feat, ref_sub_obj), "Material"
        )
        density = FreeCAD.Units.Quantity(mat["Density"])

        # get data from the bodyheatsource_obj (DissipationRate is in power per unit mass)
        if bodyheatsource_obj.Mode == "Dissipation Rate":
            heat = bodyheatsource_obj.DissipationRate * density
        elif bodyheatsource_obj.Mode == "Total Power":
            sh = ref_feat.getSubObject(ref_sub_obj)
            volume = sh.Mass
            # if shape is face, get thickness
            if sh.ShapeType == "Face":
                thick = _search_member_property(
                    ccxwriter.member.geos_shellthickness, (ref_feat, ref_sub_obj), "Thickness"
                )
                volume *= thick.getValueAs("mm")
            heat = bodyheatsource_obj.TotalPower / FreeCAD.Units.Quantity(volume, "mm^3")

        # write to file
        if bodyheatsource_obj.EnableAmplitude:
            f.write(f"*DFLUX, AMPLITUDE={bodyheatsource_obj.Name}\n")
        else:
            f.write("*DFLUX\n")
        elset_name = f"{bodyheatsource_obj.Name}_{index}"
        f.write("{},BF,{:.13G}\n".format(elset_name, heat.getValueAs("t/(mm*s^3)").Value))
        f.write("\n")


def _search_member_property(it, ref, prop_name):
    value = None
    for member in it:
        member_ref = [
            *itertools.chain(
                *[itertools.product([i[0]], i[1]) for i in member["Object"].References]
            )
        ]
        if ref in member_ref:
            value = member["Object"].getPropertyByName(prop_name)
            break

    if not value:
        # search membererial without references
        for member in it:
            if not member["Object"].References:
                value = member["Object"].getPropertyByName(prop_name)
                break

    return value
