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

"""Persistent hierarchical T-mesh topology for locally refinable Forms.

A face always has four *parametric* sides.  A regular side contains its two
corner vertices; a T-side may contain additional vertices between those
corners. Keeping sides explicit avoids pretending that a T-face is an n-gon
and preserves the rectangular parameter domain used by the evaluator.
"""

from dataclasses import dataclass
import json
import math

from .topology import validate_manifold_boundary
from .limits import check_topology


def _edge(first, second):
    return tuple(sorted((int(first), int(second))))


def _parameter_level(*points):
    """Return the smallest dyadic level containing all parameter points."""
    for level in range(31):
        scale = 1 << level
        if all(
            math.isclose(value * scale, round(value * scale), rel_tol=0.0, abs_tol=1.0e-10)
            for point in points
            for value in point
        ):
            return level
    raise ValueError("T-mesh parameters exceed the supported dyadic depth")


@dataclass(frozen=True)
class TMeshFace:
    """One stable-ID rectangular leaf, possibly with T-points on its sides."""

    id: int
    sides: tuple
    root: int = -1
    parameters: tuple = ()
    level: int = 0

    def __post_init__(self):
        object.__setattr__(self, "id", int(self.id))
        object.__setattr__(
            self,
            "sides",
            tuple(tuple(int(vertex) for vertex in side) for side in self.sides),
        )
        object.__setattr__(self, "root", int(self.root if self.root >= 0 else self.id))
        parameters = self.parameters or ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))
        object.__setattr__(
            self,
            "parameters",
            tuple(tuple(float(value) for value in point) for point in parameters),
        )
        object.__setattr__(self, "level", int(self.level))

    @property
    def corners(self):
        return tuple(side[0] for side in self.sides)

    @property
    def boundary(self):
        return tuple(vertex for side in self.sides for vertex in side[:-1])

    @property
    def t_vertices(self):
        return tuple(vertex for side in self.sides for vertex in side[1:-1])

    @classmethod
    def from_corners(cls, face_id, corners, root=None, parameters=None, level=0):
        corners = tuple(int(vertex) for vertex in corners)
        if len(corners) != 4:
            raise ValueError("A regular T-mesh face requires four corners")
        return cls(
            face_id,
            tuple((corner, corners[(index + 1) % 4]) for index, corner in enumerate(corners)),
            face_id if root is None else root,
            parameters or (),
            level,
        )


