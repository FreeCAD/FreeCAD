# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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

"""
Machine-agnostic orientation solver for indexed 3+2 positioning.

Replaces the hardcoded C-A rotation generator with a solver that derives
all behavior from the Machine data model.
"""

import math
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import FreeCAD
import Path
from Machine.models.machine import Machine, RotaryAxis, AxisRole

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


__title__ = "Rotation Solver"
__author__ = "FreeCAD"
__url__ = "https://www.freecad.org"
__doc__ = "Machine-agnostic 3+2 orientation solver"


@dataclass
class SolveResult:
    """Result of orientation solve operation."""

    success: bool
    angles: Dict[str, float]  # {axis_name: value_degrees}
    deltas: Dict[str, float]  # {axis_name: delta_from_current}
    cost: float
    singular: bool
    error_norm: float
    reason: str = ""  # non-empty on failure


def _wrap_angle(angle: float) -> float:
    """Wrap angle to [-180, 180] range."""
    return ((angle + 180) % 360) - 180


def build_kinematic_chain(
    machine: Machine, restrict_axes: Optional[List[str]] = None
) -> List[RotaryAxis]:
    """Build ordered list of rotary axes from root to tool tip.

    The root axis is the outermost (closest to the machine base),
    and the last axis is the innermost (closest to the workpiece/tool).

    For a C-A table-table machine where C is root and A is child:
      chain = [C, A]   (C is root/outer, A is leaf/inner)

    Args:
        machine: Machine configuration
        restrict_axes: Optional list of axis names to include

    Returns:
        Ordered list of RotaryAxis objects from root to tool tip
    """
    # Get all rotary axes, optionally filtered
    if restrict_axes:
        rotary_axes = {
            name: axis for name, axis in machine.rotary_axes.items() if name in restrict_axes
        }
    else:
        rotary_axes = machine.rotary_axes.copy()

    if not rotary_axes:
        return []

    # Build parent-child relationships
    # Find root axes (those with no parent or parent not in rotary set)
    root_axes = [
        name
        for name, axis in rotary_axes.items()
        if axis.parent is None or axis.parent not in rotary_axes
    ]

    if not root_axes:
        # Circular dependency or all axes have parents - pick one as root
        # This shouldn't happen with well-formed machine configs
        root_axes = [list(rotary_axes.keys())[0]]
        Path.Log.warning(f"Kinematic chain has no clear root, using {root_axes[0]}")

    # Build chain by following parent relationships from each root
    chain = []
    visited = set()

    for root in root_axes:
        current = root
        while current and current not in visited:
            if current in rotary_axes:
                chain.append(rotary_axes[current])
            visited.add(current)

            # Find next axis that has current as parent
            next_axis = None
            for name, axis in rotary_axes.items():
                if axis.parent == current and name not in visited:
                    next_axis = name
                    break

            current = next_axis

    # Traversal goes root→leaf, which is the correct tree order per spec §4.1.
    return chain


def compute_rotation_matrix(chain: List[RotaryAxis], angles: Dict[str, float]) -> FreeCAD.Rotation:
    """Compute cumulative rotation matrix for given axis chain and angles.

    For table-table machines the axes are sorted so the azimuth
    (Z-rotation) axis is applied first (rightmost in the matrix product)
    and the tilt axis is applied last (leftmost).  This gives
    ``R_tilt · R_azimuth`` regardless of the parent/child order in the
    kinematic tree, matching the analytical decomposition used by the
    solver.

    For head or mixed machines the chain order is used directly with
    left-multiply (``R_leaf · … · R_root``).

    Args:
        chain: Ordered list of RotaryAxis objects (root first, leaf last)
        angles: Dictionary of axis angles in degrees

    Returns:
        FreeCAD.Rotation representing cumulative transformation
    """
    if not chain:
        return FreeCAD.Rotation()

    # For table-table machines, sort so azimuth (Z-rot) iterates first.
    # Left-multiply then gives R_tilt · R_azimuth.
    all_table = all(ax.role == AxisRole.TABLE_ROTARY for ax in chain)
    if all_table and len(chain) >= 2:
        ordered = sorted(chain, key=lambda ax: 0 if abs(ax.rotation_vector.z) > 0.9 else 1)
    else:
        ordered = chain

    total_rotation = FreeCAD.Rotation()

    for axis in ordered:
        if axis.name in angles:
            rot = FreeCAD.Rotation(axis.rotation_vector, angles[axis.name])
            total_rotation = rot.multiply(total_rotation)

    return total_rotation


