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

__title__ = "FreeCAD Fenics mesh reader and writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"


# TODO: check for second order elements
# TODO: export mesh functions (to be defined, cell functions, vertex functions, facet functions)


## @package importFenicsMesh
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh reader and writer for FEM workbench

import FreeCAD
import importToolsFem
import os
import itertools
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
    import_fenics_mesh(filename)


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

########## Export Section ###################
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

############ Import Section ############


def import_fenics_mesh(filename, analysis=None):
    '''insert a FreeCAD FEM Mesh object in the ActiveDocument
    '''
    mesh_data = read_fenics_mesh(filename)
    mesh_name = os.path.basename(os.path.splitext(filename)[0])
    femmesh = importToolsFem.make_femmesh(mesh_data)
    if femmesh:
        mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = femmesh


def read_fenics_mesh(xmlfilename):
    '''
        Returns element dictionary to be evaluated by make_femmesh later
    '''

    Fenics_to_FreeCAD_dict = {
        "triangle": "tria3",
        "tetrahedron": "tetra4",
        "hexahedron": "hexa8",
        "interval": "seg2",
        "quadrilateral": "quad4",
    }

    def read_mesh_block(mesh_block):
        '''
            Reading mesh block from XML file.
            The mesh block only contains cells and vertices.
        '''
        dim = int(mesh_block.get("dim"))
        cell_type = mesh_block.get("celltype")

        vertex_size = 0

        print("Mesh dimension: %d" % (dim,))
        print("Mesh cell type: %s" % (cell_type,))

        cells_parts_dim = {'point': {0: 1},
                           'interval': {0: 2, 1: 1},
                           'triangle': {0: 3, 1: 3, 2: 1},
                           'tetrahedron': {0: 4, 1: 6, 2: 4, 3: 1},
                           'quadrilateral': {0: 4, 1: 4, 2: 1},
                           'hexahedron': {0: 8, 1: 12, 2: 6, 3: 1}}

        find_vertices = mesh_block.find("vertices")
        find_cells = mesh_block.find("cells")

        nodes_dict = {}
        cell_dict = {}

        if find_vertices is None:
            print("No vertices found!")
        else:
            vertex_size = int(find_vertices.attrib.get("size"))
            print("Reading %d vertices" % (vertex_size,))

            for vertex in find_vertices:
                ind = int(vertex.get("index"))

                if vertex.tag.lower() == 'vertex':
                    [node_x, node_y, node_z] = [float(vertex.get(coord, 0.)) for coord in ["x", "y", "z"]]

                    nodes_dict[ind + 1] = FreeCAD.Vector(node_x, node_y, node_z)
                    # increase node index by one, since fenics starts at 0, FreeCAD at 1
                    # print("%d %f %f %f" % (ind, node_x, node_y, node_z))
                else:
                    print("found strange vertex tag: %s" % (vertex.tag,))

        if find_cells is None:
            print("No cells found!")
        else:
            print("Reading %d cells" % (int(find_cells.attrib.get("size")),))
            for cell in find_cells:
                ind = int(cell.get("index"))

                if cell.tag.lower() != cell_type.lower():
                    print("Strange mismatch between cell type %s and cell tag %s" % (cell_type, cell.tag.lower()))
                num_vertices = cells_parts_dim[cell_type][0]

                vtupel = tuple([int(cell.get("v" + str(vnum))) + 1 for vnum in range(num_vertices)])
                # generate "v0", "v1", ... from dimension lookup table
                # increase numbers by one to match FC numbering convention

                cell_dict[ind + 1] = vtupel

                # valtupel = tuple([ind] + list(vtupel))
                # print(("%d " + ("%d "*len(vtupel))) % valtupel)

        return (nodes_dict, cell_dict, cell_type, dim)

    def generate_lower_dimensional_structures(nodes, cell_dict, cell_type, dim):

        def correct_volume_det(element_dict):
            '''
                Checks whether the cell elements
                all have the same volume (<0?)
                sign (is necessary to avoid negative
                Jacobian errors).
                Works only with tet4 and tri3 elements at the moment
            '''
            if dim == 3:
                for (ind, tet) in element_dict['tetra4'].iteritems():
                    v0 = nodes[tet[0]]
                    v1 = nodes[tet[1]]
                    v2 = nodes[tet[2]]
                    v3 = nodes[tet[3]]
                    a = v1 - v0
                    b = v2 - v0
                    c = v3 - v0
                    if a.dot(b.cross(c)) > 0:
                        element_dict['tetra4'][ind] = (tet[1], tet[0], tet[2], tet[3])
            if dim == 2:
                nz = FreeCAD.Vector(0., 0., 1.)
                for (ind, tria) in element_dict['tria3'].iteritems():
                    v0 = nodes[tria[0]]
                    v1 = nodes[tria[1]]
                    v2 = nodes[tria[2]]
                    a = v1 - v0
                    b = v2 - v0
                    if nz.dot(a.cross(b)) < 0:
                        element_dict['tria3'][ind] = (tria[1], tria[0], tria[2])

        element_dict = {}
        element_counter = {}

        # TODO: remove upper level lookup
        for (key, val) in Fenics_to_FreeCAD_dict.iteritems():
            element_dict[val] = {}
            element_counter[key] = 0  # count every distinct element and sub element type

        def addtupletodict(di, tpl, counter):
            sortedtpl = tuple(sorted(tpl))
            if di.get(sortedtpl) is None:
                di[sortedtpl] = counter
                counter += 1
            return counter

        def invertdict(dic):
            invdic = {}
            for (key, it) in dic.iteritems():
                invdic[it] = key
            return invdic

        num_vert_dict = {'interval': 2,
                         'triangle': 3,
                         'tetrahedron': 4,
                         'hexahedron': 8,
                         'quadrilateral': 4}
        lower_dims_dict = {'interval': [],
                           'triangle': ['interval'],
                           'tetrahedron': ['triangle', 'interval'],
                           'hexahedron': ['quadrilateral', 'interval'],
                           'quadrilateral': ['interval']}

        for (cell_index, cell) in cell_dict.iteritems():
            cell_lower_dims = lower_dims_dict[cell_type]
            element_counter[cell_type] += 1
            element_dict[Fenics_to_FreeCAD_dict[cell_type]][cell] = element_counter[cell_type]
            for ld in cell_lower_dims:
                for vertextuple in itertools.combinations(cell, num_vert_dict[ld]):
                    element_counter[ld] = addtupletodict(
                        element_dict[Fenics_to_FreeCAD_dict[ld]],
                        vertextuple,
                        element_counter[ld])

        length_counter = len(nodes)
        for (key, val_dict) in element_dict.iteritems():
            # to ensure distinct indices for FreeCAD
            for (vkey, it) in val_dict.iteritems():
                val_dict[vkey] = it + length_counter
            length_counter += len(val_dict)
            # inverse of the dict (dict[key] = val -> dict[val] = key)
            element_dict[key] = invertdict(val_dict)

        correct_volume_det(element_dict)

        return element_dict

    nodes = {}
    element_dict = {}
    # TODO: remove two times initialization
    for val in Fenics_to_FreeCAD_dict.itervalues():
        element_dict[val] = {}

    tree = etree.parse(xmlfilename)
    root = tree.getroot()

    if root.tag.lower() != "dolfin":
        print("Strange root tag, should be dolfin!")

    find_mesh = root.find("mesh")
    if find_mesh is not None:  # these are consistency checks of the XML structure
        print("Mesh found")
        (nodes, cells_dict, cell_type, dim) = read_mesh_block(find_mesh)
        element_dict = generate_lower_dimensional_structures(nodes, cells_dict, cell_type, dim)
    else:
        print("No mesh found")

    if root.find("data") is not None:
        print("Internal mesh data found")

    return {'Nodes': nodes,
            'Hexa8Elem': {}, 'Penta6Elem': {}, 'Tetra4Elem': element_dict['tetra4'], 'Tetra10Elem': {},
            'Penta15Elem': {}, 'Hexa20Elem': {}, 'Tria3Elem': element_dict['tria3'], 'Tria6Elem': {},
            'Quad4Elem': element_dict['quad4'], 'Quad8Elem': {}, 'Seg2Elem': element_dict['seg2']
            }
