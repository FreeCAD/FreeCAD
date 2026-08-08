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
################################################################################

"""Dispatch, precedence, and trim collection for BIM wall relations.

The concrete two-wall and multi-wall solvers intentionally do not import each
other.  This module is the composition layer that knows about both solvers and
owns relation-wide precedence and end-condition collection.  A relation is
considered to lose only when an enabled, successfully solved relation with a
smaller ``(Priority, Name)`` key claims the same wall end.  Solving competing
relations with ``include_conflicts=False`` prevents arbitration recursion.

``collect_wall_relation_endings()`` is the wall-facing API: it ignores disabled
and unsuccessful relations, selects the first claim by the same stable key,
and returns at most one ``WallEndCondition`` per wall end.
"""

import ArchWallEndCondition
import ArchWallJunctionSolver
import ArchWallRelation


def solve_wall_relation(relation, include_conflicts=True):
    """Solve a relation and optionally apply end-ownership conflicts.

    The returned object is either a two-wall or junction solution.  With
    conflict handling enabled, a losing solution receives status ``Conflict``
    and its claims are excluded from wall recompute.
    """
    if ArchWallRelation.is_wall_joint(relation):
        solution = ArchWallRelation.solve_wall_joint(relation)
    elif ArchWallRelation.is_wall_junction(relation):
        solution = ArchWallJunctionSolver.solve_wall_junction(relation)
    else:
        return ArchWallRelation.WallJointSolution(
            status="SolverError",
            status_message="Unsupported wall relation object.",
        )

    if include_conflicts:
        apply_relation_conflicts(relation, solution)
    return solution


def get_joint_conflicts(joint, solution=None):
    """Return structured conflicts for a two-wall relation."""
    return get_relation_conflicts(joint, solution)


def apply_relation_conflicts(relation, solution):
    """Apply precedence conflicts to an already solved relation.

    This is intentionally side-effect free: callers can use it for a
    hypothetical solution, such as a task-panel preview, without modifying
    the relation or triggering a document recompute.
    """
    if solution is None or not solution.is_ok():
        return solution

    conflicts = get_relation_conflicts(relation, solution)
    if not conflicts:
        return solution

    if ArchWallRelation.is_wall_joint(relation):
        _apply_joint_conflicts(solution, conflicts)
    elif ArchWallRelation.is_wall_junction(relation):
        ArchWallJunctionSolver.apply_conflicts(solution, conflicts)
    return solution


def get_relation_conflicts(relation, solution=None):
    """Return claims blocked by an earlier relation in the same document."""
    if relation is None:
        return []
    if solution is None:
        solution = solve_wall_relation(relation, include_conflicts=False)
    if solution is None or not solution.is_ok():
        return []

    conflicts = []
    for claim in _iter_solution_trim_claims(solution):
        wall_obj = claim.wall
        end_name = claim.end_name
        for other in ArchWallRelation.iter_wall_relations(wall_obj):
            if other == relation or not getattr(other, "Enabled", False):
                continue
            if not _relation_precedes(other, relation):
                continue
            other_solution = solve_wall_relation(other, include_conflicts=False)
            if other_solution is None or not other_solution.is_ok():
                continue
            other_claim = _claim_for_wall(other_solution, wall_obj)
            if other_claim is None or other_claim.end_name != end_name:
                continue
            conflicts.append(
                ArchWallRelation.WallJointConflict(
                    wall_key=_wall_key(relation, claim),
                    wall_object=wall_obj,
                    wall_end=end_name,
                    other_relation=other,
                    other_relation_type=_get_relation_type_name(other),
                    other_relation_label=other.Label,
                    message=(
                        f"{wall_obj.Label} {end_name} is already trimmed by "
                        f"{_get_relation_kind_name(other)} {other.Label}."
                    ),
                )
            )
    return conflicts


def joint_has_conflict(joint, solution=None):
    """Return whether another relation already owns one of the joint's ends."""
    return bool(get_joint_conflicts(joint, solution))


def collect_wall_relation_endings(wall):
    """Collect the winning relation-derived trim plane for each wall end."""
    claims = {"Start": [], "End": []}
    for relation in ArchWallRelation.iter_wall_relations(wall):
        if not getattr(relation, "Enabled", False):
            continue
        solution = solve_wall_relation(relation, include_conflicts=True)
        if solution is None or not solution.is_ok():
            continue
        for claim in solution.trim_claims:
            if claim.wall == wall:
                claims[claim.end_name].append((relation, claim))

    result = {"Start": None, "End": None}
    for end_name, entries in claims.items():
        if not entries:
            continue
        entries.sort(key=lambda entry: _relation_order_key(entry[0]))
        result[end_name] = ArchWallEndCondition.WallEndCondition(
            source="Relation",
            placement=entries[0][1].plane,
            is_global=True,
            extension=entries[0][1].extension,
        )
    return result


def _iter_solution_trim_claims(solution):
    yield from solution.trim_claims


def _claim_for_wall(solution, wall):
    return next((claim for claim in solution.trim_claims if claim.wall == wall), None)


def _wall_key(relation, claim):
    if ArchWallRelation.is_wall_joint(relation):
        return "A" if claim.wall == relation.WallA else "B"
    return claim.wall.Name


def _relation_precedes(left, right):
    if not left or not right or left == right:
        return False
    if left.Document and left.Document == right.Document:
        return _relation_order_key(left) < _relation_order_key(right)
    return False


def _relation_order_key(relation):
    """Return the explicit stable precedence key for relation ownership."""
    if relation is None:
        return (float("inf"), "")
    return (int(relation.Priority), relation.Name)


def _get_relation_kind_name(relation):
    if ArchWallRelation.is_wall_joint(relation):
        return "joint"
    if ArchWallRelation.is_wall_junction(relation):
        return "junction"
    return "relation"


def _get_relation_type_name(relation):
    if ArchWallRelation.is_wall_joint(relation):
        return relation.JointType
    if ArchWallRelation.is_wall_junction(relation):
        return "WallJunction"
    return getattr(getattr(relation, "Proxy", None), "Type", None) or "Relation"


def _apply_joint_conflicts(result, conflicts):
    result.status = "Conflict"
    result.conflicts = conflicts
    unique_messages = []
    for conflict in conflicts:
        wall_key = conflict.wall_key
        if wall_key == "A" and result.conflict_joint_a is None:
            result.conflict_joint_a = conflict.other_joint
            result.conflict_joint_label_a = conflict.other_joint_label
            result.conflict_message_a = conflict.message
        elif wall_key == "B" and result.conflict_joint_b is None:
            result.conflict_joint_b = conflict.other_joint
            result.conflict_joint_label_b = conflict.other_joint_label
            result.conflict_message_b = conflict.message
        if conflict.message not in unique_messages:
            unique_messages.append(conflict.message)
    result.status_message = "Conflict: " + "; ".join(unique_messages)
