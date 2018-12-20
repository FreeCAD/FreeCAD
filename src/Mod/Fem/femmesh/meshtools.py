# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "Tools for the work with FEM meshes"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD


def get_femnodes_by_femobj_with_references(femmesh, femobj):
    node_set = []
    if femmesh.GroupCount:
        node_set = get_femmesh_groupdata_sets_by_name(femmesh, femobj, 'Node')
        # print('node_set_group: ', node_set)
        if node_set:
            print("  nodes where retrieved from existent FEM mesh group data")
    if not node_set:
        print("  nodes will be retrieved by searching the appropriate nodes in the FEM mesh")
        node_set = get_femnodes_by_references(femmesh, femobj['Object'].References)
        # print('node_set_nogroup: ', node_set)

    return node_set


def get_femelements_by_references(femmesh, femelement_table, references, femnodes_ele_table=None):
    '''get the femelements for a list of references
    '''
    references_femelements = []
    for ref in references:
        ref_femnodes = get_femnodes_by_refshape(femmesh, ref)  # femnodes for the current ref
        if femnodes_ele_table:
            # blind fast binary search, works for volumes only
            references_femelements += get_femelements_by_femnodes_bin(femelement_table, femnodes_ele_table, ref_femnodes)  # femelements for all references
        else:
            # standard search
            references_femelements += get_femelements_by_femnodes_std(femelement_table, ref_femnodes)  # femelements for all references
    return references_femelements


def get_femnodes_by_references(femmesh, references):
    '''get the femnodes for a list of references
    '''
    references_femnodes = []
    for ref in references:
        references_femnodes += get_femnodes_by_refshape(femmesh, ref)

    # return references_femnodes  # keeps duplicate nodes, keeps node order

    # if nodes are used for nodesets, duplicates should be removed
    return list(set(references_femnodes))  # removes duplicate nodes, sorts node order


def get_femnodes_by_refshape(femmesh, ref):
    nodes = []
    for refelement in ref[1]:
        r = get_element(ref[0], refelement)  # the method getElement(element) does not return Solid elements
        print('  ReferenceShape ... Type: ' + r.ShapeType + ', Object name: ' + ref[0].Name + ', Object label: ' + ref[0].Label + ', Element name: ' + refelement)
        if r.ShapeType == 'Vertex':
            nodes += femmesh.getNodesByVertex(r)
        elif r.ShapeType == 'Edge':
            nodes += femmesh.getNodesByEdge(r)
        elif r.ShapeType == 'Face':
            nodes += femmesh.getNodesByFace(r)
        elif r.ShapeType == 'Solid':
            nodes += femmesh.getNodesBySolid(r)
        else:
            print('  No Vertice, Edge, Face or Solid as reference shapes!')
    return nodes


def get_femelement_table(femmesh):
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
        FreeCAD.Console.PrintError('Neither solid nor face nor edge femmesh!\n')
    return femelement_table


def get_femelement_volumes_table(femmesh):
    """ get_femelement_volumes_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    for i in femmesh.Volumes:
        table[i] = femmesh.getElementNodes(i)
    return table


def get_femelement_faces_table(femmesh, faces_only=None):
    """ get_femelement_faces_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    if not faces_only:
        faces_only = femmesh.FacesOnly
    for i in faces_only:
        table[i] = femmesh.getElementNodes(i)
    return table


def get_femelement_edges_table(femmesh, edges_only=None):
    """ get_femelement_edges_table(femmesh): { elementid : [ nodeid, nodeid, ... , nodeid ] }"""
    table = {}
    if not edges_only:
        edges_only = femmesh.EdgesOnly
    for i in edges_only:
        table[i] = femmesh.getElementNodes(i)
    return table


def get_femnodes_ele_table(femnodes_mesh, femelement_table):
    '''the femnodes_ele_table contains for each node its membership in elements
    {nodeID : [[eleID, NodePosition], [], ...], nodeID : [[], [], ...], ...}
    stored informatation are:
    element number, the number of nodes per element, the position of the node in the element.
    The position of the node in the element is coded as a set bit at that position in a bit array (integer)
    Fixme: the number of nodes per element should be replaced by the type of the element
    but I did not know, how to get this from the mesh.
    Since the femelement_table contains either volume or face or edgemesh the femnodes_ele_table only
    has either volume or face or edge elements, see get_femelement_table()
    '''
    femnodes_ele_table = {}  # node_dict in ulrichs class
    for n in femnodes_mesh:  # initialize it with sorted node keys and empty lists
        femnodes_ele_table[n] = []
    for ele in femelement_table:
        ele_list = femelement_table[ele]
        # print(ele_list)
        pos = int(1)
        for ele_node in ele_list:
            femnodes_ele_table[ele_node].append([ele, pos])
            pos = pos << 1
    print('len femnodes_ele_table: ' + str(len(femnodes_ele_table)))
    # print('femnodes_ele_table: ', femnodes_ele_table)
    return femnodes_ele_table


def get_copy_of_empty_femelement_table(femelement_table):
    '''{eleID : 0, eleID : 0, ...}
    '''
    empty_femelement_table = {}
    for ele in femelement_table:  # initialize it with sorted element keys and empty int
        empty_femelement_table[ele] = 0
    return empty_femelement_table.copy()


def get_bit_pattern_dict(femelement_table, femnodes_ele_table, node_set):
    '''Now we are looking for nodes inside of the Faces = filling the bit_pattern_dict
    {eleID : [lenEleNodes, binary_position]}
    see forum post for a very good explanation of what's really happening
    http://forum.freecadweb.org/viewtopic.php?f=18&p=141133&sid=013c93f496a63872951d2ce521702ffa#p141108
    The bit_pattern_dict holds later an integer (bit array) for each element, which gives us
    the information we are searching for:
    Is this element part of the node list (searching for elements) or has this element a face we are searching for?
    The number in the ele_dict is organized as a bit array.
    The corresponding bit is set, if the node of the node_set is contained in the element.
    '''
    print('len femnodes_ele_table: ' + str(len(femnodes_ele_table)))
    print('len node_set: ' + str(len(node_set)))
    # print('node_set: ', node_set)
    bit_pattern_dict = get_copy_of_empty_femelement_table(femelement_table)
    # # initializing the bit_pattern_dict
    for ele in femelement_table:
        len_ele = len(femelement_table[ele])
        bit_pattern_dict[ele] = [len_ele, 0]
    for node in node_set:
        for nList in femnodes_ele_table[node]:
            bit_pattern_dict[nList[0]][1] += nList[1]
    print('len bit_pattern_dict: ' + str(len(bit_pattern_dict)))
    # print('bit_pattern_dict: ', bit_pattern_dict)
    return bit_pattern_dict


def get_ccxelement_faces_from_binary_search(bit_pattern_dict):
    '''get the CalculiX element face numbers
    '''
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
    print('found Faces: ', len(faces))
    print('faces: ', faces)
    return faces


def get_femelements_by_femnodes_bin(femelement_table, femnodes_ele_table, node_list):
    '''for every femelement of femelement_table
    if all nodes of the femelement are in node_list,
    the femelement is added to the list which is returned
    blind fast binary search, but works for volumes only
    '''
    print('binary search: get_femelements_by_femnodes_bin')
    vol_masks = {
        4: 15,
        6: 63,
        8: 255,
        10: 1023,
        15: 32767,
        20: 1048575}
    # Now we are looking for nodes inside of the Volumes = filling the bit_pattern_dict
    print('len femnodes_ele_table: ' + str(len(femnodes_ele_table)))
    bit_pattern_dict = get_bit_pattern_dict(femelement_table, femnodes_ele_table, node_list)
    # search
    ele_list = []  # The ele_list contains the result of the search.
    for ele in bit_pattern_dict:
        # print('bit_pattern_dict[ele][0]: ', bit_pattern_dict[ele][0])
        if bit_pattern_dict[ele][1] == vol_masks[bit_pattern_dict[ele][0]]:
            ele_list.append(ele)
    print('found Volumes: ', len(ele_list))
    print('   volumes: ', len(ele_list))
    return ele_list


