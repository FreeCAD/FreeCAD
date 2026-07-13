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

"""Two-wall wall-relation geometry and shared relation helpers.

The functions in this module are the document-independent part of the
relation system.  They validate straight finite baselines, normalize wall
sections, compute global cutting-plane placements for miter, butt, and tee
joins, and return typed solution data.  They do not mutate wall geometry or
perform relation precedence; :mod:`ArchWallRelationResolver` owns arbitration
and end-condition collection.

The solver intersects infinite baseline lines and then rejects points outside
the finite wall segments with ``RequiresExtension``.  ``Start`` and ``End``
are resolved against the original finite path endpoints.  A successful result
has status ``OK`` and may contain one trim claim per selected wall end;
``Conflict`` is applied later by the resolver when a claim loses ownership.
"""

from dataclasses import dataclass, field, replace

import ArchWallPath
import FreeCAD

JOINT_TYPES = ("Miter", "Butt", "Tee")
RELATION_STATUSES = (
    "OK",
    "Disabled",
    "MissingWall",
    "UnsupportedBaseline",
    "NoIntersection",
    "RequiresExtension",
    "Conflict",
    "UnsupportedTopology",
    "SolverError",
)


@dataclass
class WallJointConflict:
    """Structured description of a relation-end conflict."""

    wall_key: str
    wall_object: object
    wall_end: str
    other_relation: object
    other_relation_type: str
    other_relation_label: str
    message: str

    @property
    def other_joint(self):
        return self.other_relation

    @property
    def other_joint_type(self):
        return self.other_relation_type

    @property
    def other_joint_label(self):
        return self.other_relation_label


@dataclass
class WallJointSolution:
    """Typed solver output for a two-wall relation.

    ``plane_a`` and ``plane_b`` are global placements.  The corresponding
    ``resolved_end_*`` value is ``None`` when that wall is not trimmed.  Miter
    ``extension_*`` values are temporary construction distances used before a
    cut and do not change the wall's persisted baseline or length.
    """

    status: str
    status_message: str
    intersection: FreeCAD.Vector = field(default_factory=FreeCAD.Vector)
    resolved_end_a: str = None
    resolved_end_b: str = None
    plane_a: FreeCAD.Placement = None
    plane_b: FreeCAD.Placement = None
    extension_a: float = 0.0
    extension_b: float = 0.0
    wall_a: object = None
    wall_b: object = None
    conflicts: list = field(default_factory=list)
    conflict_joint_a: object = None
    conflict_joint_b: object = None
    conflict_joint_label_a: str = ""
    conflict_joint_label_b: str = ""
    conflict_message_a: str = ""
    conflict_message_b: str = ""

    def is_ok(self):
        return self.status == "OK"

    def trim_for_wall(self, wall):
        if not self.is_ok():
            return None, None, 0.0
        if wall == self.wall_a:
            return self.resolved_end_a, self.plane_a, self.extension_a
        if wall == self.wall_b:
            return self.resolved_end_b, self.plane_b, self.extension_b
        return None, None, 0.0


def is_wall_joint(obj):
    """Returns True when the given object is a BIM wall joint."""
    return _proxy_type(obj) == "WallJoint"


def is_wall_junction(obj):
    """Returns True when the given object is a BIM wall junction."""
    return _proxy_type(obj) == "WallJunction"


def is_wall_relation(obj):
    """Returns True when the given object is a BIM wall relation."""
    return is_wall_joint(obj) or is_wall_junction(obj)


def iter_wall_relations(wall):
    """Yields wall relations that reference the given wall."""
    if wall is None:
        return
    for obj in wall.InList:
        if is_wall_joint(obj) or is_wall_junction(obj):
            yield obj


def get_relation_walls(relation):
    """Returns the walls referenced by a wall relation object."""
    if is_wall_joint(relation):
        return [relation.WallA, relation.WallB]
    if is_wall_junction(relation):
        return list(relation.Walls)
    return []


def get_next_relation_priority(document, exclude=None):
    """Return the next concrete priority for a wall relation document."""
    priorities = [
        int(obj.Priority)
        for obj in document.Objects
        if obj != exclude and (is_wall_joint(obj) or is_wall_junction(obj)) and obj.Priority >= 0
    ]
    return max(priorities, default=-1) + 1


def iter_wall_joints(wall):
    """Yields joint relations that reference the given wall."""
    if wall is None:
        return
    for obj in wall.InList:
        if is_wall_joint(obj):
            yield obj