def _relAngle(vec: FreeCAD.Vector, ref_axis: FreeCAD.Vector, plane_normal: FreeCAD.Vector) -> float:
    """Compute relative angle of a vector projected onto a plane, measured from a reference axis.

    This replicates the legacy relAngle() logic from rotation.py but accepts
    arbitrary axis vectors instead of hardcoded enum values.

    Args:
        vec: Input vector
        ref_axis: Reference axis to measure angle from (unit vector)
        plane_normal: Normal of the plane to project onto (unit vector)

    Returns:
        Angle in degrees (signed)
    """
    norm = FreeCAD.Vector(vec)  # copy
    norm.projectToPlane(FreeCAD.Vector(0, 0, 0), plane_normal)

    if norm.Length < 1e-12:
        return 0.0

    rot = FreeCAD.Rotation(norm, ref_axis)
    ang = math.degrees(rot.Angle)
    angle = ang * plane_normal.dot(rot.Axis)
    return angle


def _decompose_2axis(
    first_axis: RotaryAxis, second_axis: RotaryAxis, desired_axis: FreeCAD.Vector
) -> List[Dict[str, float]]:
    """Decompose desired orientation into two rotary axis angles.

    Solves: R_second(θ₂) · R_first(θ₁) · desired = Z

    Uses the _relAngle projection approach: solve first_axis angle from
    the azimuthal projection of desired, then apply that rotation and
    solve the second_axis angle from the rotated vector.

    Args:
        first_axis: Axis to solve first (rightmost in composition)
        second_axis: Axis to solve second (leftmost in composition)
        desired_axis: Unit vector for desired tool direction

    Returns:
        List of candidate angle dictionaries
    """
    first_ref, first_plane = _get_relangle_params(first_axis.rotation_vector)

    z_target = FreeCAD.Vector(0, 0, 1)
    v2 = second_axis.rotation_vector.normalize()
    required_proj = z_target.dot(v2)  # component along v2 that intermediate must have

    # ---- Generate first-axis candidates from multiple strategies ----
    first_candidates = set()

    # Strategy 1: _relAngle heuristic (works well for azimuthal axes)
    base_angle = _relAngle(desired_axis, first_ref, first_plane)
    first_candidates.add(base_angle)
    if base_angle == 0:
        first_candidates.add(180.0)
    elif base_angle == 180:
        first_candidates.add(0.0)
    elif base_angle >= 0:
        first_candidates.update([base_angle - 180, 180 + base_angle, base_angle - 360])
    else:
        first_candidates.update([base_angle + 180, -180 + base_angle, base_angle + 360])

    # Strategy 2: constraint-based candidates for tilt axes.
    # After R_first(θ₁) · desired, the component along v2 must equal required_proj.
    # Solve: (R_first(θ₁) · desired) · v2 = required_proj
    # This is a scalar equation in θ₁ that can be solved with atan2.
    v1 = first_axis.rotation_vector.normalize()
    # Decompose desired into components parallel and perpendicular to v1
    d_par = v1 * desired_axis.dot(v1)  # unchanged by rotation about v1
    d_perp = desired_axis - d_par
    if d_perp.Length > 1e-12:
        # After rotation by θ₁ about v1:
        # R(θ₁) · desired = d_par + cos(θ₁)·d_perp + sin(θ₁)·(v1 × d_perp)
        # Dot with v2:
        # d_par·v2 + cos(θ₁)·(d_perp·v2) + sin(θ₁)·((v1×d_perp)·v2) = required_proj
        A_coeff = d_perp.dot(v2)
        B_coeff = v1.cross(d_perp).dot(v2)
        C_const = d_par.dot(v2)
        # A·cos(θ) + B·sin(θ) = required_proj - C
        rhs = required_proj - C_const
        amplitude = math.hypot(A_coeff, B_coeff)
        if amplitude > 1e-12 and abs(rhs / amplitude) <= 1.0 + 1e-9:
            ratio = max(-1.0, min(1.0, rhs / amplitude))
            base = math.acos(ratio)
            phase = math.atan2(B_coeff, A_coeff)
            for sign in [1, -1]:
                theta = math.degrees(sign * base + phase)
                first_candidates.update([theta, theta + 360, theta - 360])
    else:
        # Singularity: desired is parallel to first axis rotation vector.
        # Any first-axis angle leaves desired unchanged.  Try 0 and ±90.
        first_candidates.update([0.0, 90.0, -90.0, 180.0, -180.0])

    # Filter by first axis limits
    first_valid = [a for a in first_candidates if first_axis.min_limit <= a <= first_axis.max_limit]
    # ---- For each valid first-axis angle, compute second axis ----
    z_axis = FreeCAD.Vector(0, 0, 1)
    candidates = []
    for first_val in first_valid:
        # Apply first rotation to get intermediate vector
        first_rot = FreeCAD.Rotation(first_axis.rotation_vector, first_val)
        newvec = first_rot.multVec(desired_axis)

        # Find the angle that rotates newvec to Z about second_axis.rotation_vector.
        # Project both newvec and Z into the plane perpendicular to the rotation
        # vector, then compute the signed angle from newvec's projection to Z's
        # projection.
        v2 = second_axis.rotation_vector.normalize()
        newvec_proj_along = newvec.dot(v2)
        z_proj_along = z_axis.dot(v2)

        if abs(newvec_proj_along - z_proj_along) > 1e-6:
            # Component along rotation axis differs — no single-axis rotation
            # can map newvec to Z.  Skip this candidate.
            continue

        newvec_in_plane = newvec - v2 * newvec_proj_along
        z_in_plane = z_axis - v2 * z_proj_along

        if newvec_in_plane.Length < 1e-12 and z_in_plane.Length < 1e-12:
            # Both along rotation axis — any angle works
            second_val = 0.0
        elif newvec_in_plane.Length < 1e-12 or z_in_plane.Length < 1e-12:
            continue
        else:
            # Signed angle from newvec_in_plane to z_in_plane about v2
            cos_a = newvec_in_plane.dot(z_in_plane) / (newvec_in_plane.Length * z_in_plane.Length)
            cos_a = max(-1.0, min(1.0, cos_a))
            cross = newvec_in_plane.cross(z_in_plane)
            sin_a = cross.dot(v2) / (newvec_in_plane.Length * z_in_plane.Length)
            second_val = math.degrees(math.atan2(sin_a, cos_a))

        # Check second axis limits with k*360 variants
        for offset in [0, 360, -360]:
            sv = second_val + offset
            if sv >= second_axis.min_limit and sv <= second_axis.max_limit:
                candidates.append({first_axis.name: first_val, second_axis.name: sv})

    return candidates


