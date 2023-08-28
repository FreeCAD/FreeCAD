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


def get_constraint_title():
    return "Initial temperature constraint"


def get_before_write_constraint():
    return "*INITIAL CONDITIONS,TYPE=TEMPERATURE\n"


def get_after_write_constraint():
    return ""


def write_constraint(f, femobj, inittemp_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    f.write("{},{}\n".format(
        ccxwriter.ccx_nall,
        Units.Quantity(inittemp_obj.initialTemperature.getValueAs("K"))
    ))


# Should only be one object in the analysis
