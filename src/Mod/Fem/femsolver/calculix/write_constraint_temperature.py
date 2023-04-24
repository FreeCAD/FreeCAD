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

__title__ = "FreeCAD FEM calculix constraint temperature"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import FreeCAD


def get_analysis_types():
    return ["thermomech"]


# name must substitute underscores for whitespace (#7360)
def get_sets_name():
    return "constraints_temperature_node_sets"


def get_constraint_title():
    return "Fixed temperature constraint applied"


def write_meshdata_constraint(f, femobj, temp_obj, ccxwriter):
    f.write("*NSET,NSET={}\n".format(temp_obj.Name))
    for n in femobj["Nodes"]:
        f.write("{},\n".format(n))


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, temp_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    NumberOfNodes = len(femobj["Nodes"])
    if temp_obj.ConstraintType == "Temperature":
        f.write("*BOUNDARY\n")
        f.write(
            "{},11,11,{}\n".format(
                temp_obj.Name, FreeCAD.Units.Quantity(temp_obj.Temperature.getValueAs("K"))
            )
        )
        f.write("\n")
    elif temp_obj.ConstraintType == "CFlux":
        f.write("*CFLUX\n")
        # CFLUX has to be specified in mW
        f.write(
            "{},11,{}\n".format(
                temp_obj.Name,
                FreeCAD.Units.Quantity(temp_obj.CFlux.getValueAs("mW")) / NumberOfNodes
            )
        )
        f.write("\n")
