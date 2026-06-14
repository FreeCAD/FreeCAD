# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TechDraw.Drawview import DrawView


@export(
    Include="Mod/TechDraw/App/DrawViewAnnotation.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPy.h",
)
class DrawViewAnnotation(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Annotation Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
