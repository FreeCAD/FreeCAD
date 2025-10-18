# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView


@export(
    Include="Mod/TechDraw/App/DrawViewClip.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawViewClip(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Clip Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def addView(self) -> Any:
        """addView(DrawView) - Add a View to this ClipView"""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView) - Remove specified View to this ClipView"""
        ...

    def getChildViewNames(self) -> Any:
        """getChildViewNames() - get a list of the DrawViews in this ClipView"""
        ...
