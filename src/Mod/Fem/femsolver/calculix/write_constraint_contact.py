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

__title__ = "FreeCAD FEM calculix constraint contact"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return "all"  # write for all analysis types


def get_sets_name():
    return "constraints_contact_surface_sets"


def get_constraint_title():
    return "Contact Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, contact_obj, ccxwriter):
    # slave DEP
    f.write(f"*SURFACE, NAME=DEP{contact_obj.Name}\n")
    for refs, surf, is_sub_el in femobj["ContactSlaveFaces"]:
        if is_sub_el:
            for elem, fno in surf:
                f.write(f"{elem},S{fno}\n")
        else:
            for elem in surf:
                f.write(f"{elem},S2\n")

    # master IND
    f.write(f"*SURFACE, NAME=IND{contact_obj.Name}\n")
    for refs, surf, is_sub_el in femobj["ContactMasterFaces"]:
        if is_sub_el:
            for elem, fno in surf:
                f.write(f"{elem},S{fno}\n")
        else:
            for elem in surf:
                f.write(f"{elem},S2\n")


def write_constraint(f, femobj, contact_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    adjust = ""
    if contact_obj.Adjust.Value > 0:
        adjust = ", ADJUST={:.13G}".format(contact_obj.Adjust.getValueAs("mm").Value)

    f.write(
        "*CONTACT PAIR, INTERACTION=INT{}, TYPE=SURFACE TO SURFACE{}\n".format(
            contact_obj.Name, adjust
        )
    )
    ind_surf = "IND" + contact_obj.Name
    dep_surf = "DEP" + contact_obj.Name
    f.write(f"{dep_surf}, {ind_surf}\n")
    f.write(f"*SURFACE INTERACTION, NAME=INT{contact_obj.Name}\n")
    if contact_obj.SurfaceBehavior == "Linear":
        f.write("*SURFACE BEHAVIOR, PRESSURE-OVERCLOSURE=LINEAR\n")
        slope = contact_obj.Slope.getValueAs("MPa/mm").Value
        f.write(f"{slope:.13G}\n")
    elif contact_obj.SurfaceBehavior == "Hard":
        f.write("*SURFACE BEHAVIOR, PRESSURE-OVERCLOSURE=HARD\n")
    elif contact_obj.SurfaceBehavior == "Tied":
        f.write("*SURFACE BEHAVIOR, PRESSURE-OVERCLOSURE=TIED\n")
        slope = contact_obj.Slope.getValueAs("MPa/mm").Value
        f.write(f"{slope:.13G}\n")
    else:
        return
    if contact_obj.Friction:
        f.write("*FRICTION\n")
        friction = contact_obj.FrictionCoefficient
        stick = contact_obj.StickSlope.getValueAs("MPa/mm").Value
        f.write(f"{friction:.13G}, {stick:.13G}\n")
    if contact_obj.EnableThermalContact:
        f.write("*GAP CONDUCTANCE\n")
        for value in contact_obj.ThermalContactConductance:
            f.write(f"{value}\n")
        f.write("\n")
