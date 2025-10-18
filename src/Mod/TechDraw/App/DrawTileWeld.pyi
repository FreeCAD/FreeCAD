# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TechDraw.DrawTile import DrawTile


@export(
    Include="Mod/TechDraw/App/DrawTileWeld.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawTilePy.h",
)
class DrawTileWeld(DrawTile):
    """
    Feature for adding welding tiles to leader lines

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
