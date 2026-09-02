# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``Part.ShapeFix`` submodule helpers.

This source-adjacent stub file carries the standalone shape-fixing helpers
exposed directly by the Part ShapeFix submodule.
"""

from __future__ import annotations

from Part import Shape

def sameParameter(shape: Shape, enforce: bool, prec: float = 0.0, /) -> bool:
    """Recompute same-parameter consistency on one shape and report whether it succeeded."""
    ...

def encodeRegularity(shape: Shape, tolerance: float = 1.0e-10, /) -> None:
    """Encode one shape's edge regularity relationships using the given angular tolerance."""
    ...

def removeSmallEdges(shape: Shape, tolerance: float, /) -> Shape:
    """Return one copy of the shape with edges smaller than the given tolerance removed."""
    ...

def fixVertexPosition(shape: Shape, tolerance: float, /) -> bool:
    """Adjust vertex positions on one shape when their stored tolerances exceed the limit."""
    ...

def leastEdgeSize(shape: Shape, /) -> float:
    """Return the length of the smallest edge contained in one shape."""
    ...
