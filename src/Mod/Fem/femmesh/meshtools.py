# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Tools for the work with finite element meshes"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD

from femtools import geomtools
import numpy as np

# ************************************************************************************************
def get_femnodes_by_femobj_with_references(
    femmesh,
    femobj
):
    node_set = []
    if femmesh.GroupCount:
        node_set = get_femmesh_groupdata_sets_by_name(femmesh, femobj, "Node")
        # FreeCAD.Console.PrintMessage("node_set_group: {}\n".format(node_set))
        if node_set:
            FreeCAD.Console.PrintLog(
                "    Finite element mesh nodes where retrieved "
                "from existent finite element mesh group data.\n"
            )
    if not node_set:
        FreeCAD.Console.PrintLog(
            "    Finite element mesh nodes will be retrieved "
            "by searching the appropriate nodes in the finite element mesh.\n"
        )
        node_set = get_femnodes_by_references(femmesh, femobj["Object"].References)
        # FreeCAD.Console.PrintMessage("node_set_nogroup: {}\n".format(node_set))

    # use set for node sets to be sure all nodes are unique
    # use sorted to be sure the order is the same on different runs
    # be aware a sorted set returns a list, because set are not sorted by default
    return sorted(set(node_set))


# ************************************************************************************************
def get_femelements_by_references(
    femmesh,
    femelement_table,
    references,
    femnodes_ele_table=None
):
    """get the femelements for a list of references
    """
    references_femelements = []
    for ref in references:
        # femnodes for the current ref
        ref_femnodes = get_femnodes_by_refshape(femmesh, ref)
        if femnodes_ele_table:
            # blind fast binary search, works for volumes only
            # femelements for all references
            references_femelements += get_femelements_by_femnodes_bin(
                femelement_table,
                femnodes_ele_table,
                ref_femnodes
            )
        else:
            # standard search
            # femelements for all references
            references_femelements += get_femelements_by_femnodes_std(
                femelement_table,
                ref_femnodes
            )
    return references_femelements


# ************************************************************************************************
def get_femnodes_by_references(
    femmesh,
    references
):
    """get the femnodes for a list of references
    """
    references_femnodes = []
    for ref in references:
        references_femnodes += get_femnodes_by_refshape(femmesh, ref)

    # return references_femnodes  # keeps duplicate nodes, keeps node order

    # if nodes are used for nodesets, duplicates should be removed
    return list(set(references_femnodes))  # removes duplicate nodes, sorts node order


def get_femnodes_by_refshape(
    femmesh,
    ref
):
    nodes = []
    for refelement in ref[1]:
        # the following method getElement(element) does not return Solid elements
        r = geomtools.get_element(ref[0], refelement)
        FreeCAD.Console.PrintMessage(
            "    "
            "ReferenceShape ... Type: {0}, "
            "Object name: {1}, "
            "Object label: {2}, "
            "Element name: {3}\n"
            .format(r.ShapeType, ref[0].Name, ref[0].Label, refelement)
        )
        if r.ShapeType == "Vertex":
            nodes += femmesh.getNodesByVertex(r)
        elif r.ShapeType == "Edge":
            nodes += femmesh.getNodesByEdge(r)
        elif r.ShapeType == "Face":
            nodes += femmesh.getNodesByFace(r)
        elif r.ShapeType == "Solid":
            nodes += femmesh.getNodesBySolid(r)
        else:
            FreeCAD.Console.PrintMessage(
                "  "
                "No Vertice, Edge, Face or Solid as reference shapes!\n"
            )
    return nodes


# ************************************************************************************************
def get_femelement_table(
    femmesh
):
    """ get_femelement_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    femelement_table = {}
    if is_solid_femmesh(femmesh):
        for i in femmesh.Volumes:
            femelement_table[i] = femmesh.getElementNodes(i)
    elif is_face_femmesh(femmesh):
        for i in femmesh.Faces:
            femelement_table[i] = femmesh.getElementNodes(i)
    elif is_edge_femmesh(femmesh):
        for i in femmesh.Edges:
            femelement_table[i] = femmesh.getElementNodes(i)
    else:
        FreeCAD.Console.PrintError("Neither solid nor face nor edge femmesh!\n")
    return femelement_table


# ************************************************************************************************
def get_femelement_volumes_table(
    femmesh
):
    """ get_femelement_volumes_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    for i in femmesh.Volumes:
        table[i] = femmesh.getElementNodes(i)
    return table


# ************************************************************************************************
def get_femelement_faces_table(
    femmesh,
    faces_only=None
):
    """ get_femelement_faces_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    if not faces_only:
        faces_only = femmesh.FacesOnly
    for i in faces_only:
        table[i] = femmesh.getElementNodes(i)
    return table


# ************************************************************************************************
def get_femelement_edges_table(
    femmesh,
    edges_only=None
):
    """ get_femelement_edges_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    if not edges_only:
        edges_only = femmesh.EdgesOnly
    for i in edges_only:
        table[i] = femmesh.getElementNodes(i)
    return table


# ************************************************************************************************
def get_femnodes_ele_table(
    femnodes_mesh,
    femelement_table
):
    """the femnodes_ele_table contains for each node its membership in elements
    {nodeID : [[eleID, NodePosition], [], ...], nodeID : [[], [], ...], ...}
    stored information is:
    element number, the number of nodes per element
    the position of the node in the element.
    The position of the node in the element is coded
    as a set bit at that position in a bit array (integer)
    FIXME: the number of nodes per element should be
    replaced by the type of the element
    but I did not know how to get this from the mesh.
    Since the femelement_table contains either
    volume or face or edgemesh the femnodes_ele_table only
    has either volume or face or edge elements
    see get_femelement_table()
    """
    femnodes_ele_table = {}  # node_dict in ulrichs class
    for n in femnodes_mesh:  # initialize it with sorted node keys and empty lists
        femnodes_ele_table[n] = []
    for ele in femelement_table:
        ele_list = femelement_table[ele]
        # FreeCAD.Console.PrintMessage("{}\n".format(ele_list))
        pos = int(1)
        for ele_node in ele_list:
            femnodes_ele_table[ele_node].append([ele, pos])
            pos = pos << 1
    FreeCAD.Console.PrintLog(
        "len femnodes_ele_table: {}\n"
        .format(len(femnodes_ele_table))
    )
    FreeCAD.Console.PrintLog("femnodes_ele_table: {}\n".format(femnodes_ele_table))
    return femnodes_ele_table


# ************************************************************************************************
def get_copy_of_empty_femelement_table(
    femelement_table
):
    """{eleID : 0, eleID : 0, ...}
    """
    empty_femelement_table = {}
    for ele in femelement_table:  # initialize it with sorted element keys and empty int
        empty_femelement_table[ele] = 0
    return empty_femelement_table.copy()


# ************************************************************************************************
def get_bit_pattern_dict(
    femelement_table,
    femnodes_ele_table,
    node_set
):
    """Now we are looking for nodes inside of the Faces = filling the bit_pattern_dict
    {eleID : [lenEleNodes, binary_position]}
    see forum post for a very good explanation of what"s really happening
    https://forum.freecadweb.org/viewtopic.php?f=18&t=17318&start=50#p141108
    The bit_pattern_dict holds later an integer (bit array) for each element, which gives us
    the information we are searching for:
    Is this element part of the node list (searching for elements)
    or has this element a face we are searching for?
    The number in the ele_dict is organized as a bit array.
    The corresponding bit is set, if the node of the node_set is contained in the element.
    """
    FreeCAD.Console.PrintLog("len femnodes_ele_table: " + str(len(femnodes_ele_table)) + "\n")
    FreeCAD.Console.PrintLog("len node_set: " + str(len(node_set)) + "\n")
    FreeCAD.Console.PrintLog("node_set: {}\n".format(node_set))
    bit_pattern_dict = get_copy_of_empty_femelement_table(femelement_table)
    # # initializing the bit_pattern_dict
    for ele in femelement_table:
        len_ele = len(femelement_table[ele])
        bit_pattern_dict[ele] = [len_ele, 0]
    for node in node_set:
        for nList in femnodes_ele_table[node]:
            bit_pattern_dict[nList[0]][1] += nList[1]
    FreeCAD.Console.PrintLog("len bit_pattern_dict: " + str(len(bit_pattern_dict)) + "\n")
    # FreeCAD.Console.PrintMessage("bit_pattern_dict: {}\n".format(bit_pattern_dict))
    return bit_pattern_dict


# ************************************************************************************************
def get_ccxelement_faces_from_binary_search(
    bit_pattern_dict
):
    """get the CalculiX element face numbers
    """
    # the forum topic discussion with ulrich1a and others ... Better mesh last instead of mesh first
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=17318#p137171
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=17318&start=60#p141484
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=17318&start=50#p141108
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=17318&start=40#p140371
    tet10_mask = {
        119: 1,
        411: 2,
        717: 3,
        814: 4}
    tet4_mask = {
        7: 1,
        11: 2,
        13: 3,
        14: 4}
    hex8_mask = {
        240: 1,
        15: 2,
        102: 3,
        204: 4,
        153: 5,
        51: 6}
    hex20_mask = {
        61680: 1,
        3855: 2,
        402022: 3,
        804044: 4,
        624793: 5,
        201011: 6}
    pent6_mask = {
        56: 1,
        7: 2,
        54: 3,
        45: 4,
        27: 5}
    pent15_mask = {
        3640: 1,
        455: 2,
        25782: 3,
        22829: 4,
        12891: 5}
    vol_dict = {
        4: tet4_mask,
        6: pent6_mask,
        8: hex8_mask,
        10: tet10_mask,
        15: pent15_mask,
        20: hex20_mask}
    faces = []
    for ele in bit_pattern_dict:
        mask_dict = vol_dict[bit_pattern_dict[ele][0]]
        for key in mask_dict:
            if (key & bit_pattern_dict[ele][1]) == key:
                faces.append([ele, mask_dict[key]])
    FreeCAD.Console.PrintLog("found Faces: {}\n".format(len(faces)))
    # FreeCAD.Console.PrintMessage("faces: {}\n".format(faces))
    return faces


# ************************************************************************************************
def get_femelements_by_femnodes_bin(
    femelement_table,
    femnodes_ele_table,
    node_list
):
    """for every femelement of femelement_table
    if all nodes of the femelement are in node_list,
    the femelement is added to the list which is returned
    blind fast binary search, but works for volumes only
    """
    FreeCAD.Console.PrintMessage("binary search: get_femelements_by_femnodes_bin\n")
    vol_masks = {
        4: 15,
        6: 63,
        8: 255,
        10: 1023,
        15: 32767,
        20: 1048575}
    # Now we are looking for nodes inside of the Volumes = filling the bit_pattern_dict
    FreeCAD.Console.PrintMessage(
        "len femnodes_ele_table: {}\n"
        .format(len(femnodes_ele_table))
    )
    bit_pattern_dict = get_bit_pattern_dict(femelement_table, femnodes_ele_table, node_list)
    # search
    ele_list = []  # The ele_list contains the result of the search.
    for ele in bit_pattern_dict:
        FreeCAD.Console.PrintLog(
            "bit_pattern_dict[ele][0]: {}\n".format(bit_pattern_dict[ele][0])
        )
        if bit_pattern_dict[ele][1] == vol_masks[bit_pattern_dict[ele][0]]:
            ele_list.append(ele)
    FreeCAD.Console.PrintMessage("found Volumes: {}\n".format(len(ele_list)))
    # FreeCAD.Console.PrintMessage("   volumes: {}\n".format(ele_list))
    return ele_list


