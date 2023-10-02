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

__title__ = "FreeCAD Fenics XML mesh reader"
__author__ = "Johannes Hartung"
__url__ = "https://www.freecad.org"

## @package importFenicsXML
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh XML reader for FEM workbench

import itertools
from xml.etree import ElementTree as ET

import FreeCAD
from FreeCAD import Console


def read_fenics_mesh_xml(xmlfilename):
    """
        Returns element dictionary to be evaluated by make_femmesh later
    """

    Fenics_to_FreeCAD_dict = {
        "triangle": "tria3",
        "tetrahedron": "tetra4",
        "hexahedron": "hexa8",
        "interval": "seg2",
        "quadrilateral": "quad4",
    }

    def read_mesh_block(mesh_block):
        """
            Reading mesh block from XML file.
            The mesh block only contains cells and vertices.
        """
        dim = int(mesh_block.get("dim"))
        cell_type = mesh_block.get("celltype")

        vertex_size = 0

        Console.PrintLog(f"Mesh dimension: {dim}\n")
        Console.PrintLog(f"Mesh cell type: {cell_type}\n")

        # every cell type contains a dict with key=dimension and value=number

        cells_parts_dim = {"point": {0: 1},
                           "interval": {0: 2, 1: 1},
                           "triangle": {0: 3, 1: 3, 2: 1},
                           "tetrahedron": {0: 4, 1: 6, 2: 4, 3: 1},
                           "quadrilateral": {0: 4, 1: 4, 2: 1},
                           "hexahedron": {0: 8, 1: 12, 2: 6, 3: 1}}

        find_vertices = mesh_block.find("vertices")
        find_cells = mesh_block.find("cells")

        nodes_dict = {}
        cell_dict = {}

        if find_vertices is None:
            Console.PrintWarning("No vertices found!\n")
        else:
            vertex_size = int(find_vertices.attrib.get("size"))
            Console.PrintLog(f"Reading {vertex_size} vertices\n")

            for vertex in find_vertices:
                ind = int(vertex.get("index"))

                if vertex.tag.lower() == "vertex":
                    [node_x, node_y, node_z] = [
                        float(vertex.get(coord, 0.)) for coord in ["x", "y", "z"]
                    ]

                    nodes_dict[ind + 1] = FreeCAD.Vector(node_x, node_y, node_z)
                    # increase node index by one, since fenics starts at 0, FreeCAD at 1
                    # print("%d %f %f %f" % (ind, node_x, node_y, node_z))
                else:
                    Console.PrintWarning(f"found strange vertex tag: {vertex.tag}\n")

        if find_cells is None:
            Console.PrintWarning("No cells found!\n")
        else:
            Console.PrintLog(f"Reading {int(find_cells.attrib.get('size'))} cells\n")
            for cell in find_cells:
                ind = int(cell.get("index"))

                if cell.tag.lower() != cell_type.lower():
                    Console.PrintWarning("Strange mismatch between cell type " +
                                         f"{cell_type} and " +
                                         f"cell tag {cell.tag.lower()}\n")
                num_vertices = cells_parts_dim[cell_type][0]

                vtupel = tuple([
                    int(cell.get("v" + str(vnum))) + 1 for vnum in range(num_vertices)
                ])
                # generate "v0", "v1", ... from dimension lookup table
                # increase numbers by one to match FC numbering convention

                cell_dict[ind + 1] = vtupel

                # valtupel = tuple([ind] + list(vtupel))
                # print(("%d " + ("%d "*len(vtupel))) % valtupel)

        return (nodes_dict, cell_dict, cell_type, dim)

    def generate_lower_dimensional_structures(nodes, cell_dict, cell_type, dim):

        def correct_volume_det(element_dict):
            """
                Checks whether the cell elements
                all have the same volume (<0?)
                sign (is necessary to avoid negative
                Jacobian errors).
                Works only with tet4 and tri3 elements at the moment
            """
            if dim == 3:
                for (ind, tet) in list(element_dict["tetra4"].items()):
                    v0 = nodes[tet[0]]
                    v1 = nodes[tet[1]]
                    v2 = nodes[tet[2]]
                    v3 = nodes[tet[3]]
                    a = v1 - v0
                    b = v2 - v0
                    c = v3 - v0
                    if a.dot(b.cross(c)) > 0:
                        element_dict["tetra4"][ind] = (tet[1], tet[0], tet[2], tet[3])
            if dim == 2:
                nz = FreeCAD.Vector(0., 0., 1.)
                for (ind, tria) in list(element_dict["tria3"].items()):
                    v0 = nodes[tria[0]]
                    v1 = nodes[tria[1]]
                    v2 = nodes[tria[2]]
                    a = v1 - v0
                    b = v2 - v0
                    if nz.dot(a.cross(b)) < 0:
                        element_dict["tria3"][ind] = (tria[1], tria[0], tria[2])

        element_dict = {}
        element_counter = {}

        # TODO: remove upper level lookup
        for (key, val) in list(Fenics_to_FreeCAD_dict.items()):
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
            for (key, it) in list(dic.items()):
                invdic[it] = key
            return invdic

        num_vert_dict = {"interval": 2,
                         "triangle": 3,
                         "tetrahedron": 4,
                         "hexahedron": 8,
                         "quadrilateral": 4}
        lower_dims_dict = {"interval": [],
                           "triangle": ["interval"],
                           "tetrahedron": ["triangle", "interval"],
                           "hexahedron": ["quadrilateral", "interval"],
                           "quadrilateral": ["interval"]}

        # generate cell list from file
        # read vertex list from cells
        # generate lower dimensional objects in mesh from cell

        for (cell_index, cell) in list(cell_dict.items()):
            cell_lower_dims = lower_dims_dict[cell_type]
            element_counter[cell_type] += 1
            element_dict[Fenics_to_FreeCAD_dict[cell_type]][cell] = element_counter[cell_type]
            for ld in cell_lower_dims:
                for vertextuple in itertools.combinations(cell, num_vert_dict[ld]):
                    element_counter[ld] = addtupletodict(
                        element_dict[Fenics_to_FreeCAD_dict[ld]],
                        vertextuple,
                        element_counter[ld])

        length_counter = len(nodes)  # maintain distinct counting values
        # print("nodes")
        # print("len & len counter", length_counter)
        for (key, val_dict) in list(element_dict.items()):
            # to ensure distinct indices for FreeCAD
            # print("key: ", key)
            for (vkey, it) in list(val_dict.items()):
                val_dict[vkey] = it + length_counter  # maintain distinct element numbers
            len_val_dict = len(val_dict)
            if len_val_dict > 0:
                length_counter += len_val_dict + 1  # only if preceding list is not empty
            # print("len: ", len_val_dict)
            # print("lencounter: ", length_counter)
            # inverse of the dict (dict[key] = val -> dict[val] = key)
            element_dict[key] = invertdict(val_dict)

        correct_volume_det(element_dict)  # corrects negative determinants

        return element_dict  # returns complete element dictionary

    nodes = {}
    element_dict = {}
    # TODO: remove two times initialization
    for val in list(Fenics_to_FreeCAD_dict.values()):
        element_dict[val] = {}

    tree = ET.parse(xmlfilename)
    root = tree.getroot()

    if root.tag.lower() != "dolfin":
        Console.PrintWarning("Strange root tag, should be dolfin!\n")

    find_mesh = root.find("mesh")
    if find_mesh is not None:  # these are consistency checks of the XML structure
        Console.PrintMessage("Mesh found\n")
        (nodes, cells_dict, cell_type, dim) = read_mesh_block(find_mesh)
        element_dict = generate_lower_dimensional_structures(nodes, cells_dict, cell_type, dim)
        Console.PrintMessage("Show min max element dict\n")
        for (elm, numbers) in list(element_dict.items()):
            lst = sorted(list(numbers.items()), key=lambda x: x[0])
            if lst != []:
                Console.PrintMessage(f"{elm} min: {lst[0]} max: {lst[-1]}\n")
    else:
        Console.PrintError("No mesh found\n")

    if root.find("data") is not None:
        Console.PrintLog("Internal mesh data found\n")

    return {
        "Nodes": nodes,
        "Seg2Elem": element_dict["seg2"],
        "Seg3Elem": {},
        "Tria3Elem": element_dict["tria3"],
        "Tria6Elem": {},
        "Quad4Elem": element_dict["quad4"],
        "Quad8Elem": {},
        "Tetra4Elem": element_dict["tetra4"],
        "Tetra10Elem": {},
        "Hexa8Elem": {},
        "Hexa20Elem": {},
        "Penta6Elem": {},
        "Penta15Elem": {}
    }
