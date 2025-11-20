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
from FreeCAD import Console
import Part
from femtools import geomtools

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

@dataclass(frozen=True)
class TFSurfaceDefinition:
    Recombine: bool = False
    Orientation: str = "Left"
    VertexIdx: str = ""

    def to_gmshtools_setting(self):

        settings = {}
        settings["recombine"] = self.Recombine
        settings["orientation"] = self.Orientation
        if self.VertexIdx:
            settings["nodes"] = self.VertexIdx

        return settings

    @staticmethod
    def vertexIdx_string_from_list(vidx):
        return ",".join(str(i) for i in vidx)

    @staticmethod
    def from_tfsurface_obj(obj):
        return TFSurfaceDefinition(obj.Recombine, obj.TriangleOrientation)


class Creation(Enum):
    Undefined           = auto()
    User                = auto() # user created
    AutomaticSurface    = auto() # created by surface automation
    AutomaticVolume     = auto() # created by volume automation

@dataclass
class EdgeData:

    Creation: Creation          = Creation.Undefined
    Data:     TFCurveDefinition = field(default_factory=TFCurveDefinition)

    @property
    def IsDefined(self):
        return self.Creation != Creation.Undefined

    @property
    def IsAutomatic(self):
        return (self.Creation == Creation.AutomaticSurface or
                self.Creation == Creation.AutomaticVolume)

@dataclass
class FaceData:

    Creation: Creation            = Creation.Undefined
    Data:     TFSurfaceDefinition = field(default_factory=TFSurfaceDefinition)
    GuideVertex: int              = 0       # The vertex that decides which are the two guide edges on a 3-sided face

    @property
    def IsDefined(self):
        return self.Creation != Creation.Undefined

    @property
    def IsAutomatic(self):
        # Automatic Surface is invalid for face, as it can only be created automaticall by volume automation
        return self.Creation == Creation.AutomaticVolume

@dataclass
class Key:

    Shape: object

    def __eq__(self, other):
        return self.Shape.isSame(other.Shape)

    def __hash__(self):
        return self.Shape.hashCode()


def _get_common_faces(face_list_1, face_list_2):

    result = []
    for face1 in face_list_1:
        for face2 in face_list_2:
            if face1.isSame(face2):
               result.append(face1)

    return result

def _get_opposing_edge(surface_map, face, edge):
    # Within face, find the edge that is opposite of the given one for transfinite meshing

    match len(face.Edges):
        case 3:
            # only return opposite if we ask for a guiding edge
            face_key = Key(face)
            guide_vertex = face.Vertexes[0]
            if face_key in surface_map:
                guide_vertex = face.Vertexes[surface_map[face_key].GuideVertex]

            # is the edge we non-guiding in this face?
            if not (edge.Vertexes[0].isSame(guide_vertex) or
                    edge.Vertexes[1].isSame(guide_vertex)):
                return  None

            # the edge is guiding for this face, so find the other guiding edge to return
            for face_edge in face.Edges:
                if face_edge.isSame(edge):
                    continue

                if (face_edge.Vertexes[0].isSame(guide_vertex) or
                    face_edge.Vertexes[1].isSame(guide_vertex)):
                    return face_edge

            # we should never be here!
            raise Exception("Cannot determine guiding edges; abort")

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


