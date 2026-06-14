# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import constmethod, export


@export(
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    Constructor=True,
    Delete=True,
)
class CosmeticVertex(PyObjectBase):
    """
    CosmeticVertex specifies an extra (cosmetic) vertex in Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this CosmeticVertex"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this CosmeticVertex"""
        ...

    Tag: Final[str]
    """Gives the tag of the CosmeticVertex as string."""

    Point: Any
    """Gives the position of this CosmeticVertex as vector."""

    Show: bool
    """Show/hide the vertex."""

    Color: Any  # type: tuple[float, float, float, float]]
    """set/return the vertex's colour using a tuple (rgba)."""

    Size: Any
    """set/return the vertex's radius in mm."""

    Style: Any
    """set/return the vertex's style as integer."""
