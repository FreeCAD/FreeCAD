# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Solver helpers for BIM wall junction relations.

The wall-junction model groups three or more walls around one carrier wall and
computes direct trim claims for branch walls that terminate at one common
intersection point.  A valid topology has one carrier passing through that
point and every other wall ending there.  ``CarrierMode='Explicit'`` uses the
linked carrier; ``Auto`` tests each wall as a candidate and keeps the valid
candidate whose carrier endpoint is farthest from the common point.  No hidden
two-wall joints are synthesized for unsupported topologies.
"""

from dataclasses import dataclass, field

import FreeCAD

import ArchWallEndCondition
import ArchWallRelation

END_TOLERANCE = 1e-4
INTERSECTION_TOLERANCE = 1e-4


@dataclass
class WallJunctionSolution:
    """Typed solver output for a wall-junction relation.

    ``trim_claims`` contains one :class:`WallTrimClaim` for each branch wall
    end.  Junctions do not currently need an extension, so their claims use
    the default value of zero.
    """

    status: str
    status_message: str
    intersection: FreeCAD.Vector = field(default_factory=FreeCAD.Vector)
    carrier_wall: object = None
    branch_walls: list = field(default_factory=list)
    walls: list = field(default_factory=list)
    trim_claims: list = field(default_factory=list)
    conflicts: list = field(default_factory=list)
    conflict_walls: list = field(default_factory=list)
    conflict_relation_labels: list = field(default_factory=list)
    conflict_messages: list = field(default_factory=list)

    def is_ok(self):
        return self.status == "OK"

    def trim_for_wall(self, wall):
        """Return this solution's typed claim for ``wall``, if any."""
        return next((claim for claim in self.trim_claims if claim.wall == wall), None)


def solve_wall_junction(junction):
    """Solve a wall junction without relation conflict arbitration."""
    if not junction:
        return WallJunctionSolution("MissingWall", "The wall junction object is missing.")
    if not getattr(junction, "Enabled", True):
        return WallJunctionSolution("Disabled", "The wall junction is disabled.")
    solution = solve_wall_junction_inputs(
        getattr(junction, "Walls", []),
        getattr(junction, "CarrierMode", "Auto"),
        getattr(junction, "CarrierWall", None),
    )
    return solution


def solve_wall_junction_inputs(walls, carrier_mode="Auto", carrier_wall=None):
    """Solve a junction configuration from walls and carrier settings.

    Every wall must have a supported straight finite baseline.  Pairwise
    intersections with the candidate carrier must agree within
    ``INTERSECTION_TOLERANCE``; branch intersections outside finite segments
    report ``RequiresExtension`` and are not silently extended.
    """
    walls = _unique_walls(walls)
    if len(walls) < 3:
        return WallJunctionSolution(
            "MissingWall",
            "A wall junction needs at least 3 unique walls.",
            walls=walls,
        )

    paths = {}
    for wall in walls:
        path = ArchWallRelation.get_join_path(wall)
        if not path:
            return WallJunctionSolution(
                "UnsupportedBaseline",
                f"The wall junction only supports walls with a single straight baseline: {wall.Label}",
                walls=walls,
            )
        paths[wall] = path

    reference_normal = paths[walls[0]].normal
    if any(reference_normal.dot(path.normal) < 1.0 - 1e-9 for path in paths.values()):
        return WallJunctionSolution(
            "UnsupportedTopology",
            "The wall junction only supports walls with a shared section normal.",
            walls=walls,
        )

    carrier_mode = carrier_mode if carrier_mode in ("Auto", "Explicit") else "Auto"
    if carrier_mode == "Explicit":
        if carrier_wall not in walls:
            return WallJunctionSolution(
                "MissingWall",
                "The explicit carrier wall must be part of the wall junction.",
                walls=walls,
            )
        return _solve_carrier_candidate(walls, paths, carrier_wall)

    best = None
    best_score = None
    saw_no_intersection = False
    saw_requires_extension = False
    saw_unsupported_topology = False
    for candidate in walls:
        solution = _solve_carrier_candidate(walls, paths, candidate)
        if solution.is_ok():
            score = ArchWallRelation.get_join_path(candidate).nearest_end_distance(
                solution.intersection
            )
            if (best is None) or (score > best_score):
                best = solution
                best_score = score
        elif solution.status == "NoIntersection":
            saw_no_intersection = True
        elif solution.status == "RequiresExtension":
            saw_requires_extension = True
        elif solution.status == "UnsupportedTopology":
            saw_unsupported_topology = True

    if best:
        return best
    if saw_unsupported_topology:
        return WallJunctionSolution(
            "UnsupportedTopology",
            "The walls do not form a supported carrier-and-branches junction.",
            walls=walls,
        )
    if saw_no_intersection:
        return WallJunctionSolution(
            "NoIntersection",
            "The walls do not meet at a common junction point.",
            walls=walls,
        )
    if saw_requires_extension:
        return WallJunctionSolution(
            "RequiresExtension",
            "The wall baselines intersect only outside their finite segments; extending walls is not supported.",
            walls=walls,
        )
    return WallJunctionSolution("SolverError", "The wall junction solver failed.", walls=walls)


