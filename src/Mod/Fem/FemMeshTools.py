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


import FreeCAD


def get_femelements_by_references(femmesh, femelement_table, references):
    '''get the femelements for a list of references
    '''
    references_femelements = []
    for ref in references:
        ref_femnodes = get_femnodes_by_refshape(femmesh, ref)  # femnodes for the current ref
        references_femelements += get_femelements_by_femnodes(femelement_table, ref_femnodes)  # femelements for all references
    return references_femelements


def get_femnodes_by_references(femmesh, references):
    '''get the femnodes for a list of references
    '''
    references_femnodes = []
    for ref in references:
        references_femnodes += get_femnodes_by_refshape(femmesh, ref)

    # return references_femnodes  # keeps duplicate nodes, keeps node order

    # if nodes are used for nodesets, duplicats should be removed
    return list(set(references_femnodes))  # removes duplicate nodes, sortes node order


def get_femnodes_by_refshape(femmesh, ref):
    if ref[1]:
        r = ref[0].Shape.getElement(ref[1])  # Vertex, Edge, Face
    else:
        r = ref[0].Shape  # solid
    # print('  ReferenceShape : ', r.ShapeType, ', ', ref[0].Name, ', ', ref[0].Label, ' --> ', ref[1])
    if r.ShapeType == 'Vertex':
        nodes = femmesh.getNodesByVertex(r)
    elif r.ShapeType == 'Edge':
        nodes = femmesh.getNodesByEdge(r)
    elif r.ShapeType == 'Face':
        nodes = femmesh.getNodesByFace(r)
    elif r.ShapeType == 'Solid':
        nodes = femmesh.getNodesBySolid(r)
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


def get_femelements_by_femnodes(femelement_table, node_list):
    '''for every femelement of femelement_table
    if all nodes of the femelement are in node_list,
    the femelement is added to the list which is returned
    e: elementlist
    nodes: nodelist '''
    e = []  # elementlist
    for elementID in sorted(femelement_table):
        nodecount = 0
        for nodeID in femelement_table[elementID]:
            if nodeID in node_list:
                nodecount = nodecount + 1
        if nodecount == len(femelement_table[elementID]):   # all nodes of the element are in the node_list!
            e.append(elementID)
    return e


def get_femelement_sets(femmesh, femelement_table, fem_objects):  # fem_objects = FreeCAD FEM document objects
    # get femelements for reference shapes of each obj.References
    count_femelements = 0
    referenced_femelements = []
    has_remaining_femelements = None
    for fem_object_i, fem_object in enumerate(fem_objects):
        obj = fem_object['Object']
        fem_object['ShortName'] = get_elset_short_name(obj, fem_object_i)  # unique short identifier
        if obj.References:
            ref_shape_femelements = []
            ref_shape_femelements = get_femelements_by_references(femmesh, femelement_table, obj.References)
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
    if not femelements_count_ok(femelement_table, count_femelements):
        FreeCAD.Console.PrintError('Error in get_femelement_sets -- > femelements_count_ok() failed!\n')


def get_elset_short_name(obj, i):
    if hasattr(obj, "Proxy") and obj.Proxy.Type == 'MechanicalMaterial':
        return 'Mat' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'FemBeamSection':
        return 'Beam' + str(i)
    elif hasattr(obj, "Proxy") and obj.Proxy.Type == 'FemShellThickness':
        return 'Shell' + str(i)
    else:
        print('Error: ', obj.Name, ' --> ', obj.Proxy.Type)