# ************************************************************************************************
def get_femelements_by_femnodes_std(
    femelement_table,
    node_list
):
    """for every femelement of femelement_table
    if all nodes of the femelement are in node_list,
    the femelement is added to the list which is returned
    e: elementlist
    nodes: nodelist """
    FreeCAD.Console.PrintMessage("std search: get_femelements_by_femnodes_std\n")
    e = []  # elementlist
    for elementID in sorted(femelement_table):
        nodecount = 0
        for nodeID in femelement_table[elementID]:
            if nodeID in node_list:
                nodecount = nodecount + 1
        # all nodes of the element are in the node_list!
        if nodecount == len(femelement_table[elementID]):
            e.append(elementID)
    return e


def get_femvolumeelements_by_femfacenodes(
    femelement_table,
    node_list
):
    """assume femelement_table only has volume elements
    for every femvolumeelement of femelement_table
    for tetra4 and tetra10 the C++ methods could be used --> test again to be sure
    if hexa8 volume element
        --> if exact 4 element nodes are in node_list --> add femelement
    if hexa20 volume element
        --> if exact 8 element nodes are in node_list --> add femelement
    if penta6 volume element
        --> if exact 3 or 6 element nodes are in node_list --> add femelement
    if penta15 volume element
        --> if exact 6 or 8 element nodes are in node_list --> add femelement
    e: elementlist
    nodes: nodelist """
    e = []  # elementlist
    for elementID in sorted(femelement_table):
        nodecount = 0
        el_nd_ct = len(femelement_table[elementID])
        if el_nd_ct == 4:  # tetra4
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 3:
                e.append(elementID)
        elif el_nd_ct == 10:  # tetra10
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 4:
                e.append(elementID)
        elif el_nd_ct == 8:  # hexa8
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 4:
                e.append(elementID)
        elif el_nd_ct == 20:  # hexa20
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 8:
                e.append(elementID)
        elif el_nd_ct == 6:  # penta6
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 3 or nodecount == 4:
                e.append(elementID)
        elif el_nd_ct == 15:  # penta15
            for nodeID in femelement_table[elementID]:
                if nodeID in node_list:
                    nodecount = nodecount + 1
            if nodecount == 6 or nodecount == 8:
                e.append(elementID)
        else:
            FreeCAD.Console.PrintError(
                "Error in get_femvolumeelements_by_femfacenodes(): "
                "unknown volume element: {}\n"
                .format(el_nd_ct)
            )
    # FreeCAD.Console.PrintMessage("{}\n".format(sorted(e)))
    return e


# ************************************************************************************************
def get_femelement_sets(
    femmesh,
    femelement_table,
    fem_objects,
    femnodes_ele_table=None
):
    # fem_objects = FreeCAD FEM document objects
    # get femelements for reference shapes of each obj.References
    count_femelements = 0
    referenced_femelements = np.zeros((max(femelement_table.keys())+1,),dtype=np.int)
    has_remaining_femelements = None
    for fem_object_i, fem_object in enumerate(fem_objects):
        obj = fem_object["Object"]
        FreeCAD.Console.PrintMessage(
            "Constraint: {} --> We're going to search "
            "in the mesh for the element ID's.\n"
            .format(obj.Name)
        )
        # unique short identifier
        fem_object["ShortName"] = get_elset_short_name(obj, fem_object_i)
        if obj.References:
            ref_shape_femelements = []
            ref_shape_femelements = get_femelements_by_references(
                femmesh, femelement_table,
                obj.References,
                femnodes_ele_table
            )
            ref_shape_femelements_array = np.zeros_like(referenced_femelements)
            ref_shape_femelements_array[ref_shape_femelements] = 1
            referenced_femelements += ref_shape_femelements_array
            count_femelements += len(ref_shape_femelements)
            fem_object["FEMElements"] = ref_shape_femelements
        else:
            has_remaining_femelements = obj.Name
    # get remaining femelements for the fem_objects
    if has_remaining_femelements:
        femelement_table_array = np.zeros_like(referenced_femelements)
        femelement_table_array[list(femelement_table.keys())] = 1
        remaining_femelements_array = femelement_table_array > referenced_femelements
        remaining_femelements = [ i.item() for i in np.nditer(remaining_femelements_array.nonzero()) ]
        count_femelements += len(remaining_femelements)
        for fem_object in fem_objects:
            obj = fem_object["Object"]
            if obj.Name == has_remaining_femelements:
                fem_object["FEMElements"] = sorted(remaining_femelements)
    # check if all worked out well
    if femelements_count_ok(len(femelement_table), count_femelements):
        return True
    else:
        FreeCAD.Console.PrintError(
            "Error in get_femelement_sets -- > femelements_count_ok() failed!\n"
        )
        return False


# ************************************************************************************************
def get_femelement_direction1D_set(
    femmesh,
    femelement_table,
    beamrotation_objects,
    theshape=None
):
    """
    get for each geometry edge direction, the local direction m and the element ids and
    # write all into the beamrotation_objects
    means no return value, we're going to write into the beamrotation_objects dictionary
    FEMRotations1D is a list of dictionaries for every beamdirection of all edges
    beamrot_obj["FEMRotations1D"] = [{
        "ids" : [theids],
        "direction" : direction,
        "beam_axis_m" : beam_axis_m
    }, ... ]
    """
    if len(beamrotation_objects) == 0:
        # no beamrotation document object, all beams use standard rotation of 0 degree (angle)
        # we need theshape (the shape which was meshed)
        # since ccx needs to split them in sets anyway we need to take care of this too
        rotations_ids = get_femelement_directions_theshape(femmesh, femelement_table, theshape)
        # add normals for each direction
        rotation_angle = 0
        for rot in rotations_ids:
            rot["beam_axis_m"] = get_beam_main_axis_m(rot["direction"], rotation_angle)
        # key "Object" will be empty
        beamrotation_objects.append({"FEMRotations1D": rotations_ids, "ShortName": "Rstd"})
    elif len(beamrotation_objects) == 1:
        # one beamrotation document object with no references
        # all beams use rotation from this object
        # we need theshape (the shape which was meshed)
        # since ccx needs to split them in sets anyway we need to take care of this too
        rotations_ids = get_femelement_directions_theshape(femmesh, femelement_table, theshape)
        # add normals for each direction
        rotation_angle = beamrotation_objects[0]["Object"].Rotation
        for rot in rotations_ids:
            rot["beam_axis_m"] = get_beam_main_axis_m(rot["direction"], rotation_angle)
        beamrotation_objects[0]["FEMRotations1D"] = rotations_ids
        beamrotation_objects[0]["ShortName"] = "R0"
    elif len(beamrotation_objects) > 1:
        # multiple beam rotation document objects
        # rotations defined by reference shapes, TODO implement this
        # do not forget all the corner cases:
        # one beam rotation object, but not all edges are ref shapes
        # more than one beam rotation object, but not all edges are in the ref shapes
        # for the both cases above, all other edges get standard rotation.
        # more than one beam rotation objects and on has no ref shapes
        # all edges no in an rotation object use this rotation
        # one edge is in more than one beam rotation object, error
        # pre check, only one beam rotation with empty ref shapes is allowed
        # we need theshape for multiple rotations too
        # because of the corner cases mentioned above
        FreeCAD.Console.PrintError("Multiple Rotations not yet supported!\n")
    for rot_object in beamrotation_objects:  # debug output
        FreeCAD.Console.PrintMessage("{}\n".format(rot_object["FEMRotations1D"]))


# ************************************************************************************************
def get_femelement_directions_theshape(femmesh, femelement_table, theshape):
    # see get_femelement_direction1D_set
    rotations_ids = []
    # add directions and all ids for each direction
    for e in theshape.Shape.Edges:
        the_edge = {}
        the_edge["direction"] = e.Vertexes[1].Point - e.Vertexes[0].Point
        edge_femnodes = femmesh.getNodesByEdge(e)  # femnodes for the current edge
        # femelements for this edge
        the_edge["ids"] = get_femelements_by_femnodes_std(femelement_table, edge_femnodes)
        for rot in rotations_ids:
            # tolerance will be managed by FreeCAD
            # see https://forum.freecadweb.org/viewtopic.php?f=22&t=14179
            if rot["direction"] == the_edge["direction"]:
                rot["ids"] += the_edge["ids"]
                break
        else:
            rotations_ids.append(the_edge)
    return rotations_ids


# ************************************************************************************************
def get_beam_main_axis_m(beam_direction, defined_angle):

    # former name was get_beam_normal
    # see forum topic https://forum.freecadweb.org/viewtopic.php?f=18&t=24878
    # beam_direction ... FreeCAD vector
    # defined_angle ... degree
    # base for the rotation:
    # a beam_direction = (1, 0, 0) and angle = 0, returns (-0, 1, 0)
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=24878&start=30#p195567
    # https://forum.freecadweb.org/viewtopic.php?f=13&t=59239&start=140#p521999
    # changing the angle, changes the normal accordingly, 360 would again return (0,1,0)

    # CalxuliX uses negative z axis as base, if nothing is given in input file
    # see the standard direction of 1-direction in CalxuliX manual

    # here the local main axis is called beam_axis_m the minor axis will be beam_axis_n

    # eventually a better name might be get_beam_rotation

    # FIXME: since we fix the base ange we would get this information out of the mesh edge too
    print("beam_axis_m is retrieved from the geometry but we could get if from mesh edge too")
    # print("beam_direction: {}".format(beam_direction))
    # print("defined_angle: {}".format(defined_angle))

    import math
    vector_a = beam_direction
    angle_rad = (math.pi / 180) * defined_angle
    nx = abs(math.cos(angle_rad))
    ny = abs(math.sin(angle_rad))
    if nx < 0.0000001:
        nx = 0
    if ny < 0.0000001:
        ny = 0
    # vector_n = [nx, ny]  # not used ATM

    if vector_a[0] != 0:
        temp_valx = -(vector_a[1] + vector_a[2]) / vector_a[0]
    else:
        temp_valx = 0
    if vector_a[1] != 0:
        temp_valy = -(vector_a[0] + vector_a[2]) / vector_a[1]
    else:
        temp_valy = 0
    if vector_a[2] != 0:
        temp_valz = -(vector_a[0] + vector_a[1]) / vector_a[2]
    else:
        temp_valz = 0

    # Dot_product_check
    dot_x = None
    dot_y = None
    dot_z = None
    dot_nt = None
    if vector_a[0] != 0 and vector_a[1] == 0 and vector_a[2] == 0:
        normal_n = [temp_valx, nx, ny]
        dot_x = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] != 0 and vector_a[2] == 0:
        normal_n = [nx, temp_valy, ny]
        dot_y = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] == 0 and vector_a[2] != 0:
        normal_n = [nx, ny, temp_valz]
        dot_z = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] != 0 and vector_a[2] != 0:
        normal_n = [nx, temp_valy, ny]
        dot_y = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] != 0 and vector_a[1] == 0 and vector_a[2] != 0:
        normal_n = [nx, ny, temp_valz]
        dot_z = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    else:
        normal_n = [temp_valx, nx, ny]
        dot_nt = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]

    dot = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    FreeCAD.Console.PrintLog("{}\n".format(dot))
    FreeCAD.Console.PrintLog("{}\n".format(normal_n))

    # dummy usage of the axis dot to get flake8 quiet
    del dot_x, dot_y, dot_z, dot, dot_nt

    # print("normal_n: {}".format(normal_n))
    return normal_n


