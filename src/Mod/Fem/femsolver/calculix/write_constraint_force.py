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
__url__ = "https://www.freecadweb.org"


def get_analysis_types():
    return ["buckling", "static", "thermomech"]


def get_sets_name():
    return "constraints_force_node_loads"


def get_before_write_meshdata_constraint():
    return "*CLOAD\n"


def get_after_write_meshdata_constraint():
    return ""


def write_meshdata_constraint(f, femobj, force_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    direction_vec = femobj["Object"].DirectionVector
    for ref_shape in femobj["NodeLoadTable"]:
        f.write("** " + ref_shape[0] + "\n")
        for n in sorted(ref_shape[1]):
            node_load = ref_shape[1][n]
            if (direction_vec.x != 0.0):
                v1 = "{:.13E}".format(direction_vec.x * node_load)
                f.write("{},1,{}\n".format(n, v1))
            if (direction_vec.y != 0.0):
                v2 = "{:.13E}".format(direction_vec.y * node_load)
                f.write("{},2,{}\n".format(n, v2))
            if (direction_vec.z != 0.0):
                v3 = "{:.13E}".format(direction_vec.z * node_load)
                f.write("{},3,{}\n".format(n, v3))
        f.write("\n")
    f.write("\n")
