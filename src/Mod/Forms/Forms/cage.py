# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Canonical control-cage data model and Forms object adapters.

Topology commands operate on :class:`ControlCage` instead of manipulating
FreeCAD properties directly.  This keeps serialization, validation,
sharpness remapping, and BRep regeneration in one place as the topology tool
set grows.
"""

import heapq
import math
import re

import FreeCAD as App
import Part

from .brep import (
    ConversionError,
    apply_local_edge_inserts,
    cage_to_solid,
    cage_to_surface,
    decode_local_edge_inserts,
    dissolved_control_faces,
    hierarchical_cage_to_shape,
    hierarchical_control_surface_points,
    tmesh_cage_to_shape,
    tmesh_control_surface_points,
)
from .topology import (
    cage_edges,
    catmull_clark_limit_points,
    catmull_clark_patch_grids,
)


def _edge(start, end):
    return tuple(sorted((int(start), int(end))))


def canonical_subelement_name(name):
    """Extract a stable element token from old or mapped element names."""
    matches = re.findall(r"(?:Vertex|Edge|Face)\d+", str(name))
    # Complex mapped names retain historical tokens before the current
    # terminal element (for example ``;Face1;...Face24``). The last token is
    # the subelement exposed by the object's present shape.
    return matches[-1] if matches else ""


from .controlcage import ControlCage
from .model import object_tmesh as _object_tmesh


def update_object_shape(obj):
    """Regenerate a solid or surface according to the cage boundary topology."""
    curvature_match = bool(getattr(obj, "MatchBoundary", ())) and str(
        getattr(obj, "MatchContinuity", "")
    ) == "Curvature"
    if hasattr(obj, "MatchStatus"):
        obj.MatchStatus = ""
    try:
        # Match is part of conversion: a deleted or invalid support must produce
        # a normal failed-conversion state instead of escaping Feature::execute
        # and leaving a stale shape in the document.
        from .match_constraints import apply_match_constraints

        apply_match_constraints(obj)
        cage = ControlCage.from_object(obj)
        local_inserts = decode_local_edge_inserts(getattr(obj, "LocalEdgeInserts", ()))
        local_points = [
            (point.x, point.y, point.z) for point in getattr(obj, "LocalControlPoints", ())
        ]
        tmesh_data = str(getattr(obj, "TMeshData", "") or "")
        dissolved_edges = []
        for encoded_edge in getattr(obj, "DissolvedEdges", ()):
            try:
                first, second = str(encoded_edge).split()
                dissolved_edges.append(tuple(sorted((int(first), int(second)))))
            except (TypeError, ValueError):
                raise ValueError("A dissolved edge record is invalid")
        if tmesh_data and dissolved_edges:
            raise ValueError("Edge dissolve cannot be combined with local T-mesh refinement")
        if local_inserts and dissolved_edges:
            raise ValueError("Edge dissolve cannot be combined with local edge insertion")
        if tmesh_data:
            mesh = _object_tmesh(obj, cage)
            shape, deviation, level = tmesh_cage_to_shape(
                cage.vertices,
                cage.faces,
                mesh,
                mesh.is_closed,
                obj.BRepTolerance.Value,
                obj.MaxRefinement,
                cage.edge_sharpness,
                cage.vertex_sharpness,
            )
        elif local_inserts and all(insert.points for insert in local_inserts):
            shape, deviation, level = hierarchical_cage_to_shape(
                cage.vertices,
                cage.faces,
                local_inserts,
                local_points,
                cage.is_closed,
                obj.BRepTolerance.Value,
                obj.MaxRefinement,
                cage.edge_sharpness,
                cage.vertex_sharpness,
            )
        else:
            components = cage.connected_components() if not dissolved_edges else (cage,)
            if len(components) > 1:
                converted = [
                    (cage_to_solid if component.is_closed else cage_to_surface)(
                        component.vertices,
                        component.faces,
                        obj.BRepTolerance.Value,
                        obj.MaxRefinement,
                        component.edge_sharpness,
                        component.vertex_sharpness,
                    )
                    for component in components
                ]
                shape = Part.makeCompound([result[0] for result in converted])
                deviation = max(result[1] for result in converted)
                level = max(result[2] for result in converted)
            else:
                converter = cage_to_solid if cage.is_closed else cage_to_surface
                shape, deviation, level = converter(
                    cage.vertices,
                    cage.faces,
                    obj.BRepTolerance.Value,
                    obj.MaxRefinement,
                    cage.edge_sharpness,
                    cage.vertex_sharpness,
                    dissolved_edges=dissolved_edges,
                    **({"min_refinement": int(obj.MaxRefinement), "curvature_boundary": True}
                       if curvature_match else {}),
                )
            shape = apply_local_edge_inserts(
                shape,
                cage.vertices,
                cage.faces,
                level,
                local_inserts,
                obj.BRepTolerance.Value,
                cage.edge_sharpness,
                cage.vertex_sharpness,
            )
        from .elementmap import map_form_shape

        if curvature_match:
            from .curvature import validate_curvature_shape
            validate_curvature_shape(obj, shape)
        obj.Shape = map_form_shape(obj, shape)
        obj.MaximumDeviation = deviation
        obj.ConversionLevel = level
        kind = "solid" if shape.ShapeType == "Solid" else "surface"
        if deviation <= obj.BRepTolerance.Value:
            obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Valid %1").replace(
                "%1", kind
            )
        else:
            obj.ConversionStatus = App.Qt.translate(
                "Forms_Conversion", "Valid %1; requested deviation was not reached"
            ).replace("%1", kind)
    except (ConversionError, Part.OCCError, ValueError, RuntimeError) as error:
        if curvature_match:
            obj.MatchStatus = str(error)
        obj.Shape = Part.Shape()
        obj.MaximumDeviation = 0.0
        obj.ConversionLevel = 0
        obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Failed: %1").replace(
            "%1", str(error)
        )


def control_surface_points(obj):
    """Return the generated BRep corner corresponding to each cage vertex."""
    cage = ControlCage.from_object(obj)
    if (
        str(getattr(obj, "FormType", "")) == "Forms::Face"
        and not getattr(obj, "MatchBoundary", ())
        and not getattr(obj, "ProfileShape", Part.Shape()).isNull()
        and bool(getattr(getattr(obj, "Proxy", None), "_show_edit_shape", False))
        and len(getattr(obj, "ProfileControlPoints", ())) == len(cage.vertices)
        and all(
            App.Vector(current).sub(initial).Length <= 1.0e-9
            for current, initial in zip(
                getattr(obj, "ControlPoints", ()),
                getattr(obj, "ProfileControlPoints", ()),
            )
        )
    ):
        return [App.Vector(*point) for point in cage.vertices]
    if str(getattr(obj, "FormType", "")) == "Forms::Surface":
        # A Part Design Form Surface is a boundary-constrained filling rather
        # than the standalone Catmull-Clark BRep stored by other Forms.
        return [App.Vector(*point) for point in cage.vertices]
    inserts = decode_local_edge_inserts(getattr(obj, "LocalEdgeInserts", ()))
    local_points = [(point.x, point.y, point.z) for point in getattr(obj, "LocalControlPoints", ())]
    tmesh_data = str(getattr(obj, "TMeshData", "") or "")
    if tmesh_data:
        mesh = _object_tmesh(obj, cage)
        return [
            App.Vector(*point)
            for point in tmesh_control_surface_points(
                cage.vertices,
                cage.faces,
                mesh,
                cage.edge_sharpness,
                cage.vertex_sharpness,
            )
        ]
    if inserts and all(insert.points for insert in inserts):
        return [
            App.Vector(*point)
            for point in hierarchical_control_surface_points(
                cage.vertices,
                cage.faces,
                inserts,
                local_points,
                cage.edge_sharpness,
                cage.vertex_sharpness,
            )
        ]
    level = max(int(getattr(obj, "ConversionLevel", 1)), 1)
    try:
        fit_vertices, fit_faces, fit_corners = cage.vertices, cage.faces, cage.vertex_sharpness
        if getattr(obj, "MatchBoundary", ()) and str(obj.MatchContinuity) == "Curvature":
            from .curvature import extended_cage
            fit_vertices, fit_faces, fit_corners = extended_cage(
                fit_vertices, fit_faces, fit_corners)
        grids = catmull_clark_patch_grids(
            fit_vertices,
            fit_faces,
            level,
            cage.edge_sharpness,
            fit_corners,
        )
        result = [None] * len(cage.vertices)
        for face, grid in zip(cage.faces, grids):
            corners = (grid[0][0], grid[-1][0], grid[-1][-1], grid[0][-1])
            for index, point in zip(face, corners):
                result[index] = App.Vector(*point)
        if all(point is not None for point in result):
            return result
    except ValueError:
        # Unsupported patch topology falls back to direct limit-point evaluation below.
        pass
    return [
        App.Vector(*point)
        for point in catmull_clark_limit_points(
            cage.vertices,
            cage.faces,
            cage.edge_sharpness,
            cage.vertex_sharpness,
        )
    ]


class ControlElementMapper:
    """Efficiently map several generated BRep elements to one control cage."""

    def __init__(self, obj):
        self.obj = obj
        self.cage = ControlCage.from_object(obj)
        form_shape = getattr(obj, "FormShape", None)
        self.shape = form_shape if form_shape is not None and not form_shape.isNull() else obj.Shape
        encoded = str(getattr(obj, "TMeshData", "") or "")
        self.mesh = _object_tmesh(obj, self.cage) if encoded else None
        self.logical_faces = list(self.cage.faces)
        self.logical_face_groups = [(index,) for index in range(len(self.cage.faces))]
        if self.mesh is None:
            dissolved = []
            for encoded_edge in getattr(obj, "DissolvedEdges", ()):
                try:
                    first, second = str(encoded_edge).split()
                    dissolved.append(tuple(sorted((int(first), int(second)))))
                except (TypeError, ValueError):
                    raise ValueError("A dissolved edge record is invalid")
            if dissolved:
                self.logical_faces, self.logical_face_groups = dissolved_control_faces(
                    self.cage.faces, dissolved
                )
        self.points = control_surface_points(obj)
        self.component_shapes = []
        self.control_component = {}
        components = self.cage.face_components()
        if len(components) > 1 and self.shape.ShapeType == "Compound":
            try:
                children = list(self.shape.childShapes())
            except (Part.OCCError, RuntimeError):
                children = []
            if len(children) == len(components):
                for component_index, (face_ids, child) in enumerate(
                    zip(components, children)
                ):
                    controls = frozenset(
                        vertex
                        for face_id in face_ids
                        for vertex in self.cage.faces[face_id]
                    )
                    self.component_shapes.append((child, controls))
                    for control in controls:
                        self.control_component[control] = component_index
        self.form_surface_faces = [
            self.shape.Faces[index - 1]
            for index in getattr(obj, "FormSurfaceFaces", ())
            if 1 <= index <= len(self.shape.Faces)
        ]
        self.form_surface_face_controls = []
        for encoded in getattr(obj, "FormSurfaceFaceMap", ()):
            try:
                values = tuple(int(value) for value in str(encoded).split())
                face_index, controls = values[0], values[1:]
                if 1 <= face_index <= len(self.shape.Faces) and controls:
                    self.form_surface_face_controls.append(
                        (self.shape.Faces[face_index - 1], controls)
                    )
            except (TypeError, ValueError):
                continue
        diagonal = self.shape.BoundBox.DiagonalLength if not self.shape.isNull() else 0.0
        self.tolerance = max(diagonal * 1.0e-4, 1.0e-6)
        self._refined_edge_controls = None
        self._refined_edge_parameters = None

    def _mapped_indices(self, vertices, require_all=True):
        candidate_controls = range(len(self.points))
        for child, controls in self.component_shapes:
            child_vertices = child.Vertexes
            if vertices and all(
                any(vertex.isPartner(candidate) for candidate in child_vertices)
                for vertex in vertices
            ):
                candidate_controls = controls
                break
        mapped = []
        for vertex in vertices:
            distance, index = min(
                (self.points[index].sub(vertex.Point).Length, index)
                for index in candidate_controls
            )
            if distance > self.tolerance:
                if require_all:
                    return ()
                continue
            if index not in mapped:
                mapped.append(index)
        return tuple(mapped)

    def _adjacent_faces(self, edge):
        return [
            face
            for face in self.shape.Faces
            if any(candidate.isSame(edge) for candidate in face.Edges)
        ]

    def _control_edge_for_refined_edge(self, element):
        """Map a child patch edge back to its polygon-cage segment."""
        if self._refined_edge_controls is None:
            shape_vertices = list(self.shape.Vertexes)
            shape_edges = list(self.shape.Edges)
            adjacency = {index: [] for index in range(len(shape_vertices))}
            for edge_index, shape_edge in enumerate(shape_edges):
                endpoints = [
                    next(
                        index
                        for index, candidate in enumerate(shape_vertices)
                        if candidate.isSame(vertex)
                    )
                    for vertex in shape_edge.Vertexes
                ]
                if len(endpoints) != 2:
                    continue
                first, second = endpoints
                weight = max(float(shape_edge.Length), 1.0e-12)
                adjacency[first].append((second, weight, edge_index))
                adjacency[second].append((first, weight, edge_index))

            control_vertices = {}
            for control_index, point in enumerate(self.points):
                candidates = range(len(shape_vertices))
                component_index = self.control_component.get(control_index)
                if component_index is not None:
                    child = self.component_shapes[component_index][0]
                    child_vertices = child.Vertexes
                    candidates = [
                        index
                        for index, vertex in enumerate(shape_vertices)
                        if any(vertex.isPartner(candidate) for candidate in child_vertices)
                    ]
                    if not candidates:
                        continue
                distance, shape_index = min(
                    (point.sub(shape_vertices[index].Point).Length, index)
                    for index in candidates
                )
                if distance <= self.tolerance:
                    control_vertices[control_index] = shape_index
            logical_edges = (
                self.mesh.atomic_edges()
                if self.mesh is not None
                else set(cage_edges(self.logical_faces))
            )
            mapped = {}
            parameter_map = {}
            for control_edge in logical_edges:
                if any(index not in control_vertices for index in control_edge):
                    continue
                start, target = (control_vertices[index] for index in control_edge)
                distances = {start: 0.0}
                previous = {}
                pending = [(0.0, start)]
                while pending:
                    distance, current = heapq.heappop(pending)
                    if distance != distances.get(current):
                        continue
                    if current == target:
                        break
                    for neighbor, weight, edge_index in adjacency[current]:
                        candidate = distance + weight
                        if candidate < distances.get(neighbor, math.inf):
                            distances[neighbor] = candidate
                            previous[neighbor] = (current, edge_index)
                            heapq.heappush(pending, (candidate, neighbor))
                current = target
                path = []
                while current != start and current in previous:
                    prior, edge_index = previous[current]
                    path.append((prior, current, edge_index))
                    current = prior
                if current != start:
                    continue
                path.reverse()
                total = sum(shape_edges[item[2]].Length for item in path)
                travelled = 0.0
                for segment_start, segment_end, edge_index in path:
                    mapped.setdefault(edge_index, []).append(tuple(sorted(control_edge)))
                    length = float(shape_edges[edge_index].Length)
                    first_shape_vertex = next(
                        index
                        for index, candidate in enumerate(shape_vertices)
                        if candidate.isSame(shape_edges[edge_index].Vertexes[0])
                    )
                    interval = (travelled / total, (travelled + length) / total)
                    if first_shape_vertex != segment_start:
                        interval = tuple(reversed(interval))
                    parameter_map.setdefault(edge_index, []).append(
                        (tuple(sorted(control_edge)), *interval)
                    )
                    travelled += length
            self._refined_edge_controls = [
                (shape_edges[index], controls[0])
                for index, controls in mapped.items()
                if len(set(controls)) == 1
            ]
            self._refined_edge_parameters = [
                (shape_edges[index], records[0])
                for index, records in parameter_map.items()
                if len({record[0] for record in records}) == 1
            ]
        return next(
            (
                controls
                for shape_edge, controls in self._refined_edge_controls
                if shape_edge.isSame(element)
            ),
            None,
        )

    def refined_edge_parameter_range(self, element):
        """Return a child edge's oriented fraction range on its control edge."""
        self._control_edge_for_refined_edge(element)
        return next(
            (
                record
                for shape_edge, record in self._refined_edge_parameters
                if shape_edge.isSame(element)
            ),
            None,
        )

    def _split_parent_face(self, element):
        """Return the logical parent of an exact local split element."""
        if element.ShapeType == "Edge":
            adjacent = self._adjacent_faces(element)
            if len(adjacent) != 2:
                return None
            vertices = [vertex for face in adjacent for vertex in face.Vertexes]
        elif element.ShapeType == "Face":
            vertices = list(element.Vertexes)
            for edge in element.Edges:
                if self._mapped_indices(edge.Vertexes, require_all=False):
                    continue
                adjacent = self._adjacent_faces(edge)
                if len(adjacent) == 2:
                    vertices.extend(
                        vertex
                        for face in adjacent
                        if not face.isSame(element)
                        for vertex in face.Vertexes
                    )
                    break
        else:
            return None
        mapped = self._mapped_indices(vertices, require_all=False)
        face_id = self.face_id(mapped)
        if face_id is None:
            return None
        return (
            self.mesh.faces[face_id].corners if self.mesh is not None else self.cage.faces[face_id]
        )

    def face_id(self, vertex_indices):
        """Return the stable logical face ID matching mapped BRep corners."""
        target = frozenset(int(index) for index in vertex_indices)
        if self.mesh is not None:
            matches = [
                face_id
                for face_id, face in self.mesh.faces.items()
                if frozenset(face.boundary) == target or frozenset(face.corners) == target
            ]
            return matches[0] if len(matches) == 1 else None
        exact = [
            face_id
            for face_id, face in enumerate(self.logical_faces)
            if frozenset(face) == target
        ]
        if len(exact) == 1:
            return exact[0]
        containing = [
            face_id
            for face_id, face in enumerate(self.logical_faces)
            if target and target.issubset(frozenset(face))
        ]
        return containing[0] if len(containing) == 1 else None

    def target(self, element):
        """Return ``(control indices, optional dragger anchor)`` for an element."""
        if element.ShapeType == "Face":
            mapped_face = next(
                (
                    controls
                    for face, controls in self.form_surface_face_controls
                    if element.isSame(face)
                ),
                None,
            )
            if mapped_face is not None:
                return tuple(mapped_face), App.Vector(element.CenterOfMass)
            if any(element.isSame(face) for face in self.form_surface_faces):
                return tuple(range(len(self.cage.vertices))), App.Vector(element.CenterOfMass)
        vertices = [element] if element.ShapeType == "Vertex" else element.Vertexes
        mapped = self._mapped_indices(vertices)
        if element.ShapeType == "Face" and mapped:
            face_id = self.face_id(mapped)
            if face_id is not None:
                face = (
                    self.mesh.faces[face_id].boundary
                    if self.mesh is not None
                    else self.logical_faces[face_id]
                )
                return tuple(face), App.Vector(element.CenterOfMass)
        if mapped:
            return mapped, None
        if element.ShapeType == "Edge":
            refined_edge = self._control_edge_for_refined_edge(element)
            if refined_edge is not None:
                return refined_edge, None
        parent_face = self._split_parent_face(element)
        if parent_face is None:
            return (), None
        return tuple(parent_face), App.Vector(element.CenterOfMass)

    def indices(self, element):
        return self.target(element)[0]


def control_indices_for_element(obj, element):
    """Map one generated BRep vertex, edge, or face back to cage vertices."""
    return ControlElementMapper(obj).indices(element)


# Keep the historical scripting imports available after separating implementation modules.
__all__ = [
    "ControlCage",
    "ControlElementMapper",
    "canonical_subelement_name",
    "control_indices_for_element",
    "control_surface_points",
    "update_object_shape",
]
