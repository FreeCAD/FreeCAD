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


import FreeCAD


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


def write_constraint(f, femobj, rb_obj, ccxwriter):

    rb_obj_idx = ccxwriter.analysis.Group.index(rb_obj)
    node_count = ccxwriter.mesh_object.FemMesh.NodeCount
    # factor 2 is to prevent conflict with other rigid body constraint
    ref_node_idx = node_count + 2 * rb_obj_idx + 1
    rot_node_idx = node_count + 2 * rb_obj_idx + 2

    def write_mode(mode, node, dof, constraint, load):
        if mode == "Constraint":
            f.write("*BOUNDARY\n")
            f.write(f"{node},{dof},{dof},{constraint:.13G}\n")
        elif mode == "Load":
            f.write("*CLOAD\n")
            f.write(f"{node},{dof},{load:.13G}\n")

    mode = [rb_obj.TranslationalModeX, rb_obj.TranslationalModeY, rb_obj.TranslationalModeZ]
    constraint = rb_obj.Displacement
    load = [rb_obj.ForceX, rb_obj.ForceY, rb_obj.ForceZ]

    for i in range(3):
        write_mode(mode[i], ref_node_idx, i + 1, constraint[i], load[i].getValueAs("N").Value)

    mode = [rb_obj.RotationalModeX, rb_obj.RotationalModeY, rb_obj.RotationalModeZ]
    load = [rb_obj.MomentX, rb_obj.MomentY, rb_obj.MomentZ]

    # write rotation components according to rotational mode
    rot = rb_obj.Rotation
    proj_axis = [rot.Axis[i] if mode[i] == "Constraint" else 0 for i in range(3)]
    # proj_axis could be null
    try:
        constraint = FreeCAD.Vector(proj_axis).normalize() * rot.Angle
    except:
        constraint = FreeCAD.Vector(0, 0, 0)

    for i in range(3):
        write_mode(mode[i], rot_node_idx, i + 1, constraint[i], load[i].getValueAs("N*mm").Value)

    f.write("\n")
