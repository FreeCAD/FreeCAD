# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewPart import DrawViewPart


@export(
    Include="Mod/TechDraw/App/DrawBrokenView.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewPartPy.h",
)
class DrawBrokenView(DrawViewPart):
    """
    Feature for creating and manipulating Technical Drawing broken views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def mapPoint3dToView(self) -> Any:
        """point2d = mapPoint3dToView(point3d) - returns the position of the 3d point within the broken view."""
        ...

    def mapPoint2dFromView(self) -> Any:
        """point2d = mapPoint2dFromView(point3d) - returns the position of the 2d point within an unbroken view."""
        ...

    def getCompressedCenter(self) -> Any:
        """point3d = getCompressedCenter() - returns the geometric center of the source shapes after break cuts and gap compression."""
        ...
