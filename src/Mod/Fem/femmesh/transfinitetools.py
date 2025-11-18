# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "FreeCAD transfinite automation functions"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package transfinitetools
#  \ingroup FEM
#  \brief Tools to help automating application of gmsh transfinite meshes

import FreeCAD
import Part

from enum import Enum, auto
from dataclasses import dataclass, field

@dataclass(frozen=True)
class TFCurveDefinition:
    Nodes: int = 2                  # number of nodes
    Coefficient: float = 1.2        # the distribution coeficcient
    Distribution: str = "Constant"  # the curve type
    Invert: bool = False            # invert direction?

    def tag_prefix(self):
        if self.Invert and (self.Distribution == "Progression"):
            return "-"

        return ""

    def to_gmshtools_setting(self):

        settings = {}
        coef = self.Coefficient

        if self.Invert and (self.Distribution == "Bump"):
                coef = 1.0/coef

        settings["numNodes"] = self.Nodes
        if self.Distribution != "Constant":
            settings["meshType"] = self.Distribution
            settings["coef"] = coef

        return settings

    @staticmethod
    def from_tfcurve_obj(obj):
        return TFCurveDefinition(obj.Nodes, obj.Coefficient,
                                 obj.Distribution, obj.Invert)


class Creation(Enum):
    Undefined = auto()
    User      = auto()
    Automatic = auto()

@dataclass
class EdgeData:

    Creation: Creation          = Creation.Undefined
    Data:     TFCurveDefinition = field(default_factory=TFCurveDefinition)


    @property
    def IsDefined(self):
        return self.Creation != Creation.Undefined

    @property
    def IsAutomatic(self):
        return self.Creation == Creation.Automatic

@dataclass
class EdgeKey:

    Edge: object

    def __eq__(self, other):
        return self.Edge.isSame(other.Edge)

    def __hash__(self):
        return self.Edge.hashCode()


def _get_common_faces(face_list_1, face_list_2):

    result = []
    for face1 in face_list_1:
        for face2 in face_list_2:
            if face1.isSame(face2):
               result.append(face1)

    return result

def _get_opposing_edge(face, edge):
    # Within face, find the edge that is opposite of the given one for transfinite meshing

    match len(face.Edges):
        case 3:
            # actually both other edges would work, for now we just use one without much thougth
            for candidate in face.Edges:
                if candidate.isSame(edge):
                    continue

                return candidate

        case 4:
            # find the edge that does not share a vertex.
            for candidate in face.Edges:
                if candidate.isSame(edge):
                    continue

                if (candidate.Vertexes[0].isSame(edge.Vertexes[0]) or
                    candidate.Vertexes[1].isSame(edge.Vertexes[0]) or
                    candidate.Vertexes[0].isSame(edge.Vertexes[1]) or
                    candidate.Vertexes[1].isSame(edge.Vertexes[1])):

                    continue

                return candidate

        case _:
            raise Exception("Only 3 or 4 sided faces can be automated")


    # if we are here something went terrible wrong, that should not happen
    raise Exception("Could not find opposing edge")


def _propagate_edge(edges_map, shape, faces, edge_key, origin_face):
    # Propagates the edge values through the map
    #
    # edges_map:    The map of all edge keys to edge data
    # shape:        The shape all faces and edges belong
    # faces:        All faces that should be made transfinite
    # edge_key:     The edge key which shal be propagaed
    # origin_face:  The face the propagated edge was itself propagated from (to not go backwards)

    # 1. Get all faces the edge belongs to (from user provided list)
    edge_faces = shape.ancestorsOfType(edge_key.Edge, Part.Face)
    propagate_faces = _get_common_faces(edge_faces, faces)

    # 3. Find opposing edges and apply value if not set yet
    for face in propagate_faces:

        # do not go backward
        if origin_face and face.isSame(origin_face):
            continue

        opposite = EdgeKey(_get_opposing_edge(face, edge_key.Edge))
        if edges_map[opposite].IsDefined:
            # check if compatibel, and raise error if not
            if edges_map[opposite].Data.Nodes != edges_map[edge_key].Data.Nodes:
                raise Exception("Transfinite curve data is inconsitent, cannot apply automatic algorithm")

            # and go on to the next face
            continue

        else:
            # transfer the data as automatic definition
            edges_map[opposite].Data = edges_map[edge_key].Data
            edges_map[opposite].Creation = Creation.Automatic

            # propagate the opposite edge further
            _propagate_edge(edges_map, shape, faces, opposite, face)


def _get_automatic_transfinite_edges(shape, faces, tf_curve_objs, auto_curve_data):
    # Returns a map from EdgeKey to EdgeData for all edges that require transfinite curve definitions
    #
    # shape: The Part shape object all the faces belong to
    # faces: A list of faces which shall be transfinite
    # tf_curve_objects: The transfinite curve document objects governing the faces
    # auto_curve_data: A TFCurveDefinition object which will be applied on all undefined edges

    # 1. get the full edge map (careful to not doulbe add edges)
    edges_map = {}
    for face in faces:
        for edge in face.Edges:
            if not EdgeKey(edge) in edges_map:
                edges_map[EdgeKey(edge)] = EdgeData()

    # 2. Setup the user defined values
    user_edges = set()
    for tf in tf_curve_objs:

        if tf.Suppressed:
            continue

        data = TFCurveDefinition(tf.Nodes, tf.Coefficient, tf.Distribution, tf.Invert)

        for ref in tf.References:
            obj = ref[0]
            for sub in ref[1]:
                if not "Edge" in sub:
                    continue

                key = EdgeKey(obj.getSubObject(sub))
                edges_map[key] = EdgeData(Creation=Creation.User, Data=data)
                user_edges.add(key)

    # 3. Propagate all user defined values through the map
    for user_edge in user_edges:
        _propagate_edge(edges_map, shape, faces, user_edge, None)

    # 4. Check if we have further undefined edges, and use the default data on them
    for key, data in edges_map.items():
        if not data.IsDefined:
            data.Data = auto_curve_data
            data.Creation = Creation.Automatic

    return edges_map

def get_automatic_transfinite_edge_sets(shape, faces, tf_curve_objs, auto_curve_data):
    # returns map of TFCurveDefinition to [EdgeID]

    # faces names to faces shape
    face_shapes = [shape.getElement(name) for name in faces]

    # get transfinite data for all edges
    edge_map = _get_automatic_transfinite_edges(shape, face_shapes, tf_curve_objs, auto_curve_data)

    # convert into individual curve definitions
    result = {}
    for edge, edge_data in edge_map.items():

        if not edge_data.IsAutomatic:
            continue

        if not edge_data.Data in result:
            result[edge_data.Data] = []

        name, idx = shape.findSubShape(edge.Edge)
        result[edge_data.Data].append(idx)

    return result