def get_femelements_by_femnodes_std(femelement_table, node_list):
    '''for every femelement of femelement_table
    if all nodes of the femelement are in node_list,
    the femelement is added to the list which is returned
    e: elementlist
    nodes: nodelist '''
    print('std search: get_femelements_by_femnodes_std')
    e = []  # elementlist
    for elementID in sorted(femelement_table):
        nodecount = 0
        for nodeID in femelement_table[elementID]:
            if nodeID in node_list:
                nodecount = nodecount + 1
        if nodecount == len(femelement_table[elementID]):   # all nodes of the element are in the node_list!
            e.append(elementID)
    return e


def get_femvolumeelements_by_femfacenodes(femelement_table, node_list):
    '''assume femelement_table only has volume elements
    for every femvolumeelement of femelement_table
    for tetra4 and tetra10 the C++ methods could be used --> test again to be sure
    if hexa8 volume element --> if exact 4 element nodes are in node_list --> add femelement
    if hexa20 volume element --> if exact 8 element nodes are in node_list --> add femelement
    if penta6 volume element --> if exact 3 or 6 element nodes are in node_list --> add femelement
    if penta15 volume element --> if exact 6 or 8 element nodes are in node_list --> add femelement
    e: elementlist
    nodes: nodelist '''
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
            FreeCAD.Console.PrintError('Error in get_femvolumeelements_by_femfacenodes(): unknown volume element: ' + el_nd_ct + '\n')
    # print(sorted(e))
    return e


def get_femelement_sets(femmesh, femelement_table, fem_objects, femnodes_ele_table=None):  # fem_objects = FreeCAD FEM document objects
    # get femelements for reference shapes of each obj.References
    count_femelements = 0
    referenced_femelements = []
    has_remaining_femelements = None
    for fem_object_i, fem_object in enumerate(fem_objects):
        obj = fem_object['Object']
        print("Constraint: " + obj.Name + " --> " + "We're going to search in the mesh for the element ID's.")
        fem_object['ShortName'] = get_elset_short_name(obj, fem_object_i)  # unique short identifier
        if obj.References:
            ref_shape_femelements = []
            ref_shape_femelements = get_femelements_by_references(femmesh, femelement_table, obj.References, femnodes_ele_table)
            referenced_femelements += ref_shape_femelements
            count_femelements += len(ref_shape_femelements)
            fem_object['FEMElements'] = ref_shape_femelements
        else:
            has_remaining_femelements = obj.Name
    # get remaining femelements for the fem_objects
    if has_remaining_femelements:
        remaining_femelements = []
        for elemid in femelement_table:
            if elemid not in referenced_femelements:
                remaining_femelements.append(elemid)
        count_femelements += len(remaining_femelements)
        for fem_object in fem_objects:
            obj = fem_object['Object']
            if obj.Name == has_remaining_femelements:
                fem_object['FEMElements'] = sorted(remaining_femelements)
    # check if all worked out well
    if femelements_count_ok(len(femelement_table), count_femelements):
        return True
    else:
        FreeCAD.Console.PrintError('Error in get_femelement_sets -- > femelements_count_ok() failed!\n')
        return False


def get_femelement_direction1D_set(femmesh, femelement_table, beamrotation_objects, theshape=None):
    '''
    get for each geometry edge direction, the normal and the element ids and write all into the beamrotation_objects
    means no return value, we're going to write into the beamrotation_objects dictionary
    FEMRotations1D is a list of dictionaries for every beamdirection of all edges
    beamrot_obj['FEMRotations1D'] = [ {'ids' : [theids],
                                       'direction' : direction,
                                       'normal' : normal},
                                      ...
                                    ]
    '''
    if len(beamrotation_objects) == 0:
        # no beamrotation document object, all beams use standard rotation of 0 degree (angle), we need theshape (the shape which was meshed)
        # since ccx needs to split them in sets anyway we need to take care of this too
        rotations_ids = get_femelement_directions_theshape(femmesh, femelement_table, theshape)
        # add normals for each direction
        rotation_angle = 0
        for rot in rotations_ids:
            rot['normal'] = get_beam_normal(rot['direction'], rotation_angle)
        beamrotation_objects.append({'FEMRotations1D': rotations_ids, 'ShortName': 'Rstd'})  # key 'Object' will be empty
    elif len(beamrotation_objects) == 1:
        # one beamrotation document object with no references, all beams use rotation from this object, we need theshape (the shape which was meshed)
        # since ccx needs to split them in sets anyway we need to take care of this too
        rotations_ids = get_femelement_directions_theshape(femmesh, femelement_table, theshape)
        # add normals for each direction
        rotation_angle = beamrotation_objects[0]['Object'].Rotation
        for rot in rotations_ids:
            rot['normal'] = get_beam_normal(rot['direction'], rotation_angle)
        beamrotation_objects[0]['FEMRotations1D'] = rotations_ids
        beamrotation_objects[0]['ShortName'] = 'R0'
    elif len(beamrotation_objects) > 1:
        # multiple beam rotation document objects, rotations defined by reference shapes, TODO implement this
        # do not forget all the corner cases:
        # one beam rotation object, but not all edges are ref shapes
        # more than one beam rotation object, but not all edges are in the ref shapes
        # for the both cases above, all other edges get standard rotation.
        # more than one beam rotation objects and on has no ref shapes, all edges no in an rotation object use this rotation
        # one edge is in more than one beam rotation object, error
        # pre check, only one beam rotation with empty ref shapes is allowed
        # we need theshape for multiple rotations too, because of the corner cases mentioned above
        FreeCAD.Console.PrintError('Multiple Rotations not yet supported!\n')
    for rot_object in beamrotation_objects:  # debug print
        print(rot_object['FEMRotations1D'])


def get_femelement_directions_theshape(femmesh, femelement_table, theshape):
    # see get_femelement_direction1D_set
    rotations_ids = []
    # add directions and all ids for each direction
    for e in theshape.Shape.Edges:
        the_edge = {}
        the_edge['direction'] = e.Vertexes[1].Point - e.Vertexes[0].Point
        edge_femnodes = femmesh.getNodesByEdge(e)  # femnodes for the current edge
        the_edge['ids'] = get_femelements_by_femnodes_std(femelement_table, edge_femnodes)  # femelements for this edge
        for rot in rotations_ids:
            if rot['direction'] == the_edge['direction']:  # tolerance will be managed by FreeCAD see https://forum.freecadweb.org/viewtopic.php?f=22&t=14179
                rot['ids'] += the_edge['ids']
                break
        else:
            rotations_ids.append(the_edge)
    return rotations_ids


def get_beam_normal(beam_direction, defined_angle):
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

    Dot_product_check_x = None
    Dot_product_check_y = None
    Dot_product_check_z = None
    Dot_product_check_nt = None
    if vector_a[0] != 0 and vector_a[1] == 0 and vector_a[2] == 0:
        normal_n = [temp_valx, nx, ny]
        Dot_product_check_x = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] != 0 and vector_a[2] == 0:
        normal_n = [nx, temp_valy, ny]
        Dot_product_check_y = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] == 0 and vector_a[2] != 0:
        normal_n = [nx, ny, temp_valz]
        Dot_product_check_z = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] == 0 and vector_a[1] != 0 and vector_a[2] != 0:
        normal_n = [nx, temp_valy, ny]
        Dot_product_check_y = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    elif vector_a[0] != 0 and vector_a[1] == 0 and vector_a[2] != 0:
        normal_n = [nx, ny, temp_valz]
        Dot_product_check_z = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    else:
        normal_n = [temp_valx, nx, ny]
        Dot_product_check_nt = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]

    Dot_product_check = vector_a[0] * normal_n[0] + vector_a[1] * normal_n[1] + vector_a[2] * normal_n[2]
    # print(Dot_product_check)
    # print(normal_n)

    # dummy usage of the axis Dot_product_check to get flake8 quiet
    del Dot_product_check_x, Dot_product_check_y, Dot_product_check_z, Dot_product_check, Dot_product_check_nt

    return normal_n