def _decompose_2axis_head(
    first_axis: RotaryAxis, second_axis: RotaryAxis, desired_axis: FreeCAD.Vector
) -> List[Dict[str, float]]:
    """Decompose for head rotaries: find angles where R.multVec(Z) = desired.

    Solves: Rot(second, s) * Rot(first, f) . multVec(Z) = desired

    The first axis tilts Z away from vertical. The tilt angle is determined
    by the component of desired perpendicular to the second axis's rotation
    plane. The second axis then rotates the tilted vector azimuthally.

    Args:
        first_axis: Axis to apply first (tilt)
        second_axis: Axis to apply second (azimuthal)
        desired_axis: Target unit vector

    Returns:
        List of candidate angle dictionaries
    """
    z_axis = FreeCAD.Vector(0, 0, 1)

    # The tilt angle is the angle between Z and desired, measured in the
    # plane that contains both Z and the first_axis rotation vector.
    # For standard axes this simplifies to acos(dz).
    tilt = math.degrees(z_axis.getAngle(desired_axis))

    # The first axis rotation direction determines the sign.
    # Rot(first_axis, +angle) tilts Z toward the cross product direction.
    # We need to determine the correct sign by checking which direction
    # the first axis tilts Z relative to desired.
    cross = first_axis.rotation_vector.cross(z_axis)
    if cross.Length > 1e-12:
        # Check if desired has a component in the cross direction
        # If positive, tilt is positive; if negative, tilt is negative
        if desired_axis.dot(cross.normalize()) < 0:
            tilt = -tilt

    # Generate first axis candidates
    first_candidates = [tilt, tilt + 180, tilt - 180, tilt + 360, tilt - 360]
    first_valid = [a for a in first_candidates if first_axis.min_limit <= a <= first_axis.max_limit]

    second_ref, second_plane = _get_relangle_params(second_axis.rotation_vector)

    candidates = []
    for first_val in first_valid:
        # Apply first rotation to Z to get intermediate vector
        first_rot = FreeCAD.Rotation(first_axis.rotation_vector, first_val)
        intermediate = first_rot.multVec(z_axis)

        # Find second axis angle that rotates intermediate toward desired.
        # Both vectors should have the same Z-component (same tilt from Z).
        # The second axis rotates in the plane perpendicular to its rotation
        # vector, so we find the angle between the projections.
        intermediate_angle = _relAngle(intermediate, second_ref, second_plane)
        desired_angle = _relAngle(desired_axis, second_ref, second_plane)
        second_val = intermediate_angle - desired_angle

        # Normalize to [-180, 180]
        while second_val > 180:
            second_val -= 360
        while second_val <= -180:
            second_val += 360

        if second_val >= second_axis.min_limit and second_val <= second_axis.max_limit:
            candidates.append({first_axis.name: first_val, second_axis.name: second_val})

    return candidates


