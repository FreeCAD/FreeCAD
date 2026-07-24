# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, sequence_protocol
from Base.Vector import Vector
from Point import Point
from TopoShape import TopoShape
from typing import Final, overload

@export(
    Twin="TopoShape",
    TwinPointer="TopoShape",
    FatherInclude="Mod/Part/App/TopoShapePy.h",
    Include="Mod/Part/App/TopoShape.h",
    Constructor=True,
    RichCompare=True,
)
@sequence_protocol(
    sq_length=True,
    sq_concat=False,
    sq_repeat=False,
    sq_item=True,
    mp_subscript=False,
    sq_ass_item=False,
    mp_ass_subscript=False,
    sq_contains=False,
    sq_inplace_concat=False,
    sq_inplace_repeat=False,
)
class TopoShapeVertex(TopoShape):
    """
    TopoShapeVertex is the OpenCasCade topological vertex wrapper

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    """

    @overload
    def __init__(self, x: float = ..., y: float = ..., z: float = ..., /) -> None: ...
    @overload
    def __init__(self, coordinates: Vector, /) -> None: ...
    @overload
    def __init__(self, coordinates: tuple[float, float, float], /) -> None: ...
    @overload
    def __init__(self, point: Point, /) -> None: ...
    @overload
    def __init__(self, shape: TopoShape, /) -> None: ...

    X: Final[float] = ...
    """X component of this Vertex."""

    Y: Final[float] = ...
    """Y component of this Vertex."""

    Z: Final[float] = ...
    """Z component of this Vertex."""

    Point: Final[Vector] = ...
    """Position of this Vertex as a Vector"""

    Tolerance: float = ...
    """Set or get the tolerance of the vertex"""
