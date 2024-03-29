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

__title__ = "FreeCAD FEM calculix constraint contact"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return "all"    # write for all analysis types


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
    f.write("*SURFACE, NAME=DEP{}\n".format(contact_obj.Name))
    for i in femobj["ContactSlaveFaces"]:
        f.write("{},S{}\n".format(i[0], i[1]))
    # master IND
    f.write("*SURFACE, NAME=IND{}\n".format(contact_obj.Name))
    for i in femobj["ContactMasterFaces"]:
        f.write("{},S{}\n".format(i[0], i[1]))


def write_constraint(f, femobj, contact_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    adjust = ""
    if contact_obj.Adjust.Value > 0:
        adjust = ", ADJUST={:.13G}".format(
            contact_obj.Adjust.getValueAs("mm").Value)

    f.write(
        "*CONTACT PAIR, INTERACTION=INT{}, TYPE=SURFACE TO SURFACE{}\n"
        .format(contact_obj.Name, adjust)
    )
    ind_surf = "IND" + contact_obj.Name
    dep_surf = "DEP" + contact_obj.Name
    f.write("{}, {}\n".format(dep_surf, ind_surf))
    f.write("*SURFACE INTERACTION, NAME=INT{}\n".format(contact_obj.Name))
    f.write("*SURFACE BEHAVIOR, PRESSURE-OVERCLOSURE=LINEAR\n")
    slope = contact_obj.Slope.getValueAs("MPa/mm").Value
    f.write("{:.13G}\n".format(slope))
    if contact_obj.Friction:
        f.write("*FRICTION\n")
        friction = contact_obj.FrictionCoefficient
        stick = contact_obj.StickSlope.getValueAs("MPa/mm").Value
        f.write("{:.13G}, {:.13G}\n".format(friction, stick))
