# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TechDraw.DrawViewPart import DrawViewPart


@export(
    Include="Mod/TechDraw/App/DrawAuxiliaryView.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPartPy.h",
)
class DrawAuxiliaryView(DrawViewPart):
    """
    Feature for creating and manipulating Technical Drawing auxiliary views

    License: LGPL-2.1-or-later
    """

    pass