# ************************************************************************************************
def get_femmesh_groupdata_sets_by_name(
    femmesh,
    fem_object,
    group_data_type
):
    # get ids from femmesh groupdata for reference shapes of each obj.References
    # we assume the mesh group data fits with the reference shapes
    # no check is done in this regard !!!
    # we just check for the group name and the group data type
    # what happens if a reference shape was changed
    # but the mesh and the mesh groups were not created new !?!
    obj = fem_object["Object"]
    if femmesh.GroupCount:
        for g in femmesh.Groups:
            grp_name = femmesh.getGroupName(g)
            if grp_name.startswith(obj.Name + "_"):
                if femmesh.getGroupElementType(g) == group_data_type:
                    FreeCAD.Console.PrintMessage(
                        "  found mesh group for the IDs: {}, Type: {}\n"
                        .format(grp_name, group_data_type)
                    )
                    return femmesh.getGroupElements(g)  # == ref_shape_femelements
    return ()  # an empty tuple is returned if no group data IDs where found


# ************************************************************************************************
def get_femelement_sets_from_group_data(
    femmesh,
    fem_objects
):
    # get femelements from femmesh groupdata for reference shapes of each obj.References
    count_femelements = 0
    sum_group_elements = []
    for fem_object_i, fem_object in enumerate(fem_objects):
        obj = fem_object["Object"]
        FreeCAD.Console.PrintMessage(
            "Constraint: {} --> We have mesh groups. "
            "We will search for appropriate group data.\n"
            .format(obj.Name)
        )
        # unique short identifier
        fem_object["ShortName"] = get_elset_short_name(obj, fem_object_i)
        # see comments over there !
        group_elements = get_femmesh_groupdata_sets_by_name(femmesh, fem_object, "Volume")
        sum_group_elements += group_elements
        count_femelements += len(group_elements)
        fem_object["FEMElements"] = group_elements
    # check if all worked out well
    if not femelements_count_ok(femmesh.VolumeCount, count_femelements):
        FreeCAD.Console.PrintError(
            "Error in get_femelement_sets_from_group_data -- > "
            "femelements_count_ok() failed!\n"
        )
        return False
    else:
        return True


# ************************************************************************************************
def get_elset_short_name(
    obj,
    i
):
    # ATM for CalculiX needed for all objects which will write element sets into solver input file
    from femtools.femutils import is_of_type
    if is_of_type(obj, "Fem::MaterialCommon"):
        return "M" + str(i)
    elif is_of_type(obj, "Fem::ElementGeometry1D"):
        return "B" + str(i)
    elif is_of_type(obj, "Fem::ElementRotation1D"):
        return "R" + str(i)
    elif is_of_type(obj, "Fem::ElementFluid1D"):
        return "F" + str(i)
    elif is_of_type(obj, "Fem::ElementGeometry2D"):
        return "S" + str(i)
    elif is_of_type(obj, "Fem::ConstraintCentrif"):
        return "C" + str(i)
    else:
        FreeCAD.Console.PrintError(
            "Error in creating short elset name "
            "for obj: {} --> Proxy.Type: {}\n"
            .format(obj.Name, obj.Proxy.Type)
        )


# ************************************************************************************************
# ***** methods for retrieving nodes and node load values for constraint force *******************
# ***** Vertex loads *****************************************************************************
def get_force_obj_vertex_nodeload_table(
    femmesh,
    frc_obj
):
    # force_obj_node_load_table:
    #     [
    #         ("refshape_name.elemname", node_load_table),
    #         ...,
    #         ("refshape_name.elemname", node_load_table)
    #     ]
    force_obj_node_load_table = []
    node_load = frc_obj.Force / len(frc_obj.References)
    for o, elem_tup in frc_obj.References:
        node_count = len(elem_tup)
        for elem in elem_tup:
            ref_node = o.Shape.getElement(elem)
            FreeCAD.Console.PrintMessage(
                "    "
                "ReferenceShape ... Type: {0}, "
                "Object name: {1}, "
                "Object label: {2}, "
                "Element name: {3}\n"
                .format(ref_node.ShapeType, o.Name, o.Label, elem)
            )
            node = femmesh.getNodesByVertex(ref_node)
            elem_info_string = "node load on shape: " + o.Name + ":" + elem
            if len(node) == 1:
                force_obj_node_load_table.append(
                    (elem_info_string, {node[0]: node_load / node_count})
                )
            else:
                FreeCAD.Console.PrintError(
                    "    Problem on retrieving mesh node for: {}\n"
                    .format(elem_info_string)
                )
    return force_obj_node_load_table


# ***** Edge loads *******************************************************************************
# get_force_obj_edge_nodeload_table
# get_ref_edgenodes_table
# get_ref_edgenodes_lengths
def get_force_obj_edge_nodeload_table(
    femmesh,
    femelement_table,
    femnodes_mesh,
    frc_obj
):
    # force_obj_node_load_table:
    #     [
    #         ("refshape_name.elemname", node_load_table),
    #         ...,
    #         ("refshape_name.elemname", node_load_table)
    #     ]
    force_obj_node_load_table = []
    sum_ref_edge_length = 0
    sum_ref_edge_node_length = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_edge = o.Shape.getElement(elem)
            FreeCAD.Console.PrintMessage(
                "    "
                "ReferenceShape ... Type: {0}, "
                "Object name: {1}, "
                "Object label: {2}, "
                "Element name: {3}\n"
                .format(ref_edge.ShapeType, o.Name, o.Label, elem)
            )
            sum_ref_edge_length += ref_edge.Length
    if sum_ref_edge_length != 0:
        force_per_sum_ref_edge_length = frc_obj.Force / sum_ref_edge_length
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_edge = o.Shape.getElement(elem)

            # edge_table:
            #     { meshedgeID : ( nodeID, ... , nodeID ) }
            edge_table = get_ref_edgenodes_table(femmesh, femelement_table, ref_edge)

            # node_length_table:
            #     [ (nodeID, length), ... , (nodeID, length) ]
            # some nodes will have more than one entry
            node_length_table = get_ref_edgenodes_lengths(femnodes_mesh, edge_table)

            # node_sum_length_table:
            #     { nodeID : Length, ... , nodeID : Length }
            # LengthSum for each node, one entry for each node
            node_sum_length_table = get_ref_shape_node_sum_geom_table(node_length_table)

            # node_load_table:
            #     { nodeID : NodeLoad, ... , nodeID : NodeLoad }
            # NodeLoad for each node, one entry for each node
            node_load_table = {}
            sum_node_lengths = 0  # for debugging
            for node in node_sum_length_table:
                sum_node_lengths += node_sum_length_table[node]  # for debugging
                node_load_table[node] = node_sum_length_table[node] * force_per_sum_ref_edge_length
            ratio_refedge_lengths = sum_node_lengths / ref_edge.Length
            if ratio_refedge_lengths < 0.99 or ratio_refedge_lengths > 1.01:
                FreeCAD.Console.PrintError(
                    "Error on: " + frc_obj.Name + " --> " + o.Name + "." + elem + "\n"
                )
                FreeCAD.Console.PrintMessage("  sum_node_lengths: {}\n".format(sum_node_lengths))
                FreeCAD.Console.PrintMessage("  refedge_length: {}\n".format(ref_edge.Length))
                bad_refedge = ref_edge
            sum_ref_edge_node_length += sum_node_lengths

            elem_info_string = "node loads on shape: " + o.Name + ":" + elem
            force_obj_node_load_table.append((elem_info_string, node_load_table))

    for ref_shape in force_obj_node_load_table:
        for node in ref_shape[1]:
            sum_node_load += ref_shape[1][node]  # for debugging

    ratio = sum_node_load / frc_obj.Force
    if ratio < 0.99 or ratio > 1.01:
        FreeCAD.Console.PrintMessage(
            "Deviation  sum_node_load to frc_obj.Force is more than 1% : {}\n"
            .format(ratio)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_ref_edge_node_length: {}\n"
            .format(sum_ref_edge_node_length)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_ref_edge_length:      {}\n"
            .format(sum_ref_edge_length)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_node_load:            {}\n"
            .format(sum_node_load)
        )
        FreeCAD.Console.PrintMessage(
            "  frc_obj.Force:            {}\n"
            .format(frc_obj.Force)
        )
        FreeCAD.Console.PrintMessage(
            "  the reason could be simply a circle length --> "
            "see method get_ref_edge_node_lengths\n"
        )
        FreeCAD.Console.PrintMessage(
            "  the reason could also be a problem in "
            "retrieving the ref_edge_node_length\n"
        )

        # try debugging of the last bad refedge
        FreeCAD.Console.PrintMessage("DEBUGGING\n")
        FreeCAD.Console.PrintMessage("\n".format(bad_refedge))

        FreeCAD.Console.PrintMessage("bad_refedge_nodes\n")
        bad_refedge_nodes = femmesh.getNodesByEdge(bad_refedge)
        FreeCAD.Console.PrintMessage("{}\n".format(len(bad_refedge_nodes)))
        FreeCAD.Console.PrintMessage("{}\n".format(bad_refedge_nodes))
        # import FreeCADGui
        # frc_obj.Document.Compound_Mesh.HighlightedNodes = bad_refedge_nodes

        FreeCAD.Console.PrintMessage("bad_edge_table\n")
        # bad_edge_table:
        #     { meshedgeID : ( nodeID, ... , nodeID ) }
        bad_edge_table = get_ref_edgenodes_table(femmesh, femelement_table, bad_refedge)
        FreeCAD.Console.PrintMessage("{}\n".format(len(bad_edge_table)))
        bad_edge_table_nodes = []
        for elem in bad_edge_table:
            FreeCAD.Console.PrintMessage(elem, " --> \n".format(bad_edge_table[elem]))
            for node in bad_edge_table[elem]:
                if node not in bad_edge_table_nodes:
                    bad_edge_table_nodes.append(node)
        FreeCAD.Console.PrintMessage("sorted(bad_edge_table_nodes)\n")
        # should be == bad_refedge_nodes
        FreeCAD.Console.PrintMessage("{}\n".format(sorted(bad_edge_table_nodes)))
        # import FreeCADGui
        # frc_obj.Document.Compound_Mesh.HighlightedNodes = bad_edge_table_nodes
        # bad_node_length_table:
        #     [ (nodeID, length), ... , (nodeID, length) ]
        # some nodes will have more than one entry

        FreeCAD.Console.PrintMessage("good_edge_table\n")
        good_edge_table = delete_duplicate_mesh_elements(bad_edge_table)
        for elem in good_edge_table:
            FreeCAD.Console.PrintMessage("{} --> {}\n".format(elem, bad_edge_table[elem]))

        FreeCAD.Console.PrintMessage("bad_node_length_table\n")
        bad_node_length_table = get_ref_edgenodes_lengths(femnodes_mesh, bad_edge_table)
        for n, l in bad_node_length_table:
            FreeCAD.Console.PrintMessage("{} --> {}\n".format(n, l))

    return force_obj_node_load_table


# ************************************************************************************************
def get_ref_edgenodes_table(
    femmesh,
    femelement_table,
    refedge
):
    edge_table = {}  # { meshedgeID : ( nodeID, ... , nodeID ) }
    refedge_nodes = femmesh.getNodesByEdge(refedge)
    if is_solid_femmesh(femmesh):
        refedge_fem_volumeelements = []
        # if at least two nodes of a femvolumeelement are in
        # refedge_nodes the volume is added to refedge_fem_volumeelements
        for elem in femelement_table:
            nodecount = 0
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    nodecount += 1
            if nodecount > 1:
                refedge_fem_volumeelements.append(elem)
        # for every refedge_fem_volumeelement look which of its nodes
        # is in refedge_nodes --> add all these nodes to edge_table
        for elem in refedge_fem_volumeelements:
            fe_refedge_nodes = []
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    fe_refedge_nodes.append(node)
                # { volumeID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
                edge_table[elem] = fe_refedge_nodes
        # FIXME: duplicate_mesh_elements: as soon as contact and springs are supported
        # the user should decide on which edge the load is applied
        edge_table = delete_duplicate_mesh_elements(edge_table)
    elif is_face_femmesh(femmesh):
        refedge_fem_faceelements = []
        # if at least two nodes of a femfaceelement are in
        # refedge_nodes the volume is added to refedge_fem_volumeelements
        for elem in femelement_table:
            nodecount = 0
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    nodecount += 1
            if nodecount > 1:
                refedge_fem_faceelements.append(elem)
        # for every refedge_fem_faceelement look which of its nodes is in
        # refedge_nodes --> add all these nodes to edge_table
        for elem in refedge_fem_faceelements:
            fe_refedge_nodes = []
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    fe_refedge_nodes.append(node)
                # { faceID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
                edge_table[elem] = fe_refedge_nodes
        # FIXME: duplicate_mesh_elements: as soon as contact and springs are supported
        # the user should decide on which edge the load is applied
        edge_table = delete_duplicate_mesh_elements(edge_table)
    elif is_edge_femmesh(femmesh):
        refedge_fem_edgeelements = get_femelements_by_femnodes_std(
            femelement_table,
            refedge_nodes
        )
        for elem in refedge_fem_edgeelements:
            # { edgeID : ( nodeID, ... , nodeID  )} # all nodes off this femedgeelement
            edge_table[elem] = femelement_table[elem]
    return edge_table


