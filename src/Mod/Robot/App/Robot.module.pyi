# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Robot`` module-level helpers.

This source-adjacent stub file carries the standalone robot simulation helper
exposed directly by the Robot application module.
"""

from __future__ import annotations

def simulateToFile(
    robot: Robot6Axis,
    trajectory: Trajectory,
    tick_size: float,
    file_name: str,
    /,
) -> float:
    """Run one robot simulation, write its result to one file, and return the numeric status."""
    ...
