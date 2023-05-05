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

__title__ = "FreeCAD FEM calculix constraint fixed"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return "all"    # write for all analysis types


def get_sets_name():
    return "constraints_fixed_node_sets"


def get_constraint_title():
    return "Fixed Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, fix_obj, ccxwriter):
    if (
        ccxwriter.femmesh.Volumes
        and (
            len(ccxwriter.member.geos_shellthickness) > 0
            or len(ccxwriter.member.geos_beamsection) > 0
        )
    ):
        if len(femobj["NodesSolid"]) > 0:
            f.write("*NSET,NSET={}Solid\n".format(fix_obj.Name))
            for n in femobj["NodesSolid"]:
                f.write("{},\n".format(n))
        if len(femobj["NodesFaceEdge"]) > 0:
            f.write("*NSET,NSET={}FaceEdge\n".format(fix_obj.Name))
            for n in femobj["NodesFaceEdge"]:
                f.write("{},\n".format(n))
    else:
        f.write("*NSET,NSET=" + fix_obj.Name + "\n")
        for n in femobj["Nodes"]:
            f.write("{},\n".format(n))


def write_constraint(f, femobj, fix_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    if (
        ccxwriter.femmesh.Volumes
        and (
            len(ccxwriter.member.geos_shellthickness) > 0
            or len(ccxwriter.member.geos_beamsection) > 0
        )
    ):
        if len(femobj["NodesSolid"]) > 0:
            f.write("*BOUNDARY\n")
            f.write(fix_obj.Name + "Solid" + ",1\n")
            f.write(fix_obj.Name + "Solid" + ",2\n")
            f.write(fix_obj.Name + "Solid" + ",3\n")
            f.write("\n")
        if len(femobj["NodesFaceEdge"]) > 0:
            f.write("*BOUNDARY\n")
            f.write(fix_obj.Name + "FaceEdge" + ",1\n")
            f.write(fix_obj.Name + "FaceEdge" + ",2\n")
            f.write(fix_obj.Name + "FaceEdge" + ",3\n")
            f.write(fix_obj.Name + "FaceEdge" + ",4\n")
            f.write(fix_obj.Name + "FaceEdge" + ",5\n")
            f.write(fix_obj.Name + "FaceEdge" + ",6\n")
            f.write("\n")
    else:
        f.write("*BOUNDARY\n")
        f.write(fix_obj.Name + ",1\n")
        f.write(fix_obj.Name + ",2\n")
        f.write(fix_obj.Name + ",3\n")
        if ccxwriter.member.geos_beamsection or ccxwriter.member.geos_shellthickness:
            f.write(fix_obj.Name + ",4\n")
            f.write(fix_obj.Name + ",5\n")
            f.write(fix_obj.Name + ",6\n")
        f.write("\n")
