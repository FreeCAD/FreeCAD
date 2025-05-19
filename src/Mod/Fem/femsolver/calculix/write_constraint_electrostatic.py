# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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
# ***************************************************************************

__title__ = "FreeCAD FEM calculix constraint electrostatic"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import FreeCAD


def get_analysis_types():
    return ["electromagnetic"]


def get_sets_name():
    return "constraints_electrostaticpotential_node_sets"


def get_constraint_title():
    return "Fixed electrostatic constraint applied"


def write_meshdata_constraint(f, femobj, pot_obj, ccxwriter):

    if ccxwriter.solver_obj.ElectromagneticMode != "electrostatic":
        return

    if femobj["Object"].BoundaryCondition == "Dirichlet":
        f.write(f"*NSET,NSET={pot_obj.Name}\n")
        for n in femobj["Nodes"]:
            f.write(f"{n},\n")


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, pot_obj, ccxwriter):

    if ccxwriter.solver_obj.ElectromagneticMode != "electrostatic":
        return

    # floats read from ccx should use {:.13G}, see comment in writer module
    if pot_obj.BoundaryCondition == "Dirichlet":
        f.write("*BOUNDARY\n")
        f.write("{},11,11,{:.13G}\n".format(pot_obj.Name, pot_obj.Potential.getValueAs("mV").Value))
    elif pot_obj.BoundaryCondition == "Neumann":
        density = pot_obj.ElectricFluxDensity.getValueAs("C/mm^2").Value
        # check internal interface
        internal = _check_shared_interface(pot_obj)
        for feat, refs in femobj["ElectricFluxFaces"]:
            f.write("** " + feat + "\n")
            f.write("*DFLUX\n")
            for ref in refs:
                d = density
                if ref[0] in internal:
                    d = density / 2
                for face, fno in ref[1]:
                    f.write("{},S{},{:.13G}\n".format(face, fno, d))

    f.write("\n")


def _check_shared_interface(pot_obj):
    """
    Check if reference is internal shared subshape
    For example, shared face in compsolid
    """
    internal = []
    for o, sub in pot_obj.References:
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