# ************************************************************************************************
def get_ref_edgenodes_lengths(
    femnodes_mesh,
    edge_table
):
    # calculate the appropriate node_length for every node of every mesh edge (me)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff

    #  [ (nodeID, length), ... , (nodeID, length) ]
    # some nodes will have more than one entry
    if (not femnodes_mesh) or (not edge_table):
        FreeCAD.Console.PrintError(
            "Error in get_ref_edgenodes_lengths(): "
            "Empty femnodes_mesh or edge_table!\n"
        )
        return []
    node_length_table = []
    mesh_edge_length = 0
    # FreeCAD.Console.PrintMessage("{}\n".format(len(edge_table)))
    for me in edge_table:
        femmesh_edgetype = len(edge_table[me])
        if femmesh_edgetype == 2:  # 2 node femmesh edge
            # end_node_length = mesh_edge_length / 2
            #    ______
            #  P1      P2
            P1 = femnodes_mesh[edge_table[me][0]]
            P2 = femnodes_mesh[edge_table[me][1]]
            edge_vec = P2 - P1
            mesh_edge_length = edge_vec.Length
            # FreeCAD.Console.PrintMessage("{}\n".format(mesh_edge_length))
            end_node_length = mesh_edge_length / 2.0
            node_length_table.append((edge_table[me][0], end_node_length))
            node_length_table.append((edge_table[me][1], end_node_length))

        elif femmesh_edgetype == 3:  # 3 node femmesh edge
            # end_node_length = mesh_edge_length / 6
            # middle_node_length = mesh_edge_length * 2 / 3
            #   _______ _______
            # P1       P3      P2
            P1 = femnodes_mesh[edge_table[me][0]]
            P2 = femnodes_mesh[edge_table[me][1]]
            P3 = femnodes_mesh[edge_table[me][2]]
            edge_vec1 = P3 - P1
            edge_vec2 = P2 - P3
            mesh_edge_length = edge_vec1.Length + edge_vec2.Length
            # FreeCAD.Console.PrintMessage("{} --> {}\n".format(me, mesh_edge_length))
            end_node_length = mesh_edge_length / 6.0
            middle_node_length = mesh_edge_length * 2.0 / 3.0
            node_length_table.append((edge_table[me][0], end_node_length))
            node_length_table.append((edge_table[me][1], end_node_length))
            node_length_table.append((edge_table[me][2], middle_node_length))
    return node_length_table


# ***** Face loads *******************************************************************************
# get_force_obj_face_nodeload_table
# get_ref_facenodes_table
# get_ref_facenodes_areas
# build_mesh_faces_of_volume_elements
def get_force_obj_face_nodeload_table(
    femmesh,
    femelement_table,
    femnodes_mesh,
    frc_obj
):
    # force_obj_node_load_table:
    #     [
    #         ("refshape_name.elemname",node_load_table),
    #         ...,
    #         ("refshape_name.elemname",node_load_table)
    #     ]
    force_obj_node_load_table = []
    sum_ref_face_area = 0
    sum_ref_face_node_area = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_face = o.Shape.getElement(elem)
            FreeCAD.Console.PrintMessage(
                "    "
                "ReferenceShape ... Type: {0}, "
                "Object name: {1}, "
                "Object label: {2}, "
                "Element name: {3}\n"
                .format(ref_face.ShapeType, o.Name, o.Label, elem)
            )
            sum_ref_face_area += ref_face.Area
    if sum_ref_face_area != 0:
        force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_face = o.Shape.getElement(elem)

            # face_table:
            #    { meshfaceID : ( nodeID, ... , nodeID ) }
            face_table = get_ref_facenodes_table(femmesh, femelement_table, ref_face)

            # node_area_table:
            #    [ (nodeID, Area), ... , (nodeID, Area) ]
            # some nodes will have more than one entry
            node_area_table = get_ref_facenodes_areas(femnodes_mesh, face_table)

            # node_sum_area_table:
            #    { nodeID : Area, ... , nodeID : Area }
            # AreaSum for each node, one entry for each node
            node_sum_area_table = get_ref_shape_node_sum_geom_table(node_area_table)

            # node_load_table:
            #    { nodeID : NodeLoad, ... , nodeID : NodeLoad }
            # NodeLoad for each node, one entry for each node
            node_load_table = {}
            sum_node_areas = 0  # for debugging
            for node in node_sum_area_table:
                sum_node_areas += node_sum_area_table[node]  # for debugging
                node_load_table[node] = node_sum_area_table[node] * force_per_sum_ref_face_area
            ratio_refface_areas = sum_node_areas / ref_face.Area
            if ratio_refface_areas < 0.99 or ratio_refface_areas > 1.01:
                FreeCAD.Console.PrintError(
                    "Error on: " + frc_obj.Name + " --> " + o.Name + "." + elem + "\n"
                )
                FreeCAD.Console.PrintMessage("  sum_node_areas: {}\n".format(sum_node_areas))
                FreeCAD.Console.PrintMessage("  ref_face_area:  {}\n".format(ref_face.Area))
            sum_ref_face_node_area += sum_node_areas

            elem_info_string = "node loads on shape: " + o.Name + ":" + elem
            force_obj_node_load_table.append((elem_info_string, node_load_table))

    for ref_shape in force_obj_node_load_table:
        for node in ref_shape[1]:
            sum_node_load += ref_shape[1][node]  # for debugging

    ratio = sum_node_load / frc_obj.Force
    if ratio < 0.99 or ratio > 1.01:
        FreeCAD.Console.PrintMessage(
            "Deviation sum_node_load to frc_obj.Force is more than 1% :  {}\n"
            .format(ratio)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_ref_face_node_area: {}\n"
            .format(sum_ref_face_node_area)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_ref_face_area:      {}\n"
            .format(sum_ref_face_area)
        )
        FreeCAD.Console.PrintMessage(
            "  sum_node_load:          {}\n"
            .format(sum_node_load)
        )
        FreeCAD.Console.PrintMessage(
            "  frc_obj.Force:          {}\n"
            .format(frc_obj.Force)
        )
        FreeCAD.Console.PrintMessage(
            "  the reason could be simply a circle area --> "
            "see method get_ref_face_node_areas\n"
        )
        FreeCAD.Console.PrintMessage(
            "  the reason could also be a problem in "
            "retrieving the ref_face_node_area\n"
        )

    return force_obj_node_load_table


# ************************************************************************************************
def get_ref_facenodes_table(
    femmesh,
    femelement_table,
    ref_face
):
    face_table = {}  # { meshfaceID : ( nodeID, ... , nodeID ) }
    if is_solid_femmesh(femmesh):
        if has_no_face_data(femmesh):
            FreeCAD.Console.PrintMessage(
                "  No face data in finite volume element mesh. "
                "FreeCAD uses getccxVolumesByFace() "
                "to retrieve the volume elements of the ref_face.\n"
            )
            # there is no face data
            # if we retrieve the nodes ourself we will have a problem:
            # they are not sorted, we just have the nodes.
            # We need to sort them according to the
            # shell mesh notation of tria3, tria6, quad4, quad8
            ref_face_nodes = femmesh.getNodesByFace(ref_face)
            # try to use getccxVolumesByFace() to get the volume ids
            # of element with elementfaces on the ref_face
            # --> should work for tetra4 and tetra10
            # list of tupels (mv, ccx_face_nr)
            ref_face_volume_elements = femmesh.getccxVolumesByFace(ref_face)
            if ref_face_volume_elements:  # mesh with tetras
                FreeCAD.Console.PrintLog(
                    "  Use of getccxVolumesByFace() has "
                    "returned volume elements of the ref_face.\n"
                )
                for ve in ref_face_volume_elements:
                    veID = ve[0]
                    ve_ref_face_nodes = []
                    for nodeID in femelement_table[veID]:
                        if nodeID in ref_face_nodes:
                            ve_ref_face_nodes.append(nodeID)
                    # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
                    face_table[veID] = ve_ref_face_nodes
            else:  # mesh with hexa or penta
                FreeCAD.Console.PrintLog(
                    "  The use of getccxVolumesByFace() has NOT returned "
                    "volume elements of the ref_face. "
                    "FreeCAD tries to use get_femvolumeelements_by_femfacenodes().\n"
                )
                # list of integer [mv]
                ref_face_volume_elements = get_femvolumeelements_by_femfacenodes(
                    femelement_table,
                    ref_face_nodes
                )
                for veID in ref_face_volume_elements:
                    ve_ref_face_nodes = []
                    for nodeID in femelement_table[veID]:
                        if nodeID in ref_face_nodes:
                            ve_ref_face_nodes.append(nodeID)
                    # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
                    face_table[veID] = ve_ref_face_nodes
                # we need to resort the nodes to make them build an element face
                face_table = build_mesh_faces_of_volume_elements(face_table, femelement_table)
        else:  # the femmesh has face_data
            faces = femmesh.getFacesByFace(ref_face)   # (mv, mf)
            for mf in faces:
                face_table[mf] = femmesh.getElementNodes(mf)
    elif is_face_femmesh(femmesh):
        ref_face_nodes = femmesh.getNodesByFace(ref_face)
        ref_face_elements = get_femelements_by_femnodes_std(femelement_table, ref_face_nodes)
        for mf in ref_face_elements:
            face_table[mf] = femelement_table[mf]
    # FreeCAD.Console.PrintMessage("{}\n".format(face_table))
    return face_table


