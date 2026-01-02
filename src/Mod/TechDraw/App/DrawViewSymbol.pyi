# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawView import DrawView


@export(
    Include="Mod/TechDraw/App/DrawViewSymbol.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawViewSymbol(DrawView):
    """
    Feature for creating and manipulating Drawing SVG Symbol Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def dumpSymbol(self) -> Any:
        """dumpSymbol(fileSpec) - dump the contents of Symbol to a file"""
        ...
