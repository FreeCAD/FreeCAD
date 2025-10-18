# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from PyObjectBase import PyObjectBase
from Vector import Vector
from Placement import Placement
from typing import overload

@export(
    Constructor=True,
    Delete=True,
)
class Axis(PyObjectBase):
    """
    Base.Axis class.

    An Axis defines a direction and a position (base) in 3D space.

    The following constructors are supported:

    Axis()
    Empty constructor.

    Axis(axis)
    Copy constructor.
    axis : Base.Axis

    Axis(base, direction)
    Define from a position and a direction.
    base : Base.Vector
    direction : Base.Vector
    """

    Base: Vector = ...
    """Base position vector of the Axis."""

    Direction: Vector = ...
    """Direction vector of the Axis."""

    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, axis: Axis) -> None: ...
    @overload
    def __init__(self, base: Vector, direction: Vector) -> None: ...
    # fmt: on

    def copy(self) -> Axis:
        """
        Returns a copy of this Axis.
        """
        ...

    def move(self, vector: Vector, /) -> None:
        """
        Move the axis base along the given vector.

        vector : Base.Vector
            Vector by which to move the axis.
        """
        ...

    def multiply(self, placement: Placement, /) -> Axis:
        """
        Multiply this axis by a placement.

        placement : Base.Placement
            Placement by which to multiply the axis.
        """
        ...

    def reversed(self) -> Axis:
        """
        Compute the reversed axis. This returns a new Base.Axis with
        the original direction reversed.
        """
        ...
