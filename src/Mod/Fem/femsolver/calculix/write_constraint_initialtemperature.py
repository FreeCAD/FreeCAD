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

__title__ = "FreeCAD FEM calculix constraint initialtemperature"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

from FreeCAD import Units


def get_analysis_types():
    return ["thermomech"]


def get_sets_name():
    return "constraints_initial_temperature_node_sets"


def get_constraint_title():
    return "Initial temperature constraint"


def write_meshdata_constraint(f, femobj, inittemp_obj, ccxwriter):
    if inittemp_obj.References and len(inittemp_obj.References) > 0:
        f.write(f"*NSET,NSET={inittemp_obj.Name}\n")
        for n in femobj["Nodes"]:
            f.write(f"{n},\n")
    else:
        return


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return "*INITIAL CONDITIONS,TYPE=TEMPERATURE\n"


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, inittemp_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    initialtemp = inittemp_obj.initialTemperature.getValueAs("K")

    if inittemp_obj.References and len(inittemp_obj.References) > 0:
        f.write(f"{inittemp_obj.Name},{initialtemp}\n")
    else:
        f.write(f"{ccxwriter.ccx_nall},{initialtemp}\n")