# ************************************************************************************************
def get_ref_facenodes_areas(
    femnodes_mesh,
    face_table
):
    # calculate the appropriate node_areas for every node of every mesh face (mf)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff
    # FIXME: only gives exact results in case of a real triangle. If for S6 or C3D10 elements
    # the midnodes are not on the line between the end nodes the area will not be a triangle
    # see http://forum.freecadweb.org/viewtopic.php?f=18&t=10939&start=40#p91355  and ff
    # same applies for the quads
    # results are exact only if mid nodes are on the line between corner nodes

    #  [ (nodeID,Area), ... , (nodeID,Area) ]  some nodes will have more than one entry
    if (not femnodes_mesh) or (not face_table):
        FreeCAD.Console.PrintError("Error: Empty femnodes_mesh or face_table!\n")
        return []
    node_area_table = []
    mesh_face_area = 0
    for mf in face_table:
        femmesh_facetype = len(face_table[mf])
        # nodes in face_table need to be in the right node order for the following calculations
        if femmesh_facetype == 3:  # 3 node femmesh face triangle
            # corner_node_area = mesh_face_area / 3.0
            #      P3
            #      /\
            #     /  \
            #    /____\
            #  P1      P2
            P1 = femnodes_mesh[face_table[mf][0]]
            P2 = femnodes_mesh[face_table[mf][1]]
            P3 = femnodes_mesh[face_table[mf][2]]

            mesh_face_area = get_triangle_area(P1, P2, P3)
            corner_node_area = mesh_face_area / 3.0

            node_area_table.append((face_table[mf][0], corner_node_area))
            node_area_table.append((face_table[mf][1], corner_node_area))
            node_area_table.append((face_table[mf][2], corner_node_area))

        elif femmesh_facetype == 4:  # 4 node femmesh face quad
            # corner_node_area = mesh_face_area / 4.0
            #  P4_______P3
            #    |     /|
            #    | t2 / |
            #    |   /  |
            #    |  /   |
            #    | / t1 |
            #    |/_____|
            #  P1       P2
            P1 = femnodes_mesh[face_table[mf][0]]
            P2 = femnodes_mesh[face_table[mf][1]]
            P3 = femnodes_mesh[face_table[mf][2]]
            P4 = femnodes_mesh[face_table[mf][3]]

            mesh_face_t1_area = get_triangle_area(P1, P2, P3)
            mesh_face_t2_area = get_triangle_area(P1, P3, P4)
            mesh_face_area = mesh_face_t1_area + mesh_face_t2_area
            corner_node_area = mesh_face_area / 4.0

            node_area_table.append((face_table[mf][0], corner_node_area))
            node_area_table.append((face_table[mf][1], corner_node_area))
            node_area_table.append((face_table[mf][2], corner_node_area))
            node_area_table.append((face_table[mf][3], corner_node_area))

        elif femmesh_facetype == 6:  # 6 node femmesh face triangle
            # corner_node_area = 0
            # middle_node_area = mesh_face_area / 3.0
            #         P3
            #         /\
            #        /t3\
            #       /    \
            #     P6------P5
            #     / \ t4 / \
            #    /t1 \  /t2 \
            #   /_____\/_____\
            # P1      P4      P2
            P1 = femnodes_mesh[face_table[mf][0]]
            P2 = femnodes_mesh[face_table[mf][1]]
            P3 = femnodes_mesh[face_table[mf][2]]
            P4 = femnodes_mesh[face_table[mf][3]]
            P5 = femnodes_mesh[face_table[mf][4]]
            P6 = femnodes_mesh[face_table[mf][5]]

            mesh_face_t1_area = get_triangle_area(P1, P4, P6)
            mesh_face_t2_area = get_triangle_area(P2, P5, P4)
            mesh_face_t3_area = get_triangle_area(P3, P6, P5)
            mesh_face_t4_area = get_triangle_area(P4, P5, P6)
            mesh_face_area = (
                mesh_face_t1_area
                + mesh_face_t2_area
                + mesh_face_t3_area
                + mesh_face_t4_area
            )
            middle_node_area = mesh_face_area / 3.0

            node_area_table.append((face_table[mf][0], 0))
            node_area_table.append((face_table[mf][1], 0))
            node_area_table.append((face_table[mf][2], 0))
            node_area_table.append((face_table[mf][3], middle_node_area))
            node_area_table.append((face_table[mf][4], middle_node_area))
            node_area_table.append((face_table[mf][5], middle_node_area))

        elif femmesh_facetype == 8:  # 8 node femmesh face quad
            # corner_node_area = -mesh_face_area / 12.0  (negative!)
            # mid-side nodes = mesh_face_area / 3.0
            #  P4_________P7________P3
            #    |      / |  \      |
            #    | t4 /   |    \ t3 |
            #    |  /     |      \  |
            #    |/       |        \|
            #  P8|    t5  |   t6    |P6
            #    |\       |       / |
            #    |  \     |     /   |
            #    | t1 \   |   /  t2 |
            #    |______\_|_/_______|
            #  P1         P5        P2
            P1 = femnodes_mesh[face_table[mf][0]]
            P2 = femnodes_mesh[face_table[mf][1]]
            P3 = femnodes_mesh[face_table[mf][2]]
            P4 = femnodes_mesh[face_table[mf][3]]
            P5 = femnodes_mesh[face_table[mf][4]]
            P6 = femnodes_mesh[face_table[mf][5]]
            P7 = femnodes_mesh[face_table[mf][6]]
            P8 = femnodes_mesh[face_table[mf][7]]

            mesh_face_t1_area = get_triangle_area(P1, P5, P8)
            mesh_face_t2_area = get_triangle_area(P5, P2, P6)
            mesh_face_t3_area = get_triangle_area(P6, P3, P7)
            mesh_face_t4_area = get_triangle_area(P7, P4, P8)
            mesh_face_t5_area = get_triangle_area(P5, P7, P8)
            mesh_face_t6_area = get_triangle_area(P5, P6, P7)
            mesh_face_area = (
                mesh_face_t1_area
                + mesh_face_t2_area
                + mesh_face_t3_area
                + mesh_face_t4_area
                + mesh_face_t5_area
                + mesh_face_t6_area
            )
            corner_node_area = -mesh_face_area / 12.0
            middle_node_area = mesh_face_area / 3.0

            node_area_table.append((face_table[mf][0], corner_node_area))
            node_area_table.append((face_table[mf][1], corner_node_area))
            node_area_table.append((face_table[mf][2], corner_node_area))
            node_area_table.append((face_table[mf][3], corner_node_area))
            node_area_table.append((face_table[mf][4], middle_node_area))
            node_area_table.append((face_table[mf][5], middle_node_area))
            node_area_table.append((face_table[mf][6], middle_node_area))
            node_area_table.append((face_table[mf][7], middle_node_area))
    return node_area_table


# ************************************************************************************************
def build_mesh_faces_of_volume_elements(
    face_table,
    femelement_table
):
    # node index of facenodes in femelementtable volume element
    # if we know the position of the node
    # we can build the element face out of the unsorted face nodes
    face_nodenumber_table = {}  # { volumeID : ( index, ... , index ) }
    for veID in face_table:
        face_nodenumber_table[veID] = []
        for n in face_table[veID]:
            index = femelement_table[veID].index(n)
            # FreeCAD.Console.PrintMessage("{}\n".format(index))
            # local node number = index + 1
            face_nodenumber_table[veID].append(index + 1)
        FreeCAD.Console.PrintLog("VolElement: {}\n".format(veID))
        FreeCAD.Console.PrintLog("  --> {}\n".format(femelement_table[veID]))
        FreeCAD.Console.PrintLog("  --> {}\n".format(face_table[veID]))
        FreeCAD.Console.PrintLog("  --> {}\n".format(face_nodenumber_table[veID]))

    for veID in face_nodenumber_table:
        FreeCAD.Console.PrintLog("VolElement: {}\n".format(veID))
        vol_node_ct = len(femelement_table[veID])
        face_node_indexs = sorted(face_nodenumber_table[veID])
        node_numbers = ()
        if vol_node_ct == 10:
            FreeCAD.Console.PrintLog("  --> tetra10 --> tria6 face\n")
            # node order of face in tetra10 volume element
            if face_node_indexs == [1, 2, 3, 5, 6, 7]:
                # node order of a tria6 face of tetra10
                node_numbers = (1, 2, 3, 5, 6, 7)
            elif face_node_indexs == [1, 2, 4, 5, 8, 9]:
                node_numbers = (1, 4, 2, 8, 9, 5)
            elif face_node_indexs == [1, 3, 4, 7, 8, 10]:
                node_numbers = (1, 3, 4, 7, 10, 8)
            elif face_node_indexs == [2, 3, 4, 6, 9, 10]:
                node_numbers = (2, 4, 3, 9, 10, 6)
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "tetra10: face not found! {}\n"
                    .format(face_node_indexs)
                )
        elif vol_node_ct == 4:
            FreeCAD.Console.PrintLog("  --> tetra4 --> tria3 face\n")
            # node order of face in tetra4 volume element
            if face_node_indexs == [1, 2, 3]:
                # node order of a tria3 face of tetra4
                node_numbers = (1, 2, 3)
            elif face_node_indexs == [1, 2, 4]:
                node_numbers = (1, 4, 2, 8)
            elif face_node_indexs == [1, 3, 4]:
                node_numbers = (1, 3, 4)
            elif face_node_indexs == [2, 3, 4]:
                node_numbers = (2, 4, 3)
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "tetra4: face not found! {}\n"
                    .format(face_node_indexs)
                )
        elif vol_node_ct == 20:
            FreeCAD.Console.PrintLog("  --> hexa20 --> quad8 face\n")
            # node order of face in hexa20 volume element
            if face_node_indexs == [1, 2, 3, 4, 9, 10, 11, 12]:
                # node order of a quad8 face of hexa20
                node_numbers = (1, 2, 3, 4, 9, 10, 11, 12)
            elif face_node_indexs == [5, 6, 7, 8, 13, 14, 15, 16]:
                node_numbers = (5, 8, 7, 6, 16, 15, 14, 13)
            elif face_node_indexs == [1, 2, 5, 6, 9, 13, 17, 18]:
                node_numbers = (1, 5, 6, 2, 17, 13, 18, 9)
            elif face_node_indexs == [3, 4, 7, 8, 11, 15, 19, 20]:
                node_numbers = (3, 7, 8, 4, 19, 15, 20, 11)
            elif face_node_indexs == [1, 4, 5, 8, 12, 16, 17, 20]:
                node_numbers = (1, 4, 8, 5, 12, 20, 16, 17)
            elif face_node_indexs == [2, 3, 6, 7, 10, 14, 18, 19]:
                node_numbers = (2, 6, 7, 3, 18, 14, 19, 10)
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "hexa20: face not found! {}\n"
                    .format(face_node_indexs)
                )
        elif vol_node_ct == 8:
            FreeCAD.Console.PrintLog("  --> hexa8 --> quad4 face\n")
            # node order of face in hexa8 volume element
            if face_node_indexs == [1, 2, 3, 4]:
                # node order of a quad8 face of hexa8
                node_numbers = (1, 2, 3, 4)
            elif face_node_indexs == [5, 6, 7, 8]:
                node_numbers = (5, 8, 7, 6)
            elif face_node_indexs == [1, 2, 5, 6]:
                node_numbers = (1, 5, 6, 2)
            elif face_node_indexs == [3, 4, 7, 8]:
                node_numbers = (3, 7, 8, 4)
            elif face_node_indexs == [1, 4, 5, 8]:
                node_numbers = (1, 4, 8, 5)
            elif face_node_indexs == [2, 3, 6, 7]:
                node_numbers = (2, 6, 7, 3)
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "hexa8: face not found! {}\n"
                    .format(face_node_indexs)
                )
        elif vol_node_ct == 15:
            FreeCAD.Console.PrintLog("  --> penta15 --> tria6 and quad8 faces\n")
            # node order of face in penta15 volume element
            if face_node_indexs == [1, 2, 3, 7, 8, 9]:
                # node order of a tria6 face of penta15
                node_numbers = (1, 2, 3, 7, 8, 9)
            elif face_node_indexs == [4, 5, 6, 10, 11, 12]:
                node_numbers = (4, 6, 5, 12, 11, 10)  # tria6
            elif face_node_indexs == [1, 2, 4, 5, 7, 10, 13, 14]:
                node_numbers = (1, 4, 5, 2, 13, 10, 14, 7)  # quad8
            elif face_node_indexs == [1, 3, 4, 6, 9, 12, 13, 15]:
                node_numbers = (1, 3, 6, 4, 9, 15, 12, 13)  # quad8
            elif face_node_indexs == [2, 3, 5, 6, 8, 11, 14, 15]:
                node_numbers = (2, 5, 6, 3, 14, 11, 15, 8)  # quad8
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "penta15: face not found! {}\n"
                    .format(face_node_indexs)
                )
        elif vol_node_ct == 6:
            FreeCAD.Console.PrintLog("  --> penta6 --> tria3 and quad4 faces\n")
            # node order of face in penta6 volume element
            if face_node_indexs == [1, 2, 3]:
                # node order of a tria3 face of penta6
                node_numbers = (1, 2, 3)
            elif face_node_indexs == [4, 5, 6]:
                node_numbers = (4, 6, 5)  # tria3
            elif face_node_indexs == [1, 2, 4, 5]:
                node_numbers = (1, 4, 5, 2)  # quad4
            elif face_node_indexs == [1, 3, 4, 6]:
                node_numbers = (1, 3, 6, 4)  # quad4
            elif face_node_indexs == [2, 3, 5, 6]:
                node_numbers = (2, 5, 6, 3)  # quad4
            else:
                FreeCAD.Console.PrintError(
                    "Error in build_mesh_faces_of_volume_elements(): "
                    "penta6: face not found! {}\n"
                    .format(face_node_indexs)
                )
        else:
            FreeCAD.Console.PrintError(
                "Error in build_mesh_faces_of_volume_elements(): "
                "Volume not implemented: volume node count {}\n"
                .format(vol_node_ct)
            )
        face_nodes = []
        for i in node_numbers:
            # node_number starts with 1
            # index starts with 0 -->
            # index = node number - 1
            i -= 1
            face_nodes.append(femelement_table[veID][i])
        face_table[veID] = face_nodes  # reset the entry in face_table
        FreeCAD.Console.PrintLog("  --> {}\n".format(face_table[veID]))
    return face_table