def _solve_analytical_2axis(
    chain: List[RotaryAxis], desired_axis: FreeCAD.Vector, current_state: Dict[str, float]
) -> List[Dict[str, float]]:
    """Solve for 2 rotary axes using analytical decomposition.

    Tries decomposition in both orders (chain order and reversed) to handle
    cases where the desired vector is parallel to one axis's rotation vector
    (singularity in one decomposition order but not the other).

    For head rotaries, uses a dedicated decomposition that solves
    R.multVec(Z) = desired instead of R.multVec(desired) = Z.

    Forward kinematics validation in the caller filters out incorrect solutions.

    Args:
        chain: List of exactly 2 RotaryAxis objects
        desired_axis: Unit vector for desired tool direction
        current_state: Current axis angles

    Returns:
        List of candidate angle dictionaries
    """
    if len(chain) != 2:
        return []

    all_head = all(ax.role == AxisRole.HEAD_ROTARY for ax in chain)

    # Try both decomposition orders and combine candidates.
    # For table-table, the _relAngle decomposition works best when the
    # azimuthal (Z-rotation) axis is solved first, matching legacy behavior.
    # FK validation filters out incorrect candidates from either order.
    if all_head:
        # The tilt (non-Z-rotation) axis must be solved first
        tilt_idx = None
        for i, ax in enumerate(chain):
            if abs(ax.rotation_vector.z) < 0.9:
                tilt_idx = i
                break
        if tilt_idx is not None:
            azimuth_idx = 1 - tilt_idx
            candidates = _decompose_2axis_head(chain[tilt_idx], chain[azimuth_idx], desired_axis)
            candidates += _decompose_2axis_head(chain[azimuth_idx], chain[tilt_idx], desired_axis)
        else:
            candidates = _decompose_2axis_head(chain[0], chain[1], desired_axis)
            candidates += _decompose_2axis_head(chain[1], chain[0], desired_axis)
    else:
        # Try both decomposition orders; FK validation filters incorrect ones.
        # _decompose_2axis(first, second) solves R_second · R_first · desired = Z.
        # compute_rotation_matrix sorts axes: azimuth (Z-rot) first, tilt last,
        # giving R_tilt · R_azimuth.  Match by decomposing azimuth-first.
        # Identify azimuth (Z-rotation) and tilt axes.
        azimuth_idx = None
        for i, ax in enumerate(chain):
            if abs(ax.rotation_vector.z) > 0.9:
                azimuth_idx = i
                break
        if azimuth_idx is not None:
            tilt_idx = 1 - azimuth_idx
            # Primary: azimuth first → R_tilt · R_azimuth
            candidates = _decompose_2axis(chain[azimuth_idx], chain[tilt_idx], desired_axis)
            candidates += _decompose_2axis(chain[tilt_idx], chain[azimuth_idx], desired_axis)
        else:
            # No clear azimuth axis; try both orders
            candidates = _decompose_2axis(chain[0], chain[1], desired_axis)
            candidates += _decompose_2axis(chain[1], chain[0], desired_axis)

    return candidates


