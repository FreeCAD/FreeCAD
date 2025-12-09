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
class GeomFormat(PyObjectBase):
    """
    GeomFormat specifies appearance parameters for TechDraw Geometry objects

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this geomformat"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this geomformat"""
        ...

    Tag: Final[str]
    """Gives the tag of the GeomFormat as string."""