def find_existing_joint(doc, wall_a, wall_b):
    """Finds an existing joint between two walls, regardless of link order."""
    for obj in doc.Objects:
        if not is_wall_joint(obj):
            continue
        if {obj.WallA, obj.WallB} == {wall_a, wall_b}:
            return obj
    return None


def get_join_path(wall):
    """Returns the normalized supported path used by the wall-joint solver."""
    if wall is None:
        return None
    proxy = getattr(wall, "Proxy", None)
    get_baseline = getattr(proxy, "get_global_baseline", None)
    if not callable(get_baseline):
        return None
    baseline = get_baseline(wall)
    return ArchWallPath.WallPath.from_baseline(baseline) if baseline else None


def get_join_section(wall):
    """Returns the wall's authoritative resolved section for joining."""
    if wall is None:
        return None
    proxy = getattr(wall, "Proxy", None)
    get_section = getattr(proxy, "get_resolved_section", None)
    if not callable(get_section):
        return None
    return get_section(wall)


def solve_wall_joint(joint):
    """Solves a wall joint relation and returns derived trim data."""
    if not joint:
        return _status_result("MissingWall", "The joint object is missing.")
    if not joint.Enabled:
        return _status_result(
            "Disabled", "The joint is disabled.", wall_a=joint.WallA, wall_b=joint.WallB
        )

    return solve_wall_joint_settings(
        joint,
        joint.JointType,
        joint.ButtTrimmed,
        joint.TeeStem,
        joint.EndA,
        joint.EndB,
    )


def solve_wall_joint_settings(
    joint,
    joint_type,
    butt_trimmed="Auto",
    tee_stem="Auto",
    end_a="Auto",
    end_b="Auto",
):
    """Solve settings without relation conflict arbitration.

    This entry point is also used by the edit task panel for preview.  It must
    remain side-effect free so an invalid preview cannot modify the document.
    """
    if not joint:
        return _status_result("MissingWall", "The joint object is missing.")

    result = solve_wall_joint_inputs(
        joint.WallA,
        joint.WallB,
        joint_type,
        butt_trimmed,
        tee_stem,
        end_a,
        end_b,
    )
    return result


