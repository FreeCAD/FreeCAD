# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Johannes Hartung <j.hartung@gmx.net>             *
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
from __future__ import print_function

__title__ = "FreeCAD Fenics mesh writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package importFenicsMesh
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh writer for FEM workbench

import FreeCAD
import os
from lxml import etree  # parsing xml files and exporting


# Template copied from importZ88Mesh.py. Thanks Bernd!
########## generic FreeCAD import and export methods ##########
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    FreeCAD.Console.PrintError("Not yet implemented")
    # import_fenics_mesh(filename)


def export(objectslist, filename):
    "called when freecad exports a file"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return

    write_fenics_mesh(obj, filename)


########## module specific methods ##########
# Helper
def get_FemMeshObjectDimension(fem_mesh_obj):
    """ Count all entities in an abstract sense, to distinguish which dimension the mesh is
        (i.e. linemesh, facemesh, volumemesh)
    """
    dim = None

    if fem_mesh_obj.FemMesh.Nodes != ():
        dim = 0
    if fem_mesh_obj.FemMesh.Edges != ():
        dim = 1
    if fem_mesh_obj.FemMesh.Faces != ():
        dim = 2
    if fem_mesh_obj.FemMesh.Volumes != ():
        dim = 3

    return dim


def get_FemMeshObjectElementTypes(fem_mesh_obj, remove_zero_element_entries=True):
    """
        Spit out all elements in the mesh with their appropriate dimension.
    """
    FreeCAD_element_names = [
        "Node", "Edge", "Hexa", "Polygon", "Polyhedron",
        "Prism", "Pyramid", "Quadrangle", "Tetra", "Triangle"]
    FreeCAD_element_dims = [0, 1, 3, 2, 3, 3, 3, 2, 3, 2]

    elements_list_with_zero = [(eval("fem_mesh_obj.FemMesh." + s + "Count"), s, d) for (s, d) in zip(FreeCAD_element_names, FreeCAD_element_dims)]
    # ugly but necessary
    if remove_zero_element_entries:
        elements_list = [(num, s, d) for (num, s, d) in elements_list_with_zero if num > 0]
    else:
        elements_list = elements_list_with_zero

    return elements_list


def get_MaxDimElementFromList(elem_list):
    """
        Gets element with the maximal dimension in the mesh to determine cells.
    """
    elem_list.sort(key=lambda (num, s, d): d)
    return elem_list[-1]


def write_fenics_mesh(fem_mesh_obj, outputfile):
    """
        For the export, we only have to use the highest dimensional entities and their
        vertices to be exported. (For second order elements, we have to delete the mid element nodes.)
    """
    # TODO: check for second order elements
    # TODO: export mesh functions (to be defined, cell functions, vertex functions, facet functions)

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

    print("Converting " + fem_mesh_obj.Label + " to fenics XML File")
    print("Dimension of mesh: %d" % (get_FemMeshObjectDimension(fem_mesh_obj),))

    elements_in_mesh = get_FemMeshObjectElementTypes(fem_mesh_obj)
    print("Elements appearing in mesh: %s" % (str(elements_in_mesh),))
    celltype_in_mesh = get_MaxDimElementFromList(elements_in_mesh)
    (num_cells, cellname_fc, dim_cell) = celltype_in_mesh
    cellname_fenics = FreeCAD_to_Fenics_dict[cellname_fc]
    print("Celltype in mesh -> %s and its Fenics name: %s" % (str(celltype_in_mesh), cellname_fenics))

    root = etree.Element("dolfin", dolfin="http://fenicsproject.org")
    meshchild = etree.SubElement(root, "mesh", celltype=cellname_fenics, dim=str(dim_cell))
    vertices = etree.SubElement(meshchild, "vertices", size=str(fem_mesh_obj.FemMesh.NodeCount))

    for (nodeind, fc_vec) in fem_mesh_obj.FemMesh.Nodes.iteritems():  # python2
        etree.SubElement(
            vertices, "vertex", index=str(nodeind - 1),
            # FC starts from 1, fenics starts from 0 to size-1
            x=str(fc_vec[0]), y=str(fc_vec[1]), z=str(fc_vec[2]))

    cells = etree.SubElement(meshchild, "cells", size=str(num_cells))
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
            cell_args["v" + str(vi)] = str(ni - 1)
        # generate as many v entries in dict as nodes are listed in cell (works only for first order elements)

        etree.SubElement(cells, cellname_fenics, index=str(fen_ind), **cell_args)

    etree.SubElement(meshchild, "data")

    fp = pyopen(outputfile, "w")
    fp.write(etree.tostring(root, pretty_print=True))
    fp.close()