# ***** helper for Face and Edge loads ***********************************************************
def get_ref_shape_node_sum_geom_table(
    node_geom_table
):
    # shape could be Edge or Face, geom could be length or area
    # sum of length or area for each node of the ref_shape
    node_sum_geom_table = {}
    for n, A in node_geom_table:
        # FreeCAD.Console.PrintMessage("{} --> {}\n".format(n, A))
        if n in node_sum_geom_table:
            node_sum_geom_table[n] = node_sum_geom_table[n] + A
        else:
            node_sum_geom_table[n] = A
    return node_sum_geom_table


# ************************************************************************************************
# ***** methods for retrieving element face sets *************************************************
# ***** pressure faces ***************************************************************************
def get_pressure_obj_faces(
    femmesh,
    femelement_table,
    femnodes_ele_table,
    femobj
):
    # see get_ccxelement_faces_from_binary_search for more information
    if is_solid_femmesh(femmesh):
        # get the nodes
        # sorted and duplicates removed
        prs_face_node_set = get_femnodes_by_femobj_with_references(femmesh, femobj)
        # FreeCAD.Console.PrintMessage("prs_face_node_set: {}\n".format(prs_face_node_set))
        # fill the bit_pattern_dict and search for the faces
        bit_pattern_dict = get_bit_pattern_dict(
            femelement_table,
            femnodes_ele_table,
            prs_face_node_set
        )
        pressure_faces = get_ccxelement_faces_from_binary_search(bit_pattern_dict)
    elif is_face_femmesh(femmesh):
        pressure_faces = []
        # normally we should call get_femelements_by_references and
        # the group check should be integrated there
        if femmesh.GroupCount:
            meshfaces = get_femmesh_groupdata_sets_by_name(femmesh, femobj, "Face")
            # FreeCAD.Console.PrintMessage("{}\n".format(meshfaces))
            if not meshfaces:
                FreeCAD.Console.PrintError(
                    "Error: Something went wrong in getting the group element faces.\n"
                )
            else:
                for mf in meshfaces:
                    # pressure_faces.append([mf, 0])
                    pressure_faces.append([mf, -1])
                    # 0 if femmeshface normal == reference face normal direction
                    # -1 if femmeshface normal opposite reference face normal direction
                    # easy on plane faces, but on a half sphere ... ?!?
                    # might be useful to add ...
                    # How to find the orientation of a FEM mesh face?
                    # https://forum.freecadweb.org/viewtopic.php?f=18&t=51898
        else:
            FreeCAD.Console.PrintError(
                "Pressure on shell mesh at the moment only "
                "supported for meshes with appropriate group data.\n"
            )
    return pressure_faces


# ***** deprecated method for retrieving pressure faces *****************************************
# for constraint pressure and finite solid element mesh
# it was switched to the method get_ccxelement_faces_from_binary_search
# because of performance and the support of all solid elements
# see get_ccxelement_faces_from_binary_search for more information
def get_pressure_obj_faces_depreciated(
    femmesh,
    femobj
):
    pressure_faces = []
    for o, elem_tup in femobj["Object"].References:
        for elem in elem_tup:
            ref_shape = o.Shape.getElement(elem)
            elem_info_string = "face load on shape: " + o.Name + ":" + elem
            FreeCAD.Console.PrintMessage("{}\n".format(elem_info_string))
            if ref_shape.ShapeType == "Face":
                pressure_faces.append(
                    (elem_info_string, femmesh.getccxVolumesByFace(ref_shape))
                )
    return pressure_faces


# ***** contact faces ****************************************************************************
def get_contact_obj_faces(
    femmesh,
    femelement_table,
    femnodes_ele_table,
    femobj
):
    # see comment on get_pressure_obj_faces_depreciated in the regard of getccxVolumesByFace()

    # sets are needed for each of the references separated
    # BTW constraint tie works the same way AFAIK
    # TODO it might be useful to introduce a Reference_master and Reference_slave attribute

    # groups makes no sense, since group would be needed for each contact face (master and slave)

    # slave is DEP1 and master is IND1 in input file
    # first element face or ref_shape is slave, second is master

    # TODO above pre check in ccxtools
    # TODO ref_shape_type should be Face

    slave_faces, master_faces = [], []

    contact_obj = femobj["Object"]
    if len(contact_obj.References) == 1 and len(contact_obj.References[0][1]) == 2:
        # [(<Part::PartFeature>, ('Face7', 'Face3'))]
        # refs are merged because they are on the same doc obj
        # but only one element face for each contact face (Gui, TaskPael contact)
        ref_obj = contact_obj.References[0][0]
        ref_ele = contact_obj.References[0][1]
        slave_ref = (ref_obj, (ref_ele[0],))  # the comma is needed!
        master_ref = (ref_obj, (ref_ele[1],))  # the comma is needed!
    elif (
        len(contact_obj.References) == 2
        and len(contact_obj.References[0][1]) == 1
        and len(contact_obj.References[1][1]) == 1
    ):
        # [(<Part::PartFeature>, ('Face3',)), (<Part::PartFeature>, ('Face7',))]
        # refs are on different objects
        # but only one element face for each contact face (Gui, TaskPael contact)
        slave_ref = contact_obj.References[0]
        master_ref = contact_obj.References[1]
    else:
        FreeCAD.Console.PrintError(
            "Not valid (example: only master or slave defined) "
            "or not supported reference shape elements, contact face combination "
            "(example: multiple element faces per master or slave\n"
        )
        return [[], []]

    FreeCAD.Console.PrintLog("    Slave: {}, {}\n".format(slave_ref[0].Name, slave_ref))
    FreeCAD.Console.PrintLog("    Master: {}, {}\n".format(master_ref[0].Name, master_ref))

    if is_solid_femmesh(femmesh):
        FreeCAD.Console.PrintLog("    Get the nodes, sorted and duplicates removed.\n")
        slaveface_nds = sorted(list(set(get_femnodes_by_refshape(femmesh, slave_ref))))
        masterface_nds = sorted(list(set(get_femnodes_by_refshape(femmesh, master_ref))))
        FreeCAD.Console.PrintLog("    slaveface_nds: {}\n".format(slaveface_nds))
        FreeCAD.Console.PrintLog("    masterface_nds: {}\n".format(slaveface_nds))

        FreeCAD.Console.PrintLog("    Fill the bit_pattern_dict and search for the faces.\n")
        slave_bit_pattern_dict = get_bit_pattern_dict(
            femelement_table,
            femnodes_ele_table,
            slaveface_nds
        )
        master_bit_pattern_dict = get_bit_pattern_dict(
            femelement_table,
            femnodes_ele_table,
            masterface_nds
        )

        FreeCAD.Console.PrintLog("    Get the FaceIDs.\n")
        slave_faces = get_ccxelement_faces_from_binary_search(slave_bit_pattern_dict)
        master_faces = get_ccxelement_faces_from_binary_search(master_bit_pattern_dict)

    elif is_face_femmesh(femmesh):
        slave_ref_shape = slave_ref[0].Shape.getElement(slave_ref[1][0])
        master_ref_shape = master_ref[0].Shape.getElement(master_ref[1][0])

        FreeCAD.Console.PrintLog("    Get the FaceIDs.\n")
        slave_face_ids = femmesh.getFacesByFace(slave_ref_shape)
        master_face_ids = femmesh.getFacesByFace(master_ref_shape)

        # build slave_faces and master_faces
        # face 2 for tria6 element
        # is it face 2 for all shell elements
        for fid in slave_face_ids:
            slave_faces.append([fid, 2])
        for fid in master_face_ids:
            master_faces.append([fid, 2])

    FreeCAD.Console.PrintLog("    Master and slave face ready to use for writer:\n")
    FreeCAD.Console.PrintLog("    slave_faces: {}\n".format(slave_faces))
    FreeCAD.Console.PrintLog("    master_faces: {}\n".format(master_faces))
    if len(slave_faces) == 0:
        FreeCAD.Console.PrintError("No faces found for contact slave face.\n")
    if len(master_faces) == 0:
        FreeCAD.Console.PrintError("No faces found for contact master face.\n")
    return [slave_faces, master_faces]


# ***** tie faces ****************************************************************************
def get_tie_obj_faces(
    femmesh,
    femelement_table,
    femnodes_ele_table,
    femobj
):
    # see comment get_contact_obj_faces
    # solid mesh is same as contact, but face mesh is not allowed for tie
    # TODO get rid of duplicate code for contact and tie

    slave_faces, master_faces = [], []

    tie_obj = femobj["Object"]
    if len(tie_obj.References) == 1 and len(tie_obj.References[0][1]) == 2:
        # [(<Part::PartFeature>, ('Face7', 'Face3'))]
        # refs are merged because they are on the same doc obj
        # but only one element face for each contact face (Gui, TaskPael tie)
        ref_obj = tie_obj.References[0][0]
        ref_ele = tie_obj.References[0][1]
        slave_ref = (ref_obj, (ref_ele[0],))  # the comma is needed!
        master_ref = (ref_obj, (ref_ele[1],))  # the comma is needed!
    elif (
        len(tie_obj.References) == 2
        and len(tie_obj.References[0][1]) == 1
        and len(tie_obj.References[1][1]) == 1
    ):
        # [(<Part::PartFeature>, ('Face3',)), (<Part::PartFeature>, ('Face7',))]
        # refs are on different objects
        # but only one element face for each contact face (Gui, TaskPael tie)
        slave_ref = tie_obj.References[0]
        master_ref = tie_obj.References[1]
    else:
        FreeCAD.Console.PrintError(
            "Not valid (example: only master or slave defined) "
            "or not supported reference shape elements, contact face combination "
            "(example: multiple element faces per master or slave\n"
        )
        return [[], []]

    FreeCAD.Console.PrintLog("Slave: {}, {}\n".format(slave_ref[0].Name, slave_ref))
    FreeCAD.Console.PrintLog("Master: {}, {}\n".format(master_ref[0].Name, master_ref))

    if is_solid_femmesh(femmesh):
        # get the nodes, sorted and duplicates removed
        slaveface_nds = sorted(list(set(get_femnodes_by_refshape(femmesh, slave_ref))))
        masterface_nds = sorted(list(set(get_femnodes_by_refshape(femmesh, master_ref))))
        # FreeCAD.Console.PrintLog("slaveface_nds: {}\n".format(slaveface_nds))
        # FreeCAD.Console.PrintLog("masterface_nds: {}\n".format(slaveface_nds))

        # fill the bit_pattern_dict and search for the faces
        slave_bit_pattern_dict = get_bit_pattern_dict(
            femelement_table,
            femnodes_ele_table,
            slaveface_nds
        )
        master_bit_pattern_dict = get_bit_pattern_dict(
            femelement_table,
            femnodes_ele_table,
            masterface_nds
        )

        # get the faces ids
        slave_faces = get_ccxelement_faces_from_binary_search(slave_bit_pattern_dict)
        master_faces = get_ccxelement_faces_from_binary_search(master_bit_pattern_dict)

    elif is_face_femmesh(femmesh):
        FreeCAD.Console.PrintError(
            "Shell mesh is not allowed for constraint tie.\n"
        )

    FreeCAD.Console.PrintLog("slave_faces: {}\n".format(slave_faces))
    FreeCAD.Console.PrintLog("master_faces: {}\n".format(master_faces))
    return [slave_faces, master_faces]


