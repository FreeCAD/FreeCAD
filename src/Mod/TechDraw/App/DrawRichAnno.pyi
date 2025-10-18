# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TechDraw.DrawView import DrawView


@export(
    Include="Mod/TechDraw/App/DrawRichAnno.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawRichAnno(DrawView):
    """
    Feature for adding rich annotation blocks to Technical Drawings

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
