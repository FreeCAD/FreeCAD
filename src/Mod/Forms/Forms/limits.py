# SPDX-License-Identifier: LGPL-2.1-or-later
"""Allocation budgets shared by topology edits, previews and conversion."""

MAX_SAMPLE_FACES = 250_000
MAX_ROOT_GRID_SIDE = 129
MAX_TOPOLOGY_FACES = 250_000


def check_sampling(face_count, level, root_grid=False):
    """Reject excessive work before subdivision or dense fitting allocates it."""
    level = int(level)
    if level < 0 or level > 16 or int(face_count) * (4 ** level) > MAX_SAMPLE_FACES:
        raise ValueError("The cage/refinement combination exceeds the 250000-face sampling limit")
    if root_grid and (2 ** max(0, level - 1) + 1) > MAX_ROOT_GRID_SIDE:
        raise ValueError("Local refinement exceeds the supported B-spline grid size")


def check_topology(face_count):
    if int(face_count) > MAX_TOPOLOGY_FACES:
        raise ValueError("The requested subdivision exceeds the topology budget")