def _get_relangle_params(rotation_vector: FreeCAD.Vector) -> Tuple[FreeCAD.Vector, FreeCAD.Vector]:
    """Get the reference axis and plane normal for relAngle computation.

    Maps rotation axis direction to the appropriate reference/plane pair
    following the legacy convention:
      - X rotation (A): ref=Z, plane=X  (project onto YZ, measure from Z)
      - Y rotation (B): ref=Z, plane=Y  (project onto XZ, measure from Z)
      - Z rotation (C): ref=Y, plane=Z  (project onto XY, measure from Y)

    Args:
        rotation_vector: The axis's rotation vector (unit vector)

    Returns:
        Tuple of (reference_axis, plane_normal)
    """
    if abs(rotation_vector.x) > 0.9:
        return FreeCAD.Vector(0, 0, 1), FreeCAD.Vector(1, 0, 0)
    elif abs(rotation_vector.y) > 0.9:
        return FreeCAD.Vector(0, 0, 1), FreeCAD.Vector(0, 1, 0)
    elif abs(rotation_vector.z) > 0.9:
        return FreeCAD.Vector(0, 1, 0), FreeCAD.Vector(0, 0, 1)
    else:
        # Non-standard axis - best effort
        return FreeCAD.Vector(0, 0, 1), rotation_vector.normalize()


def _solve_generic_2axis(
    chain: List[RotaryAxis], desired_axis: FreeCAD.Vector, current_state: Dict[str, float]
) -> List[Dict[str, float]]:
    """Fallback generic 2-axis solve for non-standard configurations."""
    axis1, axis2 = chain

    # For now, return empty - will be implemented with numerical solver
    Path.Log.info(f"Generic 2-axis solve not implemented for {axis1.name}-{axis2.name}")
    return []


def _solve_single_axis(
    chain: List[RotaryAxis], desired_axis: FreeCAD.Vector, current_state: Dict[str, float]
) -> List[Dict[str, float]]:
    """Solve for single rotary axis.

    Uses the same relAngle approach as the legacy generator.
    """
    if len(chain) != 1:
        return []

    axis = chain[0]
    rot_vec = axis.rotation_vector.normalize()

    # Choose reference and plane based on rotation axis direction
    # This mirrors the legacy relAngle conventions
    if abs(rot_vec.x) > 0.9:
        # X rotation (A-like): measure relative to Z, project onto YZ (plane normal = X)
        ref = FreeCAD.Vector(0, 0, 1)
        plane = FreeCAD.Vector(1, 0, 0)
    elif abs(rot_vec.y) > 0.9:
        # Y rotation (B-like): measure relative to Z, project onto XZ (plane normal = Y)
        ref = FreeCAD.Vector(0, 0, 1)
        plane = FreeCAD.Vector(0, 1, 0)
    elif abs(rot_vec.z) > 0.9:
        # Z rotation (C-like): measure relative to Y, project onto XY (plane normal = Z)
        ref = FreeCAD.Vector(0, 1, 0)
        plane = FreeCAD.Vector(0, 0, 1)
    else:
        # Non-standard axis - use Z as reference
        ref = FreeCAD.Vector(0, 0, 1)
        plane = rot_vec

    angle = _relAngle(desired_axis, ref, plane)

    # Generate candidates within limits
    candidates = []

    # Base solution
    if axis.min_limit <= angle <= axis.max_limit:
        candidates.append({axis.name: angle})

    # k*360° alternatives
    for offset in [-360, 360]:
        alt = angle + offset
        if axis.min_limit <= alt <= axis.max_limit:
            candidates.append({axis.name: alt})

    # Flip if allowed
    if axis.allow_flip:
        for flip_offset in [180, -180]:
            flip = angle + flip_offset
            if axis.min_limit <= flip <= axis.max_limit:
                candidates.append({axis.name: flip})

    return candidates


