# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Annotated, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import cxx_type, export
from Base import Vector


@export(
    Include="Mod/TechDraw/App/Cosmetic.h",
    Delete=True,
)
class CosmeticEdge(PyObjectBase):
    """
    CosmeticEdge specifies an extra (cosmetic) edge in Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def __init__(self) -> None: ...

    Tag: Final[str]
    """Gives the tag of the CosmeticEdge as string."""

    Start: Annotated[Vector, cxx_type("Vector")]
    """Gives the position of one end of this CosmeticEdge as vector."""

    End: Annotated[Vector, cxx_type("Vector")]
    """Gives the position of one end of this CosmeticEdge as vector."""

    Center: Annotated[Vector, cxx_type("Vector")]
    """Gives the position of center point of this CosmeticEdge as vector."""

    Radius: float
    """Gives the radius of CosmeticEdge in mm."""

    Format: dict
    """The appearance attributes (style, weight, color, visible) for this CosmeticEdge."""
