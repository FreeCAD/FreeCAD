# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``PathApp`` module-level helpers.

This source-adjacent stub file carries the G-code I/O and path-construction
helpers exposed directly by the CAM application module.
"""

from __future__ import annotations

from collections.abc import Sequence
from typing import Literal, TypeAlias, overload

from Base.Metadata import deprecated
from FreeCAD import DocumentObject
from FreeCAD.Base import Vector
from Part import Shape, Wire

_ShapeInput: TypeAlias = Shape | Sequence[Shape]
_ArcPlane: TypeAlias = Literal[0, 1, 2, 3, 4, 5] | int
_SortMode: TypeAlias = Literal[0, 1, 2, 3] | int
_Orientation: TypeAlias = Literal[0, 1] | int
_Direction: TypeAlias = Literal[0, 1, 2, 3, 4, 5, 6] | int
_RetractAxis: TypeAlias = Literal[0, 1, 2] | int
_WireSortResult: TypeAlias = tuple[list[Wire], Vector] | tuple[list[Wire], Vector, int]

def write(obj: DocumentObject, filename: str, /) -> None:
    """Export one Path feature document object to one G-code file."""
    ...

@overload
def read(filename: str, /) -> None:
    """Import one G-code file into the active document, creating one if needed."""
    ...

@overload
def read(filename: str, doc_name: str, /) -> None:
    """Import one G-code file into the named document, creating it if needed."""
    ...

def show(toolpath: Path, name: str = "Path", /) -> None:
    """Create one document path feature from one standalone `Path` object."""
    ...

@deprecated(deprecated_in="26.3", removed_in="27.2", replacement="fromShapes")
def fromShape(shape: Shape, /) -> Path:
    """Convert one wire shape into one basic path object."""
    ...

@overload
def fromShapes(
    shapes: _ShapeInput,
    start: Vector | None = None,
    return_end: Literal[False] = False,
    arc_plane: _ArcPlane = 1,
    sort_mode: _SortMode = 1,
    abscissa: float = 3.0,
    nearest_k: int = 3,
    orientation: _Orientation = 0,
    direction: _Direction = 0,
    threshold: float = 0.0,
    retract_axis: _RetractAxis = 2,
    retraction: float = 0.0,
    resume_height: float = 0.0,
    segmentation: float = 0.0,
    feedrate: float = 0.0,
    feedrate_v: float = 0.0,
    verbose: bool = True,
    abs_center: bool = False,
    preamble: bool = True,
    deflection: float = 0.01,
) -> Path:
    """Convert one shape or shape sequence into one path object."""
    ...

@overload
def fromShapes(
    shapes: _ShapeInput,
    start: Vector | None = None,
    return_end: Literal[True] = True,
    arc_plane: _ArcPlane = 1,
    sort_mode: _SortMode = 1,
    abscissa: float = 3.0,
    nearest_k: int = 3,
    orientation: _Orientation = 0,
    direction: _Direction = 0,
    threshold: float = 0.0,
    retract_axis: _RetractAxis = 2,
    retraction: float = 0.0,
    resume_height: float = 0.0,
    segmentation: float = 0.0,
    feedrate: float = 0.0,
    feedrate_v: float = 0.0,
    verbose: bool = True,
    abs_center: bool = False,
    preamble: bool = True,
    deflection: float = 0.01,
) -> tuple[Path, Vector]:
    """Convert one shape or shape sequence into one path object and return its end position."""
    ...

@overload
def sortWires(
    shapes: _ShapeInput,
    start: Vector | None = None,
    arc_plane: Literal[1] = 1,
    sort_mode: _SortMode = 1,
    abscissa: float = 3.0,
    nearest_k: int = 3,
    orientation: _Orientation = 0,
    direction: _Direction = 0,
    threshold: float = 0.0,
    retract_axis: _RetractAxis = 2,
) -> tuple[list[Wire], Vector, int]:
    """Sort one wire set and also return the resolved arc plane when auto-plane mode is used."""
    ...

@overload
def sortWires(
    shapes: _ShapeInput,
    start: Vector | None = None,
    arc_plane: _ArcPlane = 1,
    sort_mode: _SortMode = 1,
    abscissa: float = 3.0,
    nearest_k: int = 3,
    orientation: _Orientation = 0,
    direction: _Direction = 0,
    threshold: float = 0.0,
    retract_axis: _RetractAxis = 2,
) -> _WireSortResult:
    """Sort one wire set and return the sorted wires plus their final end position."""
    ...