def get_femmesh_groupdata_sets_by_name(femmesh, fem_object, group_data_type):
    # get ids from femmesh groupdata for reference shapes of each obj.References
    # we assume the mesh group data fits with the reference shapes, no check is done in this regard !!!
    # we just check for the group name and the group data type
    # what happens if a reference shape was changed, but the mesh and the mesh groups were not created new !?!
    obj = fem_object['Object']
    if femmesh.GroupCount:
        for g in femmesh.Groups:
            grp_name = femmesh.getGroupName(g)
            if grp_name.startswith(obj.Name + "_"):
                if femmesh.getGroupElementType(g) == group_data_type:
                    print("  found mesh group for the IDs: " + grp_name + ', Type: ' + group_data_type)
                    return femmesh.getGroupElements(g)  # == ref_shape_femelements
    return ()  # an empty tuple is returned if no group data IDs where found


def get_femelement_sets_from_group_data(femmesh, fem_objects):
    # get femelements from femmesh groupdata for reference shapes of each obj.References
    count_femelements = 0
    sum_group_elements = []
    for fem_object_i, fem_object in enumerate(fem_objects):
        obj = fem_object['Object']
        print("Constraint: " + obj.Name + " --> " + "We have mesh groups. We will search for appropriate group data.")
        fem_object['ShortName'] = get_elset_short_name(obj, fem_object_i)  # unique short identifier
        group_elements = get_femmesh_groupdata_sets_by_name(femmesh, fem_object, 'Volume')  # see comments over there !
        sum_group_elements += group_elements
        count_femelements += len(group_elements)
        fem_object['FEMElements'] = group_elements
    # check if all worked out well
    if not femelements_count_ok(femmesh.VolumeCount, count_femelements):
        FreeCAD.Console.PrintError('Error in get_femelement_sets_from_group_data -- > femelements_count_ok() failed!\n')
        return False
    else:
        return True
    # print("")


def get_elset_short_name(obj, i):
    if hasattr(obj, "Proxy") and obj.Proxy.Type == 'Fem::Material':
        return 'M' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'Fem::FemElementGeometry1D':
        return 'B' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'Fem::FemElementRotation1D':
        return 'R' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'Fem::FemElementFluid1D':
        return 'F' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'Fem::FemElementGeometry2D':
        return 'S' + str(i)
    else:
        FreeCAD.Console.PrintError('Error in creating short elset name for obj: ' + obj.Name + ' --> Proxy.Type: ' + str(obj.Proxy.Type) + '\n')


