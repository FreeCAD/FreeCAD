# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView


@export(
    Include="Mod/TechDraw/App/DrawViewCollection.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawViewCollection(DrawView):
    """
    Feature for creating and manipulating Technical Drawing View Collections

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def addView(self) -> Any:
        """addView(DrawView object) - Add a new View to this Group. Returns count of views."""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView object) - Remove specified Viewfrom this Group. Returns count of views in Group."""
        ...
