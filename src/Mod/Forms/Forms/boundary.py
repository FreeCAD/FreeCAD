# SPDX-License-Identifier: LGPL-2.1-or-later
"""Quadratic boundary extension for curvature-continuous subdivision."""

from .controlcage import ControlCage
from .errors import ConversionError


def extended_cage(vertices, faces, corners):
    """Continue the boundary quadratically with an evaluation-only outer ring.

    Ordinary open subdivision extrapolates linearly across the boundary, which
    forces zero transverse normal curvature there. Quadratic continuation gives
    the two interior rows independent first and second derivative control. These
    ghost faces are discarded after sampling; the editable topology is unchanged.
    """
    try:
        import numpy as np
    except ImportError as error:
        raise ConversionError("Curvature Match requires NumPy") from error
    cage = ControlCage(vertices, faces, corners)
    boundary = set(v for edge in cage.boundary_edges for v in edge)
    if any(cage.vertex_sharpness[v] >= 10.0 for v in boundary):
        # Keep explicit corner rules. Their curvature may be incompatible with
        # a curved support, in which case the measured match must fail.
        return vertices, faces, cage.vertex_sharpness
    neighbors = {v: set() for v in range(len(vertices))}
    for a, b in cage.edge_counts():
        neighbors[a].add(b)
        neighbors[b].add(a)
    points = [np.asarray(p, dtype=float) for p in vertices]
    ghost = {}
    for vertex in sorted(boundary):
        inward = neighbors[vertex] - boundary
        if not inward:
            # A rectangular corner has only boundary neighbors; use its diagonal.
            inward = {v for face in faces if vertex in face for v in face if v not in boundary}
        if not inward:
            raise ConversionError("Curvature Match needs at least two interior control rows")
        first = sum((points[v] for v in inward), np.zeros(3)) / len(inward)
        second_indices = set().union(*(neighbors[v] for v in inward)) - boundary - inward
        # The opposite row lies beyond the first one, rather than along it.
        second_indices = {v for v in second_indices if not neighbors[v].intersection(boundary)}
        if not second_indices:
            nearby = set().union(*(neighbors[v] for v in inward)) - boundary - inward
            second_indices = set().union(*(neighbors[v] for v in nearby)) - boundary - inward
            second_indices = {v for v in second_indices if not neighbors[v].intersection(boundary)}
        if not second_indices:
            raise ConversionError("Curvature Match needs at least two interior control rows")
        second = sum((points[v] for v in second_indices), np.zeros(3)) / len(second_indices)
        ghost[vertex] = len(points)
        points.append(3 * points[vertex] - 3 * first + second)
    extended = list(faces)
    for loop in cage.boundary_loops():
        for index, a in enumerate(loop):
            b = loop[(index + 1) % len(loop)]
            # Find the original directed edge so the new face uses its reverse.
            if not any(any(face[i] == a and face[(i+1) % len(face)] == b
                           for i in range(len(face))) for face in faces):
                a, b = b, a
            extended.append((b, a, ghost[a], ghost[b]))
    return [tuple(p) for p in points], extended, list(cage.vertex_sharpness) + [0.0] * len(ghost)