def get_force_obj_vertex_nodeload_table(femmesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    node_load = frc_obj.Force / len(frc_obj.References)
    for o, elem_tup in frc_obj.References:
        node_count = len(elem_tup)
        for elem in elem_tup:
            ref_node = o.Shape.getElement(elem)
            node = femmesh.getNodesByVertex(ref_node)
            elem_info_string = 'node load on shape: ' + o.Name + ':' + elem
            force_obj_node_load_table.append((elem_info_string, {node[0]: node_load / node_count}))
    return force_obj_node_load_table


def get_force_obj_edge_nodeload_table(femmesh, femelement_table, femnodes_mesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    sum_ref_edge_length = 0
    sum_ref_edge_node_length = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            sum_ref_edge_length += o.Shape.getElement(elem).Length
    if sum_ref_edge_length != 0:
        force_per_sum_ref_edge_length = frc_obj.Force / sum_ref_edge_length
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_edge = o.Shape.getElement(elem)

            # edge_table = { meshedgeID : ( nodeID, ... , nodeID ) }
            edge_table = get_ref_edgenodes_table(femmesh, femelement_table, ref_edge)

            # node_length_table = [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry
            node_length_table = get_ref_edgenodes_lengths(femnodes_mesh, edge_table)

            # node_sum_length_table = { nodeID : Length, ... , nodeID : Length }  LengthSum for each node, one entry for each node
            node_sum_length_table = get_ref_shape_node_sum_geom_table(node_length_table)

            # node_load_table = { nodeID : NodeLoad, ... , nodeID : NodeLoad }  NodeLoad for each node, one entry for each node
            node_load_table = {}
            sum_node_lengths = 0  # for debugging
            for node in node_sum_length_table:
                sum_node_lengths += node_sum_length_table[node]  # for debugging
                node_load_table[node] = node_sum_length_table[node] * force_per_sum_ref_edge_length
            ratio_refedge_lengths = sum_node_lengths / ref_edge.Length
            if ratio_refedge_lengths < 0.99 or ratio_refedge_lengths > 1.01:
                FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
                print('  sum_node_lengths: ', sum_node_lengths)
                print('  refedge_length:  ', ref_edge.Length)
                bad_refedge = ref_edge
            sum_ref_edge_node_length += sum_node_lengths

            elem_info_string = 'node loads on shape: ' + o.Name + ':' + elem
            force_obj_node_load_table.append((elem_info_string, node_load_table))

    for ref_shape in force_obj_node_load_table:
        for node in ref_shape[1]:
            sum_node_load += ref_shape[1][node]  # for debugging

    ratio = sum_node_load / frc_obj.Force
    if ratio < 0.99 or ratio > 1.01:
        print('Deviation  sum_node_load to frc_obj.Force is more than 1% :  ', ratio)
        print('  sum_ref_edge_node_length: ', sum_ref_edge_node_length)
        print('  sum_ref_edge_length:      ', sum_ref_edge_length)
        print('  sum_node_load:          ', sum_node_load)
        print('  frc_obj.Force:          ', frc_obj.Force)
        print('  the reason could be simply a circle length --> see method get_ref_edge_node_lengths')
        print('  the reason could also be a problem in retrieving the ref_edge_node_length')

        # try debugging of the last bad refedge
        print('DEBUGGING')
        print(bad_refedge)

        print('bad_refedge_nodes')
        bad_refedge_nodes = femmesh.getNodesByEdge(bad_refedge)
        print(len(bad_refedge_nodes))
        print(bad_refedge_nodes)
        # import FreeCADGui
        # FreeCADGui.ActiveDocument.Compound_Mesh.HighlightedNodes = bad_refedge_nodes

        print('bad_edge_table')
        # bad_edge_table = { meshedgeID : ( nodeID, ... , nodeID ) }
        bad_edge_table = get_ref_edgenodes_table(femmesh, femelement_table, bad_refedge)
        print(len(bad_edge_table))
        bad_edge_table_nodes = []
        for elem in bad_edge_table:
            print(elem, ' --> ', bad_edge_table[elem])
            for node in bad_edge_table[elem]:
                if node not in bad_edge_table_nodes:
                    bad_edge_table_nodes.append(node)
        print('sorted(bad_edge_table_nodes)')
        print(sorted(bad_edge_table_nodes))   # should be == bad_refedge_nodes
        # import FreeCADGui
        # FreeCADGui.ActiveDocument.Compound_Mesh.HighlightedNodes = bad_edge_table_nodes
        # bad_node_length_table = [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry

        print('good_edge_table')
        good_edge_table = delete_duplicate_mesh_elements(bad_edge_table)
        for elem in good_edge_table:
            print(elem, ' --> ', bad_edge_table[elem])

        print('bad_node_length_table')
        bad_node_length_table = get_ref_edgenodes_lengths(femnodes_mesh, bad_edge_table)
        for n, l in bad_node_length_table:
            print(n, ' --> ', l)

    return force_obj_node_load_table


def get_pressure_obj_faces_depreciated(femmesh, femobj):
    pressure_faces = []
    for o, elem_tup in femobj['Object'].References:
        for elem in elem_tup:
            ref_shape = o.Shape.getElement(elem)
            elem_info_string = 'face load on shape: ' + o.Name + ':' + elem
            print(elem_info_string)
            if ref_shape.ShapeType == 'Face':
                pressure_faces.append((elem_info_string, femmesh.getccxVolumesByFace(ref_shape)))
    return pressure_faces


def get_pressure_obj_faces(femmesh, femelement_table, femnodes_ele_table, femobj):
    if is_solid_femmesh(femmesh):
        # get the nodes
        prs_face_node_set = get_femnodes_by_femobj_with_references(femmesh, femobj)  # sorted and duplicates removed
        # print('prs_face_node_set: ', prs_face_node_set)
        # fill the bit_pattern_dict and search for the faces
        bit_pattern_dict = get_bit_pattern_dict(femelement_table, femnodes_ele_table, prs_face_node_set)
        pressure_faces = get_ccxelement_faces_from_binary_search(bit_pattern_dict)
    elif is_face_femmesh(femmesh):
        pressure_faces = []
        # normally we should call get_femelements_by_references and the group check should be integrated there
        if femmesh.GroupCount:
            meshfaces = get_femmesh_groupdata_sets_by_name(femmesh, femobj, 'Face')
            # print(meshfaces)
            if not meshfaces:
                FreeCAD.Console.PrintError("Error: Something went wrong in getting the group element faces.\n")
            else:
                for mf in meshfaces:
                    # pressure_faces.append([mf, 0])
                    pressure_faces.append([mf, -1])
                    # 0 if femmeshface normal == reference face normal direction
                    # -1 if femmeshface normal opposite reference face normal direction
                    # easy on plane faces, but on a half sphere ... ?!?
        else:
            FreeCAD.Console.PrintError("Pressure on shell mesh at the moment only supported for meshes with appropriate group data.\n")
    return pressure_faces


def get_force_obj_face_nodeload_table(femmesh, femelement_table, femnodes_mesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    sum_ref_face_area = 0
    sum_ref_face_node_area = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            sum_ref_face_area += o.Shape.getElement(elem).Area
    if sum_ref_face_area != 0:
        force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area
    for o, elem_tup in frc_obj.References:
        for elem in elem_tup:
            ref_face = o.Shape.getElement(elem)

            # face_table = { meshfaceID : ( nodeID, ... , nodeID ) }
            face_table = get_ref_facenodes_table(femmesh, femelement_table, ref_face)

            # node_area_table = [ (nodeID, Area), ... , (nodeID, Area) ]  some nodes will have more than one entry
            node_area_table = get_ref_facenodes_areas(femnodes_mesh, face_table)

            # node_sum_area_table = { nodeID : Area, ... , nodeID : Area }  AreaSum for each node, one entry for each node
            node_sum_area_table = get_ref_shape_node_sum_geom_table(node_area_table)

            # node_load_table = { nodeID : NodeLoad, ... , nodeID : NodeLoad }  NodeLoad for each node, one entry for each node
            node_load_table = {}
            sum_node_areas = 0  # for debugging
            for node in node_sum_area_table:
                sum_node_areas += node_sum_area_table[node]  # for debugging
                node_load_table[node] = node_sum_area_table[node] * force_per_sum_ref_face_area
            ratio_refface_areas = sum_node_areas / ref_face.Area
            if ratio_refface_areas < 0.99 or ratio_refface_areas > 1.01:
                FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
                print('  sum_node_areas: ', sum_node_areas)
                print('  ref_face_area:  ', ref_face.Area)
            sum_ref_face_node_area += sum_node_areas

            elem_info_string = 'node loads on shape: ' + o.Name + ':' + elem
            force_obj_node_load_table.append((elem_info_string, node_load_table))

    for ref_shape in force_obj_node_load_table:
        for node in ref_shape[1]:
            sum_node_load += ref_shape[1][node]  # for debugging

    ratio = sum_node_load / frc_obj.Force
    if ratio < 0.99 or ratio > 1.01:
        print('Deviation sum_node_load to frc_obj.Force is more than 1% :  ', ratio)
        print('  sum_ref_face_node_area: ', sum_ref_face_node_area)
        print('  sum_ref_face_area:      ', sum_ref_face_area)
        print('  sum_node_load:          ', sum_node_load)
        print('  frc_obj.Force:          ', frc_obj.Force)
        print('  the reason could be simply a circle area --> see method get_ref_face_node_areas')
        print('  the reason could also be a problem in retrieving the ref_face_node_area')

    return force_obj_node_load_table


def get_ref_edgenodes_table(femmesh, femelement_table, refedge):
    edge_table = {}  # { meshedgeID : ( nodeID, ... , nodeID ) }
    refedge_nodes = femmesh.getNodesByEdge(refedge)
    if is_solid_femmesh(femmesh):
        refedge_fem_volumeelements = []
        # if at least two nodes of a femvolumeelement are in refedge_nodes the volume is added to refedge_fem_volumeelements
        for elem in femelement_table:
            nodecount = 0
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    nodecount += 1
            if nodecount > 1:
                refedge_fem_volumeelements.append(elem)
        # for every refedge_fem_volumeelement look which of its nodes is in refedge_nodes --> add all these nodes to edge_table
        for elem in refedge_fem_volumeelements:
            fe_refedge_nodes = []
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    fe_refedge_nodes.append(node)
                edge_table[elem] = fe_refedge_nodes  # { volumeID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
        # FIXME: duplicate_mesh_elements: as soon as contact and springs are supported the user should decide on which edge the load is applied
        edge_table = delete_duplicate_mesh_elements(edge_table)
    elif is_face_femmesh(femmesh):
        refedge_fem_faceelements = []
        # if at least two nodes of a femfaceelement are in refedge_nodes the volume is added to refedge_fem_volumeelements
        for elem in femelement_table:
            nodecount = 0
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    nodecount += 1
            if nodecount > 1:
                refedge_fem_faceelements.append(elem)
        # for every refedge_fem_faceelement look which of his nodes is in refedge_nodes --> add all these nodes to edge_table
        for elem in refedge_fem_faceelements:
            fe_refedge_nodes = []
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    fe_refedge_nodes.append(node)
                edge_table[elem] = fe_refedge_nodes  # { faceID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
        # FIXME: duplicate_mesh_elements: as soon as contact and springs are supported the user should decide on which edge the load is applied
        edge_table = delete_duplicate_mesh_elements(edge_table)
    elif is_edge_femmesh(femmesh):
        refedge_fem_edgeelements = get_femelements_by_femnodes_std(femelement_table, refedge_nodes)
        for elem in refedge_fem_edgeelements:
            edge_table[elem] = femelement_table[elem]  # { edgeID : ( nodeID, ... , nodeID  )} # all nodes off this femedgeelement
    return edge_table


def get_ref_edgenodes_lengths(femnodes_mesh, edge_table):
    # calculate the appropriate node_length for every node of every mesh edge (me)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff

    #  [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry
    if (not femnodes_mesh) or (not edge_table):
        FreeCAD.Console.PrintError("Error in get_ref_edgenodes_lengths(): Empty femnodes_mesh or edge_table!\n")
        return []
    node_length_table = []
    mesh_edge_length = 0
    # print(len(edge_table))
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
            # print(mesh_edge_length)
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
            # print(me, ' --> ', mesh_edge_length)
            end_node_length = mesh_edge_length / 6.0
            middle_node_length = mesh_edge_length * 2.0 / 3.0
            node_length_table.append((edge_table[me][0], end_node_length))
            node_length_table.append((edge_table[me][1], end_node_length))
            node_length_table.append((edge_table[me][2], middle_node_length))
    return node_length_table


def get_ref_facenodes_table(femmesh, femelement_table, ref_face):
    face_table = {}  # { meshfaceID : ( nodeID, ... , nodeID ) }
    if is_solid_femmesh(femmesh):
        if has_no_face_data(femmesh):
            print('No face date in volume mesh. We try to use getccxVolumesByFace() to retrieve the volume elements of the ref_face!')
            # there is no face data
            # the problem if we retrieve the nodes ourself is they are not sorted we just have the nodes.
            # We need to sort them according the shell mesh notation of tria3, tria6, quad4, quad8
            ref_face_nodes = femmesh.getNodesByFace(ref_face)
            # try to use getccxVolumesByFace() to get the volume ids of element with elementfaces on the ref_face --> should work for tetra4 and tetra10
            ref_face_volume_elements = femmesh.getccxVolumesByFace(ref_face)  # list of tupels (mv, ccx_face_nr)
            if ref_face_volume_elements:  # mesh with tetras
                print('Use of getccxVolumesByFace() has returned volume elements of the ref_face!')
                for ve in ref_face_volume_elements:
                    veID = ve[0]
                    ve_ref_face_nodes = []
                    for nodeID in femelement_table[veID]:
                        if nodeID in ref_face_nodes:
                            ve_ref_face_nodes.append(nodeID)
                    face_table[veID] = ve_ref_face_nodes  # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
            else:  # mesh with hexa or penta
                print('Use of getccxVolumesByFace() has NOT returned volume elements of the ref_face! We try to use get_femvolumeelements_by_femfacenodes()!')
                ref_face_volume_elements = get_femvolumeelements_by_femfacenodes(femelement_table, ref_face_nodes)  # list of integer [mv]
                for veID in ref_face_volume_elements:
                    ve_ref_face_nodes = []
                    for nodeID in femelement_table[veID]:
                        if nodeID in ref_face_nodes:
                            ve_ref_face_nodes.append(nodeID)
                    face_table[veID] = ve_ref_face_nodes  # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
                face_table = build_mesh_faces_of_volume_elements(face_table, femelement_table)  # we need to resort the nodes to make them build an element face
        else:  # the femmesh has face_data
            faces = femmesh.getFacesByFace(ref_face)   # (mv, mf)
            for mf in faces:
                face_table[mf] = femmesh.getElementNodes(mf)
    elif is_face_femmesh(femmesh):
        ref_face_nodes = femmesh.getNodesByFace(ref_face)
        ref_face_elements = get_femelements_by_femnodes_std(femelement_table, ref_face_nodes)
        for mf in ref_face_elements:
            face_table[mf] = femelement_table[mf]
    # print(face_table)
    return face_table


def build_mesh_faces_of_volume_elements(face_table, femelement_table):
    # node index of facenodes in femelementtable volume element
    # if we know the position of the node we can build the element face out of the unsorted face nodes
    face_nodenumber_table = {}  # { volumeID : ( index, ... , index ) }
    for veID in face_table:
        face_nodenumber_table[veID] = []
        for n in face_table[veID]:
            index = femelement_table[veID].index(n)
            # print(index)
            face_nodenumber_table[veID].append(index + 1)  # local node number = index + 1
        # print('VolElement: ', veID)
        # print('  --> ', femelement_table[veID])
        # print('  --> ', face_table[veID])
        # print('  --> ', face_nodenumber_table[veID])
    for veID in face_nodenumber_table:
        vol_node_ct = len(femelement_table[veID])
        face_node_indexs = sorted(face_nodenumber_table[veID])
        if vol_node_ct == 10:  # tetra10 --> tria6 face
            if face_node_indexs == [1, 2, 3, 5, 6, 7]:  # node order of face in tetra10 volume element
                node_numbers = (1, 2, 3, 5, 6, 7)       # node order of a tria6 face of tetra10
            elif face_node_indexs == [1, 2, 4, 5, 8, 9]:
                node_numbers = (1, 4, 2, 8, 9, 5)
            elif face_node_indexs == [1, 3, 4, 7, 8, 10]:
                node_numbers = (1, 3, 4, 7, 10, 8)
            elif face_node_indexs == [2, 3, 4, 6, 9, 10]:
                node_numbers = (2, 4, 3, 9, 10, 6)
            else:
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): hexa20: face not found!" + str(face_node_indexs) + "\n")
        elif vol_node_ct == 4:  # tetra4 --> tria3 face
            if face_node_indexs == [1, 2, 3]:  # node order of face in tetra4 volume element
                node_numbers = (1, 2, 3)       # node order of a tria3 face of tetra4
            elif face_node_indexs == [1, 2, 4]:
                node_numbers = (1, 4, 2, 8)
            elif face_node_indexs == [1, 3, 4]:
                node_numbers = (1, 3, 4)
            elif face_node_indexs == [2, 3, 4]:
                node_numbers = (2, 4, 3)
            else:
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): hexa20: face not found!" + str(face_node_indexs) + "\n")
        elif vol_node_ct == 20:  # hexa20 --> quad8 face
            if face_node_indexs == [1, 2, 3, 4, 9, 10, 11, 12]:  # node order of face in hexa20 volume element
                node_numbers = (1, 2, 3, 4, 9, 10, 11, 12)       # node order of a quad8 face of hexa20
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
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): hexa20: face not found!" + str(face_node_indexs) + "\n")
        elif vol_node_ct == 8:  # hexa8 --> quad4 face
            face_node_indexs = sorted(face_nodenumber_table[veID])
            if face_node_indexs == [1, 2, 3, 4]:  # node order of face in hexa8 volume element
                node_numbers = (1, 2, 3, 4)       # node order of a quad8 face of hexa8
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
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): hexa20: face not found!" + str(face_node_indexs) + "\n")
        elif vol_node_ct == 15:  # penta15 --> tria6 and quad8 faces
            if face_node_indexs == [1, 2, 3, 7, 8, 9]:  # node order of face in penta15 volume element
                node_numbers = (1, 2, 3, 7, 8, 9)       # node order of a tria6 face of penta15
            elif face_node_indexs == [4, 5, 6, 10, 11, 12]:
                node_numbers = (4, 6, 5, 12, 11, 10)  # tria6
            elif face_node_indexs == [1, 2, 4, 5, 7, 10, 13, 14]:
                node_numbers = (1, 4, 5, 2, 13, 10, 14, 7)  # quad8
            elif face_node_indexs == [1, 3, 4, 6, 9, 12, 13, 15]:
                node_numbers = (1, 3, 6, 4, 9, 15, 12, 13)  # quad8
            elif face_node_indexs == [2, 3, 5, 6, 8, 11, 14, 15]:
                node_numbers = (2, 5, 6, 3, 14, 11, 15, 8)  # quad8
            else:
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): penta15: face not found!" + str(face_node_indexs) + "\n")
        elif vol_node_ct == 6:  # penta6 --> tria3 and quad4 faces
            if face_node_indexs == [1, 2, 3]:  # node order of face in penta6 volume element
                node_numbers = (1, 2, 3)       # node order of a tria3 face of penta6
            elif face_node_indexs == [4, 5, 6]:
                node_numbers = (4, 6, 5)  # tria3
            elif face_node_indexs == [1, 2, 4, 5]:
                node_numbers = (1, 4, 5, 2)  # quad4
            elif face_node_indexs == [1, 3, 4, 6]:
                node_numbers = (1, 3, 6, 4)  # quad4
            elif face_node_indexs == [2, 3, 5, 6]:
                node_numbers = (2, 5, 6, 3)  # quad4
            else:
                FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): pent6: face not found!" + str(face_node_indexs) + "\n")
        else:
            FreeCAD.Console.PrintError("Error in build_mesh_faces_of_volume_elements(): Volume not implemented: volume node count" + str(vol_node_ct) + "\n")
        face_nodes = []
        for i in node_numbers:
            i -= 1  # node_number starts with 1, index starts with 0 --> index = node number - 1
            face_nodes.append(femelement_table[veID][i])
        face_table[veID] = face_nodes  # reset the entry in face_table
        # print('  --> ', face_table[veID])
    return face_table