def solve_wall_joint_inputs(
    wall_a,
    wall_b,
    joint_type,
    butt_trimmed="Auto",
    tee_stem="Auto",
    end_a="Auto",
    end_b="Auto",
):
    """Solve a wall-joint configuration from walls and relation settings.

    Miter trims both selected ends using the angle-bisector planes and may
    return temporary extensions.  Butt trims both selected ends against the
    selected supporting-wall face.  Tee trims only the selected stem wall;
    ``Auto`` chooses the wall whose endpoint is nearest the intersection.
    Explicit ``EndA``/``EndB`` values can suppress or override those automatic
    end selections.
    """
    result = _status_result("SolverError", "The joint solver failed.", wall_a=wall_a, wall_b=wall_b)
    if wall_a is None or wall_b is None:
        return _status_result(
            "MissingWall",
            "The joint must reference two walls.",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if wall_a == wall_b:
        return _status_result(
            "MissingWall",
            "A wall joint cannot reference the same wall twice.",
            wall_a=wall_a,
            wall_b=wall_b,
        )

    path_a = get_join_path(wall_a)
    path_b = get_join_path(wall_b)
    section_a = get_join_section(wall_a)
    section_b = get_join_section(wall_b)
    if path_a is None:
        return _status_result(
            "UnsupportedBaseline",
            f"The joint only supports walls with a single straight baseline: {_wall_label(wall_a)}",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if path_b is None:
        return _status_result(
            "UnsupportedBaseline",
            f"The joint only supports walls with a single straight baseline: {_wall_label(wall_b)}",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if path_a.normal.dot(path_b.normal) < 1.0 - 1e-9:
        return _status_result(
            "UnsupportedTopology",
            "The joint only supports walls with a shared section normal.",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if not section_a:
        return _status_result(
            "SolverError",
            f"The joint could not determine the wall section: {_wall_label(wall_a)}",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if not section_b:
        return _status_result(
            "SolverError",
            f"The joint could not determine the wall section: {_wall_label(wall_b)}",
            wall_a=wall_a,
            wall_b=wall_b,
        )

    intersection, auto_end_a, auto_end_b = find_best_intersection(path_a, path_b)
    if not intersection:
        return _status_result(
            "NoIntersection",
            "The baselines of the selected walls do not intersect.",
            wall_a=wall_a,
            wall_b=wall_b,
        )
    if not path_a.contains_point(intersection) or not path_b.contains_point(intersection):
        return _status_result(
            "RequiresExtension",
            "The wall baselines intersect only outside their finite segments; extending walls is not supported.",
            wall_a=wall_a,
            wall_b=wall_b,
        )

    result = replace(
        result,
        status="OK",
        status_message="",
        intersection=intersection,
        wall_a=wall_a,
        wall_b=wall_b,
    )

    joint_type = _normalize_enum(joint_type, JOINT_TYPES, "Miter")
    if joint_type == "Miter":
        plane_a, plane_b = calculate_miter_cutting_planes(path_a, path_b, intersection)
        extension_a, extension_b = calculate_miter_extensions(path_a, path_b, section_a, section_b)
        resolved_end_a = _resolve_end(auto_end_a, end_a)
        resolved_end_b = _resolve_end(auto_end_b, end_b)
        result = replace(
            result,
            resolved_end_a=resolved_end_a,
            resolved_end_b=resolved_end_b,
            plane_a=plane_a if resolved_end_a else None,
            plane_b=plane_b if resolved_end_b else None,
            extension_a=extension_a if resolved_end_a else 0.0,
            extension_b=extension_b if resolved_end_b else 0.0,
        )
        return result

    if joint_type == "Butt":
        butt_trimmed = _normalize_enum(butt_trimmed, ("Auto", "WallA", "WallB"), "Auto")
        if butt_trimmed in ("Auto", "WallB"):
            plane_a, plane_b = calculate_butt_cutting_planes(
                path_a, path_b, intersection, section_a, section_b
            )
        else:
            plane_b, plane_a = calculate_butt_cutting_planes(
                path_b, path_a, intersection, section_b, section_a
            )
        resolved_end_a = _resolve_end(auto_end_a, end_a)
        resolved_end_b = _resolve_end(auto_end_b, end_b)
        result = replace(
            result,
            resolved_end_a=resolved_end_a,
            resolved_end_b=resolved_end_b,
            plane_a=plane_a if resolved_end_a else None,
            plane_b=plane_b if resolved_end_b else None,
        )
        return result

    tee_stem = _normalize_enum(tee_stem, ("Auto", "WallA", "WallB"), "Auto")
    auto_stem = get_auto_tee_stem_role(path_a, path_b, intersection)
    if tee_stem == "Auto":
        tee_stem = auto_stem

    if tee_stem == "WallA":
        resolved_end_a = _resolve_end(auto_end_a, end_a)
        result = replace(
            result,
            resolved_end_a=resolved_end_a,
            resolved_end_b=None,
            plane_a=(
                calculate_tee_cutting_plane(path_a, path_b, intersection, section_b)
                if resolved_end_a
                else None
            ),
            plane_b=None,
        )
        return result

    resolved_end_b = _resolve_end(auto_end_b, end_b)
    result = replace(
        result,
        resolved_end_a=None,
        resolved_end_b=resolved_end_b,
        plane_a=None,
        plane_b=(
            calculate_tee_cutting_plane(path_b, path_a, intersection, section_a)
            if resolved_end_b
            else None
        ),
    )
    return result


def get_trim_for_wall(solution, wall):
    """Returns the resolved end and plane for the requested wall."""
    if solution is None:
        return None, None, 0.0
    trim = solution.trim_for_wall(wall)
    if len(trim) == 2:
        end_name, plane = trim
        return end_name, plane, 0.0
    return trim


def find_best_intersection(path1, path2):
    """Finds the intersection point of two baselines and the closest end on each line."""
    return ArchWallPath.find_path_intersection(path1, path2)


def calculate_miter_cutting_planes(path1, path2, intersection):
    """Calculate global angle-bisector cutting planes for a miter joint."""
    dir1 = path1.direction_away_from(intersection)
    dir2 = path2.direction_away_from(intersection)
    bisector = dir1 + dir2
    if bisector.Length <= 1e-9:
        return None, None
    bisector_normal = bisector.normalize()
    axis_x = dir1
    axis_y = path1.normal
    axis_z = bisector_normal.cross(axis_y).normalize()
    rotation = FreeCAD.Rotation(axis_x, axis_y, axis_z, "ZXY")
    plane1 = FreeCAD.Placement(intersection, rotation)
    plane2 = plane1.copy()
    plane2.rotate(intersection, path1.normal, 180)
    return plane1, plane2


def calculate_miter_extensions(path1, path2, section1, section2):
    """Return temporary distances needed for closed architectural miters.

    For each wall, the other wall's larger lateral section extent is divided
    by the sine of the angle between the baseline directions.  Parallel paths
    therefore return zero rather than an unbounded extension.
    """
    return _miter_extension_for_wall(path1, path2, section2), _miter_extension_for_wall(
        path2, path1, section1
    )


def _miter_extension_for_wall(path, other_path, other_section):
    tangent = path.direction()
    other_tangent = other_path.direction()
    sine = tangent.cross(other_tangent).Length
    if sine <= 1e-9:
        return 0.0
    lateral = other_path.lateral_direction()
    positive = other_section.offset_towards(lateral, lateral)
    negative = other_section.offset_towards(lateral, lateral * -1)
    other_extent = max(abs(positive or 0.0), abs(negative or 0.0))
    return other_extent / sine


def calculate_butt_cutting_planes(path1, path2, intersection, section1, section2):
    """Calculate global cutting planes for a butt joint.

    The planes are offset to the selected section faces, so the wall chosen as
    the butt support remains visually continuous while the terminating wall is
    cut flush to it.
    """
    offset_1 = _get_section_face_offset_vector(path2, section2, path1.center() - intersection)
    offset_2 = _get_section_face_offset_vector(path1, section1, path2.center() - intersection)
    if offset_1 is None or offset_2 is None:
        return None, None

    axis_x_2 = path1.direction()
    axis_y_2 = path1.normal
    axis_z_2 = axis_x_2.cross(axis_y_2).normalize()
    rotation_2 = FreeCAD.Rotation(axis_x_2, axis_y_2, axis_z_2, "ZXY")
    plane2 = FreeCAD.Placement(intersection.add(offset_2), rotation_2)

    dir2 = path2.direction()
    axis_x_1 = dir2
    axis_y_1 = path2.normal
    axis_z_1 = axis_x_1.cross(axis_y_1).normalize()
    rotation_1 = FreeCAD.Rotation(axis_x_1, axis_y_1, axis_z_1, "ZXY")
    plane1 = FreeCAD.Placement(intersection.add(offset_1), rotation_1)
    return plane1, plane2


def calculate_tee_cutting_plane(stem_path, top_path, intersection, top_section):
    """Calculate the global cutting plane for the stem wall in a tee joint."""
    offset = _get_section_face_offset_vector(
        top_path,
        top_section,
        stem_path.center() - intersection,
    )
    if offset is None:
        return None

    plane_normal = stem_path.direction()
    rotation = FreeCAD.Rotation(stem_path.normal, plane_normal)

    vec_to_stem = stem_path.center() - intersection
    plane_position = intersection.add(offset)
    if vec_to_stem.Length > 1e-9:
        plane_position = plane_position.add(vec_to_stem.normalize() * 1e-6)
    return FreeCAD.Placement(plane_position, rotation)


def get_auto_tee_stem_role(path_a, path_b, intersection):
    """Return the wall whose finite endpoint is nearest a tee intersection."""
    dist_to_end_a = path_a.nearest_end_distance(intersection)
    dist_to_end_b = path_b.nearest_end_distance(intersection)
    return "WallA" if dist_to_end_a < dist_to_end_b else "WallB"


def _normalize_enum(value, allowed, default):
    return value if value in allowed else default


def _resolve_end(auto_end, override):
    override = _normalize_enum(override, ("Auto", "Start", "End", "None"), "Auto")
    if override == "Auto":
        return auto_end
    if override == "None":
        return None
    return override


def _status_result(status, message, wall_a=None, wall_b=None):
    return WallJointSolution(status=status, status_message=message, wall_a=wall_a, wall_b=wall_b)


def _wall_label(wall):
    return getattr(wall, "Label", getattr(wall, "Name", "<unsupported object>"))


def _proxy_type(obj):
    return getattr(getattr(obj, "Proxy", None), "Type", None)


def _get_section_face_offset_vector(path, section, towards_vector):
    """Returns a signed lateral offset from a wall centerline to the requested section face."""
    if path is None:
        return None
    lateral = path.lateral_direction()
    offset = section.offset_towards(lateral, towards_vector)
    if offset is None:
        return None
    return lateral * offset
