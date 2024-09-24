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

__title__ = "FreeCAD FEM calculix constraint tie"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


from FreeCAD import Units, Vector


def get_analysis_types():
    return "all"  # write for all analysis types


def get_sets_name():
    return "constraints_tie_surface_sets"


def get_constraint_title():
    return "Tie Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, tie_obj, ccxwriter):
    # slave DEP
    f.write(f"*SURFACE, NAME=TIE_DEP{tie_obj.Name}\n")
    for i in femobj["TieSlaveFaces"]:
        f.write(f"{i[0]},S{i[1]}\n")
    # master IND
    f.write(f"*SURFACE, NAME=TIE_IND{tie_obj.Name}\n")
    for i in femobj["TieMasterFaces"]:
        f.write(f"{i[0]},S{i[1]}\n")


def write_constraint(f, femobj, tie_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    tolerance = tie_obj.Tolerance.getValueAs("mm").Value
    adjust = ""
    symmetry = ""
    tie_name = tie_obj.Name
    if not tie_obj.Adjust:
        adjust = ", ADJUST=NO"

    if tie_obj.CyclicSymmetry:
        symmetry = ", CYCLIC SYMMETRY"

    f.write(
        "*TIE, POSITION TOLERANCE={:.13G}{}{}, NAME=TIE{}\n".format(
            tolerance, adjust, symmetry, tie_name
        )
    )
    ind_surf = f"TIE_IND{tie_name}"
    dep_surf = f"TIE_DEP{tie_name}"
    f.write(f"{dep_surf}, {ind_surf}\n")

    # write CYCLIC SYMMETRY MODEL card
    if tie_obj.CyclicSymmetry:
        f.write(
            "*CYCLIC SYMMETRY MODEL, N={}, NGRAPH={}, TIE=TIE{}, ELSET=Eall\n".format(
                tie_obj.Sectors, tie_obj.ConnectedSectors, tie_name
            )
        )

        # get symmetry axis points
        vec_a = tie_obj.SymmetryAxis.Base
        vec_b = tie_obj.SymmetryAxis * Vector(0, 0, 1)

        set_unit = lambda x: Units.Quantity(x, Units.Length).getValueAs("mm").Value
        point_a = [set_unit(coord) for coord in vec_a]
        point_b = [set_unit(coord) for coord in vec_b]

        f.write("{:.13G},{:.13G},{:.13G},{:.13G},{:.13G},{:.13G}\n".format(*point_a, *point_b))