def get_ref_facenodes_areas(femnodes_mesh, face_table):
    # calculate the appropriate node_areas for every node of every mesh face (mf)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff
    # FIXME: only gives exact results in case of a real triangle. If for S6 or C3D10 elements
    # the midnodes are not on the line between the end nodes the area will not be a triangle
    # see http://forum.freecadweb.org/viewtopic.php?f=18&t=10939&start=40#p91355  and ff
    # same applies for the quads, results are exact only if mid nodes are on the line between corner nodes

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
            mesh_face_area = mesh_face_t1_area + mesh_face_t1_area
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
            mesh_face_area = mesh_face_t1_area + mesh_face_t2_area + mesh_face_t3_area + mesh_face_t4_area
            middle_node_area = mesh_face_area / 3.0

            node_area_table.append((face_table[mf][0], 0))
            node_area_table.append((face_table[mf][1], 0))
            node_area_table.append((face_table[mf][2], 0))
            node_area_table.append((face_table[mf][3], middle_node_area))
            node_area_table.append((face_table[mf][4], middle_node_area))
            node_area_table.append((face_table[mf][5], middle_node_area))

        elif femmesh_facetype == 8:  # 8 node femmesh face quad
            # corner_node_area = -mesh_face_area / 12.0  (negativ!)
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
            mesh_face_area = mesh_face_t1_area + mesh_face_t2_area + mesh_face_t3_area + mesh_face_t4_area + mesh_face_t5_area + mesh_face_t6_area
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


