# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Jakub Michalski <jakub.j.michalski[at]gmail.com>         *
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

__title__ = "FreeCAD FEM calculix amplitude"
__author__ = "Jakub Michalski"
__url__ = "https://www.freecad.org"


def write_amplitude(f, ccxwriter):

    # write amplitude definitions for all analysis features that use them

    def write_obj_amplitude(obj):
        if obj.EnableAmplitude:
            f.write(f"*AMPLITUDE, NAME={obj.Name}\n")
            for value in obj.AmplitudeValues:
                f.write(f"{value}\n")
            f.write("\n")

    constraint_lists = [
        ccxwriter.member.cons_force,
        ccxwriter.member.cons_pressure,
        ccxwriter.member.cons_displacement,
        ccxwriter.member.cons_heatflux,
        ccxwriter.member.cons_temperature,
        ccxwriter.member.cons_bodyheatsource,
    ]

    for constraint_list in constraint_lists:
        for entry in constraint_list:
            write_obj_amplitude(entry["Object"])
