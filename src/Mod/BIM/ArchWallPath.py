# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public                       #
#   License as published by the Free Software Foundation, either version 2    #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of              #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             #
#   GNU Lesser General Public License for more details.                        #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Value objects and operations for an already-resolved finite wall baseline.

``WallBaseline`` and ``WallPath`` are deliberately independent of wall
proxies and FreeCAD wall properties.  Callers provide global straight
geometry; this module validates it and exposes path-to-path operations only.
"""

from dataclasses import dataclass

import DraftGeomUtils
import FreeCAD
import Part


@dataclass(frozen=True)
class WallBaseline:
    """An oriented, resolved wall baseline in global coordinates.

    ``start_point`` and ``end_point`` are semantic wall endpoints.  They are
    stored explicitly so callers never need to infer Start/End from the
    topological ordering of a rebuilt shape.
    """

    edge: Part.Edge
    normal: FreeCAD.Vector
    start_point: FreeCAD.Vector
    end_point: FreeCAD.Vector

    def __post_init__(self):
        normal = FreeCAD.Vector(self.normal)
        normal.normalize()
        object.__setattr__(self, "normal", normal)
        object.__setattr__(self, "start_point", FreeCAD.Vector(self.start_point))
        object.__setattr__(self, "end_point", FreeCAD.Vector(self.end_point))


class WallPath:
    """A strict finite straight baseline in global coordinates.

    The edge's vertex order defines Start and End.  The supplied section
    normal is copied and normalized, so relation code can use the value object
    without reinterpreting placements or mutating caller-owned vectors.
    """

    def __init__(self, edge, normal):
        """Validate and store one global baseline and section normal.

        ``edge`` must be a non-degenerate straight ``Part.Edge`` with exactly
        two vertices.  ``normal`` must be non-zero and non-parallel to the
        baseline.  Wall proxies are intentionally not accepted or inspected.
        """
        if not isinstance(edge, Part.Edge):
            raise TypeError("WallPath requires a Part.Edge")
        if edge.Curve.TypeId != "Part::GeomLine":
            raise ValueError("WallPath requires a straight edge")
        if len(edge.Vertexes) != 2:
            raise ValueError("WallPath requires an edge with two vertices")
        if edge.Length <= 1e-9:
            raise ValueError("WallPath requires a non-degenerate edge")
        if normal is None or normal.Length <= 1e-9:
            raise ValueError("WallPath requires a non-zero section normal")
        normal = FreeCAD.Vector(normal)
        normal.normalize()
        direction = edge.Vertexes[-1].Point.sub(edge.Vertexes[0].Point)
        if direction.cross(normal).Length <= 1e-9:
            raise ValueError("WallPath baseline cannot be parallel to its section normal")
        self.edge = edge
        self.normal = normal

    @classmethod
    def from_baseline(cls, baseline):
        """Create a strict path from a resolved :class:`WallBaseline`."""
        if not isinstance(baseline, WallBaseline):
            raise TypeError("WallPath.from_baseline requires a WallBaseline")
        return cls(baseline.edge, baseline.normal)

    @property
    def start_point(self):
        """Return the first ordered endpoint of the global baseline."""
        return self.edge.Vertexes[0].Point

    @property
    def end_point(self):
        """Return the second ordered endpoint of the global baseline."""
        return self.edge.Vertexes[-1].Point

    def vector(self):
        """Return the oriented vector from Start to End."""
        return self.end_point.sub(self.start_point)

    def direction(self):
        """Return the normalized direction from Start to End."""
        return self.vector().normalize()

    def center(self):
        """Return the midpoint of the finite baseline."""
        return (self.start_point + self.end_point) * 0.5

    def nearest_end_name(self, point):
        """Return the nearest endpoint name, using End to break ties."""
        start_distance = point.distanceToPoint(self.start_point)
        end_distance = point.distanceToPoint(self.end_point)
        return "Start" if start_distance < end_distance else "End"

    def nearest_end_distance(self, point):
        """Return the distance from a point to the nearer endpoint."""
        return min(
            point.distanceToPoint(self.start_point),
            point.distanceToPoint(self.end_point),
        )

    def contains_point(self, point, tolerance=1e-4):
        """Return whether a point lies on this finite segment.

        The tolerance applies both to the segment bounds and to the distance
        from the point to its orthogonal projection on the baseline.
        """
        segment = self.vector()
        length = segment.Length
        if length <= 1e-9:
            return False
        parameter = point.sub(self.start_point).dot(segment) / (length * length)
        if parameter < -tolerance / length or parameter > 1.0 + tolerance / length:
            return False
        projected = self.start_point + segment * parameter
        return projected.distanceToPoint(point) <= tolerance

    def direction_away_from(self, point):
        """Return the unit direction away from the nearer endpoint to point."""
        direction = self.vector()
        if point.distanceToPoint(self.start_point) > point.distanceToPoint(self.end_point):
            direction.multiply(-1)
        return direction.normalize()

    def lateral_direction(self):
        """Return the unit lateral direction implied by baseline and normal."""
        lateral = self.direction().cross(self.normal)
        if lateral.Length <= 1e-9:
            raise ValueError("WallPath has no lateral direction")
        return lateral.normalize()


def find_path_intersection(path_a, path_b):
    """Intersect two paths' infinite baseline lines.

    Return ``(point, end_a, end_b)`` where the endpoint names classify the
    intersection relative to each finite path.  The intersection itself may
    lie outside either finite segment because relation resolution considers
    the supporting lines.  Parallel or otherwise non-intersecting paths
    return ``(None, None, None)``.
    """
    if not isinstance(path_a, WallPath) or not isinstance(path_b, WallPath):
        raise TypeError("find_path_intersection requires WallPath values")
    intersections = DraftGeomUtils.findIntersection(
        path_a.edge, path_b.edge, infinite1=True, infinite2=True
    )
    if not intersections:
        return None, None, None
    point = intersections[0]
    return point, path_a.nearest_end_name(point), path_b.nearest_end_name(point)
