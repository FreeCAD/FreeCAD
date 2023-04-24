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

__title__ = "FreeCAD FEM calculix constraint selfweight"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return ["buckling", "static", "thermomech"]


def get_constraint_title():
    return "Self weight Constraint"


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, selwei_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    f.write("*DLOAD\n")
    f.write(
        # elset, GRAV, magnitude, direction x, dir y ,dir z
        "{},GRAV,{:.13G},{:.13G},{:.13G},{:.13G}\n"
        .format(
            ccxwriter.ccx_eall,
            ccxwriter.gravity,  # actual magnitude of gravity vector
            selwei_obj.Gravity_x,  # coordinate x of normalized gravity vector
            selwei_obj.Gravity_y,  # y
            selwei_obj.Gravity_z  # z
        )
    )
    f.write("\n")


# grav (erdbeschleunigung) is equal for all elements
# should be only one constraint
# different element sets for different density
# are written in the material element sets already