def get_force_obj_vertex_nodeload_table(femmesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    node_load = frc_obj.Force / len(frc_obj.References)
    for o, elem in frc_obj.References:
        elem_o = o.Shape.getElement(elem)
        node = femmesh.getNodesByVertex(elem_o)
        elem_info_string = 'node load on shape: ' + o.Name + ':' + elem
        force_obj_node_load_table.append((elem_info_string, {node[0]: node_load}))

    return force_obj_node_load_table


def get_force_obj_edge_nodeload_table(femmesh, femelement_table, femnodes_mesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    sum_ref_edge_length = 0
    sum_ref_edge_node_length = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem in frc_obj.References:
        elem_o = o.Shape.getElement(elem)
        sum_ref_edge_length += elem_o.Length
    if sum_ref_edge_length != 0:
        force_per_sum_ref_edge_length = frc_obj.Force / sum_ref_edge_length
    for o, elem in frc_obj.References:
        elem_o = o.Shape.getElement(elem)
        ref_edge = elem_o

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
        ratio_refedge_lengths = sum_node_lengths / elem_o.Length
        if ratio_refedge_lengths < 0.99 or ratio_refedge_lengths > 1.01:
            FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
            print('  sum_node_lengths:', sum_node_lengths)
            print('  refedge_length:  ', elem_o.Length)
            bad_refedge = elem_o
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
        print('  the reason could also be an problem in retrieving the ref_edge_node_length')

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


def get_force_obj_face_nodeload_table(femmesh, femelement_table, femnodes_mesh, frc_obj):
    # force_obj_node_load_table = [('refshape_name.elemname',node_load_table), ..., ('refshape_name.elemname',node_load_table)]
    force_obj_node_load_table = []
    sum_ref_face_area = 0
    sum_ref_face_node_area = 0  # for debugging
    sum_node_load = 0  # for debugging
    for o, elem in frc_obj.References:
        elem_o = o.Shape.getElement(elem)
        sum_ref_face_area += elem_o.Area
    if sum_ref_face_area != 0:
        force_per_sum_ref_face_area = frc_obj.Force / sum_ref_face_area
    for o, elem in frc_obj.References:
        elem_o = o.Shape.getElement(elem)
        ref_face = elem_o

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
        ratio_refface_areas = sum_node_areas / elem_o.Area
        if ratio_refface_areas < 0.99 or ratio_refface_areas > 1.01:
            FreeCAD.Console.PrintError('Error on: ' + frc_obj.Name + ' --> ' + o.Name + '.' + elem + '\n')
            print('  sum_node_lengths:', sum_node_areas)
            print('  refedge_length:  ', elem_o.Area)
        sum_ref_face_node_area += sum_node_areas

        elem_info_string = 'node loads on shape: ' + o.Name + ':' + elem
        force_obj_node_load_table.append((elem_info_string, node_load_table))

    for ref_shape in force_obj_node_load_table:
        for node in ref_shape[1]:
            sum_node_load += ref_shape[1][node]  # for debugging

    ratio = sum_node_load / frc_obj.Force
    if ratio < 0.99 or ratio > 1.01:
        print('Deviation  sum_node_load to frc_obj.Force is more than 1% :  ', ratio)
        print('  sum_ref_face_node_area: ', sum_ref_face_node_area)
        print('  sum_ref_face_area:      ', sum_ref_face_area)
        print('  sum_node_load:          ', sum_node_load)
        print('  frc_obj.Force:          ', frc_obj.Force)
        print('  the reason could be simply a circle area --> see method get_ref_face_node_areas')
        print('  the reason could also be an problem in retrieving the ref_face_node_area')

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
        # for every refedge_fem_volumeelement look which of his nodes is in refedge_nodes --> add all these nodes to edge_table
        for elem in refedge_fem_volumeelements:
            fe_refedge_nodes = []
            for node in femelement_table[elem]:
                if node in refedge_nodes:
                    fe_refedge_nodes.append(node)
                edge_table[elem] = fe_refedge_nodes  # { volumeID : ( edgenodeID, ... , edgenodeID  )} # only the refedge nodes
        #  FIXME duplicate_mesh_elements: as soon as contact ans springs are supported the user should decide on which edge the load is applied
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
        #  FIXME duplicate_mesh_elements: as soon as contact ans springs are supported the user should decide on which edge the load is applied
        edge_table = delete_duplicate_mesh_elements(edge_table)
    elif is_edge_femmesh(femmesh):
        refedge_fem_edgeelements = get_femelements_by_femnodes(femelement_table, refedge_nodes)
        for elem in refedge_fem_edgeelements:
            edge_table[elem] = femelement_table[elem]  # { edgeID : ( nodeID, ... , nodeID  )} # all nodes off this femedgeelement
    return edge_table


def get_ref_edgenodes_lengths(femnodes_mesh, edge_table):
    # calulate the appropriate node_length for every node of every mesh edge (me)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff

    #  [ (nodeID, length), ... , (nodeID, length) ]  some nodes will have more than one entry
    if (not femnodes_mesh) or (not edge_table):
        print('Error: empty femnodes_mesh or edge_table')
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
            # middle_node_length = mesh_face_area * 2 / 3
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
            # there is no face data, the volumeID is used as key { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
            ref_face_volume_elements = femmesh.getccxVolumesByFace(ref_face)  # list of tupels (mv, ccx_face_nr)
            ref_face_nodes = femmesh.getNodesByFace(ref_face)
            for ve in ref_face_volume_elements:
                veID = ve[0]
                ve_ref_face_nodes = []
                for nodeID in femelement_table[veID]:
                    if nodeID in ref_face_nodes:
                        ve_ref_face_nodes.append(nodeID)
                face_table[veID] = ve_ref_face_nodes  # { volumeID : ( facenodeID, ... , facenodeID ) } only the ref_face nodes
        else:  # the femmesh has face_data
            faces = femmesh.getFacesByFace(ref_face)   # (mv, mf)
            for mf in faces:
                face_table[mf] = femmesh.getElementNodes(mf)
    elif is_face_femmesh(femmesh):
        ref_face_nodes = femmesh.getNodesByFace(ref_face)
        ref_face_elements = get_femelements_by_femnodes(femelement_table, ref_face_nodes)
        for mf in ref_face_elements:
            face_table[mf] = femelement_table[mf]
    return face_table


def get_ref_facenodes_areas(femnodes_mesh, face_table):
    # calulate the appropriate node_areas for every node of every mesh face (mf)
    # G. Lakshmi Narasaiah, Finite Element Analysis, p206ff
    # FIXME only gives exact results in case of a real triangle. If for S6 or C3D10 elements
    # the midnodes are not on the line between the end nodes the area will not be a triangle
    # see http://forum.freecadweb.org/viewtopic.php?f=18&t=10939&start=40#p91355  and ff

    #  [ (nodeID,Area), ... , (nodeID,Area) ]  some nodes will have more than one entry
    if (not femnodes_mesh) or (not face_table):
        print('Error: empty femnodes_mesh or face_table')
        return []
    node_area_table = []
    mesh_face_area = 0
    for mf in face_table:
        femmesh_facetype = len(face_table[mf])
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
            FreeCAD.Console.PrintError('Face load on 4 node quad faces are not supported\n')

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
            FreeCAD.Console.PrintError('Face load on 8 node quad faces are not supported\n')
    return node_area_table


def get_ref_shape_node_sum_geom_table(node_geom_table):
    # shape could be Edge or Face, geom could be lenght or area
    # summ of legth or area for each node of the ref_shape
    node_sum_geom_table = {}
    for n, A in node_geom_table:
        # print(n, ' --> ', A)
        if n in node_sum_geom_table:
            node_sum_geom_table[n] = node_sum_geom_table[n] + A
        else:
            node_sum_geom_table[n] = A
    return node_sum_geom_table


def femelements_count_ok(femelement_table, count_femelements):
    if count_femelements == len(femelement_table):
        # print('Count FEM elements for the calculated node load distribution: ', count_femelements)
        # print('Count FEM elements of the FreeCAD FEM mesh:  ', len(femelement_table))
        return True
    else:
        print('ERROR: femelement_table != count_femelements')
        print('Count FEM elements for the calculated node load distribution: ', count_femelements)
        print('Count FEM Elements of the FreeCAD FEM Mesh:  ', len(femelement_table))
        return False


def delete_duplicate_mesh_elements(refelement_table):
    new_refelement_table = {}  # duplicates deleted
    for elem, nodes in refelement_table.items():
        if sorted(nodes) not in sortlistoflistvalues(new_refelement_table.values()):
            new_refelement_table[elem] = nodes
    return new_refelement_table


def get_triangle_area(P1, P2, P3):
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


def make_femmesh(mesh_data):
    ''' makes an FreeCAD FEM Mesh object from FEM Mesh data
    '''
    import Fem
    mesh = Fem.FemMesh()
    m = mesh_data
    if ('Nodes' in m) and (len(m['Nodes']) > 0):
        if (('Hexa8Elem' in m) or ('Penta6Elem' in m) or ('Tetra4Elem' in m) or ('Tetra10Elem' in m) or
           ('Penta6Elem' in m) or ('Hexa20Elem' in m) or ('Tria3Elem' in m) or ('Tria6Elem' in m) or
           ('Quad4Elem' in m) or ('Quad8Elem' in m) or ('Seg2Elem' in m)):
            nds = m['Nodes']

            for i in nds:
                n = nds[i]
                mesh.addNode(n[0], n[1], n[2], i)
            elms_hexa8 = m['Hexa8Elem']
            for i in elms_hexa8:
                e = elms_hexa8[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_penta6 = m['Penta6Elem']
            for i in elms_penta6:
                e = elms_penta6[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_tetra4 = m['Tetra4Elem']
            for i in elms_tetra4:
                e = elms_tetra4[i]
                mesh.addVolume([e[0], e[1], e[2], e[3]], i)
            elms_tetra10 = m['Tetra10Elem']
            for i in elms_tetra10:
                e = elms_tetra10[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9]], i)
            elms_penta15 = m['Penta15Elem']
            for i in elms_penta15:
                e = elms_penta15[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14]], i)
            elms_hexa20 = m['Hexa20Elem']
            for i in elms_hexa20:
                e = elms_hexa20[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14], e[15], e[16], e[17], e[18], e[19]], i)
            elms_tria3 = m['Tria3Elem']
            for i in elms_tria3:
                e = elms_tria3[i]
                mesh.addFace([e[0], e[1], e[2]], i)
            elms_tria6 = m['Tria6Elem']
            for i in elms_tria6:
                e = elms_tria6[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_quad4 = m['Quad4Elem']
            for i in elms_quad4:
                e = elms_quad4[i]
                mesh.addFace([e[0], e[1], e[2], e[3]], i)
            elms_quad8 = m['Quad8Elem']
            for i in elms_quad8:
                e = elms_quad8[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_seg2 = m['Seg2Elem']
            for i in elms_seg2:
                e = elms_seg2[i]
                mesh.addEdge(e[0], e[1])
            print("imported mesh: {} nodes, {} HEXA8, {} PENTA6, {} TETRA4, {} TETRA10, {} PENTA15".format(
                  len(nds), len(elms_hexa8), len(elms_penta6), len(elms_tetra4), len(elms_tetra10), len(elms_penta15)))
            print("imported mesh: {} HEXA20, {} TRIA3, {} TRIA6, {} QUAD4, {} QUAD8, {} SEG2".format(
                  len(elms_hexa20), len(elms_tria3), len(elms_tria6), len(elms_quad4), len(elms_quad8), len(elms_seg2)))
    else:
        FreeCAD.Console.PrintError("No Nodes found!\n")
    return mesh
