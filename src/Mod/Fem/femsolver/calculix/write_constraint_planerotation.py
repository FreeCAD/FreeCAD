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

__title__ = "FreeCAD FEM calculix constraint planerotation"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


from femmesh import meshtools


def get_analysis_types():
    return "all"    # write for all analysis types


def get_sets_name():
    return "constraints_planerotation_node_sets"


def get_constraint_title():
    return "PlaneRotation Constraints"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def get_before_write_constraint():
    return ""


def get_after_write_constraint():
    return ""


def write_meshdata_constraint(f, femobj, fric_obj, ccxwriter):
    # write nodes to file
    if not ccxwriter.femnodes_mesh:
        ccxwriter.femnodes_mesh = ccxwriter.femmesh.Nodes
    # info about ccxwriter.constraint_conflict_nodes:
    # is used to check if MPC and constraint fixed and
    # constraint displacement share same nodes
    # because MPC"s and constraints fixed and
    # constraints displacement can't share same nodes.
    # Thus call write_node_sets_constraints_planerotation has to be
    # after constraint fixed and constraint displacement
    l_nodes = femobj["Nodes"]
    f.write("*NSET,NSET={}\n".format(fric_obj.Name))
    # Code to extract nodes and coordinates on the PlaneRotation support face
    nodes_coords = []
    for node in l_nodes:
        nodes_coords.append((
            node,
            ccxwriter.femnodes_mesh[node].x,
            ccxwriter.femnodes_mesh[node].y,
            ccxwriter.femnodes_mesh[node].z
        ))
    node_planerotation = meshtools.get_three_non_colinear_nodes(nodes_coords)
    for i in range(len(l_nodes)):
        if l_nodes[i] not in node_planerotation:
            node_planerotation.append(l_nodes[i])
    MPC_nodes = []
    for i in range(len(node_planerotation)):
        cnt = 0
        for j in range(len(ccxwriter.constraint_conflict_nodes)):
            if node_planerotation[i] == ccxwriter.constraint_conflict_nodes[j]:
                cnt = cnt + 1
        if cnt == 0:
            MPC = node_planerotation[i]
            MPC_nodes.append(MPC)
    for i in range(len(MPC_nodes)):
        f.write("{},\n".format(MPC_nodes[i]))


def write_constraint(f, femobj, fric_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    f.write("*MPC\n")
    f.write("PLANE,{}\n".format(fric_obj.Name))
