#/***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

__title__="BOPTools.ShapeMerge module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "Tools for merging shapes with shared elements. Useful for final processing of results of Part.Shape.generalFuse()."

import Part
from .Utils import HashableShape

def findSharedElements(shape_list, element_extractor):
    if len(shape_list) < 2:
        raise ValueError("findSharedElements: at least two shapes must be provided (have {num})".format(num= len(shape_list)))

    all_elements = [] #list of sets of HashableShapes
    for shape in shape_list:
        all_elements.append(set(
            [HashableShape(sh) for sh in element_extractor(shape)]
          ))
    shared_elements = None
    for elements in all_elements:
        if shared_elements is None:
            shared_elements = elements
        else:
            shared_elements.intersection_update(elements)
    return [el.Shape for el in shared_elements]

def isConnected(shape1, shape2, shape_dim = -1):
    if shape_dim == -1:
        shape_dim = dimensionOfShapes([shape1, shape2])
    extractor = {0: None,
                 1: (lambda sh: sh.Vertexes),
                 2: (lambda sh: sh.Edges),
                 3: (lambda sh: sh.Faces)    }[shape_dim]
    return len(findSharedElements([shape1, shape2], extractor))>0

def splitIntoGroupsBySharing(list_of_shapes, element_extractor, split_connections = []):
    """splitIntoGroupsBySharing(list_of_shapes, element_type, split_connections = []): find,
    which shapes in list_of_shapes are connected into groups by sharing elements.

    element_extractor: function that takes shape as input, and returns list of shapes.

    split_connections: list of shapes to exclude when testing for connections. Use to
    split groups on purpose.

    return: list of lists of shapes. Top-level list is list of groups; bottom level lists
    enumerate shapes of a group."""

    split_connections = set([HashableShape(element) for element in split_connections])

    groups = [] #list of tuples (shapes,elements). Shapes is a list of plain shapes. Elements is a set of HashableShapes - all elements of shapes in the group, excluding split_connections.

    # add shapes to the list of groups, one by one. If not connected to existing groups,
    # new group is created. If connected, shape is added to groups, and the groups are joined.
    for shape in list_of_shapes:
        shape_elements = set([HashableShape(element) for element in element_extractor(shape)])
        shape_elements.difference_update(split_connections)
        #search if shape is connected to any groups
        connected_to = []
        not_in_connected_to = []
        for iGroup in range(len(groups)):
            connected = False
            for element in shape_elements:
                if element in groups[iGroup][1]:
                    connected_to.append(iGroup)
                    connected = True
                    break
            else:
                # `break` not invoked, so `connected` is false
                not_in_connected_to.append(iGroup)

        # test if we need to join groups
        if len(connected_to)>1:
            #shape bridges a gap between some groups. Join them into one.
            #rebuilding list of groups. First, add the new "supergroup", then add the rest
            groups_new = []

            supergroup = (list(),set())
            for iGroup in connected_to:
                supergroup[0].extend( groups[iGroup][0] )# merge lists of shapes
                supergroup[1].update( groups[iGroup][1] )# merge lists of elements
            groups_new.append(supergroup)

            l_groups = len(groups)
            groups_new.extend([groups[i_group] \
                               for i_group in not_in_connected_to \
                               if i_group < l_groups])
            groups = groups_new
            connected_to = [0]

        # add shape to the group it is connected to (if to many, the groups should have been unified by the above code snippet)
        if len(connected_to) > 0:
            iGroup = connected_to[0]
            groups[iGroup][0].append(shape)
            groups[iGroup][1].update( shape_elements )
        else:
            newgroup = ([shape], shape_elements)
            groups.append(newgroup)

    # done. Discard unnecessary data and return result.
    return [shapes for shapes,elements in groups]

