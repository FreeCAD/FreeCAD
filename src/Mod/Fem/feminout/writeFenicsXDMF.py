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

__title__ = "FreeCAD Fenics XDMF mesh writer"
__author__ = "Johannes Hartung"
__url__ = "https://www.freecad.org"

## @package exportFenicsXDMF
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh XDMF writer for FEM workbench

import numpy as np
from xml.etree import ElementTree as ET  # parsing xml files and exporting

from FreeCAD import Console

from .importToolsFem import get_FemMeshObjectDimension
from .importToolsFem import get_FemMeshObjectElementTypes
from .importToolsFem import get_FemMeshObjectOrder
from .importToolsFem import get_FemMeshObjectMeshGroups
from .importToolsFem import get_MaxDimElementFromList


ENCODING_ASCII = "ASCII"
ENCODING_HDF5 = "HDF5"

FreeCAD_Group_Dimensions = {
    "Vertex": 0,
    "Edge": 1,
    "Face": 2,
    "Volume": 3
}

FreeCAD_to_Fenics_XDMF_dict = {
    ("Node", 1): ("Polyvertex", 1),
    ("Edge", 1): ("Polyline", 2),
    ("Edge", 2): ("Edge_3", 3),
    ("Triangle", 1): ("Triangle", 3),
    ("Triangle", 2): ("Tri_6", 6),
    ("Tetra", 1): ("Tetrahedron", 4),
    ("Tetra", 2): ("Tet_10", 10)
}

# we need numpy functions to later access and process large data sets in a fast manner
# also the hd5 support works better together with numpy


def numpy_array_to_str(
    npa
):
    res = ""
    dt = str(npa.dtype)
    if "int" in dt:
        res = "\n".join([" ".join([("%d" % s) for s in a]) for a in npa.tolist()])
    elif "float" in dt:
        res = "\n".join([" ".join([("%3.6f" % s) for s in a]) for a in npa.tolist()])
    return res


def points_to_numpy(
    pts,
    dim=3
):
    return np.array([[p.x, p.y, p.z] for p in pts])[:, :dim]


def tuples_to_numpy(
    tpls,
    numbers_per_line
):
    return np.array([list(t) for t in tpls])[:, :numbers_per_line]


def write_fenics_mesh_points_xdmf(
    fem_mesh_obj,
    geometrynode,
    encoding=ENCODING_ASCII
):
    """
        Writes either into hdf5 file or into open mesh file
    """

    numnodes = fem_mesh_obj.FemMesh.NodeCount

    dim = get_MaxDimElementFromList(get_FemMeshObjectElementTypes(fem_mesh_obj))[2]
    effective_dim = dim
    if dim <= 2:
        effective_dim = 2  # effective dim is 2 for dim==1
        geometrynode.set("GeometryType", "XY")
    elif dim == 3:
        geometrynode.set("GeometryType", "XYZ")

    recalc_nodes_ind_dict = {}

    if encoding == ENCODING_ASCII:
        dataitem = ET.SubElement(
            geometrynode,
            "DataItem",
            Dimensions="%d %d" % (numnodes, effective_dim),
            Format="XML"
        )
        nodes = []
        for (ind, (key, node)) in enumerate(list(fem_mesh_obj.FemMesh.Nodes.items())):
            nodes.append(node)
            recalc_nodes_ind_dict[key] = ind

        dataitem.text = numpy_array_to_str(points_to_numpy(nodes, dim=effective_dim))
    elif encoding == ENCODING_HDF5:
        pass

    return recalc_nodes_ind_dict


