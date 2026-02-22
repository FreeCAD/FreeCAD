# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski[at]gmail.com>         *
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

__title__ = "FreeCAD FEM calculix amplitude"
__author__ = "Jakub Michalski"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return ["static"]


def get_sets_name():
    return "constraints_initial_temperature_node_sets"


def get_constraint_title():
    return "Final temperature constraint"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, inittemp_obj, ccxwriter):
    if inittemp_obj.EnableFinalTemperature:
        # floats read from ccx should use {:.13G}, see comment in writer module

        finaltemp = inittemp_obj.FinalTemperature.getValueAs("K")

        if inittemp_obj.EnableAmplitude:
            f.write(f"*TEMPERATURE, AMPLITUDE={inittemp_obj.Name}\n")
        else:
            f.write("*TEMPERATURE\n")
        if inittemp_obj.References:
            f.write(f"{inittemp_obj.Name},{finaltemp}\n")
        else:
            f.write(f"{ccxwriter.ccx_nall},{finaltemp}\n")