def _expand_solution_space(
    chain: List[RotaryAxis], base_solutions: List[Dict[str, float]]
) -> List[Dict[str, float]]:
    """Expand base solutions with k*360° offsets and flip equivalents.

    Args:
        chain: List of RotaryAxis objects
        base_solutions: List of base angle solutions

    Returns:
        Expanded list of candidate solutions
    """
    expanded = []

    for solution in base_solutions:
        # For each axis, generate equivalent angles within limits
        axis_variants = {}

        for axis in chain:
            if axis.name not in solution:
                continue

            base_angle = solution[axis.name]
            variants = []

            # Generate k*360° equivalents
            k_min = math.floor((axis.min_limit - base_angle) / 360)
            k_max = math.ceil((axis.max_limit - base_angle) / 360)

            for k in range(int(k_min) - 1, int(k_max) + 2):
                angle = base_angle + k * 360
                if axis.min_limit <= angle <= axis.max_limit:
                    variants.append(angle)

            # Add flipped equivalents if allowed
            if axis.allow_flip:
                flip_angle = base_angle + 180
                k_min = math.floor((axis.min_limit - flip_angle) / 360)
                k_max = math.ceil((axis.max_limit - flip_angle) / 360)

                for k in range(int(k_min) - 1, int(k_max) + 2):
                    angle = flip_angle + k * 360
                    if axis.min_limit <= angle <= axis.max_limit:
                        variants.append(angle)

            axis_variants[axis.name] = variants

        # Generate all combinations
        if not axis_variants:
            continue

        # Generate Cartesian product of all axis variants
        axis_names = [axis.name for axis in chain if axis.name in axis_variants]
        if not axis_names:
            continue

        # Build combinations iteratively
        combos = [{}]
        for aname in axis_names:
            new_combos = []
            for combo in combos:
                for val in axis_variants[aname]:
                    c = combo.copy()
                    c[aname] = val
                    new_combos.append(c)
            combos = new_combos

        expanded.extend(combos)

    return expanded


def _compute_solution_cost(
    chain: List[RotaryAxis], solution: Dict[str, float], current_state: Dict[str, float]
) -> float:
    """Compute cost of a solution relative to current state.

    Args:
        chain: List of RotaryAxis objects
        solution: Proposed solution angles
        current_state: Current axis angles

    Returns:
        Cost value (lower is better)
    """
    total_cost = 0.0

    for axis in chain:
        if axis.name not in solution:
            continue

        if axis.name in current_state:
            delta = solution[axis.name] - current_state[axis.name]
            wrapped_delta = _wrap_angle(delta)
        else:
            # No current state: use absolute angle (matches legacy shortest-transit)
            wrapped_delta = solution[axis.name]

        # Apply solution preference weighting
        weight = 1.0
        if axis.solution_preference == "positive" and wrapped_delta < 0:
            weight = 2.0  # Penalize negative movement
        elif axis.solution_preference == "negative" and wrapped_delta > 0:
            weight = 2.0  # Penalize positive movement

        total_cost += weight * abs(wrapped_delta)

    return total_cost