def get_ref_shape_node_sum_geom_table(node_geom_table):
    # shape could be Edge or Face, geom could be length or area
    # sum of length or area for each node of the ref_shape
    node_sum_geom_table = {}
    for n, A in node_geom_table:
        # print(n, ' --> ', A)
        if n in node_sum_geom_table:
            node_sum_geom_table[n] = node_sum_geom_table[n] + A
        else:
            node_sum_geom_table[n] = A
    return node_sum_geom_table


def get_mesh_group_elements(mesh_group_obj, aPart):
    '''the Reference shapes of the mesh_group_object are searched in the Shape of aPart.
       If found in shape they are added to a dict
       {MeshGroupIdentifier : ['ShapeType of the Elements'], [ElementID, ElementID, ...], ...}
    '''
    group_elements = {}  # { name : [element, element, ... , element]}
    if mesh_group_obj.References:
        grp_ele = get_reference_group_elements(mesh_group_obj, aPart)
        group_elements[grp_ele[0]] = grp_ele[1]
    else:
        FreeCAD.Console.PrintError('  Empty reference in mesh group object: ' + mesh_group_obj.Name + ' ' + mesh_group_obj.Label)
    return group_elements


def get_analysis_group_elements(aAnalysis, aPart):
    ''' all Reference shapes of all Analysis member are searched in the Shape of aPart.
        If found in shape they are added to a dict
        {ConstraintName : ['ShapeType of the Elements'], [ElementID, ElementID, ...], ...}
    '''
    group_elements = {}  # { name : [element, element, ... , element]}
    empty_references = []
    for m in aAnalysis.Group:
        if hasattr(m, "References") and "ReadOnly" not in m.getEditorMode("References"):
            # some C++ Constraints have a not used References Property, it is set to Hidden in ReadOnly and PropertyEditor
            if m.References:
                grp_ele = get_reference_group_elements(m, aPart)
                group_elements[grp_ele[0]] = grp_ele[1]
            else:
                print('  Empty reference: ' + m.Name)
                empty_references.append(m)
    if empty_references:
        if len(empty_references) == 1:
            group_elements = get_anlysis_empty_references_group_elements(group_elements, aAnalysis, aPart.Shape)
        else:
            FreeCAD.Console.PrintError('Problem: more than one object with empty references.\n')
            print('We are going to try to get the empty material references anyway.\n')
            # FemElementGeometry2D, ElementGeometry1D and FemElementFluid1D could have empty references,
            # but on solid meshes only materials should have empty references
            for er in empty_references:
                print(er.Name)
            group_elements = get_anlysis_empty_references_group_elements(group_elements, aAnalysis, aPart.Shape)
    # check if all groups have at least one element,
    # it doesn't mean ALL reference shapes for a group have been found
    for g in group_elements:
        # print(group_elements[g])
        if len(group_elements[g]) == 0:
            FreeCAD.Console.PrintError('Error: The shapes for the mesh group for the reference shapes of analysis member: ' + g + ' could not be found!\n')
    return group_elements


def get_reference_group_elements(obj, aPart):
    ''' obj is an FEM object which has reference shapes like the group object, the material, most of the constraints
    aPart is geometry feature normally CompSolid, the method searches all reference shapes of obj inside aPart even if
    the reference shapes are a totally different geometry feature.
    a tuple is returned ('Name or Label of the FEMobject', ['Element1', 'Element2', ...])
    The names in the list are the Elements of the geometry aPart whereas 'Solid1' == aPart.Shape.Solids[0]
    !!! It is strongly recommended to use as reference shapes the Solids of a CompSolid an not the Solids the CompSolid is made of !!!
    see https://forum.freecadweb.org/viewtopic.php?f=18&t=12212&p=175777#p175777 and following posts
    Occt might change the Solids a CompSolid is made of during creation of the CompSolid by adding Edges and vertices
    Thus the Elements do not have the same geometry anymore
    '''
    aShape = aPart.Shape
    if hasattr(obj, "UseLabel") and obj.UseLabel:
        key = obj.Label  # TODO: check the character of the Label, only allow underline and standard english character
    else:
        key = obj.Name
    elements = []
    stype = None
    for r in obj.References:
        parent = r[0]
        childs = r[1]
        # print(parent)
        # print(childs)
        for child in childs:
            ref_shape = get_element(parent, child)  # the method getElement(element) does not return Solid elements
            if not stype:
                stype = ref_shape.ShapeType
            elif stype != ref_shape.ShapeType:
                FreeCAD.Console.PrintError('Error, two refshapes in References with different ShapeTypes.\n')
            # print(ref_shape)
            found_element = find_element_in_shape(aShape, ref_shape)
            if found_element is not None:
                elements.append(found_element)
            else:
                FreeCAD.Console.PrintError('Problem: For the geometry of the following shape was no Shape found: ' + str(ref_shape) + '\n')
                print('    ' + obj.Name)
                print('    ' + str(obj.References))
                print('    ' + r[0].Name)
                if parent.Name != aPart.Name:
                    FreeCAD.Console.PrintError('The reference Shape is not a child nor it is the shape the mesh is made of. : ' + str(ref_shape) + '\n')
                    print(aPart.Name + '--> Name of the Feature we where searching in.')
                    print(parent.Name + '--> Name of the parent Feature of reference Shape (Use the same as in the line before and you will have less trouble :-) !!!!!!).')
                    # import Part
                    # Part.show(aShape)
                    # Part.show(ref_shape)
                else:
                    FreeCAD.Console.PrintError('This should not happen, please debug!\n')
                    # in this case we would not have needed to use the is_same_geometry() inside find_element_in_shape()
                    # AFAIK we could have used the Part methods isPartner() or even isSame()
                    # We're going to find out when we need to debug this :-)!
    return (key, sorted(elements))