def write_fenics_mesh_codim_xdmf(
    fem_mesh_obj,
    topologynode,
    nodes_dict,
    codim=0,
    encoding=ENCODING_ASCII
):
    mesh_dimension = get_FemMeshObjectDimension(fem_mesh_obj)

    element_types = get_FemMeshObjectElementTypes(fem_mesh_obj, remove_zero_element_entries=True)
    element_order = get_FemMeshObjectOrder(fem_mesh_obj)
    # we get all elements from mesh to decide which one to write by selection of codim
    """
    nodeindices = [(
        nodes_dict[ind] for ind in fem_mesh_obj.FemMesh.getElementNodes(fc_volume_ind)
    ) for (fen_ind, fc_volume_ind) in enumerate(fc_cells)]
    """
    writeout_element_dimension = mesh_dimension - codim

    (num_topo, name_topo, dim_topo) = (0, "", 0)
    for (num, name, dim) in element_types:
        if writeout_element_dimension == dim:
            (num_topo, name_topo, dim_topo) = (num, name, dim)

    (topology_type, nodes_per_element) = FreeCAD_to_Fenics_XDMF_dict[(name_topo, element_order)]

    topologynode.set("TopologyType", topology_type)
    topologynode.set("NumberOfElements", str(num_topo))
    topologynode.set("NodesPerElement", str(nodes_per_element))

    if dim_topo == 3:
        fc_topo = fem_mesh_obj.FemMesh.Volumes
    elif dim_topo == 2:
        fc_topo = fem_mesh_obj.FemMesh.Faces
    elif dim_topo == 1:
        fc_topo = fem_mesh_obj.FemMesh.Edges
    elif dim_topo == 0:
        fc_topo = fem_mesh_obj.FemMesh.Nodes
    else:
        fc_topo = []
        Console.PrintError("Dimension of mesh incompatible with export" +
                           f" XDMF function: {dim_topo}\n")

    nodeindices = [(
        nodes_dict[ind] for ind in fem_mesh_obj.FemMesh.getElementNodes(fc_topo_ind)
    ) for (fen_ind, fc_topo_ind) in enumerate(fc_topo)]

    if encoding == ENCODING_ASCII:
        dataitem = ET.SubElement(
            topologynode, "DataItem",
            NumberType="UInt",
            Dimensions="%d %d" % (num_topo, nodes_per_element),
            Format="XML"
        )
        dataitem.text = numpy_array_to_str(tuples_to_numpy(nodeindices, nodes_per_element))
    elif encoding == ENCODING_HDF5:
        pass

    return fc_topo


def write_fenics_mesh_scalar_cellfunctions(
    name, cell_array,
    attributenode,
    encoding=ENCODING_ASCII
):
    attributenode.set("AttributeType", "Scalar")
    attributenode.set("Center", "Cell")
    attributenode.set("Name", name)

    (num_cells, num_dims) = np.shape(cell_array)

    if encoding == ENCODING_ASCII:
        dataitem = ET.SubElement(
            attributenode, "DataItem",
            Dimensions="%d %d" % (num_cells, num_dims),
            Format="XML"
        )
        dataitem.text = numpy_array_to_str(cell_array)
    elif encoding == ENCODING_HDF5:
        pass


"""
Example: mesh with two topologies and one mesh function for the facet one

<?xml version="1.0"?>
<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>
<Xdmf Version="3.0" xmlns:xi="http://www.w3.org/2001/XInclude">
  <Domain>
    <Grid Name="mesh" GridType="Uniform">
      <Topology NumberOfElements="162" TopologyType="Tetrahedron" NodesPerElement="4">
        <DataItem Dimensions="162 4" NumberType="UInt" Format="XML">0 1 5 21
...
        </DataItem>
      </Topology>
      <Geometry GeometryType="XYZ">
        <DataItem Dimensions="64 3" Format="XML">0 0 0
...
        </DataItem>
      </Geometry>
    </Grid>
    <Grid Name="mesh" GridType="Uniform">
      <Topology NumberOfElements="378" TopologyType="Triangle" NodesPerElement="3">
        <DataItem Dimensions="378 3" NumberType="UInt" Format="XML">0 1 5
...
        </DataItem>
      </Topology>
      <Geometry Reference="XML">/Xdmf/Domain/Grid/Geometry</Geometry>
      <Attribute Name="f" AttributeType="Scalar" Center="Cell">
        <DataItem Dimensions="378 1" Format="XML">3
...
        </DataItem>
      </Attribute>
    </Grid>
  </Domain>
</Xdmf>


"""