def solve_orientation(
    machine: Machine,
    desired_tool_axis: FreeCAD.Vector,
    current_state: Optional[Dict[str, float]] = None,
    tolerance: float = 1e-6,
    restrict_axes: Optional[List[str]] = None,
) -> SolveResult:
    """Compute indexed rotary positions to achieve desired tool orientation.

    Args:
        machine: Machine configuration
        desired_tool_axis: Desired tool direction in world frame (unit vector)
        current_state: Current rotary axis values {axis_name: degrees}
        tolerance: Orientation error threshold
        restrict_axes: Optional list of axis names to include in solve

    Returns:
        SolveResult with angles, cost, and status
    """
    if current_state is None:
        current_state = {}

    # Normalize desired axis
    desired_tool_axis = desired_tool_axis.normalize()

    # Build kinematic chain
    chain = build_kinematic_chain(machine, restrict_axes)

    if not chain:
        return SolveResult(
            success=False,
            angles={},
            deltas={},
            cost=0.0,
            singular=False,
            error_norm=0.0,
            reason="No rotary axes available",
        )

    # Generate candidate solutions
    if len(chain) == 2:
        # Analytical solve for 2 axes
        base_solutions = _solve_analytical_2axis(chain, desired_tool_axis, current_state)
    elif len(chain) == 1:
        base_solutions = _solve_single_axis(chain, desired_tool_axis, current_state)
    else:
        return SolveResult(
            success=False,
            angles={},
            deltas={},
            cost=0.0,
            singular=False,
            error_norm=0.0,
            reason=f"Analytical solve not implemented for {len(chain)} axes",
        )
    # Expand solution space
    candidates = _expand_solution_space(chain, base_solutions)

    if not candidates:
        return SolveResult(
            success=False,
            angles={},
            deltas={},
            cost=0.0,
            singular=False,
            error_norm=0.0,
            reason="No valid solutions within axis limits",
        )

    # Evaluate candidates
    best_solution = None
    best_cost = float("inf")
    best_error = float("inf")

    for candidate in candidates:
        # Check limits
        valid = True
        for axis in chain:
            if axis.name in candidate:
                angle = candidate[axis.name]
                if not axis.min_limit <= angle <= axis.max_limit:
                    valid = False
                    break

        if not valid:
            continue

        # Validate candidate via forward kinematics.
        # Strategy depends on axis roles:
        #   All table: R.multVec(desired) ≈ Z
        #   All head:  R.multVec(Z) ≈ desired
        #   Mixed:     self-consistency of decomposition math
        error = float("inf")
        if len(chain) >= 2:
            all_table = all(ax.role == AxisRole.TABLE_ROTARY for ax in chain)
            all_head = all(ax.role == AxisRole.HEAD_ROTARY for ax in chain)

            if all_table or all_head:
                rot = compute_rotation_matrix(chain, candidate)
                if all_head:
                    achieved = rot.multVec(FreeCAD.Vector(0, 0, 1))
                    target = desired_tool_axis
                else:
                    achieved = rot.multVec(desired_tool_axis)
                    target = FreeCAD.Vector(0, 0, 1)
                error = (achieved - target).Length
            else:
                # Mixed: self-consistency check (try both decomposition orders)
                decomp_error = float("inf")
                for first_ax, second_ax in [(chain[0], chain[1]), (chain[1], chain[0])]:
                    second_ref, second_plane = _get_relangle_params(second_ax.rotation_vector)
                    first_rot = FreeCAD.Rotation(first_ax.rotation_vector, candidate[first_ax.name])
                    newvec = first_rot.multVec(desired_tool_axis)
                    check_second = _relAngle(newvec, second_ref, second_plane)
                    err = abs(_wrap_angle(check_second - candidate[second_ax.name]))
                    if err < decomp_error:
                        decomp_error = err
                error = decomp_error
        elif len(chain) == 1:
            # Single axis: the _relAngle decomposition is direct, just
            # verify the candidate is an equivalent angle (base ± k*180/360)
            axis = chain[0]
            ref, plane = _get_relangle_params(axis.rotation_vector)
            check_angle = _relAngle(desired_tool_axis, ref, plane)
            diff = abs(check_angle - candidate[axis.name])
            error = min(diff, abs(diff - 360), abs(diff + 360), abs(diff - 180), abs(diff + 180))

        if error > tolerance:
            continue

        # Compute cost
        cost = _compute_solution_cost(chain, candidate, current_state)

        # Update best solution
        if cost < best_cost or (abs(cost - best_cost) < 1e-9 and error < best_error):
            best_solution = candidate
            best_cost = cost
            best_error = error

    if best_solution is None:
        return SolveResult(
            success=False,
            angles={},
            deltas={},
            cost=0.0,
            singular=False,
            error_norm=0.0,
            reason="No solutions meet tolerance requirements",
        )

    # Compute deltas
    deltas = {}
    for name, angle in best_solution.items():
        if name in current_state:
            deltas[name] = _wrap_angle(angle - current_state[name])
        else:
            deltas[name] = angle

    return SolveResult(
        success=True,
        angles=best_solution,
        deltas=deltas,
        cost=best_cost,
        singular=False,  # TODO: Implement singularity detection
        error_norm=best_error,
        reason="",
    )
