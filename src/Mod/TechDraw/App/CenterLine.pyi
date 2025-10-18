# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import constmethod, export


@export(
    Include="Mod/TechDraw/App/CenterLine.h",
    Namespace="TechDraw",
    Constructor=True,
    Delete=True,
)
class CenterLine(PyObjectBase):
    """
    CenterLine specifies additional mark up edges in a View

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this centerline"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this centerline"""
        ...

    Tag: Final[str]
    """Gives the tag of the CenterLine as string."""

    Type: Final[int]
    """0 - face, 1 - 2 line, 2 - 2 point."""

    Mode: int
    """0 - vert/ 1 - horiz/ 2 - aligned."""

    Format: dict[str, Any]
    """The appearance attributes (style, color, weight, visible) for this CenterLine."""

    HorizShift: float
    """The left/right offset for this CenterLine."""

    VertShift: float
    """The up/down offset for this CenterLine."""

    Rotation: float
    """The rotation of the Centerline in degrees."""

    Extension: float
    """The additional length to be added to this CenterLine."""

    Flip: bool
    """Reverse the order of points for 2 point CenterLine."""

    Edges: list[Any]
    """The names of source edges for this CenterLine."""

    Faces: list[Any]
    """The names of source Faces for this CenterLine."""

    Points: list[Any]
    """The names of source Points for this CenterLine."""
