# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewPart import DrawViewPart


@export(
    Include="Mod/TechDraw/App/DrawProjGroupItem.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPartPy.h",
)
class DrawProjGroupItem(DrawViewPart):
    """
    Feature for creating and manipulating component Views Technical Drawing Projection Groups

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def autoPosition(self) -> Any:
        """autoPosition() - Move to AutoDistribute/Unlocked position on Page. Returns none."""
        ...