def _propagate_edge(surfaces_map, edges_map, shape, faces, edge_key, origin_face, creator):
    # Propagates the edge values through the map
    #
    # surfaces_map  The map of all face keys to face data
    # edges_map:    The map of all edge keys to edge data
    # shape:        The shape all faces and edges belong
    # faces:        All faces that should be made transfinite
    # edge_key:     The edge key which shal be propagaed
    # origin_face:  The face the propagated edge was itself propagated from (to not go backwards)

    # 1. Get all faces the edge belongs to (from user provided list)
    edge_faces = shape.ancestorsOfType(edge_key.Shape, Part.Face)
    propagate_faces = _get_common_faces(edge_faces, faces)

    # 3. Find opposing edges and apply value if not set yet
    for face in propagate_faces:

        # do not go backward
        if origin_face and face.isSame(origin_face):
            continue

        opposite = _get_opposing_edge(surfaces_map, face, edge_key.Shape)
        if not opposite:
            # 3 sided faces may not have a opposite edge
            continue

        opposite_key = Key(opposite)
        if edges_map[opposite_key].IsDefined:
            # check if compatibel, and raise error if not
            if edges_map[opposite_key].Data.Nodes != edges_map[edge_key].Data.Nodes:
                raise Exception("Transfinite curve data is inconsitent, cannot apply automatic algorithm")

            # and go on to the next face
            continue

        else:
            # transfer the data as automatic definition
            edges_map[opposite_key].Data = edges_map[edge_key].Data
            edges_map[opposite_key].Creation = creator

            # propagate the opposite edge further
            _propagate_edge(surfaces_map, edges_map, shape, faces, opposite_key, face, creator)


def _get_reference_elements(shape, obj):
    # basically a copy of gmshtools get_reference_elements
    # needed because of the search in reference subshape part

    # don't use set to avoid duplicates, as we need to keep the user defined order
    # of reference elements. This is important for example in transfinite surfaces
    elements = []
    for sub in obj.References:
        # check if the shape of the mesh refinements
        # is an element of the Part to mesh
        # if not try to find the element in the shape to mesh
        search_ele_in_shape_to_mesh = False
        if not shape.isSame(sub[0].Shape):
            Console.PrintLog(
                "One element of the mesh refinement {} is "
                "not an element of the Part to mesh.\n"
                "But we are going to try to find it in "
                "the Shape to mesh :-)\n".format(obj.Name)
            )
            search_ele_in_shape_to_mesh = True

        for element in sub[1]:
            if search_ele_in_shape_to_mesh:
                # we're going to try to find the element in the
                # Shape to mesh and use the found element as elems
                # the method getElement(element)
                # does not return Solid elements
                ele_shape = geomtools.get_element(sub[0], element)
                found_element = geomtools.find_element_in_shape(
                    shape, ele_shape
                )
                if found_element:
                    element = found_element
                else:
                    Console.PrintError(
                        "One element of the mesh refinement {} could not be found "
                        "in the Part to mesh. It will be ignored.\n".format(
                            obj.Name
                        )
                    )
            if not element in elements:
                elements.append(element)

    return elements

def _get_automatic_transfinite_edges(surfaces_map, edge_map, shape, faces, auto_curve_data, creator):
    # Updates the edge map for all edges that require transfinite curve definitions in faces list
    #
    # shape: The Part shape object all the faces belong to
    # faces: A list of faces which shall be transfinite
    # tf_curve_objects: The transfinite curve document objects governing the faces
    # auto_curve_data: A TFCurveDefinition object which will be applied on all undefined edges

    # 1. add all new edges not yet defined (careful to not doulbe add edges)
    for face in faces:
        for edge in face.Edges:
            if not Key(edge) in edge_map:
                edge_map[Key(edge)] = EdgeData()

    # 2. get all already defined edges
    user_edges = set()
    surf_edges = set()
    vol_edges  = set()
    for key, data in edge_map.items():

        if data.Creation == Creation.User:
            user_edges.add(key)
        elif data.Creation == Creation.AutomaticSurface:
            surf_edges.add(key)
        elif data.Creation == Creation.AutomaticVolume:
            vol_edges.add(key)

    # 3. Propagate all user defined values through the map
    for user_edge in user_edges:
        _propagate_edge(surfaces_map, edge_map, shape, faces, user_edge, None, creator)

    # 3. Propagate all already auto-surface defined values through the map (lower prio then user defined)
    #    Requried when there are multiple transfinite surface objects with automation
    for surf_edge in surf_edges:
        _propagate_edge(surfaces_map, edge_map, shape, faces, surf_edge, None, creator)

    # 4. Propagate all already auto-volume defined values through the map (lower prio then surface defined)
    #    Requried when there are multiple transfinite surface objects with automation
    for vol_edge in vol_edges:
        _propagate_edge(surfaces_map, edge_map, shape, faces, vol_edge, None, creator)

    # 5. Check if we have further undefined edges, and use the default data on them
    for key, data in edge_map.items():
        if not data.IsDefined:
            data.Data = auto_curve_data
            data.Creation = creator