def _solve_carrier_candidate(walls, paths, carrier_wall):
    carrier_section = ArchWallRelation.get_join_section(carrier_wall)
    if not carrier_section:
        return WallJunctionSolution(
            "SolverError",
            f"The wall junction could not determine the carrier section: {carrier_wall.Label}",
            walls=walls,
        )

    intersections = []
    for wall in walls:
        if wall == carrier_wall:
            continue
        intersection, _end_a, _end_b = ArchWallRelation.find_best_intersection(
            paths[carrier_wall], paths[wall]
        )
        if not intersection:
            return WallJunctionSolution(
                "NoIntersection",
                f"{carrier_wall.Label} does not intersect {wall.Label}.",
                walls=walls,
            )
        if not paths[carrier_wall].contains_point(intersection) or not paths[wall].contains_point(
            intersection
        ):
            return WallJunctionSolution(
                "RequiresExtension",
                "The wall baselines intersect only outside their finite segments; extending walls is not supported.",
                walls=walls,
            )
        intersections.append(intersection)

    common_point = intersections[0]
    for point in intersections[1:]:
        if point.distanceToPoint(common_point) > INTERSECTION_TOLERANCE:
            return WallJunctionSolution(
                "UnsupportedTopology",
                "The walls do not share one common junction point.",
                walls=walls,
            )

    carrier_distance = paths[carrier_wall].nearest_end_distance(common_point)
    if carrier_distance <= END_TOLERANCE:
        return WallJunctionSolution(
            "UnsupportedTopology",
            f"The carrier wall must pass through the junction point: {carrier_wall.Label}",
            walls=walls,
        )

    branch_walls = []
    trim_claims = []
    for wall in walls:
        if wall == carrier_wall:
            continue
        branch_distance = paths[wall].nearest_end_distance(common_point)
        if branch_distance > END_TOLERANCE:
            return WallJunctionSolution(
                "UnsupportedTopology",
                f"Only branch walls ending at the junction point are supported: {wall.Label}",
                walls=walls,
            )
        branch_walls.append(wall)
        end_name = paths[wall].nearest_end_name(common_point)
        plane = ArchWallRelation.calculate_tee_cutting_plane(
            paths[wall],
            paths[carrier_wall],
            common_point,
            carrier_section,
        )
        if not plane:
            return WallJunctionSolution(
                "SolverError",
                f"The wall junction could not compute the trim plane for {wall.Label}.",
                walls=walls,
            )
        trim_claims.append(
            ArchWallEndCondition.WallTrimClaim(
                wall=wall,
                end_name=end_name,
                plane=plane,
            )
        )

    return WallJunctionSolution(
        "OK",
        "",
        intersection=common_point,
        carrier_wall=carrier_wall,
        branch_walls=branch_walls,
        walls=walls,
        trim_claims=trim_claims,
    )


def _unique_walls(walls):
    unique = []
    seen = set()
    for wall in walls or []:
        if not wall or wall.Name in seen:
            continue
        seen.add(wall.Name)
        unique.append(wall)
    return unique


def apply_conflicts(result, conflicts):
    """Apply resolver-produced conflicts to a junction solution."""
    result.status = "Conflict"
    result.conflicts = conflicts
    unique_messages = []
    conflict_labels = []
    conflict_walls = []
    for conflict in conflicts:
        if conflict.wall_object and conflict.wall_object not in conflict_walls:
            conflict_walls.append(conflict.wall_object)
        if conflict.other_relation_label and conflict.other_relation_label not in conflict_labels:
            conflict_labels.append(conflict.other_relation_label)
        if conflict.message not in unique_messages:
            unique_messages.append(conflict.message)
    result.conflict_walls = conflict_walls
    result.conflict_relation_labels = conflict_labels
    result.conflict_messages = unique_messages
    result.status_message = "Conflict: " + "; ".join(unique_messages)
