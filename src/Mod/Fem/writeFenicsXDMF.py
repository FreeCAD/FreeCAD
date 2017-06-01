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

__title__ = "FreeCAD Fenics XDMF mesh writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package exportFenicsXDMF
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh XDMF writer for FEM workbench

from importToolsFem import get_FemMeshObjectDimension, get_FemMeshObjectElementTypes, get_MaxDimElementFromList, get_FemMeshObjectOrder
from lxml import etree  # parsing xml files and exporting
import numpy as np

ENCODING_ASCII = 'ASCII'
ENCODING_HDF5 = 'HDF5'

# TODO: export mesh functions (to be defined, cell functions, vertex functions, facet functions)
# TODO: integrate cell function
# TODO: check pyopen for other files

# we need numpy functions to later access and process large data sets in a fast manner
# also the hd5 support better works together with numpy


def numpy_array_to_str(npa):
    res = ""
    dt = str(npa.dtype)
    if 'int' in dt:
        res = "\n".join([" ".join([("%d" % s) for s in a]) for a in npa.tolist()])
    elif 'float' in dt:
        res = "\n".join([" ".join([("%3.6f" % s) for s in a]) for a in npa.tolist()])
    return res


def points_to_numpy(pts):
    return np.array([[p.x, p.y, p.z] for p in pts])


def tuples_to_numpy(tpls):
    return np.array([list(t) for t in tpls])


def write_fenics_mesh_points_xdmf(fem_mesh_obj, geometrynode, encoding=ENCODING_ASCII):
    """
        Writes either into hdf5 file or into open mesh file
    """

    numnodes = fem_mesh_obj.FemMesh.NodeCount

    # dim = get_MaxDimElementFromList(get_FemMeshObjectElementTypes(fem_mesh_obj))[2]
    # if dim == 2:
    #    geometrynode.set("GeometryType", "XY")
    # elif dim == 3:
    #    geometrynode.set("GeometryType", "XYZ")

    geometrynode.set("GeometryType", "XYZ")

    # TODO: investigate: real two dimensional geometry. At the moment it is saved as
    # flat 3d geometry.

    recalc_nodes_ind_dict = {}

    if encoding == ENCODING_ASCII:
        dataitem = etree.SubElement(geometrynode, "DataItem", Dimensions="%d %d" % (numnodes, 3), Format="XML")
        nodes = []
        for (ind, (key, node)) in enumerate(fem_mesh_obj.FemMesh.Nodes.iteritems()):
            nodes.append(node)
            recalc_nodes_ind_dict[key] = ind

        dataitem.text = numpy_array_to_str(points_to_numpy(nodes))
    elif encoding == ENCODING_HDF5:
        pass

    return recalc_nodes_ind_dict


def write_fenics_mesh_volumes_xdmf(fem_mesh_obj, topologynode, rd, encoding=ENCODING_ASCII):
    (num_cells, name_cell, dim_cell) = get_MaxDimElementFromList(get_FemMeshObjectElementTypes(fem_mesh_obj))
    element_order = get_FemMeshObjectOrder(fem_mesh_obj)

    FreeCAD_to_Fenics_XDMF_dict = {
        ("Node", 1): ("polyvertex", 1),
        ("Edge", 1): ("polyline", 2),
        ("Edge", 2): ("edge_3", 3),
        ("Triangle", 1): ("triangle", 3),
        ("Triangle", 2): ("tri_6", 6),
        ("Tetra", 1): ("tetrahedron", 4),
        ("Tetra", 2): ("tet_10", 10)
    }

    (topology_type, nodes_per_element) = FreeCAD_to_Fenics_XDMF_dict[(name_cell, element_order)]

    topologynode.set("TopologyType", topology_type)
    topologynode.set("NumberOfElements", str(num_cells))
    topologynode.set("NodesPerElement", str(nodes_per_element))

    if dim_cell == 3:
        fc_cells = fem_mesh_obj.FemMesh.Volumes
    elif dim_cell == 2:
        fc_cells = fem_mesh_obj.FemMesh.Faces
    elif dim_cell == 1:
        fc_cells = fem_mesh_obj.FemMesh.Edges
    elif dim_cell == 0:
        fc_cells = fem_mesh_obj.FemMesh.Nodes
    else:
        fc_cells = []
        print("Dimension of mesh incompatible with export XDMF function: %d" % (dim_cell,))

    nodeindices = [(rd[ind] for ind in fem_mesh_obj.FemMesh.getElementNodes(fc_volume_ind)) for (fen_ind, fc_volume_ind) in enumerate(fc_cells)]
    # FC starts after all other entities, fenics start from 0 to size-1
    # write nodeindices into dict to access them later

    if encoding == ENCODING_ASCII:
        dataitem = etree.SubElement(topologynode, "DataItem", NumberType="UInt", Dimensions="%d %d" % (num_cells, nodes_per_element), Format="XML")
        dataitem.text = numpy_array_to_str(tuples_to_numpy(nodeindices))
    elif encoding == ENCODING_HDF5:
        pass


def write_fenics_mesh_cellfunctions(fem_mesh_obj, mycellvalues, attributenode, encoding=ENCODING_ASCII):
    attributenode.set("AttributeType", "Scalar")
    attributenode.set("Center", "Cell")
    attributenode.set("Name", "f")

    (num_cells, name_cell, dim_cell) = get_MaxDimElementFromList(get_FemMeshObjectElementTypes(fem_mesh_obj))

    if encoding == ENCODING_ASCII:
        dataitem = etree.SubElement(attributenode, "DataItem", Dimensions="%d %d" % (num_cells, 1), Format="XML")
        dataitem.text = numpy_array_to_str(np.random.random((num_cells, 1)))
    elif encoding == ENCODING_HDF5:
        pass


def write_fenics_mesh_xdmf(fem_mesh_obj, outputfile, encoding=ENCODING_ASCII):
    """
        For the export of xdmf.
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

    print("Converting " + fem_mesh_obj.Label + " to fenics XDMF File")
    print("Dimension of mesh: %d" % (get_FemMeshObjectDimension(fem_mesh_obj),))

    elements_in_mesh = get_FemMeshObjectElementTypes(fem_mesh_obj)
    print("Elements appearing in mesh: %s" % (str(elements_in_mesh),))
    celltype_in_mesh = get_MaxDimElementFromList(elements_in_mesh)
    (num_cells, cellname_fc, dim_cell) = celltype_in_mesh
    cellname_fenics = FreeCAD_to_Fenics_dict[cellname_fc]
    print("Celltype in mesh -> %s and its Fenics dolfin name: %s" % (str(celltype_in_mesh), cellname_fenics))

    root = etree.Element("Xdmf", version="3.0")
    domain = etree.SubElement(root, "Domain")
    grid = etree.SubElement(domain, "Grid", Name="mesh", GridType="Uniform")
    topology = etree.SubElement(grid, "Topology")
    geometry = etree.SubElement(grid, "Geometry")

    # attribute = etree.SubElement(grid, "Attribute") #  for cell functions

    recalc_dict = write_fenics_mesh_points_xdmf(fem_mesh_obj, geometry, encoding=encoding)
    write_fenics_mesh_volumes_xdmf(fem_mesh_obj, topology, recalc_dict, encoding=encoding)

    # TODO: improve cell functions support
    # write_fenics_mesh_cellfunctions(fem_mesh_obj, {}, attribute, encoding=encoding)

    fp = open(outputfile, "w")
    fp.write('''<?xml version="1.0"?>\n<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>\n''')
    fp.write(etree.tostring(root, pretty_print=True))
    fp.close()
