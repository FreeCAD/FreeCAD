# ***************************************************************************
# *   Copyright (c) 2017-2023 Johannes Hartung <j.hartung@gmx.net>          *
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

__title__ = "FreeCAD Fenics XML mesh writer"
__author__ = "Johannes Hartung"
__url__ = "https://www.freecad.org"

## @package exportFenicsXML
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh XML writer for FEM workbench


from xml.etree import ElementTree as ET  # parsing xml files and exporting

from FreeCAD import Console

from .importToolsFem import get_FemMeshObjectDimension
from .importToolsFem import get_FemMeshObjectElementTypes
from .importToolsFem import get_MaxDimElementFromList


def write_fenics_mesh_xml(fem_mesh_obj, outputfile):
    """
        For the export, we only have to use the highest dimensional entities and their
        vertices to be exported.
        For second order elements, we have to delete the mid element nodes.
    """

    # TODO: check for second order elements
    # (reduce element order would be ok - all elements have at least the first
    # necessary nodes)

    FreeCAD_to_Fenics_dict = {
        "Triangle": "triangle",
        "Tetra": "tetrahedron",
        "Hexa": "hexahedron",
        "Edge": "interval",
        "Node": "point",
        "Quadrangle": "quadrilateral",

        "Polygon": "unknown", "Polyhedron": "unknown",
        "Prism": "unknown", "Pyramid": "unknown",
    }

    XML_Number_of_Nodes_dict = {
        "point": 1,
        "interval": 2,
        "triangle": 3,
        "quadrilateral": 4,
        "tetrahedron": 4,
        "hexahedron": 8
    }

    Console.PrintMessage(f"Converting {fem_mesh_obj.Label} to fenics XML File\n")
    Console.PrintMessage(f"Dimension of mesh: {get_FemMeshObjectDimension(fem_mesh_obj)}\n")

    elements_in_mesh = get_FemMeshObjectElementTypes(fem_mesh_obj)
    Console.PrintMessage(f"Elements appearing in mesh: {str(elements_in_mesh)}\n")
    celltype_in_mesh = get_MaxDimElementFromList(elements_in_mesh)
    (num_cells, cellname_fc, dim_cell) = celltype_in_mesh
    cellname_fenics = FreeCAD_to_Fenics_dict[cellname_fc]
    num_verts_cell = XML_Number_of_Nodes_dict[cellname_fenics]
    Console.PrintMessage(f"Celltype in mesh -> {str(celltype_in_mesh)} " +
                         f"and its Fenics name: {cellname_fenics}\n")

    root = ET.Element("dolfin", dolfin="http://fenicsproject.org")
    meshchild = ET.SubElement(root, "mesh", celltype=cellname_fenics, dim=str(dim_cell))
    vertices = ET.SubElement(meshchild, "vertices", size=str(fem_mesh_obj.FemMesh.NodeCount))

    for (nodeind, fc_vec) in list(fem_mesh_obj.FemMesh.Nodes.items()):
        ET.SubElement(
            vertices, "vertex", index=str(nodeind - 1),
            # FC starts from 1, fenics starts from 0 to size-1
            x=str(fc_vec[0]), y=str(fc_vec[1]), z=str(fc_vec[2]))

    cells = ET.SubElement(meshchild, "cells", size=str(num_cells))
    if dim_cell == 3:
        fc_cells = fem_mesh_obj.FemMesh.Volumes
    elif dim_cell == 2:
        fc_cells = fem_mesh_obj.FemMesh.Faces
    elif dim_cell == 1:
        fc_cells = fem_mesh_obj.FemMesh.Edges
    else:
        fc_cells = ()

    for (fen_ind, fc_volume_ind) in enumerate(fc_cells):
        # FC starts after all other entities, fenics start from 0 to size-1
        nodeindices = fem_mesh_obj.FemMesh.getElementNodes(fc_volume_ind)

        cell_args = {}
        for (vi, ni) in enumerate(nodeindices):
            if vi < num_verts_cell:  # XML only supports first order meshs
                cell_args["v" + str(vi)] = str(ni - 1)
        # generate as many v entries in dict as nodes are listed in cell
        # works only for first order elements

        ET.SubElement(cells, cellname_fenics, index=str(fen_ind), **cell_args)

    # ET.SubElement(meshchild, "data")
    # removed to eliminate warning from meshio

    fp = open(outputfile, "wb")
    fp.write(ET.tostring(root))
    # xml core functionality does not support pretty printing
    # so the output file looks quite ugly
    fp.close()
