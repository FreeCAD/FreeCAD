# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM calculix constraint sectionprint"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return "all"  # write for all analysis types


def get_sets_name():
    return "constraints_sectionprint_surface_sets"


def get_constraint_title():
    return "SectionPrint Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, sectionprint_obj, ccxwriter):
    f.write(f"*SURFACE, NAME=SECTIONFACE{sectionprint_obj.Name}\n")

    for refs, surf, is_sub_el in femobj["SectionPrintFaces"]:
        if is_sub_el:
            for elem, fno in surf:
                f.write(f"{elem},S{fno}\n")
        else:
            for elem in surf:
                f.write(f"{elem},S2\n")


def write_constraint(f, femobj, sectionprint_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    variable = sectionprint_obj.Variable
    if variable == "Section Force":
        key = "SOF, SOM, SOAREA"
    elif variable == "Heat Flux":
        key = "FLUX"
    elif variable == "Drag Stress":
        key = "DRAG"
    elif variable == "Electric Flux":
        key = "FLUX"

    f.write(
        "*SECTION PRINT, SURFACE=SECTIONFACE{}, NAME=SECTIONPRINT{}\n".format(
            sectionprint_obj.Name, sectionprint_obj.Name
        )
    )
    f.write(key + "\n")
