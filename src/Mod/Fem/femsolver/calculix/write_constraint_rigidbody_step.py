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
    return "all"    # write for all analysis types


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

    # floats read from ccx should use {:.13G}, see comment in writer module
    is_ref_bc_defined = any((rb_obj.xDisplacement,
                             rb_obj.yDisplacement,
                             rb_obj.zDisplacement,
                             rb_obj.xForce,
                             rb_obj.yForce,
                             rb_obj.zForce))

    is_rot_bc_defined = any((rb_obj.xRotation,
                             rb_obj.yRotation,
                             rb_obj.zRotation,
                             rb_obj.xMoment,
                             rb_obj.yMoment,
                             rb_obj.zMoment))

    # FIXME: This needs to be implemented
    ref_node_idx = 10000000
    rot_node_idx = 20000000


    # TODO: Displacement definitions need fixing
    f.write("*CLOAD\n")
    f.write("{},1,{}\n".format(ref_node_idx, rb_obj.xForce))
    f.write("{},2,{}\n".format(ref_node_idx, rb_obj.yForce))
    f.write("{},3,{}\n".format(ref_node_idx, rb_obj.zForce))

    f.write("*CLOAD\n")
    f.write("{},1,{}\n".format(rot_node_idx, rb_obj.xMoment))
    f.write("{},2,{}\n".format(rot_node_idx, rb_obj.yMoment))
    f.write("{},3,{}\n".format(rot_node_idx, rb_obj.zMoment))
