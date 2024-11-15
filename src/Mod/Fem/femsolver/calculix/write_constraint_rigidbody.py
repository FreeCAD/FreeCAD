# ***************************************************************************
# *   Copyright (c) 2022 Ajinkya Dahale <dahale.a.p@gmail.com>              *
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

__title__ = "FreeCAD FEM calculix constraint rigid body"
__author__ = "Ajinkya Dahale"
__url__ = "https://www.freecadweb.org"


def get_analysis_types():
    return "all"  # write for all analysis types


def get_sets_name():
    return "constraints_rigidbody_node_sets"


def get_constraint_title():
    return "Rigid Body Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, rb_obj, ccxwriter):

    f.write("*NSET,NSET=" + rb_obj.Name + "\n")
    for n in femobj["Nodes"]:
        f.write(f"{n},\n")


def write_constraint(f, femobj, rb_obj, ccxwriter):

    rb_obj_idx = ccxwriter.analysis.Group.index(rb_obj)
    node_count = ccxwriter.mesh_object.FemMesh.NodeCount
    # factor 2 is to prevent conflict with other rigid body constraint
    ref_node_idx = node_count + 2 * rb_obj_idx + 1
    rot_node_idx = node_count + 2 * rb_obj_idx + 2

    f.write("*NODE\n")
    f.write("{},{},{},{}\n".format(ref_node_idx, *rb_obj.ReferenceNode))
    f.write("{},{},{},{}\n".format(rot_node_idx, *rb_obj.ReferenceNode))

    f.write(f"*NSET,NSET={rb_obj.Name}_RefNode\n")
    f.write(f"{ref_node_idx},\n")
    f.write(f"*NSET,NSET={rb_obj.Name}_RotNode\n")
    f.write(f"{rot_node_idx},\n")

    kw_line = f"*RIGID BODY, NSET={rb_obj.Name}, REF NODE={ref_node_idx}, ROT NODE={rot_node_idx}"

    f.write(kw_line + "\n")
