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

__title__ = "FreeCAD FEM calculix constraint force"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return ["buckling", "static", "thermomech"]


def get_sets_name():
    return "constraints_force_node_loads"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def write_meshdata_constraint(f, femobj, force_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    if force_obj.EnableAmplitude:
        f.write(f"*CLOAD, AMPLITUDE={force_obj.Name}\n")
    else:
        f.write("*CLOAD\n")
    direction_vec = femobj["Object"].DirectionVector
    dir_zero_tol = 1e-15  # TODO: should this be more generally for more values?
    # be careful with raising the tolerance, a big load would have an impact
    # but compared to the real direction the impact would be small again
    for ref_shape in femobj["NodeLoadTable"]:
        f.write(f"** {ref_shape[0]}\n")
        for n in sorted(ref_shape[1]):
            node_load = ref_shape[1][n]
            # the loads in ref_shape[1][n] are without unit
            if abs(direction_vec.x) > dir_zero_tol:
                v1 = f"{(direction_vec.x * node_load).Value:.13G}"
                f.write(f"{n},1,{v1}\n")
            if abs(direction_vec.y) > dir_zero_tol:
                v2 = f"{(direction_vec.y * node_load).Value:.13G}"
                f.write(f"{n},2,{v2}\n")
            if abs(direction_vec.z) > dir_zero_tol:
                v3 = f"{(direction_vec.z * node_load).Value:.13G}"
                f.write(f"{n},3,{v3}\n")
        f.write("\n")
    f.write("\n")