# ************************************************************************************************
# ***** groups ***********************************************************************************
def get_mesh_group_elements(
    mesh_group_obj,
    aPart
):
    """the Reference shapes of the mesh_group_object are searched in the Shape of aPart.
       If found in shape they are added to a dict
       {MeshGroupIdentifier : ["ShapeType of the Elements"], [ElementID, ElementID, ...], ...}
    """
    group_elements = {}  # { name : [element, element, ... , element]}
    if mesh_group_obj.References:
        grp_ele = get_reference_group_elements(mesh_group_obj, aPart)
        group_elements[grp_ele[0]] = grp_ele[1]
    else:
        FreeCAD.Console.PrintError(
            "  Empty reference in mesh group object: {} {}\n"
            .format(mesh_group_obj.Name, mesh_group_obj.Label)
        )
    return group_elements


# ************************************************************************************************
def get_analysis_group_elements(
    aAnalysis,
    aPart
):
    """
    all Reference shapes of all Analysis member are searched in the Shape of aPart.
    If found in shape they are added to a dict
    {ConstraintName : ["ShapeType of the Elements"], [ElementID, ElementID, ...], ...}
    """
    from femtools.femutils import is_of_type
    group_elements = {}  # { name : [element, element, ... , element]}
    empty_references = []
    # find the objects with empty references, if there are more than one of this type
    # they are for all shapes not in the references of the other objects
    # ATM: empty references if there are more than one obj of this type are allowed for:
    # solid meshes: material
    # face meshes: materials, ShellThickness
    # edge meshes: material, BeamSection/FluidSection
    # BTW: some constraints do have empty references in any case (ex. constraint self weight)
    for m in aAnalysis.Group:
        if hasattr(m, "References"):
            if len(m.References) > 0:
                grp_ele = get_reference_group_elements(m, aPart)
                group_elements[grp_ele[0]] = grp_ele[1]
            elif (
                len(m.References) == 0
                and (
                    is_of_type(m, "Fem::MaterialCommon")
                    # TODO test and implement ElementGeometry1D and ElementGeometry2D
                    # or is_of_type(m, "Fem::ElementGeometry1D")
                    # or is_of_type(m, "Fem::ElementGeometry2D")
                )
            ):
                FreeCAD.Console.PrintMessage("  Empty reference: {}\n".format(m.Name))
                empty_references.append(m)
    if empty_references:
        if len(empty_references) == 1:
            group_elements = get_anlysis_empty_references_group_elements(
                group_elements,
                aAnalysis,
                aPart.Shape
            )
        else:
            FreeCAD.Console.PrintError(
                "Problem: more than one object with empty references.\n"
            )
            FreeCAD.Console.PrintMessage(
                "We are going to try to get the empty material references anyway.\n"
            )
            for er in empty_references:
                FreeCAD.Console.PrintMessage("{}\n".format(er.Name))
            group_elements = get_anlysis_empty_references_group_elements(
                group_elements,
                aAnalysis,
                aPart.Shape
            )
    # check if all groups have at least one element,
    # it doesn"t mean ALL reference shapes for a group have been found
    for g in group_elements:
        # FreeCAD.Console.PrintMessage("{}\n".format(group_elements[g]))
        if len(group_elements[g]) == 0:
            FreeCAD.Console.PrintError(
                "Error: The shapes for the mesh group for the reference "
                "shapes of analysis member: {} could not be found!\n"
                .format(g)
            )
    return group_elements


# ************************************************************************************************
def get_reference_group_elements(
    obj,
    aPart
):
    """ obj is an FEM object which has reference shapes like the group object
    the material, most of the constraints
    aPart is geometry feature normally CompSolid
    the method searches all reference shapes of obj inside aPart even if
    the reference shapes are a totally different geometry feature.
    a tuple is returned ("Name or Label of the FEMobject", ["Element1", "Element2", ...])
    The names in the list are the Elements of the geometry aPart
    whereas "Solid1" == aPart.Shape.Solids[0]
    !!! It is strongly recommended to use as reference shapes the Solids of a CompSolid
    and not the Solids the CompSolid is made of !!!
    see https://forum.freecadweb.org/viewtopic.php?f=18&t=12212&p=175777#p175777
    and following posts
    Occt might change the Solids a CompSolid is made of during
    creation of the CompSolid by adding Edges and vertices
    Thus the Elements do not have the same geometry anymore
    """
    aShape = aPart.Shape
    if hasattr(obj, "UseLabel") and obj.UseLabel:
        # TODO: check the character of the Label
        # only allow underline and standard english character
        key = obj.Label
    else:
        key = obj.Name
    elements = []
    stype = None
    for r in obj.References:
        parent = r[0]
        childs = r[1]
        # FreeCAD.Console.PrintMessage("{}\n".format(parent))
        # FreeCAD.Console.PrintMessage("{}\n".format(childs))
        for child in childs:
            # the method getElement(element) does not return Solid elements
            ref_shape = geomtools.get_element(parent, child)
            if not stype:
                stype = ref_shape.ShapeType
            elif stype != ref_shape.ShapeType:
                FreeCAD.Console.PrintError(
                    "Error, two refshapes in References with different ShapeTypes.\n"
                )
            FreeCAD.Console.PrintLog("\n".format(ref_shape))
            found_element = geomtools.find_element_in_shape(aShape, ref_shape)
            if found_element is not None:
                elements.append(found_element)
            else:
                FreeCAD.Console.PrintError(
                    "Problem: For the geometry of the "
                    "following shape was no Shape found: {}\n"
                    .format(ref_shape)
                )
                FreeCAD.Console.PrintMessage("    " + obj.Name + "\n")
                FreeCAD.Console.PrintMessage("    " + str(obj.References) + "\n")
                FreeCAD.Console.PrintMessage("    " + r[0].Name + "\n")
                if parent.Name != aPart.Name:
                    FreeCAD.Console.PrintError(
                        "The reference Shape is not a child "
                        "nor it is the shape the mesh is made of. : {}\n"
                        .format(ref_shape)
                    )
                    FreeCAD.Console.PrintMessage(
                        "{}--> Name of the Feature we where searching in.\n"
                        .format(aPart.Name)
                    )
                    FreeCAD.Console.PrintMessage(
                        "{} --> Name of the parent Feature of reference Shape "
                        "(Use the same as in the line before and you "
                        "will have less trouble :-) !!!!!!).\n"
                        .format(parent.Name)
                    )
                    # import Part
                    # Part.show(aShape)
                    # Part.show(ref_shape)
                else:
                    FreeCAD.Console.PrintError("This should not happen, please debug!\n")
                    # in this case we would not have needed to use the
                    # is_same_geometry() inside geomtools.find_element_in_shape()
                    # AFAIK we could have used the Part methods isPartner() or even isSame()
                    # We're going to find out when we need to debug this :-)!
    return (key, sorted(elements))


# ************************************************************************************************
def get_anlysis_empty_references_group_elements(
    group_elements,
    aAnalysis,
    aShape
):
    """
    get the elementIDs if the Reference shape is empty
    see get_analysis_group_elements() for more information
    """
    # FreeCAD.Console.PrintMessage("{}\n".format(group_elements))
    material_ref_shapes = []
    material_shape_type = ""
    missed_material_refshapes = []
    empty_reference_material = None
    for m in aAnalysis.Group:
        if m.isDerivedFrom("App::MaterialObjectPython"):
            if hasattr(m, "References") and not m.References:
                if not empty_reference_material:
                    empty_reference_material = m.Name
                else:
                    FreeCAD.Console.PrintError(
                        "Problem in get_anlysis_empty_references_group_elements, "
                        "we seem to have two or more materials with empty references\n"
                    )
                    return {}
            elif hasattr(m, "References") and m.References:
                # ShapeType of the group elements, strip the number of the first group element
                # http://stackoverflow.com/questions/12851791/removing-numbers-from-string
                group_shape_type = "".join(i for i in group_elements[m.Name][0] if not i.isdigit())
                if not material_shape_type:
                    material_shape_type = group_shape_type
                elif material_shape_type != group_shape_type:
                    FreeCAD.Console.PrintError(
                        "Problem, material shape type does not match "
                        "get_anlysis_empty_references_group_elements\n"
                    )
                for ele in group_elements[m.Name]:
                    material_ref_shapes.append(ele)
    if material_shape_type == "Solid":
        # FreeCAD.Console.PrintMessage("{}\n".format(len(aShape.Solids)))
        for i in range(len(aShape.Solids)):
            ele = "Solid" + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    elif material_shape_type == "Face":
        # FreeCAD.Console.PrintMessage("{}\n".format(len(aShape.Faces)))
        for i in range(len(aShape.Faces)):
            ele = "Face" + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    elif material_shape_type == "Edge":
        # FreeCAD.Console.PrintMessage("{}\n".format(len(aShape.Edges)))
        for i in range(len(aShape.Edges)):
            ele = "Edge" + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    else:
        FreeCAD.Console.PrintMessage(
            "  One material with no reference shapes. No need to make a group for materials.\n"
        )
        # make no changes group_elements
        return group_elements
    # FreeCAD.Console.PrintMessage("{}\n".format(sorted(material_ref_shapes)))
    # FreeCAD.Console.PrintMessage("{}\n".format(sorted(missed_material_refshapes)))
    # FreeCAD.Console.PrintMessage(group_elements)
    group_elements[empty_reference_material] = sorted(missed_material_refshapes)
    # FreeCAD.Console.PrintMessage("{}\n".format(group_elements))
    return group_elements


# ************************************************************************************************
def femelements_count_ok(
    len_femelement_table,
    count_femelements
):
    FreeCAD.Console.PrintMessage(
        "Count finite elements as sum of constraints:   {}\n"
        .format(count_femelements)
    )
    FreeCAD.Console.PrintMessage(
        "Count finite elements of the finite element mesh: {}\n"
        .format(len_femelement_table)
    )
    if count_femelements == len_femelement_table:
        return True
    else:
        FreeCAD.Console.PrintMessage(
            "ERROR: femelement_table != count_femelements\n"
        )
        return False


# ************************************************************************************************
def delete_duplicate_mesh_elements(
    refelement_table
):
    new_refelement_table = {}  # duplicates deleted
    for elem, nodes in refelement_table.items():
        if sorted(nodes) not in sortlistoflistvalues(new_refelement_table.values()):
            new_refelement_table[elem] = nodes
    return new_refelement_table