def get_anlysis_empty_references_group_elements(group_elements, aAnalysis, aShape):
    '''get the elementIDs if the Reference shape is empty
    see get_analysis_group_elements() for more informatations
    on solid meshes only material objects could have an empty reference without being something wrong!
    face meshes could have empty ShellThickness and edge meshes could have empty BeamSection/FluidSection
    '''
    # print(group_elements)
    material_ref_shapes = []
    material_shape_type = ''
    missed_material_refshapes = []
    empty_reference_material = None
    for m in aAnalysis.Group:
        if m.isDerivedFrom("App::MaterialObjectPython"):
            if hasattr(m, "References") and not m.References:
                if not empty_reference_material:
                    empty_reference_material = m.Name
                else:
                    FreeCAD.Console.PrintError('Problem in get_anlysis_empty_references_group_elements, we seem to have two or more materials with empty references')
                    return {}
            elif hasattr(m, "References") and m.References:
                # ShapeType of the group elements, strip the number of the first group element
                # http://stackoverflow.com/questions/12851791/removing-numbers-from-string
                group_shape_type = ''.join(i for i in group_elements[m.Name][0] if not i.isdigit())
                if not material_shape_type:
                    material_shape_type = group_shape_type
                elif material_shape_type != group_shape_type:
                    FreeCAD.Console.PrintError('Problem, material shape type does not match get_anlysis_empty_references_group_elements')
                for ele in group_elements[m.Name]:
                    material_ref_shapes.append(ele)
    if material_shape_type == 'Solid':
        # print(len(aShape.Solids))
        for i in range(len(aShape.Solids)):
            ele = 'Solid' + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    elif material_shape_type == 'Face':
        # print(len(aShape.Faces))
        for i in range(len(aShape.Faces)):
            ele = 'Face' + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    elif material_shape_type == 'Edge':
        # print(len(aShape.Edges))
        for i in range(len(aShape.Edges)):
            ele = 'Edge' + str(i + 1)
            if ele not in material_ref_shapes:
                missed_material_refshapes.append(ele)
    else:
        print('  One material with no reference shapes. No need to make a group for materials.')
        # make no changes group_elements
        return group_elements
    # print(sorted(material_ref_shapes))
    # print(sorted(missed_material_refshapes))
    # print(group_elements)
    group_elements[empty_reference_material] = sorted(missed_material_refshapes)
    # print(group_elements)
    return group_elements


def find_element_in_shape(aShape, anElement):
    # import Part
    ele_st = anElement.ShapeType
    if ele_st == 'Solid' or ele_st == 'CompSolid':
        for index, solid in enumerate(aShape.Solids):
            # print(is_same_geometry(solid, anElement))
            if is_same_geometry(solid, anElement):
                # print(index)
                # Part.show(aShape.Solids[index])
                ele = ele_st + str(index + 1)
                return ele
        FreeCAD.Console.PrintError('Solid ' + str(anElement) + ' not found in: ' + str(aShape) + '\n')
        if ele_st == 'Solid' and aShape.ShapeType == 'Solid':
            messagePart = (
                'We have been searching for a Solid in a Solid and we have not found it. '
                'In most cases this should be searching for a Solid inside a CompSolid. Check the ShapeType of your Part to mesh.'
            )
            print(messagePart)
        # Part.show(anElement)
        # Part.show(aShape)
    elif ele_st == 'Face' or ele_st == 'Shell':
        for index, face in enumerate(aShape.Faces):
            # print(is_same_geometry(face, anElement))
            if is_same_geometry(face, anElement):
                # print(index)
                # Part.show(aShape.Faces[index])
                ele = ele_st + str(index + 1)
                return ele
    elif ele_st == 'Edge' or ele_st == 'Wire':
        for index, edge in enumerate(aShape.Edges):
            # print(is_same_geometry(edge, anElement))
            if is_same_geometry(edge, anElement):
                # print(index)
                # Part.show(aShape.Edges[index])
                ele = ele_st + str(index + 1)
                return ele
    elif ele_st == 'Vertex':
        for index, vertex in enumerate(aShape.Vertexes):
            # print(is_same_geometry(vertex, anElement))
            if is_same_geometry(vertex, anElement):
                # print(index)
                # Part.show(aShape.Vertexes[index])
                ele = ele_st + str(index + 1)
                return ele
    elif ele_st == 'Compound':
        FreeCAD.Console.PrintError('Compound is not supported.\n')