def mergeSolids(list_of_solids_compsolids, flag_single = False, split_connections = [], bool_compsolid = False):
    """mergeSolids(list_of_solids, flag_single = False): merges touching solids that share
    faces. If flag_single is True, it is assumed that all solids touch, and output is a
    single solid. If flag_single is False, the output is a compound containing all
    resulting solids.

    Note. CompSolids are treated as lists of solids - i.e., merged into solids."""

    solids = []
    for sh in list_of_solids_compsolids:
        solids.extend(sh.Solids)
    if flag_single:
        cs = Part.CompSolid(solids)
        return cs if bool_compsolid else Part.makeSolid(cs)
    else:
        if len(solids)==0:
            return Part.Compound([])
        groups = splitIntoGroupsBySharing(solids, lambda sh: sh.Faces, split_connections)
        if bool_compsolid:
            merged_solids = [Part.CompSolid(group) for group in groups]
        else:
            merged_solids = [Part.makeSolid(Part.CompSolid(group)) for group in groups]
        return Part.makeCompound(merged_solids)

def mergeShells(list_of_faces_shells, flag_single = False, split_connections = []):
    faces = []
    for sh in list_of_faces_shells:
        faces.extend(sh.Faces)
    if flag_single:
        return Part.makeShell(faces)
    else:
        groups = splitIntoGroupsBySharing(faces, lambda sh: sh.Edges, split_connections)
        return Part.makeCompound([Part.Shell(group) for group in groups])

def mergeWires(list_of_edges_wires, flag_single = False, split_connections = []):
    edges = []
    for sh in list_of_edges_wires:
        edges.extend(sh.Edges)
    if flag_single:
        return Part.Wire(edges)
    else:
        groups = splitIntoGroupsBySharing(edges, lambda sh: sh.Vertexes, split_connections)
        return Part.makeCompound([Part.Wire(Part.sortEdges(group)[0]) for group in groups])

def mergeVertices(list_of_vertices, flag_single = False, split_connections = []):
    # no comprehensive support, just following the footprint of other mergeXXX()
    return Part.makeCompound(removeDuplicates(list_of_vertices))

def mergeShapes(list_of_shapes, flag_single = False, split_connections = [], bool_compsolid = False):
    """mergeShapes(list_of_shapes, flag_single = False, split_connections = [], bool_compsolid = False):
    merges list of edges/wires into wires, faces/shells into shells, solids/compsolids
    into solids or compsolids.

    list_of_shapes: shapes to merge. Shapes must share elements in order to be merged.

    flag_single: assume all shapes in list are connected. If False, return is a compound.
    If True, return is the single piece (e.g. a shell).

    split_connections: list of shapes that are excluded when searching for connections.
    This can be used for example to split a wire in two by supplying vertices where to
    split. If flag_single is True, this argument is ignored.

    bool_compsolid: determines behavior when dealing with solids/compsolids. If True,
    result is compsolid/compound of compsolids. If False, all touching solids and
    compsolids are unified into single solids. If not merging solids/compsolids, this
    argument is ignored."""

    if len(list_of_shapes)==0:
        return Part.Compound([])
    args = [list_of_shapes, flag_single, split_connections]
    dim = dimensionOfShapes(list_of_shapes)
    if dim == 0:
        return mergeVertices(*args)
    elif dim == 1:
        return mergeWires(*args)
    elif dim == 2:
        return mergeShells(*args)
    elif dim == 3:
        args.append(bool_compsolid)
        return mergeSolids(*args)
    else:
        assert(dim >= 0 and dim <= 3)

def removeDuplicates(list_of_shapes):
    hashes = set()
    new_list = []
    for sh in list_of_shapes:
        hash = HashableShape(sh)
        if hash in hashes:
            pass
        else:
            new_list.append(sh)
            hashes.add(hash)
    return new_list

def dimensionOfShapes(list_of_shapes):
    """dimensionOfShapes(list_of_shapes): returns dimension (0D, 1D, 2D, or 3D) of shapes
    in the list. If dimension of shapes varies, TypeError is raised."""

    dimensions = [["Vertex"], ["Edge","Wire"], ["Face","Shell"], ["Solid","CompSolid"]]
    dim = -1
    for sh in list_of_shapes:
        sht = sh.ShapeType
        for iDim in range(len(dimensions)):
            if sht in dimensions[iDim]:
                if dim == -1:
                    dim = iDim
                if iDim != dim:
                    raise TypeError("Shapes are of different dimensions ({t1} and {t2}), and cannot be merged or compared.".format(t1= list_of_shapes[0].ShapeType, t2= sht))
    return dim
