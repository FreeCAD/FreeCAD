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

__title__ = "FreeCAD FEM calculix write inpfile mesh"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


import codecs
from os.path import join

from femmesh import meshtools
from .spatial_hash_merger import merge_inp_nodes


def write_mesh(ccxwriter):

    element_param = 1  # highest element order only
    group_param = False  # do not write mesh group data

    is_reduced = ccxwriter.solver_obj.ReducedIntegration

    vol_variant = "reduced" if is_reduced else "standard"
    if ccxwriter.solver_obj.ExcludeBendingStiffness:
        edge_variant = "truss"
    else:
        if is_reduced:
            edge_variant = "beam reduced"
        else:
            edge_variant = "beam"
        if ccxwriter.member.geos_fluidsection:
            edge_variant = "network"

    face_variant = "shell"

    if ccxwriter.solver_obj.ModelSpace == "3D" and ccxwriter.solver_obj.ExcludeBendingStiffness:
        face_variant = "membrane"
    elif ccxwriter.solver_obj.ModelSpace == "plane stress":
        face_variant = "stress"
    elif ccxwriter.solver_obj.ModelSpace == "plane strain":
        face_variant = "strain"
    elif ccxwriter.solver_obj.ModelSpace == "axisymmetric":
        face_variant = "axisymmetric"

    if is_reduced:
        face_variant += " reduced"

    merge_tolerance = getattr(ccxwriter.analysis_obj, "MergeTolerance", 0.1)

    if ccxwriter.split_inpfile:
        write_name = "femesh"
        file_name_split = ccxwriter.mesh_name + "_" + write_name + ".inp"
        ccxwriter.femmesh_file = join(ccxwriter.dir_name, file_name_split)

        ccxwriter.femmesh.writeABAQUS(
            ccxwriter.femmesh_file,
            element_param,
            group_param,
            volVariant=vol_variant,
            faceVariant=face_variant,
            edgeVariant=edge_variant,
        )

        merge_inp_nodes(ccxwriter.femmesh_file, tolerance=merge_tolerance)

        inpfile = codecs.open(ccxwriter.file_name, "w", encoding="utf-8")
        inpfile.write("{}\n".format(59 * "*"))
        inpfile.write(f"** {write_name}\n")
        inpfile.write(f"*INCLUDE,INPUT={file_name_split}\n")

    else:
        ccxwriter.femmesh_file = ccxwriter.file_name
        ccxwriter.femmesh.writeABAQUS(
            ccxwriter.femmesh_file,
            element_param,
            group_param,
            volVariant=vol_variant,
            faceVariant=face_variant,
            edgeVariant=edge_variant,
        )

        merge_inp_nodes(ccxwriter.femmesh_file, tolerance=merge_tolerance)

        inpfile = codecs.open(ccxwriter.femmesh_file, "a", encoding="utf-8")
        inpfile.write("\n\n")

    return inpfile