def get_vertexes_by_element(aShape, anElement):
    # we're going to extend the method find_element_in_shape and return the vertexes
    # import Part
    ele_vertexes = []
    ele_st = anElement.ShapeType
    if ele_st == 'Solid' or ele_st == 'CompSolid':
        for index, solid in enumerate(aShape.Solids):
            if is_same_geometry(solid, anElement):
                for vele in aShape.Solids[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # print('  ' + str(sorted(ele_vertexes)))
                return ele_vertexes
        FreeCAD.Console.PrintError('Error, Solid ' + str(anElement) + ' not found in: ' + str(aShape) + '\n')
    elif ele_st == 'Face' or ele_st == 'Shell':
        for index, face in enumerate(aShape.Faces):
            if is_same_geometry(face, anElement):
                for vele in aShape.Faces[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # print('  ' + str(sorted(ele_vertexes)))
                return ele_vertexes
    elif ele_st == 'Edge' or ele_st == 'Wire':
        for index, edge in enumerate(aShape.Edges):
            if is_same_geometry(edge, anElement):
                for vele in aShape.Edges[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # print('  ' + str(sorted(ele_vertexes)))
                return ele_vertexes
    elif ele_st == 'Vertex':
        for index, vertex in enumerate(aShape.Vertexes):
            if is_same_geometry(vertex, anElement):
                ele_vertexes.append(index)
                # print('  ' + str(sorted(ele_vertexes)))
                return ele_vertexes
    elif ele_st == 'Compound':
        FreeCAD.Console.PrintError('Compound is not supported.\n')


def is_same_geometry(shape1, shape2):
    # the vertexes and the CenterOfMass are compared
    # it is a hack, but I do not know any better !
    # check of Volume and Area before starting with the vertices could be added
    # BoundBox is possible too, but is BB calculations robust?!
    # print(shape1)
    # print(shape2)
    same_Vertexes = 0
    if len(shape1.Vertexes) == len(shape2.Vertexes) and len(shape1.Vertexes) > 1:
        # compare CenterOfMass
        if shape1.CenterOfMass != shape2.CenterOfMass:
            return False
        else:
            # compare the Vertexes
            for vs1 in shape1.Vertexes:
                for vs2 in shape2.Vertexes:
                    if vs1.X == vs2.X and vs1.Y == vs2.Y and vs1.Z == vs2.Z:
                        same_Vertexes += 1
                        continue
            # print(same_Vertexes)
            if same_Vertexes == len(shape1.Vertexes):
                return True
            else:
                return False
    if len(shape1.Vertexes) == len(shape2.Vertexes) and len(shape1.Vertexes) == 1:
        vs1 = shape1.Vertexes[0]
        vs2 = shape2.Vertexes[0]
        if vs1.X == vs2.X and vs1.Y == vs2.Y and vs1.Z == vs2.Z:
            return True
        else:
            return False
    else:
        return False


def get_element(part, element):
    if element.startswith('Solid'):
        index = int(element.lstrip('Solid')) - 1
        if index >= len(part.Shape.Solids):
            FreeCAD.Console.PrintError('Index out of range. This Solid does not exist in the Shape!\n')
            return None
        else:
            return part.Shape.Solids[index]  # Solid
    else:
        return part.Shape.getElement(element)  # Face, Edge, Vertex


def femelements_count_ok(len_femelement_table, count_femelements):
    if count_femelements == len_femelement_table:
        print('Count FEM elements as sum of constraints: ', count_femelements)
        print('Count FEM elements of the FreeCAD FEM mesh:  ', len_femelement_table)
        return True
    else:
        print('ERROR: femelement_table != count_femelements')
        print('Count FEM elements as sum of constraints: ', count_femelements)
        print('Count FEM elements of the FreeCAD FEM Mesh:  ', len_femelement_table)
        return False


def delete_duplicate_mesh_elements(refelement_table):
    new_refelement_table = {}  # duplicates deleted
    for elem, nodes in refelement_table.items():
        if sorted(nodes) not in sortlistoflistvalues(new_refelement_table.values()):
            new_refelement_table[elem] = nodes
    return new_refelement_table


def get_triangle_area(P1, P2, P3):
    # import Part
    # W = Part.Wire([Part.makeLine(P1,P2), Part.makeLine(P2,P3), Part.makeLine(P3,P1)])
    # Part.show(Part.Face(W))
    vec1 = P2 - P1
    vec2 = P3 - P1
    vec3 = vec1.cross(vec2)
    return 0.5 * vec3.Length


def sortlistoflistvalues(listoflists):
    new_list = []
    for l in listoflists:
        new_list.append(sorted(l))
    return new_list


def is_solid_femmesh(femmesh):
    if femmesh.VolumeCount > 0:  # solid femmesh
        return True


def has_no_face_data(femmesh):
    if femmesh.FaceCount == 0:   # femmesh has no face data, could be a edge femmesh or a solid femmesh without face data
        return True


def is_face_femmesh(femmesh):
    if femmesh.VolumeCount == 0 and femmesh.FaceCount > 0:  # face femmesh
        return True


def is_edge_femmesh(femmesh):
    if femmesh.VolumeCount == 0 and femmesh.FaceCount == 0 and femmesh.EdgeCount > 0:  # edge femmesh
        return True


def get_three_non_colinear_nodes(nodes_coords):
    # Code to obtain three non-colinear nodes on the PlaneRotation support face
    # nodes_coords --> [(nodenumber, x, y, z), (nodenumber, x, y, z), ...]
    if not nodes_coords:
        print(len(nodes_coords))
        print('Error: No nodes in nodes_coords')
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
    print([node_1, node_2, node_3])
    return [node_1, node_2, node_3]


def get_rectangular_coords(obj):
    from math import cos, sin, radians
    A = [1, 0, 0]
    B = [0, 1, 0]
    a_x = A[0]
    a_y = A[1]
    a_z = A[2]
    b_x = A[0]
    b_y = A[1]
    b_z = A[2]
    x_rot = radians(obj.X_rot)
    y_rot = radians(obj.Y_rot)
    z_rot = radians(obj.Z_rot)
    if obj.X_rot != 0:
        a_x = A[0]
        a_y = A[1] * cos(x_rot) + A[2] * sin(x_rot)
        a_z = A[2] * cos(x_rot) - A[1] * sin(x_rot)
        b_x = B[0]
        b_y = B[1] * cos(x_rot) + B[2] * sin(x_rot)
        b_z = B[2] * cos(x_rot) - B[1] * sin(x_rot)
        A = [a_x, a_y, a_z]
        B = [b_x, b_y, b_z]
    if obj.Y_rot != 0:
        a_x = A[0] * cos(y_rot) - A[2] * sin(y_rot)
        a_y = A[1]
        a_z = A[2] * cos(y_rot) + A[0] * sin(y_rot)
        b_x = B[0] * cos(y_rot) - B[2] * sin(y_rot)
        b_y = B[1]
        b_z = B[2] * cos(y_rot) + B[0] * sin(z_rot)
        A = [a_x, a_y, a_z]
        B = [b_x, b_y, b_z]
    if obj.Z_rot != 0:
        a_x = A[0] * cos(z_rot) + A[1] * sin(z_rot)
        a_y = A[1] * cos(z_rot) - A[0] * sin(z_rot)
        a_z = A[2]
        b_x = B[0] * cos(z_rot) + B[1] * sin(z_rot)
        b_y = B[1] * cos(z_rot) - B[0] * sin(z_rot)
        b_z = B[2]
        A = [a_x, a_y, a_z]
        B = [b_x, b_y, b_z]

    A_coords = str(round(A[0], 4)) + ',' + str(round(A[1], 4)) + ',' + str(round(A[2], 4))
    B_coords = str(round(B[0], 4)) + ',' + str(round(B[1], 4)) + ',' + str(round(B[2], 4))
    coords = A_coords + ',' + B_coords
    return coords


def get_cylindrical_coords(obj):
    vec = obj.Axis
    base = obj.BasePoint
    Ax = base[0] + 10 * vec[0]
    Ay = base[1] + 10 * vec[1]
    Az = base[2] + 10 * vec[2]
    Bx = base[0] - 10 * vec[0]
    By = base[1] - 10 * vec[1]
    Bz = base[2] - 10 * vec[2]
    A = [Ax, Ay, Az]
    B = [Bx, By, Bz]
    A_coords = str(A[0]) + ',' + str(A[1]) + ',' + str(A[2])
    B_coords = str(B[0]) + ',' + str(B[1]) + ',' + str(B[2])
    coords = A_coords + ',' + B_coords
    return coords


def write_D_network_element_to_inputfile(fileName):
    # replace B32 elements with D elements for fluid section
    f = open(fileName, 'r+')
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


def use_correct_fluidinout_ele_def(FluidInletoutlet_ele, fileName, fluid_inout_nodes_file):
    f = open(fileName, 'r')
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
        ind = line.find(',')
        elem = line[0:ind]
        for i in range(len(FluidInletoutlet_ele)):
            if (elem == FluidInletoutlet_ele[i][0]):
                FluidInletoutlet_ele[i][2] = cnt
        line = f.readline()
        cnt = cnt + 1
    f.close()

    # re-define elements for INLET and OUTLET
    f = open(fileName, 'r+')
    lines = f.readlines()
    f.seek(0)
    cnt = 0
    elem_counter = 0
    print('1DFlow inout nodes file: ' + fluid_inout_nodes_file + '\n')
    inout_nodes_file = open(fluid_inout_nodes_file, "w")
    for line in lines:
        new_line = ''
        for i in range(len(FluidInletoutlet_ele)):
            if (cnt == FluidInletoutlet_ele[i][2]):
                elem_counter = elem_counter + 1
                a = line.split(',')
                for j in range(len(a)):
                    if elem_counter == 1:
                        if j == 1:
                            new_line = new_line + ' 0,'
                            node1 = int(a[j + 2])
                            node2 = int(a[j + 1])
                            node3 = int(a[j])
                            inout_nodes_file.write(str(node1) + ',' + str(node2) + ',' + str(node3) + ',' + FluidInletoutlet_ele[i][1] + '\n')
                        elif j == 3:
                            new_line = new_line + a[j]
                        else:
                            new_line = new_line + a[j] + ','
                    else:
                        if j == 3:
                            new_line = new_line + ' 0\n'
                            node1 = int(a[j - 2])
                            node2 = int(a[j - 1])
                            node3 = int(a[j])
                            inout_nodes_file.write(str(node1) + ',' + str(node2) + ',' + str(node3) + ',' + FluidInletoutlet_ele[i][1] + '\n')
                        else:
                            new_line = new_line + a[j] + ','
        if new_line == '':
            f.write(line)
        else:
            f.write(new_line)
        cnt = cnt + 1
    f.truncate()
    f.close()
    inout_nodes_file.close()

##  @}
