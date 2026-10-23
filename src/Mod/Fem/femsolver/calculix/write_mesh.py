# SPDX-License-Identifier: LGPL-2.1-or-later

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


from os.path import join

import FreeCAD

from femmesh import meshtools


def is_linear_mesh(femmesh):
    # Only the highest dimension is written, its first element tells the order of the mesh
    if femmesh.VolumeCount:
        return len(femmesh.getElementNodes(femmesh.Volumes[0])) in (4, 5, 6, 8)
    if femmesh.FaceCount:
        return len(femmesh.getElementNodes(femmesh.Faces[0])) in (3, 4)
    return False


def get_element_variants(ccxwriter):
    """Volume, face and edge element variants passed to writeABAQUS"""

    formulation = ccxwriter.solver_obj.ElementFormulation
    is_reduced = formulation == "reduced integration"

    # Reduced integration solid/beam elements, incompatible mode hexahedra (the other volumes
    # standard), or trusses for beams if these settings are enabled in the ccx solver
    vol_variant = {"reduced integration": "reduced", "incompatible mode": "incompatible"}.get(
        formulation, "standard"
    )
    if ccxwriter.solver_obj.ExcludeBendingStiffness:
        edge_variant = "truss"
    else:
        if is_reduced:
            edge_variant = "beam reduced"
        else:
            edge_variant = "beam"
        # Check to see if fluid sections are in analysis and use D network element type
        if ccxwriter.member.geos_fluidsection:
            edge_variant = "network"

    face_variant = "shell"

    # Use 2D elements if model space is not set to 3D
    if ccxwriter.solver_obj.ModelSpace == "3D" and ccxwriter.solver_obj.ExcludeBendingStiffness:
        face_variant = "membrane"
    elif ccxwriter.solver_obj.ModelSpace == "plane stress":
        face_variant = "stress"
    elif ccxwriter.solver_obj.ModelSpace == "plane strain":
        face_variant = "strain"
    elif ccxwriter.solver_obj.ModelSpace == "axisymmetric":
        face_variant = "axisymmetric"

    # Add "reduced" to the face element group's name if Reduced Integration is enabled
    if is_reduced:
        face_variant += " reduced"

    # No hourglass control in a CalculiX heat transfer, linear reduced elements give
    # checkerboard temperatures there
    if is_reduced and ccxwriter.analysis_type == "thermomech" and is_linear_mesh(ccxwriter.femmesh):
        FreeCAD.Console.PrintWarning(
            "Reduced integration on a linear mesh (C3D8R, S4R, CPS4R, CPE4R, CAX4R) can give "
            "wrong temperatures in a CalculiX thermal analysis, without any solver error. "
            "Set the solver Element Formulation to standard or incompatible mode.\n"
        )

    return vol_variant, face_variant, edge_variant


def write_mesh(ccxwriter):

    element_param = 1  # Highest element order only
    group_param = False  # Do not write mesh group data

    vol_variant, face_variant, edge_variant = get_element_variants(ccxwriter)

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

        inpfile = open(ccxwriter.file_name, "w", encoding="utf-8")
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

        # reopen file with "append" to add all the rest
        inpfile = open(ccxwriter.femmesh_file, "a", encoding="utf-8")
        inpfile.write("\n\n")

    return inpfile