# ************************************************************************************************
def get_triangle_area(
    P1,
    P2,
    P3
):
    # import Part
    # W = Part.Wire([Part.makeLine(P1,P2), Part.makeLine(P2,P3), Part.makeLine(P3,P1)])
    # Part.show(Part.Face(W))
    vec1 = P2 - P1
    vec2 = P3 - P1
    vec3 = vec1.cross(vec2)
    return 0.5 * vec3.Length


# ************************************************************************************************
def sortlistoflistvalues(
    listoflists
):
    new_list = []
    for l in listoflists:
        new_list.append(sorted(l))
    return new_list


# ************************************************************************************************
def is_solid_femmesh(
    femmesh
):
    # solid femmesh
    if femmesh.VolumeCount > 0:
        return True


# ************************************************************************************************
def has_no_face_data(
    femmesh
):
    # femmesh has no face data, could be a edge femmesh or a solid femmesh without face data
    if femmesh.FaceCount == 0:
        return True


# ************************************************************************************************
def is_face_femmesh(
    femmesh
):
    # face femmesh
    if femmesh.VolumeCount == 0 and femmesh.FaceCount > 0:
        return True


# ************************************************************************************************
def is_edge_femmesh(
    femmesh
):
    # edge femmesh
    if femmesh.VolumeCount == 0 and femmesh.FaceCount == 0 and femmesh.EdgeCount > 0:
        return True


# ************************************************************************************************
def is_zplane_2D_mesh(
    femmesh
):
    # used in oofem writer to distinguish between 3D and 2D plane stress
    if is_face_femmesh(femmesh) is True:
        tol = 0.0001
        for n in femmesh.Nodes:
            z = femmesh.Nodes[n].z
            if ((0 - tol) < z < (0 + tol)) is not True:
                    return False
        return True
    else:
        return False


# ************************************************************************************************
def get_femmesh_eletype(
    femmesh,
    femelement_table=None
):
    if not femmesh:
        FreeCAD.Console.PrintError("Error: No femmesh.\n")
    if not femelement_table:
        FreeCAD.Console.PrintWarning("The femelement_table needs to be calculated.\n")
        femelement_table = get_femelement_table(femmesh)
    # in some cases lowest key in femelement_table is not [1]
    for elem in sorted(femelement_table):
        elem_length = len(femelement_table[elem])
        FreeCAD.Console.PrintLog("Node count of first element: {}\n".format(elem_length))
        break  # break after the first elem
    if is_solid_femmesh(femmesh):
        if femmesh.TetraCount == femmesh.VolumeCount:
            if elem_length == 4:
                return "tetra4"
            elif elem_length == 10:
                return "tetra10"
            else:
                FreeCAD.Console.PrintMessage("Tetra with neither 4 nor 10 nodes.\n")
                return "None"
        elif femmesh.HexaCount == femmesh.VolumeCount:
            if elem_length == 8:
                return "hexa8"
            elif elem_length == 20:
                return "hexa20"
            else:
                FreeCAD.Console.PrintError("Hexa with neither 8 nor 20 nodes.\n")
                return "None"
        else:
            FreeCAD.Console.PrintError("no tetra, no hexa or Mixed Volume Elements.\n")
    elif is_face_femmesh(femmesh):
        if femmesh.TriangleCount == femmesh.FaceCount:
            if elem_length == 3:
                return "tria3"
            elif elem_length == 6:
                return "tria6"
            else:
                FreeCAD.Console.PrintError("Tria with neither 3 nor 6 nodes.\n")
                return "None"
        elif femmesh.QuadrangleCount == femmesh.FaceCount:
            if elem_length == 4:
                return "quad4"
            elif elem_length == 8:
                return "quad8"
            else:
                FreeCAD.Console.PrintError("Quad with neither 4 nor 8 nodes.\n")
                return "None"
        else:
            FreeCAD.Console.PrintError("no tria, no quad\n")
            return "None"
    elif is_edge_femmesh(femmesh):
        if elem_length == 2:
            return "seg2"
        elif elem_length == 3:
            return "seg3"
        else:
            FreeCAD.Console.PrintError("Seg with neither 2 nor 3 nodes.\n")
            return "None"
    else:
        FreeCAD.Console.PrintError("Neither edge nor face nor solid femmesh.\n")
        return "None"
    return "None"


# ************************************************************************************************
def get_three_non_colinear_nodes(
    nodes_coords
):
    # Code to obtain three non-colinear nodes on the PlaneRotation support face
    # nodes_coords --> [(nodenumber, x, y, z), (nodenumber, x, y, z), ...]
    if not nodes_coords:
        FreeCAD.Console.PrintMessage("{}\n".format(len(nodes_coords)))
        FreeCAD.Console.PrintMessage("Error: No nodes in nodes_coords\n")
        return []
    dum_max = [1, 2, 3, 4, 5, 6, 7, 8, 0]
    for i in range(len(nodes_coords)):
        for j in range(len(nodes_coords) - 1 - i):
            x_1 = nodes_coords[j][1]
            x_2 = nodes_coords[j + 1][1]
            y_1 = nodes_coords[j][2]
            y_2 = nodes_coords[j + 1][2]
            z_1 = nodes_coords[j][3]
            z_2 = nodes_coords[j + 1][3]
            node_1 = nodes_coords[j][0]
            node_2 = nodes_coords[j + 1][0]
            distance = ((x_1 - x_2) ** 2 + (y_1 - y_2) ** 2 + (z_1 - z_2) ** 2) ** 0.5
            if distance > dum_max[8]:
                dum_max = [node_1, x_1, y_1, z_1, node_2, x_2, y_2, z_2, distance]
    node_dis = [1, 0]
    for i in range(len(nodes_coords)):
        x_1 = dum_max[1]
        x_2 = dum_max[5]
        x_3 = nodes_coords[i][1]
        y_1 = dum_max[2]
        y_2 = dum_max[6]
        y_3 = nodes_coords[i][2]
        z_1 = dum_max[3]
        z_2 = dum_max[7]
        z_3 = nodes_coords[i][3]
        node_3 = int(nodes_coords[j][0])
        distance_1 = ((x_1 - x_3) ** 2 + (y_1 - y_3) ** 2 + (z_1 - z_3) ** 2) ** 0.5
        distance_2 = ((x_3 - x_2) ** 2 + (y_3 - y_2) ** 2 + (z_3 - z_2) ** 2) ** 0.5
        tot = distance_1 + distance_2
        if tot > node_dis[1]:
            node_dis = [node_3, tot]
    node_1 = int(dum_max[0])
    node_2 = int(dum_max[4])
    FreeCAD.Console.PrintMessage("{}\n".format([node_1, node_2, node_3]))
    return [node_1, node_2, node_3]


# ************************************************************************************************
def write_D_network_element_to_inputfile(
    fileName
):
    # replace B32 elements with D elements for fluid section
    f = open(fileName, "r+")
    lines = f.readlines()
    f.seek(0)
    for line in lines:
        if line.find("B32") == -1:
            f.write(line)
        else:
            dummy = line.replace("B32", "D")
            f.write(dummy)
    f.truncate()
    f.close()


# ************************************************************************************************
def use_correct_fluidinout_ele_def(
    FluidInletoutlet_ele,
    fileName,
    fluid_inout_nodes_file
):
    f = open(fileName, "r")
    cnt = 0
    line = f.readline()

    # start reading from *ELEMENT
    while line.find("Element") == -1:
        line = f.readline()
        cnt = cnt + 1
    line = f.readline()
    cnt = cnt + 1

    # obtain element line numbers for inlet and outlet
    while (len(line) > 1):
        ind = line.find(",")
        elem = line[0:ind]
        for i in range(len(FluidInletoutlet_ele)):
            if (elem == FluidInletoutlet_ele[i][0]):
                FluidInletoutlet_ele[i][2] = cnt
        line = f.readline()
        cnt = cnt + 1
    f.close()

    # re-define elements for INLET and OUTLET
    f = open(fileName, "r+")
    lines = f.readlines()
    f.seek(0)
    cnt = 0
    elem_counter = 0
    FreeCAD.Console.PrintMessage(
        "1DFlow inout nodes file: {}\n"
        .format(fluid_inout_nodes_file)
    )
    inout_nodes_file = open(fluid_inout_nodes_file, "w")
    for line in lines:
        new_line = ""
        for i in range(len(FluidInletoutlet_ele)):
            if (cnt == FluidInletoutlet_ele[i][2]):
                elem_counter = elem_counter + 1
                a = line.split(",")
                for j in range(len(a)):
                    if elem_counter == 1:
                        if j == 1:
                            new_line = new_line + " 0,"
                            node1 = int(a[j + 2])
                            node2 = int(a[j + 1])
                            node3 = int(a[j])
                            inout_nodes_file.write("{},{},{},{}\n".format(
                                node1,
                                node2,
                                node3,
                                FluidInletoutlet_ele[i][1]
                            ))
                        elif j == 3:
                            new_line = new_line + a[j]
                        else:
                            new_line = new_line + a[j] + ","
                    else:
                        if j == 3:
                            new_line = new_line + " 0\n"
                            node1 = int(a[j - 2])
                            node2 = int(a[j - 1])
                            node3 = int(a[j])
                            inout_nodes_file.write("{},{},{},{}\n".format(
                                node1,
                                node2,
                                node3,
                                FluidInletoutlet_ele[i][1]
                            ))
                        else:
                            new_line = new_line + a[j] + ","
        if new_line == "":
            f.write(line)
        else:
            f.write(new_line)
        cnt = cnt + 1
    f.truncate()
    f.close()
    inout_nodes_file.close()


# ************************************************************************************************
def compact_mesh(
    old_femmesh
):
    """
    removes all gaps in node and element ids, start ids with 1
    returns a tuple (FemMesh, node_assignment_map, element_assignment_map)
    """
    node_map = {}  # {old_node_id: new_node_id, ...}
    elem_map = {}  # {old_elem_id: new_elem_id, ...}
    old_nodes = old_femmesh.Nodes
    import Fem
    new_mesh = Fem.FemMesh()

    if old_nodes:
        for i, n in enumerate(old_nodes):
            nid = i + 1
            new_mesh.addNode(old_nodes[n].x, old_nodes[n].y, old_nodes[n].z, nid)
            node_map[n] = nid

    # element id is one id for Edges, Faces and Volumes
    # thus should not start with 0 for each element type
    # because this will give an error for mixed meshes
    # because the id is used already
    # https://forum.freecadweb.org/viewtopic.php?t=48215
    ele_id = 1
    if old_femmesh.Edges:
        for ed in old_femmesh.Edges:
            old_elem_nodes = old_femmesh.getElementNodes(ed)
            new_elemnodes = []
            for old_node_id in old_elem_nodes:
                new_elemnodes.append(node_map[old_node_id])
            new_mesh.addEdge(new_elemnodes, ele_id)
            elem_map[ed] = ele_id
            ele_id += 1
    if old_femmesh.Faces:
        for fa in old_femmesh.Faces:
            ele_id += 1
            old_elem_nodes = old_femmesh.getElementNodes(fa)
            new_elemnodes = []
            for old_node_id in old_elem_nodes:
                new_elemnodes.append(node_map[old_node_id])
            new_mesh.addFace(new_elemnodes, ele_id)
            elem_map[fa] = ele_id
            ele_id += 1
    if old_femmesh.Volumes:
        for vo in old_femmesh.Volumes:
            old_elem_nodes = old_femmesh.getElementNodes(vo)
            new_elemnodes = []
            for old_node_id in old_elem_nodes:
                new_elemnodes.append(node_map[old_node_id])
            new_mesh.addVolume(new_elemnodes, ele_id)
            elem_map[vo] = ele_id
            ele_id += 1

    # may be return another value if the mesh was compacted, just check last map entries
    return (new_mesh, node_map, elem_map)

##  @}