class HierarchicalTMesh:
    """Stable leaf topology for a dyadically refined subdivision surface.

    The logical mesh may contain several T-points on a face. Surface evaluation
    remains well-defined because every edit is represented at a nested uniform
    Catmull-Clark level; the T-mesh determines which controls and faces are
    exposed rather than defining a second incompatible surface basis.
    """

    VERSION = 3

    def __init__(
        self,
        vertices,
        faces,
        edge_intervals=None,
        vertex_levels=None,
        next_vertex_id=None,
        next_face_id=None,
        control_locations=None,
    ):
        self.vertices = {
            int(vertex_id): tuple(float(value) for value in point)
            for vertex_id, point in dict(vertices).items()
        }
        self.vertex_levels = {
            int(vertex_id): int(level) for vertex_id, level in dict(vertex_levels or {}).items()
        }
        for vertex_id in self.vertices:
            self.vertex_levels.setdefault(vertex_id, 0)
        self.faces = {}
        for face in faces:
            face = face if isinstance(face, TMeshFace) else TMeshFace(*face)
            if face.id in self.faces:
                raise ValueError("T-mesh face IDs must be unique")
            self.faces[face.id] = face
        edges = self.atomic_edges()
        supplied = {_edge(*edge): float(value) for edge, value in (edge_intervals or {}).items()}
        if set(supplied).difference(edges):
            raise ValueError("A knot interval references an unknown T-mesh edge")
        self.edge_intervals = {edge: supplied.get(edge, 1.0) for edge in edges}
        self.next_vertex_id = int(
            next_vertex_id if next_vertex_id is not None else (max(self.vertices, default=-1) + 1)
        )
        self.next_face_id = int(
            next_face_id if next_face_id is not None else (max(self.faces, default=-1) + 1)
        )
        # Evaluator controls survive deletion/dissolution of their selectable
        # edges. Keep their root coordinates independently of leaf boundaries.
        self.control_locations = {
            int(vertex): [tuple(location) for location in locations]
            for vertex, locations in (control_locations or {}).items()
        }
        self.validate()

    @classmethod
    def from_quad_cage(cls, vertices, faces):
        return cls(
            enumerate(vertices),
            [TMeshFace.from_corners(index, face) for index, face in enumerate(faces)],
        )

    def copy(self):
        return HierarchicalTMesh(
            self.vertices,
            list(self.faces.values()),
            self.edge_intervals,
            self.vertex_levels,
            self.next_vertex_id,
            self.next_face_id,
            self.parameter_locations(),
        )

    def atomic_edges(self):
        edges = set()
        for face in self.faces.values():
            for side in face.sides:
                edges.update(_edge(first, second) for first, second in zip(side, side[1:]))
        return edges

    def edge_counts(self):
        counts = {}
        for face in self.faces.values():
            for side in face.sides:
                for first, second in zip(side, side[1:]):
                    edge = _edge(first, second)
                    counts[edge] = counts.get(edge, 0) + 1
        return counts

    @property
    def is_closed(self):
        counts = self.edge_counts()
        return bool(counts) and all(count == 2 for count in counts.values())

    def edge_loop(self, start_edge):
        """Follow an edge chain through regular vertices and split face sides."""
        start_edge = _edge(*start_edge)
        edge_faces, vertex_edges, continuations = {}, {}, {}
        for face_id, face in self.faces.items():
            for side in face.sides:
                for first, second in zip(side, side[1:]):
                    edge = _edge(first, second)
                    edge_faces.setdefault(edge, set()).add(face_id)
                    vertex_edges.setdefault(first, set()).add(edge)
                    vertex_edges.setdefault(second, set()).add(edge)
                # At a T-junction the two pieces of a coarse face side
                # continue each other; the terminating branch does not.
                for first, vertex, last in zip(side, side[1:], side[2:]):
                    incoming, outgoing = _edge(first, vertex), _edge(vertex, last)
                    continuations.setdefault((vertex, incoming), set()).add(outgoing)
                    continuations.setdefault((vertex, outgoing), set()).add(incoming)
        if start_edge not in edge_faces:
            return []
        result = {start_edge}
        for vertex in start_edge:
            incoming = start_edge
            while True:
                candidates = continuations.get((vertex, incoming))
                if candidates is None:
                    candidates = {edge for edge in vertex_edges[vertex]
                                  if edge != incoming
                                  and edge_faces[edge].isdisjoint(edge_faces[incoming])}
                if len(candidates) != 1:
                    break
                outgoing = next(iter(candidates))
                if outgoing in result:
                    break
                result.add(outgoing)
                vertex = outgoing[0] if outgoing[1] == vertex else outgoing[1]
                incoming = outgoing
        return sorted(result)

    def _side_interval(self, side):
        return sum(
            self.edge_intervals[_edge(first, second)] for first, second in zip(side, side[1:])
        )

    def validate(self):
        check_topology(len(self.faces))
        if not self.vertices or not self.faces:
            raise ValueError("A T-mesh requires vertices and faces")
        for vertex_id, point in self.vertices.items():
            if vertex_id < 0 or len(point) != 3 or not all(math.isfinite(v) for v in point):
                raise ValueError("T-mesh vertices require finite 3D coordinates")
            if self.vertex_levels.get(vertex_id, -1) < 0:
                raise ValueError("T-mesh vertices require non-negative refinement levels")
        if set(self.vertex_levels) != set(self.vertices):
            raise ValueError("Every T-mesh vertex requires one refinement level")
        for vertex, locations in self.control_locations.items():
            if vertex not in self.vertices or any(
                len(location) != 3
                or not all(math.isfinite(v) for v in location)
                or location[0] < 0 or int(location[0]) != location[0]
                or not all(0.0 <= v <= 1.0 for v in location[1:])
                for location in locations
            ):
                raise ValueError("Invalid persistent evaluator control location")
        for face_id, face in self.faces.items():
            if face_id != face.id or len(face.sides) != 4:
                raise ValueError("A T-mesh face requires a stable ID and four sides")
            if any(len(side) < 2 for side in face.sides):
                raise ValueError("A T-mesh side requires at least two vertices")
            if len(face.parameters) != 4 or any(
                len(point) != 2 or not all(math.isfinite(value) for value in point)
                for point in face.parameters
            ):
                raise ValueError("A T-mesh face requires four finite parameter corners")
            if face.level < 0:
                raise ValueError("A T-mesh face requires a non-negative refinement level")
            for index, side in enumerate(face.sides):
                if side[-1] != face.sides[(index + 1) % 4][0]:
                    raise ValueError("T-mesh face sides do not form a connected boundary")
                if any(vertex not in self.vertices for vertex in side):
                    raise ValueError("A T-mesh face references an unknown vertex")
            if len(set(face.boundary)) != len(face.boundary):
                raise ValueError("A T-mesh face repeats a boundary vertex")
        counts = self.edge_counts()
        if any(count > 2 for count in counts.values()):
            raise ValueError("The T-mesh contains a non-manifold edge")
        validate_manifold_boundary(counts, "T-mesh")
        if set(self.edge_intervals) != set(counts):
            raise ValueError("Every atomic T-mesh edge requires exactly one knot interval")
        if any(not math.isfinite(value) or value <= 0.0 for value in self.edge_intervals.values()):
            raise ValueError("T-mesh knot intervals must be finite and positive")
        for face in self.faces.values():
            totals = [self._side_interval(side) for side in face.sides]
            scale = max(totals)
            if not math.isclose(totals[0], totals[2], rel_tol=1.0e-9, abs_tol=scale * 1.0e-12):
                raise ValueError("Opposite T-mesh sides require equal knot intervals")
            if not math.isclose(totals[1], totals[3], rel_tol=1.0e-9, abs_tol=scale * 1.0e-12):
                raise ValueError("Opposite T-mesh sides require equal knot intervals")
        if self.next_vertex_id <= max(self.vertices) or self.next_face_id <= max(self.faces):
            raise ValueError("T-mesh ID allocators must be greater than existing IDs")

    def set_vertex(self, vertex_id, point):
        vertex_id = int(vertex_id)
        if vertex_id not in self.vertices:
            raise ValueError("Unknown T-mesh vertex")
        point = tuple(float(value) for value in point)
        if len(point) != 3 or not all(math.isfinite(value) for value in point):
            raise ValueError("T-mesh vertices require finite 3D coordinates")
        self.vertices[vertex_id] = point

    def parameter_locations(self):
        """Map each control ID to all ``(root, u, v)`` refinement locations."""
        result = {vertex_id: list(self.control_locations.get(vertex_id, ()))
                  for vertex_id in self.vertices}
        for face in self.faces.values():
            for side_index, side in enumerate(face.sides):
                first = face.parameters[side_index]
                second = face.parameters[(side_index + 1) % 4]
                total = self._side_interval(side)
                travelled = 0.0
                for position, vertex_id in enumerate(side):
                    if position:
                        travelled += self.edge_intervals[_edge(side[position - 1], vertex_id)]
                    fraction = travelled / total
                    location = (
                        face.root,
                        first[0] * (1.0 - fraction) + second[0] * fraction,
                        first[1] * (1.0 - fraction) + second[1] * fraction,
                    )
                    if not any(
                        root == location[0]
                        and math.isclose(u, location[1], abs_tol=1.0e-12)
                        and math.isclose(v, location[2], abs_tol=1.0e-12)
                        for root, u, v in result[vertex_id]
                    ):
                        result[vertex_id].append(location)
        return result

    def _faces_for_atomic_edge(self, face_id, first, second):
        target = _edge(first, second)
        matches = []
        for candidate_id, candidate in self.faces.items():
            if candidate_id == face_id:
                continue
            for side_index, side in enumerate(candidate.sides):
                for position, (start, end) in enumerate(zip(side, side[1:])):
                    if _edge(start, end) == target:
                        matches.append((candidate_id, side_index, position))
        if len(matches) > 1:
            raise ValueError("The selected side is non-manifold")
        return matches

    def _insert_side_vertex(self, face_id, side_index, position, vertex_id):
        face = self.faces[int(face_id)]
        sides = list(face.sides)
        side = list(sides[int(side_index)])
        side.insert(int(position) + 1, int(vertex_id))
        sides[int(side_index)] = tuple(side)
        self.faces[face.id] = TMeshFace(
            face.id, tuple(sides), face.root, face.parameters, face.level
        )

    def _split_interval(self, first, middle, second):
        old = _edge(first, second)
        value = self.edge_intervals.pop(old)
        self.edge_intervals[_edge(first, middle)] = value * 0.5
        self.edge_intervals[_edge(middle, second)] = value * 0.5

    def _side_midpoint(self, face_id, side_index, level):
        """Return ``(vertex, created)`` at half the side's knot interval."""
        face = self.faces[int(face_id)]
        side = face.sides[int(side_index)]
        intervals = [
            self.edge_intervals[_edge(first, second)] for first, second in zip(side, side[1:])
        ]
        target = sum(intervals) * 0.5
        accumulated = 0.0
        for position, (first, second, interval) in enumerate(zip(side, side[1:], intervals)):
            next_value = accumulated + interval
            if math.isclose(next_value, target, rel_tol=1.0e-10, abs_tol=1.0e-12):
                return side[position + 1], False
            if accumulated < target < next_value:
                fraction = (target - accumulated) / interval
                if not math.isclose(fraction, 0.5, rel_tol=0.0, abs_tol=1.0e-10):
                    raise ValueError("The requested split is not dyadic on this T-edge")
                vertex_id = self.next_vertex_id
                self.next_vertex_id += 1
                self.vertices[vertex_id] = tuple(
                    self.vertices[first][axis] * (1.0 - fraction)
                    + self.vertices[second][axis] * fraction
                    for axis in range(3)
                )
                self.vertex_levels[vertex_id] = int(level)
                neighbors = self._faces_for_atomic_edge(face.id, first, second)
                self._split_interval(first, vertex_id, second)
                self._insert_side_vertex(face.id, side_index, position, vertex_id)
                for neighbor_id, neighbor_side, neighbor_position in neighbors:
                    self._insert_side_vertex(
                        neighbor_id, neighbor_side, neighbor_position, vertex_id
                    )
                return vertex_id, True
            accumulated = next_value
        raise ValueError("Could not locate the midpoint of the T-edge")

    def insert_edge(self, face_id, edge, position=0.5):
        """Return a locally split mesh plus its new vertex and face IDs.

        Dyadic T-mesh subdivision requires an equal split.  The newly seeded
        vertex positions are geometric midpoints; the evaluator's knot-
        insertion transform will replace these seeds while preserving the
        limit surface.
        """
        result = self.copy()
        new_vertices, new_faces = result._insert_edge_in_place(face_id, edge, position)
        return result, new_vertices, new_faces

    def subdivide(self, face_ids, levels=1):
        """Dyadically subdivide selected logical leaves into four per level."""
        return self.subdivide_grid(face_ids, levels, levels)

    def delete_faces(self, face_ids):
        """Return a mesh without the selected logical leaves.

        Stable vertex IDs are retained so existing editable-control references
        remain valid. Knot intervals are reduced to the surviving atomic
        topology.
        """
        removed = {int(face_id) for face_id in face_ids}
        if not removed or not removed.issubset(self.faces):
            raise ValueError("No valid T-mesh faces were selected")
        if removed == set(self.faces):
            raise ValueError("Deleting every face would leave an empty T-mesh")
        result = self.copy()
        for face_id in removed:
            del result.faces[face_id]
        edges = result.atomic_edges()
        result.edge_intervals = {
            edge: value for edge, value in result.edge_intervals.items() if edge in edges
        }
        result.validate()
        return result

    def dissolve_edges(self, edges):
        """Merge rectangular leaf regions across selected internal seams."""
        selected = {_edge(*edge) for edge in edges}
        if not selected:
            raise ValueError("No T-mesh edges were selected")
        result = self.copy()
        edge_faces = {}
        for face_id, face in result.faces.items():
            for side in face.sides:
                for first, second in zip(side, side[1:]):
                    edge_faces.setdefault(_edge(first, second), []).append(face_id)
        if any(len(edge_faces.get(edge, ())) != 2 for edge in selected):
            raise ValueError("Only internal T-mesh edges can be dissolved")

        adjacency = {}
        for edge in selected:
            first, second = edge_faces[edge]
            adjacency.setdefault(first, set()).add(second)
            adjacency.setdefault(second, set()).add(first)
        unused = set(adjacency)
        groups = []
        while unused:
            first = min(unused)
            group = {first}
            pending = [first]
            while pending:
                current = pending.pop()
                for neighbor in adjacency.get(current, ()):
                    if neighbor not in group:
                        group.add(neighbor)
                        pending.append(neighbor)
            unused.difference_update(group)
            groups.append(group)

        for group in groups:
            roots = {result.faces[face_id].root for face_id in group}
            if len(roots) != 1:
                raise ValueError("A local dissolve cannot cross base patch boundaries")
            occurrences = {}
            directed = {}
            for face_id in group:
                face = result.faces[face_id]
                boundary = face.boundary
                for position, first in enumerate(boundary):
                    second = boundary[(position + 1) % len(boundary)]
                    edge = _edge(first, second)
                    occurrences[edge] = occurrences.get(edge, 0) + 1
                    directed[edge] = (first, second)
            internal = {edge for edge, count in occurrences.items() if count == 2}
            if not internal.issubset(selected):
                raise ValueError("Select every seam enclosed by the dissolved face region")
            boundary_edges = {
                edge: directed[edge] for edge, count in occurrences.items() if count == 1
            }
            following = {first: second for first, second in boundary_edges.values()}
            start = min(following)
            boundary = [start]
            current = start
            while True:
                current = following.get(current)
                if current is None:
                    raise ValueError("Dissolved T-mesh faces have an open boundary")
                if current == start:
                    break
                if current in boundary:
                    raise ValueError("Dissolved T-mesh faces have a branched boundary")
                boundary.append(current)

            parameters = result.parameter_locations()
            root = next(iter(roots))
            locations = {}
            for vertex in boundary:
                matches = [(u_value, v_value) for item_root, u_value, v_value in parameters[vertex] if item_root == root]
                if not matches:
                    raise ValueError("A dissolved boundary control has no root parameter")
                locations[vertex] = matches[0]
            u_values = [value[0] for value in locations.values()]
            v_values = [value[1] for value in locations.values()]
            corners_uv = {
                (min(u_values), min(v_values)),
                (max(u_values), min(v_values)),
                (max(u_values), max(v_values)),
                (min(u_values), max(v_values)),
            }
            corner_positions = [
                index for index, vertex in enumerate(boundary) if locations[vertex] in corners_uv
            ]
            if len(corner_positions) != 4:
                raise ValueError("Dissolved T-mesh faces must form one parameter rectangle")
            first_corner = corner_positions[0]
            boundary = boundary[first_corner:] + boundary[:first_corner]
            corner_vertices = [vertex for vertex in boundary if locations[vertex] in corners_uv]
            if len(corner_vertices) != 4:
                raise ValueError("Dissolved T-mesh boundary has ambiguous corners")
            positions = [boundary.index(vertex) for vertex in corner_vertices]
            sides = []
            for index, position in enumerate(positions):
                next_position = positions[(index + 1) % 4]
                if next_position > position:
                    side = boundary[position : next_position + 1]
                else:
                    side = boundary[position:] + boundary[: next_position + 1]
                sides.append(tuple(side))
            new_face_id = result.next_face_id
            result.next_face_id += 1
            level = min(result.faces[face_id].level for face_id in group)
            for face_id in group:
                del result.faces[face_id]
            result.faces[new_face_id] = TMeshFace(
                new_face_id,
                tuple(sides),
                root,
                tuple(locations[vertex] for vertex in corner_vertices),
                level,
            )
        atomic = result.atomic_edges()
        result.edge_intervals = {
            edge: interval for edge, interval in result.edge_intervals.items() if edge in atomic
        }
        result.validate()
        return result

    def subdivide_grid(self, face_ids, u_levels=1, v_levels=1):
        """Refine selected leaves independently in their two parameter axes."""
        result = self.copy()
        selected = tuple(dict.fromkeys(int(face_id) for face_id in face_ids))
        u_levels = int(u_levels)
        v_levels = int(v_levels)
        if not selected or min(u_levels, v_levels) < 0 or not (u_levels or v_levels):
            raise ValueError("Subdivide requires faces and at least one axis level")
        if max(u_levels, v_levels) > 16:
            raise ValueError("Subdivision exceeds the supported depth")
        check_topology(len(result.faces) + len(selected) * (2 ** (u_levels + v_levels) - 1))

        def refine_axis(face_ids, divide_axis):
            descendants = []
            for face_id in face_ids:
                if face_id not in result.faces:
                    raise ValueError("Subdivide references an unknown T-mesh face")
                face = result.faces[face_id]
                side_index = next(
                    index
                    for index, first in enumerate(face.parameters)
                    if math.isclose(
                        first[divide_axis],
                        face.parameters[(index + 1) % 4][divide_axis],
                        abs_tol=1.0e-12,
                    )
                )
                edge = result.faces[face_id].sides[side_index][:2]
                _vertices, children = result._insert_edge_in_place(face_id, edge, 0.5)
                descendants.extend(children)
            return tuple(descendants)

        # Select a side parallel to the other parameter axis. The inserted seam
        # is parallel to that side and therefore divides ``divide_axis``.
        for _level in range(u_levels):
            selected = refine_axis(selected, 0)
        for _level in range(v_levels):
            selected = refine_axis(selected, 1)
        return result, selected

    def _insert_edge_in_place(self, face_id, edge, position):
        if not math.isclose(float(position), 0.5, rel_tol=0.0, abs_tol=1.0e-12):
            raise ValueError("Dyadic local insertion currently requires 50% position")
        face_id = int(face_id)
        if face_id not in self.faces:
            raise ValueError("Unknown T-mesh face")
        face = self.faces[face_id]
        selected = _edge(*edge)
        side_index = next(
            (
                index
                for index, side in enumerate(face.sides)
                if any(_edge(first, second) == selected for first, second in zip(side, side[1:]))
            ),
            None,
        )
        if side_index is None:
            raise ValueError("The selected edge is not on this face")

        parameters = face.parameters[side_index:] + face.parameters[:side_index]

        def midpoint(first, second):
            return tuple((first[index] + second[index]) * 0.5 for index in range(2))

        first_parameter = midpoint(parameters[1], parameters[2])
        second_parameter = midpoint(parameters[3], parameters[0])
        split_level = _parameter_level(first_parameter, second_parameter)
        first_new, first_created = self._side_midpoint(face_id, (side_index + 1) % 4, split_level)
        second_new, second_created = self._side_midpoint(face_id, (side_index + 3) % 4, split_level)
        face = self.faces[face_id]
        sides = face.sides[side_index:] + face.sides[:side_index]
        selected_side, following_side, opposite_side, preceding_side = sides
        a, b = selected_side[0], selected_side[-1]
        c, d = opposite_side[0], opposite_side[-1]

        def split_side(side, vertex):
            position = side.index(vertex)
            if position == 0 or position == len(side) - 1:
                raise ValueError("The split point must lie inside its T-edge")
            return side[: position + 1], side[position:]

        following_before, following_after = split_side(following_side, first_new)
        preceding_before, preceding_after = split_side(preceding_side, second_new)
        cross_interval = self._side_interval(selected_side)
        seam = _edge(first_new, second_new)
        if seam in self.edge_intervals:
            raise ValueError("This T-mesh face is already split at the requested position")
        self.edge_intervals[seam] = cross_interval

        new_face_id = self.next_face_id
        self.next_face_id += 1
        self.faces[face_id] = TMeshFace(
            face_id,
            (selected_side, following_before, (first_new, second_new), preceding_after),
            face.root,
            (parameters[0], parameters[1], first_parameter, second_parameter),
            split_level,
        )
        self.faces[new_face_id] = TMeshFace(
            new_face_id,
            ((second_new, first_new), following_after, opposite_side, preceding_before),
            face.root,
            (second_parameter, first_parameter, parameters[2], parameters[3]),
            split_level,
        )
        self.validate()
        created = tuple(
            vertex
            for vertex, was_created in (
                (first_new, first_created),
                (second_new, second_created),
            )
            if was_created
        )
        return created, (face_id, new_face_id)

    def encode(self):
        data = {
            "version": self.VERSION,
            "next_vertex_id": self.next_vertex_id,
            "next_face_id": self.next_face_id,
            "vertices": [
                [vertex_id, *self.vertices[vertex_id]] for vertex_id in sorted(self.vertices)
            ],
            "vertex_levels": [
                [vertex_id, self.vertex_levels[vertex_id]] for vertex_id in sorted(self.vertices)
            ],
            "control_locations": self.parameter_locations(),
            "faces": [
                {
                    "id": face_id,
                    "sides": [list(side) for side in self.faces[face_id].sides],
                    "root": self.faces[face_id].root,
                    "parameters": [list(point) for point in self.faces[face_id].parameters],
                    "level": self.faces[face_id].level,
                }
                for face_id in sorted(self.faces)
            ],
            "edge_intervals": [
                [edge[0], edge[1], self.edge_intervals[edge]]
                for edge in sorted(self.edge_intervals)
            ],
        }
        return json.dumps(data, separators=(",", ":"), sort_keys=True)

    @classmethod
    def decode(cls, value):
        data = json.loads(str(value))
        version = int(data.get("version", -1))
        if version not in (1, 2, cls.VERSION):
            raise ValueError("Unsupported T-mesh data version")
        vertices = {
            int(item[0]): tuple(float(component) for component in item[1:])
            for item in data["vertices"]
        }
        faces = [
            TMeshFace(
                item["id"],
                tuple(tuple(side) for side in item["sides"]),
                item.get("root", item["id"]),
                tuple(tuple(point) for point in item.get("parameters", ())),
                item.get("level", 0),
            )
            for item in data["faces"]
        ]
        vertex_levels = {int(item[0]): int(item[1]) for item in data.get("vertex_levels", ())}
        intervals = {_edge(item[0], item[1]): float(item[2]) for item in data["edge_intervals"]}
        return cls(
            vertices,
            faces,
            edge_intervals=intervals,
            vertex_levels=vertex_levels,
            next_vertex_id=data["next_vertex_id"],
            next_face_id=data["next_face_id"],
            control_locations=data.get("control_locations"),
        )


# Compatibility for documents, scripts, and tests written against the prototype name.
DyadicTMesh = HierarchicalTMesh
