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

from collections import Counter
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
from .tmesh import HierarchicalTMesh
from .topology import (
    cage_edges,
    catmull_clark_limit_points,
    catmull_clark_patch_grids,
    validate_manifold_boundary,
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


def _object_tmesh(obj, cage):
    mesh = HierarchicalTMesh.decode(str(obj.TMeshData))
    base_count = len(cage.vertices)
    for offset, point in enumerate(getattr(obj, "LocalControlPoints", ())):
        vertex_id = base_count + offset
        if vertex_id in mesh.vertices:
            mesh.set_vertex(vertex_id, (point.x, point.y, point.z))
    return mesh


class ControlCage:
    """Editable manifold polygon cage, independent of a primitive type."""

    def __init__(
        self,
        vertices,
        faces,
        vertex_sharpness=None,
        edge_sharpness=None,
    ):
        self.vertices = [tuple(float(component) for component in point) for point in vertices]
        self.faces = [tuple(int(index) for index in face) for face in faces]
        self.vertex_sharpness = [max(0.0, float(value)) for value in (vertex_sharpness or ())]
        self.vertex_sharpness.extend([0.0] * (len(self.vertices) - len(self.vertex_sharpness)))
        self.vertex_sharpness = self.vertex_sharpness[: len(self.vertices)]
        self.edge_sharpness = {
            _edge(*edge): max(0.0, float(value))
            for edge, value in (edge_sharpness or {}).items()
            if float(value) > 0.0
        }
        self.validate()

    @classmethod
    def from_object(cls, obj):
        """Read the canonical cage representation from a Forms object."""
        vertices = [(point.x, point.y, point.z) for point in obj.ControlPoints]
        faces = [tuple(int(index) for index in face.split()) for face in obj.ControlFaces]
        edge_sharpness = {}
        for encoded in getattr(obj, "EdgeSharpness", ()):
            try:
                start, end, value = str(encoded).split()
                edge_sharpness[_edge(start, end)] = float(value)
            except (TypeError, ValueError):
                continue
        return cls(
            vertices,
            faces,
            getattr(obj, "VertexSharpness", ()),
            edge_sharpness,
        )

    def write(self, obj):
        """Store this cage and all index-dependent data on *obj*."""
        obj.ControlPoints = [App.Vector(*point) for point in self.vertices]
        obj.ControlFaces = [" ".join(str(index) for index in face) for face in self.faces]
        obj.VertexSharpness = list(self.vertex_sharpness)
        valid_edges = set(cage_edges(self.faces))
        obj.EdgeSharpness = [
            f"{edge[0]} {edge[1]} {value:.12g}"
            for edge, value in sorted(self.edge_sharpness.items())
            if edge in valid_edges and value > 0.0
        ]
        # A base-topology rewrite invalidates hierarchical face/edge IDs.
        # Local insertions do not call this method, so clearing here prevents
        # stale records after delete, fill, edge-loop, or segment operations.
        if "LocalEdgeInserts" in obj.PropertiesList:
            obj.LocalEdgeInserts = []
        if "LocalControlPoints" in obj.PropertiesList:
            obj.LocalControlPoints = []
        if "TMeshData" in obj.PropertiesList:
            obj.TMeshData = ""
        if "DissolvedEdges" in obj.PropertiesList:
            obj.DissolvedEdges = []

    def validate(self):
        if not self.vertices:
            raise ValueError("A control cage requires at least one vertex")
        if not self.faces:
            raise ValueError("A control cage requires at least one face")
        if any(
            len(point) != 3 or not all(math.isfinite(component) for component in point)
            for point in self.vertices
        ):
            raise ValueError("Control-cage vertices require finite 3D coordinates")
        if any(not math.isfinite(value) for value in self.vertex_sharpness):
            raise ValueError("Control-cage vertex sharpness must be finite")
        for face in self.faces:
            if len(face) < 3:
                raise ValueError("Control-cage faces require at least three vertices")
            if len(set(face)) != len(face):
                raise ValueError("Control-cage faces cannot repeat a vertex")
            if any(index < 0 or index >= len(self.vertices) for index in face):
                raise ValueError("Control-cage face index is out of range")
        edge_counts = self.edge_counts()
        if any(count > 2 for count in edge_counts.values()):
            raise ValueError("The control cage contains a non-manifold edge")
        validate_manifold_boundary(edge_counts)
        # Hierarchical T-mesh edges use stable IDs outside the base cage, so
        # they are intentionally allowed here. Their topology is validated by
        # HierarchicalTMesh; this layer only guards the numeric OCC boundary.
        if any(not math.isfinite(value) for value in self.edge_sharpness.values()):
            raise ValueError("Control-cage edge sharpness must be finite")

    def edge_counts(self):
        counts = Counter()
        for face in self.faces:
            for position, start in enumerate(face):
                counts[_edge(start, face[(position + 1) % len(face)])] += 1
        return counts

    @property
    def is_closed(self):
        return bool(self.faces) and all(count == 2 for count in self.edge_counts().values())

    @property
    def boundary_edges(self):
        return sorted(edge for edge, count in self.edge_counts().items() if count == 1)

    def boundary_loops(self):
        """Return consistently ordered vertex loops along every free boundary."""
        occurrences = {}
        for face in self.faces:
            for position, start in enumerate(face):
                end = face[(position + 1) % len(face)]
                occurrences.setdefault(_edge(start, end), []).append((start, end))
        directed = [items[0] for items in occurrences.values() if len(items) == 1]
        if not directed:
            return []
        outgoing = {}
        incoming = {}
        for start, end in directed:
            if start in outgoing or end in incoming:
                raise ValueError("The control cage has a branched or inconsistent boundary")
            outgoing[start] = end
            incoming[end] = start
        if set(outgoing) != set(incoming):
            raise ValueError("The control cage has an open boundary chain")

        loops = []
        unvisited = set(outgoing)
        while unvisited:
            first = min(unvisited)
            loop = []
            current = first
            while current not in loop:
                if current not in unvisited:
                    raise ValueError("Control-cage boundaries intersect")
                loop.append(current)
                unvisited.remove(current)
                current = outgoing[current]
            if current != first:
                raise ValueError("Control-cage boundaries intersect")
            loops.append(tuple(loop))
        return loops

    def face_index(self, vertex_indices):
        """Return the face matching *vertex_indices*, independent of winding."""
        target = frozenset(int(index) for index in vertex_indices)
        matches = [index for index, face in enumerate(self.faces) if frozenset(face) == target]
        return matches[0] if len(matches) == 1 else None

    def edge_ring(self, start_edge):
        """Return consistently directed edges in the quad ring of *start_edge*."""
        start = _edge(*start_edge)
        edge_faces = {}
        for face_index, face in enumerate(self.faces):
            if len(face) != 4:
                raise ValueError("Edge-ring operations require quad faces")
            for position, first in enumerate(face):
                second = face[(position + 1) % 4]
                edge_faces.setdefault(_edge(first, second), []).append(face_index)
        if start not in edge_faces:
            raise ValueError("The selected edge is not part of the control cage")

        directed = {start: start}
        pending = [start]
        while pending:
            current = pending.pop()
            first, second = directed[current]
            for face_index in edge_faces[current]:
                face = self.faces[face_index]
                position = next(
                    index
                    for index, vertex in enumerate(face)
                    if _edge(vertex, face[(index + 1) % 4]) == current
                )
                face_first = face[position]
                face_second = face[(position + 1) % 4]
                if (first, second) == (face_first, face_second):
                    opposite = (face[(position + 3) % 4], face[(position + 2) % 4])
                else:
                    opposite = (face[(position + 2) % 4], face[(position + 3) % 4])
                opposite_key = _edge(*opposite)
                previous = directed.get(opposite_key)
                if previous is not None and previous != opposite:
                    raise ValueError("The edge ring has inconsistent orientation")
                if previous is None:
                    directed[opposite_key] = opposite
                    pending.append(opposite_key)
        return directed

    def insert_edge_ring(self, start_edge, position=0.5, mode="simple"):
        """Split every face crossed by an edge ring and return its new edge loop."""
        position = float(position)
        if not 0.0 < position < 1.0:
            raise ValueError("Insert position must be strictly between zero and one")
        if mode != "simple":
            raise ValueError(f"Unsupported edge insertion mode: {mode}")
        ring = self.edge_ring(start_edge)
        vertices = list(self.vertices)
        vertex_sharpness = list(self.vertex_sharpness)
        inserted_vertices = {}
        for edge, (first, second) in ring.items():
            point = tuple(
                self.vertices[first][axis] * (1.0 - position)
                + self.vertices[second][axis] * position
                for axis in range(3)
            )
            inserted_vertices[edge] = len(vertices)
            vertices.append(point)
            vertex_sharpness.append(
                self.vertex_sharpness[first] * (1.0 - position)
                + self.vertex_sharpness[second] * position
            )

        faces = []
        inserted_edges = set()
        for face in self.faces:
            split_positions = [
                index
                for index, first in enumerate(face)
                if _edge(first, face[(index + 1) % 4]) in ring
            ]
            if not split_positions:
                faces.append(face)
                continue
            if len(split_positions) != 2 or (split_positions[1] - split_positions[0]) % 2:
                raise ValueError("The edge ring does not cross opposite sides of a quad")
            start = split_positions[0]
            rotated = tuple(face[(start + offset) % 4] for offset in range(4))
            first_new = inserted_vertices[_edge(rotated[0], rotated[1])]
            second_new = inserted_vertices[_edge(rotated[2], rotated[3])]
            faces.append((rotated[0], first_new, second_new, rotated[3]))
            faces.append((first_new, rotated[1], rotated[2], second_new))
            inserted_edges.add(_edge(first_new, second_new))

        edge_sharpness = {}
        for edge, value in self.edge_sharpness.items():
            inserted = inserted_vertices.get(edge)
            if inserted is None:
                edge_sharpness[edge] = value
                continue
            edge_sharpness[_edge(edge[0], inserted)] = value
            edge_sharpness[_edge(inserted, edge[1])] = value
        return (
            ControlCage(vertices, faces, vertex_sharpness, edge_sharpness),
            inserted_edges,
        )

    def extrude_face(self, face_index, keep_creases=False):
        """Duplicate one quad face and connect it with one ring of side quads."""
        cage, tops, side_faces = self.extrude_faces({face_index}, keep_creases)
        return cage, tops[0], side_faces

    def _face_extrusion_region(self, face_indices):
        selected = {int(face_index) for face_index in face_indices}
        if not selected or any(
            face_index < 0 or face_index >= len(self.faces) for face_index in selected
        ):
            raise ValueError("No valid control faces were selected")
        if any(len(self.faces[face_index]) != 4 for face_index in selected):
            raise ValueError("Face extrusion currently requires quad faces")

        selected_edges = {}
        face_edges = {}
        for face_index in selected:
            face = self.faces[face_index]
            edges = {
                _edge(start, face[(position + 1) % len(face)])
                for position, start in enumerate(face)
            }
            face_edges[face_index] = edges
            for edge in edges:
                selected_edges[edge] = selected_edges.get(edge, 0) + 1

        visited = {next(iter(selected))}
        while True:
            connected = {
                face_index
                for face_index in selected - visited
                if any(
                    face_edges[face_index].intersection(face_edges[current]) for current in visited
                )
            }
            if not connected:
                break
            visited.update(connected)
        if visited != selected:
            raise ValueError("Face extrusion requires one edge-connected region")

        perimeter = {edge for edge, count in selected_edges.items() if count == 1}
        if not perimeter:
            raise ValueError("The selected face region has no extrusion boundary")
        return selected, selected_edges, perimeter

    def can_extrude_faces(self, face_indices):
        """Return whether the face IDs form one extrudable quad region."""
        try:
            self._face_extrusion_region(face_indices)
        except ValueError:
            return False
        return True

    def extrude_faces(self, face_indices, keep_creases=False):
        """Extrude one connected quad region with sides only on its perimeter."""
        selected, selected_edges, perimeter = self._face_extrusion_region(face_indices)

        vertices = list(self.vertices)
        vertex_sharpness = list(self.vertex_sharpness)
        duplicate = {}
        for index in sorted(
            {vertex for face_index in selected for vertex in self.faces[face_index]}
        ):
            duplicate[index] = len(vertices)
            vertices.append(self.vertices[index])
            vertex_sharpness.append(self.vertex_sharpness[index] if keep_creases else 0.0)

        faces = list(self.faces)
        tops = []
        oriented_perimeter = {}
        for face_index in sorted(selected):
            face = self.faces[face_index]
            top = tuple(duplicate[index] for index in face)
            faces[face_index] = top
            tops.append(top)
            for position, start in enumerate(face):
                end = face[(position + 1) % len(face)]
                edge = _edge(start, end)
                if edge in perimeter:
                    oriented_perimeter[edge] = (start, end)

        first_side = len(faces)
        for edge in sorted(perimeter):
            start, end = oriented_perimeter[edge]
            faces.append((start, end, duplicate[end], duplicate[start]))

        edge_sharpness = dict(self.edge_sharpness)
        if keep_creases:
            for edge in selected_edges:
                value = self.edge_sharpness.get(edge, 0.0)
                if value:
                    edge_sharpness[_edge(duplicate[edge[0]], duplicate[edge[1]])] = value

        result = ControlCage(vertices, faces, vertex_sharpness, edge_sharpness)
        used = sorted({index for face in result.faces for index in face})
        remap = {old: new for new, old in enumerate(used)}
        compacted = result.compacted()
        return (
            compacted,
            tuple(tuple(remap[index] for index in top) for top in tops),
            tuple(range(first_side, len(faces))),
        )

    def extrude_boundary_edges(self, edges, keep_creases=False):
        """Duplicate selected boundary edges and connect them with quads."""
        selected = {_edge(int(edge[0]), int(edge[1])) for edge in edges}
        boundary = set(self.boundary_edges)
        if not selected or not selected.issubset(boundary):
            raise ValueError("Edge extrusion requires open boundary control edges")

        oriented = {}
        for face in self.faces:
            for position, start in enumerate(face):
                end = face[(position + 1) % len(face)]
                edge = _edge(start, end)
                if edge in selected:
                    oriented[edge] = (start, end)
        if set(oriented) != selected:
            raise ValueError("Could not orient the selected boundary edges")

        vertices = list(self.vertices)
        vertex_sharpness = list(self.vertex_sharpness)
        duplicate = {}
        for index in sorted({vertex for edge in selected for vertex in edge}):
            duplicate[index] = len(vertices)
            vertices.append(self.vertices[index])
            vertex_sharpness.append(self.vertex_sharpness[index] if keep_creases else 0.0)

        faces = list(self.faces)
        outer_edges = set()
        first_side = len(faces)
        for edge in sorted(selected):
            start, end = oriented[edge]
            outer = _edge(duplicate[start], duplicate[end])
            outer_edges.add(outer)
            # The existing face traverses start -> end. The new quad must use
            # the shared edge in the opposite direction to preserve winding.
            faces.append((end, start, duplicate[start], duplicate[end]))

        edge_sharpness = dict(self.edge_sharpness)
        if keep_creases:
            for edge in selected:
                value = self.edge_sharpness.get(edge, 0.0)
                if value:
                    edge_sharpness[_edge(duplicate[edge[0]], duplicate[edge[1]])] = value
        return (
            ControlCage(vertices, faces, vertex_sharpness, edge_sharpness),
            outer_edges,
            tuple(range(first_side, len(faces))),
        )

    def delete_faces(self, face_indices):
        """Return a compacted cage without the indexed faces.

        Removing unused vertices here establishes the index-remapping behavior
        needed by all later destructive topology operations.
        """
        removed = {int(index) for index in face_indices}
        if not removed or any(index < 0 or index >= len(self.faces) for index in removed):
            raise ValueError("No valid control faces were selected")
        remaining_faces = [face for index, face in enumerate(self.faces) if index not in removed]
        if not remaining_faces:
            raise ValueError("Deleting every face would leave an empty control cage")

        return ControlCage(
            self.vertices,
            remaining_faces,
            self.vertex_sharpness,
            self.edge_sharpness,
        ).compacted()

    def compacted(self):
        """Return this cage with unused vertices and stale sharpness removed."""
        used_vertices = sorted({index for face in self.faces for index in face})
        remap = {old: new for new, old in enumerate(used_vertices)}
        vertices = [self.vertices[index] for index in used_vertices]
        faces = [tuple(remap[index] for index in face) for face in self.faces]
        vertex_sharpness = [self.vertex_sharpness[index] for index in used_vertices]
        valid_edges = set(cage_edges(faces))
        edge_sharpness = {}
        for old_edge, value in self.edge_sharpness.items():
            if old_edge[0] not in remap or old_edge[1] not in remap:
                continue
            new_edge = _edge(remap[old_edge[0]], remap[old_edge[1]])
            if new_edge in valid_edges:
                edge_sharpness[new_edge] = value
        return ControlCage(vertices, faces, vertex_sharpness, edge_sharpness)

    def split_along_edges(self, selected_edges):
        """Separate the cage into the two face regions divided by a seam.

        The seam itself is not deleted from either result.  Each result owns
        its own compacted copy of those controls, turning the former internal
        edges into matching free boundaries.  This makes the operation a true
        topological inverse of :meth:`weld_boundary`, rather than a hidden or
        creased edge treatment.
        """
        seam = {_edge(*edge) for edge in selected_edges}
        counts = self.edge_counts()
        if not seam:
            raise ValueError("No control segment was selected")
        if any(counts.get(edge) != 2 for edge in seam):
            raise ValueError("Unweld requires an internal control segment")

        edge_faces = {}
        for face_index, face in enumerate(self.faces):
            for position, start in enumerate(face):
                edge = _edge(start, face[(position + 1) % len(face)])
                edge_faces.setdefault(edge, []).append(face_index)

        adjacency = {index: set() for index in range(len(self.faces))}
        for edge, adjacent in edge_faces.items():
            if edge in seam or len(adjacent) != 2:
                continue
            first, second = adjacent
            adjacency[first].add(second)
            adjacency[second].add(first)

        components = []
        remaining = set(adjacency)
        while remaining:
            component = set()
            pending = [min(remaining)]
            while pending:
                face_index = pending.pop()
                if face_index in component:
                    continue
                component.add(face_index)
                pending.extend(adjacency[face_index].difference(component))
            remaining.difference_update(component)
            components.append(component)
        if len(components) != 2:
            raise ValueError("The selected segment does not divide the Form into two sides")

        for edge in seam:
            adjacent = edge_faces[edge]
            if not all(any(face in component for face in adjacent) for component in components):
                raise ValueError("The selected edges do not form one separating segment")

        results = []
        for component in components:
            faces = [self.faces[index] for index in sorted(component)]
            valid_edges = set(cage_edges(faces))
            results.append(
                ControlCage(
                    self.vertices,
                    faces,
                    self.vertex_sharpness,
                    {
                        edge: value
                        for edge, value in self.edge_sharpness.items()
                        if edge in valid_edges
                    },
                ).compacted()
            )
        return tuple(results)

    def weld_boundary(self, other, first_edge, second_edge):
        """Identify two equal boundary loops and return their joined cage.

        ``other`` must already use this cage's coordinate system.  Boundary
        controls are paired in opposite winding, moved to their midpoint, and
        shared by the faces on both sides.  No bridge faces are introduced.
        """
        if not isinstance(other, ControlCage):
            raise TypeError("Weld requires another control cage")
        first_edge = _edge(*first_edge)
        second_edge = _edge(*second_edge)

        def containing_loop(cage, edge):
            for loop in cage.boundary_loops():
                edges = {
                    _edge(loop[index], loop[(index + 1) % len(loop)])
                    for index in range(len(loop))
                }
                if edge in edges:
                    return tuple(loop)
            raise ValueError("The selected weld edge is not on a free boundary")

        first = containing_loop(self, first_edge)
        second = tuple(reversed(containing_loop(other, second_edge)))
        if len(first) != len(second):
            raise ValueError("Weld boundary loops require equal vertex counts")

        def alignment_cost(offset):
            return sum(
                sum(
                    (
                        self.vertices[first[index]][axis]
                        - other.vertices[second[(index + offset) % len(second)]][axis]
                    ) ** 2
                    for axis in range(3)
                )
                for index in range(len(first))
            )

        offset = min(range(len(second)), key=alignment_cost)
        second = tuple(second[(index + offset) % len(second)] for index in range(len(second)))
        seam_map = dict(zip(second, first))

        vertices = list(self.vertices)
        vertex_sharpness = list(self.vertex_sharpness)
        remap = {}
        for index, point in enumerate(other.vertices):
            if index in seam_map:
                target = seam_map[index]
                vertices[target] = tuple(
                    (vertices[target][axis] + point[axis]) * 0.5 for axis in range(3)
                )
                vertex_sharpness[target] = max(
                    vertex_sharpness[target], other.vertex_sharpness[index]
                )
                remap[index] = target
            else:
                remap[index] = len(vertices)
                vertices.append(point)
                vertex_sharpness.append(other.vertex_sharpness[index])

        faces = list(self.faces)
        face_sets = {frozenset(face) for face in faces}
        for face in other.faces:
            mapped = tuple(remap[index] for index in face)
            key = frozenset(mapped)
            if key not in face_sets:
                faces.append(mapped)
                face_sets.add(key)

        edge_sharpness = dict(self.edge_sharpness)
        for edge, value in other.edge_sharpness.items():
            mapped = _edge(remap[edge[0]], remap[edge[1]])
            edge_sharpness[mapped] = max(edge_sharpness.get(mapped, 0.0), value)
        return ControlCage(vertices, faces, vertex_sharpness, edge_sharpness).compacted()

    def disjoint_union(self, other):
        """Return both cages in one Form without identifying any controls."""
        if not isinstance(other, ControlCage):
            raise TypeError("A cage union requires another control cage")
        offset = len(self.vertices)
        edge_sharpness = dict(self.edge_sharpness)
        edge_sharpness.update(
            {
                _edge(edge[0] + offset, edge[1] + offset): value
                for edge, value in other.edge_sharpness.items()
            }
        )
        return ControlCage(
            self.vertices + other.vertices,
            self.faces
            + [tuple(index + offset for index in face) for face in other.faces],
            self.vertex_sharpness + other.vertex_sharpness,
            edge_sharpness,
        )

    def connected_components(self):
        """Return compact cages for every edge-connected face component."""
        component_indices = self.face_components()
        components = []
        for component in component_indices:
            faces = [self.faces[index] for index in sorted(component)]
            valid_edges = set(cage_edges(faces))
            components.append(
                ControlCage(
                    self.vertices,
                    faces,
                    self.vertex_sharpness,
                    {
                        edge: value
                        for edge, value in self.edge_sharpness.items()
                        if edge in valid_edges
                    },
                ).compacted()
            )
        return tuple(components)

    def face_components(self):
        """Return the original face-index sets of each connected component."""
        edge_faces = {}
        for face_index, face in enumerate(self.faces):
            for position, start in enumerate(face):
                edge = _edge(start, face[(position + 1) % len(face)])
                edge_faces.setdefault(edge, []).append(face_index)
        adjacency = {index: set() for index in range(len(self.faces))}
        for adjacent in edge_faces.values():
            if len(adjacent) == 2:
                first, second = adjacent
                adjacency[first].add(second)
                adjacency[second].add(first)

        components = []
        remaining = set(adjacency)
        while remaining:
            component = set()
            pending = [min(remaining)]
            while pending:
                face_index = pending.pop()
                if face_index in component:
                    continue
                component.add(face_index)
                pending.extend(adjacency[face_index].difference(component))
            remaining.difference_update(component)
            components.append(frozenset(component))
        return tuple(components)

    def erase_and_fill(self, face_indices):
        """Remove a face region and minimally quadrangulate its exposed boundary.

        This is an atomic cage edit rather than Delete followed by a global Fill:
        only boundary loops exposed by the removed region are rebuilt.  In
        particular, erasing an extrusion cap and its side ring restores the
        original cage without filling unrelated pre-existing openings.
        """
        removed = {int(index) for index in face_indices}
        if not removed or any(index < 0 or index >= len(self.faces) for index in removed):
            raise ValueError("No valid control faces were selected")
        remaining_faces = [face for index, face in enumerate(self.faces) if index not in removed]
        if not remaining_faces:
            raise ValueError("Erasing every face would leave an empty control cage")

        edge_regions = {}
        for face_index, face in enumerate(self.faces):
            region = face_index in removed
            for position, start in enumerate(face):
                edge = _edge(start, face[(position + 1) % len(face)])
                edge_regions.setdefault(edge, set()).add(region)
        interface_edges = {
            edge for edge, regions in edge_regions.items() if regions == {False, True}
        }
        if not interface_edges:
            raise ValueError("The selected faces do not border retained cage faces")

        retained_edges = set(cage_edges(remaining_faces))
        temporary = ControlCage(
            self.vertices,
            remaining_faces,
            self.vertex_sharpness,
            {edge: value for edge, value in self.edge_sharpness.items() if edge in retained_edges},
        )
        exposed = interface_edges.intersection(temporary.boundary_edges)
        if not exposed:
            raise ValueError("Erasing the selected faces did not expose a fillable boundary")
        return temporary.fill_boundaries(exposed, mode="minimal").compacted()

    def merge_vertices(self, vertex_indices, position=None):
        """Merge exactly two controls and remove resulting degenerate faces.

        Edge collapse and vertex welding deliberately share this primitive so
        index remapping, sharpness preservation, and manifold validation cannot
        diverge as the tool set grows.
        """
        selected = sorted({int(index) for index in vertex_indices})
        if len(selected) != 2 or any(
            index < 0 or index >= len(self.vertices) for index in selected
        ):
            raise ValueError("Vertex merging requires exactly two valid controls")
        keep, remove = selected
        if position is None:
            merged_point = tuple(
                (self.vertices[keep][axis] + self.vertices[remove][axis]) * 0.5 for axis in range(3)
            )
        else:
            if len(position) != 3:
                raise ValueError("The weld position requires three coordinates")
            merged_point = tuple(float(component) for component in position)

        vertices = list(self.vertices)
        vertices[keep] = merged_point
        vertex_sharpness = list(self.vertex_sharpness)
        vertex_sharpness[keep] = max(vertex_sharpness[keep], vertex_sharpness[remove])
        faces = []
        face_sets = set()
        for face in self.faces:
            mapped = [keep if index == remove else index for index in face]
            cleaned = []
            for index in mapped:
                if not cleaned or cleaned[-1] != index:
                    cleaned.append(index)
            if len(cleaned) > 1 and cleaned[0] == cleaned[-1]:
                cleaned.pop()
            if len(cleaned) < 3:
                continue
            if len(set(cleaned)) != len(cleaned):
                raise ValueError("The merge would create a self-intersecting control face")
            candidate = tuple(cleaned)
            candidate_set = frozenset(candidate)
            if candidate_set in face_sets:
                continue
            faces.append(candidate)
            face_sets.add(candidate_set)
        if not faces:
            raise ValueError("The merge would remove every control face")

        edge_sharpness = {}
        for edge, value in self.edge_sharpness.items():
            first = keep if edge[0] == remove else edge[0]
            second = keep if edge[1] == remove else edge[1]
            if first == second:
                continue
            merged_edge = _edge(first, second)
            edge_sharpness[merged_edge] = max(edge_sharpness.get(merged_edge, 0.0), value)
        return ControlCage(
            vertices,
            faces,
            vertex_sharpness,
            edge_sharpness,
        ).compacted()

    def thickened(self, distance, sharp=True):
        """Offset an open cage and connect all boundaries with quad walls.

        This is a control-topology operation, not an OCC shell offset.  Both
        skins and every connecting wall therefore remain editable after the
        operation.  The signed distance follows the area-weighted vertex
        normals of the source cage.
        """
        distance = float(distance)
        if abs(distance) <= 1.0e-9:
            raise ValueError("Thicken requires a non-zero distance")
        if self.is_closed:
            raise ValueError("Thicken currently requires an open Form surface")
        boundary_loops = self.boundary_loops()
        if not boundary_loops:
            raise ValueError("Thicken requires at least one free boundary")
        if any(len(face) != 4 for face in self.faces):
            raise ValueError("Thicken currently requires a quad control cage")

        normals = [[0.0, 0.0, 0.0] for _point in self.vertices]
        for face in self.faces:
            origin = self.vertices[face[0]]
            face_normal = [0.0, 0.0, 0.0]
            for index in range(1, len(face) - 1):
                first = self.vertices[face[index]]
                second = self.vertices[face[index + 1]]
                first_edge = tuple(first[axis] - origin[axis] for axis in range(3))
                second_edge = tuple(second[axis] - origin[axis] for axis in range(3))
                face_normal[0] += first_edge[1] * second_edge[2] - first_edge[2] * second_edge[1]
                face_normal[1] += first_edge[2] * second_edge[0] - first_edge[0] * second_edge[2]
                face_normal[2] += first_edge[0] * second_edge[1] - first_edge[1] * second_edge[0]
            for vertex in face:
                for axis in range(3):
                    normals[vertex][axis] += face_normal[axis]

        unit_normals = []
        for normal in normals:
            length = math.sqrt(sum(component * component for component in normal))
            if length <= 1.0e-12:
                raise ValueError("Thicken cannot offset a degenerate control vertex")
            unit_normals.append(tuple(component / length for component in normal))

        source_count = len(self.vertices)
        vertices = list(self.vertices)
        vertices.extend(
            tuple(point[axis] + distance * unit_normals[index][axis] for axis in range(3))
            for index, point in enumerate(self.vertices)
        )
        duplicate = lambda index: index + source_count
        if distance > 0.0:
            faces = [tuple(reversed(face)) for face in self.faces]
            faces.extend(tuple(duplicate(index) for index in face) for face in self.faces)
        else:
            faces = list(self.faces)
            faces.extend(tuple(duplicate(index) for index in reversed(face)) for face in self.faces)

        boundary_edges = set()
        for loop in boundary_loops:
            for index, start in enumerate(loop):
                end = loop[(index + 1) % len(loop)]
                boundary_edges.add(_edge(start, end))
                wall = (start, end, duplicate(end), duplicate(start))
                faces.append(wall if distance > 0.0 else tuple(reversed(wall)))

        edge_sharpness = dict(self.edge_sharpness)
        edge_sharpness.update(
            {
                _edge(duplicate(edge[0]), duplicate(edge[1])): value
                for edge, value in self.edge_sharpness.items()
            }
        )
        if sharp:
            for edge in boundary_edges:
                edge_sharpness[edge] = 10.0
                edge_sharpness[_edge(duplicate(edge[0]), duplicate(edge[1]))] = 10.0
        return ControlCage(
            vertices,
            faces,
            self.vertex_sharpness + self.vertex_sharpness,
            edge_sharpness,
        )

    def fill_boundaries(self, selected_edges, mode="automatic"):
        """Return a cage with the selected boundary loops filled by quads.

        Four-sided boundaries retain the original topology as one quad.  Larger
        even boundaries use a reduced-star layout around one new control point.
        This keeps the cage quad-only for the current Catmull-Clark/BRep path.
        """
        if mode not in ("automatic", "single", "minimal", "reduced_star"):
            raise ValueError(f"Unsupported fill mode: {mode}")
        requested = {_edge(*edge) for edge in selected_edges}
        if not requested:
            raise ValueError("No boundary edge was selected")
        loops = self.boundary_loops()
        chosen = [
            loop
            for loop in loops
            if requested.intersection(
                {_edge(loop[index], loop[(index + 1) % len(loop)]) for index in range(len(loop))}
            )
        ]
        if not chosen:
            raise ValueError("The selected edges do not belong to a free boundary")

        vertices = list(self.vertices)
        faces = list(self.faces)
        vertex_sharpness = list(self.vertex_sharpness)
        for boundary in chosen:
            # Reverse the existing surface-boundary winding so shared edges have
            # opposite orientations in the new faces.
            fill = tuple(reversed(boundary))
            if len(fill) == 4 and mode != "reduced_star":
                faces.append(fill)
                continue
            if mode == "single":
                raise ValueError("Single-face fill requires a four-edge boundary")
            if len(fill) < 4 or len(fill) % 2:
                raise ValueError("Quad fill requires an even boundary with at least four edges")
            if mode == "minimal":
                for index in range(1, len(fill) - 2, 2):
                    faces.append(
                        (
                            fill[0],
                            fill[index],
                            fill[index + 1],
                            fill[index + 2],
                        )
                    )
                continue
            center = tuple(
                sum(vertices[index][axis] for index in fill) / len(fill) for axis in range(3)
            )
            center_index = len(vertices)
            vertices.append(center)
            vertex_sharpness.append(0.0)
            for index in range(0, len(fill), 2):
                faces.append(
                    (
                        center_index,
                        fill[index],
                        fill[(index + 1) % len(fill)],
                        fill[(index + 2) % len(fill)],
                    )
                )
        return ControlCage(
            vertices,
            faces,
            vertex_sharpness,
            self.edge_sharpness,
        )

    def bridge_boundaries(self, selected_edges):
        """Connect exactly two selected equal-sized boundary loops with quads."""
        requested = {_edge(*edge) for edge in selected_edges}
        if not requested:
            raise ValueError("No boundary edges were selected")
        loops = []
        for loop in self.boundary_loops():
            loop_edges = {
                _edge(loop[index], loop[(index + 1) % len(loop)]) for index in range(len(loop))
            }
            if requested.intersection(loop_edges):
                loops.append(tuple(loop))
        if len(loops) != 2:
            raise ValueError("Bridge requires edges from exactly two boundary loops")
        first, second = loops
        if len(first) != len(second):
            raise ValueError("Bridge boundary loops require equal vertex counts")
        second = tuple(reversed(second))

        def alignment_cost(offset):
            return sum(
                sum(
                    (
                        self.vertices[first[index]][axis]
                        - self.vertices[second[(index + offset) % len(second)]][axis]
                    )
                    ** 2
                    for axis in range(3)
                )
                for index in range(len(first))
            )

        offset = min(range(len(second)), key=alignment_cost)
        second = tuple(second[(index + offset) % len(second)] for index in range(len(second)))
        faces = list(self.faces)
        for index in range(len(first)):
            next_index = (index + 1) % len(first)
            faces.append(
                (
                    first[next_index],
                    first[index],
                    second[index],
                    second[next_index],
                )
            )
        return ControlCage(
            self.vertices,
            faces,
            self.vertex_sharpness,
            self.edge_sharpness,
        )


def update_object_shape(obj):
    """Regenerate a solid or surface according to the cage boundary topology."""
    try:
        # Match is part of conversion: a deleted or invalid support must produce
        # a normal failed-conversion state instead of escaping Feature::execute
        # and leaving a stale shape in the document.
        from .additive import apply_match_constraints

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
        grids = catmull_clark_patch_grids(
            cage.vertices,
            cage.faces,
            level,
            cage.edge_sharpness,
            cage.vertex_sharpness,
        )
        result = [None] * len(cage.vertices)
        for face, grid in zip(cage.faces, grids):
            corners = (grid[0][0], grid[-1][0], grid[-1][-1], grid[0][-1])
            for index, point in zip(face, corners):
                result[index] = App.Vector(*point)
        if all(point is not None for point in result):
            return result
    except ValueError:
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
