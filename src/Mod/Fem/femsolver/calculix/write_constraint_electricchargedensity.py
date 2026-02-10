# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM calculix constraint electric charge density"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return ["electromagnetic"]


def get_sets_name():
    return "constraints_electricchargedensity_element_sets"


def get_constraint_title():
    return "Electric charge density constraint applied"


def write_meshdata_constraint(f, femobj, den_obj, ccxwriter):

    if ccxwriter.solver_obj.ElectromagneticMode != "electrostatic":
        return

    if den_obj.Concentrated and den_obj.Mode == "Total Source":
        f.write(f"*NSET,NSET={den_obj.Name}\n")
        for n in femobj["Nodes"]:
            f.write(f"{n},\n")
        return

    if den_obj.Mode in ["Source", "Total Source"]:
        f.write(f"*ELSET,ELSET={den_obj.Name}\n")
        for refs, surf, is_sub_el in femobj["ChargeDensityElements"]:
            if not is_sub_el:
                for elem in surf:
                    f.write(f"{elem},\n")


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, den_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    if ccxwriter.solver_obj.ElectromagneticMode != "electrostatic":
        return

    if den_obj.Concentrated and den_obj.Mode == "Total Source":
        nodes = len(femobj["Nodes"])
        node_charge = den_obj.TotalCharge.getValueAs("C").Value / nodes
        f.write("*CFLUX\n")
        f.write("{},11,{:.13G}\n".format(den_obj.Name, node_charge))
        f.write("\n")
        return

    match den_obj.Mode:
        case "Source":
            density = den_obj.SourceChargeDensity
        case "Total Source":
            density = den_obj.Proxy.get_total_source_density(den_obj)
        case "Interface":
            density = den_obj.InterfaceChargeDensity
        case "Total Interface":
            density = den_obj.Proxy.get_total_interface_density(den_obj)

    if den_obj.Mode in ["Source", "Total Source"]:
        f.write("*DFLUX\n")
        f.write("{},BF,{:.13G}\n".format(den_obj.Name, density.getValueAs("C/mm^3").Value))
        f.write("\n")

    elif den_obj.Mode in ["Interface", "Total Interface"]:
        density = density.getValueAs("C/mm^2").Value
        # check internal interface
        internal = _check_shared_interface(den_obj)
        for feat, surf, is_sub_el in femobj["ChargeDensityFaces"]:
            f.write("** {0.Name}.{1[0]}\n".format(*feat))
            f.write("*DFLUX\n")
            d = density
            if feat in internal:
                d = density / 2
            for face, fno in surf:
                f.write("{},S{},{:.13G}\n".format(face, fno, d))

        f.write("\n")


def _check_shared_interface(den_obj):
    """
    Check if reference is internal shared subshape
    For example, shared face in compsolid
    """
    internal = []
    for o, sub in den_obj.References:
        for elem in sub:
            found = []
            elem_i = o.getSubObject(elem)
            if elem_i.ShapeType == "Face":
                for s in o.Shape.Solids:
                    found.append(any([q.isSame(elem_i) for q in s.Faces]))
                if sum(found) > 1:
                    internal.append((o, (elem,)))

            if elem_i.ShapeType == "Edge":
                for s in o.Shape.Faces:
                    found.append(any([q.isSame(elem_i) for q in s.Edges]))
                if sum(found) > 1:
                    internal.append((o, (elem,)))

    return internal