def setup_transfinite_edge_map(shape, tf_curve_objs):
    # builds the initial edge map with user defined edges

    edges_map = {}
    for tf in tf_curve_objs:

        if tf.Suppressed:
            continue

        data = TFCurveDefinition.from_tfcurve_obj(tf)

        for ref in _get_reference_elements(shape, tf):
            ref_shape = shape.getElement(ref)
            if ref_shape.ShapeType != "Edge":
                continue

            key = Key(ref_shape)
            if key in edges_map:
                # double definition: ignore latest
                Console.PrintError( (f"The transfinite curve {tf.Label} redefines already setup edges. Those definitions are ignored.\n") )
                continue

            edges_map[key] = EdgeData(Creation=Creation.User, Data=data)

    return edges_map

def setup_transfinite_surface_map(shape, tf_surface_objs):
    # Builds the initial surface map with user defined surfaces

    faces_map = {}
    for tf in tf_surface_objs:

        if tf.Suppressed:
            continue

        data = TFSurfaceDefinition.from_tfsurface_obj(tf)

        vertices = []
        faces = []
        for ref in _get_reference_elements(shape, tf):

            ref_shape = shape.getElement(ref)

            if ref_shape.ShapeType == "Vertex":
                vertices.append(ref_shape)

            if ref_shape.ShapeType == "Face":
                faces.append(ref_shape)

        # add face entries
        for face in faces:
            key = Key(face)
            if key in faces_map:
                # double definition: ignore latest
                Console.PrintError( (f"The transfinite surface {tf.Label} redefines an already"
                                    " setup face. Those definitions are ignored.\n") )
                continue

            faces_map[key] = FaceData(Creation=Creation.User, Data=data)

            # determine the default guiding vertex for this face
            if len(face.Edges) == 3:
                # We mimic the gmsh algorithm for determining the guiding vertex.
                # GMSH uses the first vertex of the first edge as corner: FirstEdge.FirstVertex
                #
                # However, gmsh does take the wire orientation into account, and if it is "Reversed"
                # the edge order is turned around. It then uses LastEdge.FirstVertex as corner
                if face.OuterWire.Orientation == "Reversed":
                    name, idx = face.findSubShape(face.Edges[-1].Vertexes[0])
                    faces_map[key].GuideVertex = idx - 1
                else:
                    name, idx = face.findSubShape(face.Edges[0].Vertexes[0])
                    faces_map[key].GuideVertex = idx - 1


        # handle vertex selections
        # ########################

        if vertices:
            # we need to be more carefull. user could have selected different scenarios:
            # Single face:
            #    - single vertex as corner for 3-sided face
            #    - 3 or 4 corner points
            # Multi face:
            #    - multiple vertexes, max 1 for each 3-sided face

            if len(faces) == 1:

                face = faces[0]
                key = Key(face)

                # first vertex is always the guide. Find guide_idx within the shape
                name, guide_idx = face.findSubShape(vertices[0])
                if not name:
                    raise Exception("Selected vertex is not part of selected face")

                if len(vertices) == 1:
                    if not len(face.Edges) == 3:
                        raise Exception("Invalid vertex selection: single vertex only valid for 3-sided face")

                    # change guide vertex and vertex order if we have a non-default case
                    if faces_map[key].GuideVertex != (guide_idx-1):
                        faces_map[key].GuideVertex = guide_idx - 1

                        # build vertex order (in shape idx, not face idx!)
                        order = []
                        for vertex in face.Vertexes:
                            _, idx = shape.findSubShape(vertex)
                            order.append(idx)
                        _, shape_guide_idx = shape.findSubShape(vertices[0])
                        order.insert(0, order.pop(order.index(shape_guide_idx)))
                        order_str = TFSurfaceDefinition.vertexIdx_string_from_list(order)
                        data = faces_map[key].Data
                        faces_map[key].Data = TFSurfaceDefinition(Recombine=data.Recombine, Orientation=data.Orientation, VertexIdx=order_str)

                else:
                    # 3 or 4 vertexes indicating the corner points of a multi-edges face
                    # the user should have ensured the correct number of vertexes, no testing here for now
                    faces_map[key].GuideVertex = guide_idx - 1
                    data = faces_map[key].Data

                    vidx = []
                    for vertex in vertices:
                        name, idx = shape.findSubShape(vertex)
                        vidx.append(idx)

                    order_str = TFSurfaceDefinition.vertexIdx_string_from_list(vidx)
                    faces_map[key].Data = TFSurfaceDefinition(Recombine=data.Recombine, Orientation=data.Orientation, VertexIdx=order_str)
            else:
                # find the correct vertex to use as guide if 3-sided
                for face in faces:
                    if len(face.Edges) != 3:
                        continue

                    # see if we have a corner vertex for the 3-sided face
                    for vidx, face_vertex in enumerate(face.Vertexes):
                        for sel_vertex in vertices:
                            if face_vertex.isSame(sel_vertex):

                                # found the guide!
                                face_key = Key(face)
                                face_data = faces_map[face_key].Data

                                # build vertex order (in shape idx, not face idx!)
                                order = []
                                for vertex in face.Vertexes:
                                    _, idx = shape.findSubShape(vertex)
                                    order.append(idx)
                                _, shape_guide_idx = shape.findSubShape(sel_vertex)
                                order.insert(0, order.pop(order.index(shape_guide_idx)))
                                order_str = TFSurfaceDefinition.vertexIdx_string_from_list(order)

                                faces_map[face_key].GuideVertex = vidx
                                faces_map[face_key].Data = TFSurfaceDefinition(Recombine=data.Recombine, Orientation=data.Orientation, VertexIdx=order_str)

                                break
                        else:
                            continue

                        break




    return faces_map

