# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the private ``_PartDesign`` helper module.

This source-adjacent stub file carries the geometric helper that is exposed by
the low-level PartDesign extension module.
"""

from __future__ import annotations

from typing import TypeAlias

from FreeCAD.Base import Vector

_FilletArcConstruction: TypeAlias = tuple[Vector, Vector, Vector]

def makeFilletArc(
    m1: Vector,
    p: Vector,
    q: Vector,
    n: Vector,
    radius: float,
    ccw: bool | int,
    /,
) -> _FilletArcConstruction:
    """Return the tangent points and arc center for one fillet-arc construction."""
    ...
