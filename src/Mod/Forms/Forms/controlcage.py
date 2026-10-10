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

"""Editable polygon cage data and topology operations."""

from collections import Counter
import math

import FreeCAD as App

from .topology import cage_edges, validate_manifold_boundary


def _edge(start, end):
    return tuple(sorted((int(start), int(end))))


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

    def bridge_boundaries(self, selected_edges, allow_unequal=False):
        """Connect two selected boundary loops, optionally with a density transition."""
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
        if len(first) != len(second) and not allow_unequal:
            raise ValueError("Bridge boundary loops require equal vertex counts")
        if (len(first) + len(second)) % 2:
            raise ValueError("A quad bridge requires an even total boundary edge count")
        second = tuple(reversed(second))

        def alignment_cost(offset):
            cost = 0.0
            for index, vertex in enumerate(first):
                position = index * len(second) / len(first)
                left = int(position)
                fraction = position - left
                a = self.vertices[second[(left + offset) % len(second)]]
                b = self.vertices[second[(left + offset + 1) % len(second)]]
                cost += sum((self.vertices[vertex][axis]
                             - ((1.0 - fraction) * a[axis] + fraction * b[axis])) ** 2
                            for axis in range(3))
            return cost

        offset = min(range(len(second)), key=alignment_cost)
        second = tuple(second[(index + offset) % len(second)] for index in range(len(second)))
        vertices = list(self.vertices)
        faces = list(self.faces)
        if len(first) != len(second):
            # Give every vertex of the denser perimeter a transverse edge.
            # Otherwise a two-edge transition can leave a cap corner with
            # only two incident faces, which the subdivision fan cannot use.
            dense, other = (first, second) if len(first) > len(second) else (second, first)
            ring = []
            for index, vertex in enumerate(dense):
                position = index * len(other) / len(dense)
                left = int(position)
                fraction = position - left
                a, b = vertices[other[left]], vertices[other[(left + 1) % len(other)]]
                ring.append(len(vertices))
                vertices.append(tuple((vertices[vertex][axis]
                                       + (1.0 - fraction) * a[axis] + fraction * b[axis]) / 2.0
                                      for axis in range(3)))
            for index in range(len(dense)):
                following = (index + 1) % len(dense)
                if len(first) > len(second):
                    faces.append((dense[following], dense[index], ring[index], ring[following]))
                else:
                    faces.append((ring[following], ring[index], dense[index], dense[following]))
            if len(first) > len(second):
                first = tuple(ring)
            else:
                second = tuple(ring)
        # Each quad consumes two boundary edges: one on each loop, or two
        # on the denser loop at a transition. Even total perimeter is the
        # parity condition for quadrangulating this annular strip.
        steps = (len(first) + len(second)) // 2
        i = j = 0
        for step in range(1, steps + 1):
            next_i = (2 * step * len(first) + steps) // (2 * steps)
            next_j = 2 * step - next_i
            faces.append(tuple(first[k % len(first)] for k in range(next_i, i - 1, -1))
                         + tuple(second[k % len(second)] for k in range(j, next_j + 1)))
            i, j = next_i, next_j
        return ControlCage(
            vertices,
            faces,
            self.vertex_sharpness,
            self.edge_sharpness,
        )