def write_fenics_mesh_xdmf(
    fem_mesh_obj,
    outputfile,
    group_values_dict={},
    encoding=ENCODING_ASCII
):
    """
        For the export of xdmf.
    """

    Console.PrintMessage(f"Converting {fem_mesh_obj.Label} to fenics XDMF File\n")
    Console.PrintMessage(f"Dimension of mesh: {get_FemMeshObjectDimension(fem_mesh_obj)}\n")

    elements_in_mesh = get_FemMeshObjectElementTypes(fem_mesh_obj)
    Console.PrintMessage(f"Elements appearing in mesh: {str(elements_in_mesh)}\n")
    celltype_in_mesh = get_MaxDimElementFromList(elements_in_mesh)
    (num_cells, cellname_fc, dim_cell) = celltype_in_mesh

    root = ET.Element("Xdmf", Version="3.0")
    domain = ET.SubElement(root, "Domain")
    base_grid = ET.SubElement(domain, "Grid", Name="base_mesh", GridType="Uniform")
    base_topology = ET.SubElement(base_grid, "Topology")
    base_geometry = ET.SubElement(base_grid, "Geometry")

    # TODO: for the general mesh: write out topology and geometry in grid node
    # TODO: for every marked group write own grid node with topology (ref if cells)
    #       geometry ref, attribute

    # ***********************************
    # write base topo and geometry
    nodes_dict = write_fenics_mesh_points_xdmf(
        fem_mesh_obj,
        base_geometry,
        encoding=encoding
    )
    write_fenics_mesh_codim_xdmf(
        fem_mesh_obj, base_topology,
        nodes_dict,
        codim=0,
        encoding=encoding
    )
    # ***********************************

    fem_mesh = fem_mesh_obj.FemMesh
    gmshgroups = get_FemMeshObjectMeshGroups(fem_mesh_obj)

    if gmshgroups != ():
        Console.PrintMessage("found mesh groups\n")

    for g in gmshgroups:
        mesh_function_type = fem_mesh.getGroupElementType(g)
        mesh_function_codim = dim_cell - FreeCAD_Group_Dimensions[mesh_function_type]
        mesh_function_name = fem_mesh.getGroupName(g)

        Console.PrintMessage(f"group id: {g} (label: {mesh_function_name})" +
                             f" with element type {mesh_function_type} and" +
                             " codim {mesh_function_codim}\n")

        mesh_function_grid = ET.SubElement(
            domain, "Grid",
            Name=mesh_function_name + "_mesh",
            GridType="Uniform"
        )
        mesh_function_topology = ET.SubElement(mesh_function_grid, "Topology")

        mesh_function_topology_description = write_fenics_mesh_codim_xdmf(
            fem_mesh_obj,
            mesh_function_topology,
            nodes_dict,
            codim=mesh_function_codim, encoding=encoding
        )

        mesh_function_geometry = ET.SubElement(mesh_function_grid, "Geometry",
                                               Reference="XML")
        mesh_function_geometry.text = "/Xdmf/Domain/Grid/Geometry"
        mesh_function_attribute = ET.SubElement(mesh_function_grid, "Attribute")

        elem_dict = {}
        (elem_mark_group, elem_mark_default) = group_values_dict.get(g, (1, 0))

        # TODO: is it better to save all groups each at once or collect all codim equal
        # groups to put them into one function?
        # TODO: nevertheless there has to be a dialog
        # which fixes the default value and the mark value

        for e in fem_mesh.getGroupElements(g):
            elem_dict[e] = elem_mark_group

        val_array = np.array([
            elem_dict.get(e, elem_mark_default) for e in mesh_function_topology_description
        ])
        topo_array = np.vstack((val_array,)).T
        write_fenics_mesh_scalar_cellfunctions(
            mesh_function_name,
            topo_array,
            mesh_function_attribute,
            encoding=ENCODING_ASCII
        )

    # TODO: improve cell functions support

    fp = open(outputfile, "wb")
    fp.write(b'''<?xml version="1.0"?>\n<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>\n''')
    fp.write(ET.tostring(root))
    # xml core functionality does not support pretty printing
    # so the output file looks quite ugly
    fp.close()