def map_to_definitions(edge_map, shape, only_by_creator=None):

    # convert into individual curve definitions
    result = {}
    for edge, edge_data in edge_map.items():

        if only_by_creator and edge_data.Creation != only_by_creator:
            continue

        if not edge_data.Data in result:
            result[edge_data.Data] = []

        name, idx = shape.findSubShape(edge.Shape)
        result[edge_data.Data].append(idx)

    return result


def add_automatic_transfinite_edges_from_faces(surfaces_map, edge_map, shape, faces, auto_curve_data):
    # adds automatic transfinite curves to the edge map based on the faces that should be automatically
    # extended

    # faces names to faces shape
    face_shapes = [shape.getElement(name) for name in faces]

    # get transfinite data for all edges
    _get_automatic_transfinite_edges(surfaces_map, edge_map, shape, face_shapes, auto_curve_data, Creation.AutomaticSurface)


def add_automatic_transfinite_edges_from_solids(surface_map, edge_map, shape, solids, auto_curve_data):

    # solid names to solids
    solid_shapes = [shape.Solids[int(e.lstrip("Solid")) - 1] for e in solids]

    # faces names to faces shape
    face_shapes = []
    for solid in solid_shapes:
        face_shapes += solid.Faces

    # get transfinite data for all edges
    _get_automatic_transfinite_edges(edge_map, shape, face_shapes, auto_curve_data, Creation.AutomaticVolume)


def add_automatic_transfinite_surfaces_from_solids(surface_map, shape, solids, auto_surface_data):

    # solid names to solids
    solid_shapes = [shape.Solids[int(e.lstrip("Solid")) - 1] for e in solids]

    # faces names to faces shape
    face_shapes = []
    for solid in solid_shapes:
        face_shapes += solid.Faces

    # check if faces are not defined, and add them if so
    for face in face_shapes:
        key = Key(face)
        if not key in surface_map:
            surface_map[key] = FaceData(Creation=Creation.AutomaticVolume, Data=auto_surface_data)


