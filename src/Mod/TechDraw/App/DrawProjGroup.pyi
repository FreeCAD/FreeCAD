# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from TechDraw.DrawViewCollection import DrawViewCollection


@export(
    Include="Mod/TechDraw/App/DrawProjGroup.h",
    Namespace="TechDraw",
    FatherInclude="Mod/TechDraw/App/DrawViewCollectionPy.h",
)
class DrawProjGroup(DrawViewCollection):
    """
    Feature for creating and manipulating Technical Drawing Projection Groups

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def addProjection(self) -> Any:
        """addProjection(string projectionType) - Add a new Projection Item to this Group. Returns DocObj."""
        ...

    def removeProjection(self) -> Any:
        """removeProjection(string projectionType) - Remove specified Projection Item from this Group. Returns int number of views in Group."""
        ...

    def purgeProjections(self) -> Any:
        """purgeProjections() - Remove all Projection Items from this Group. Returns int number of views in Group (0)."""
        ...

    def getItemByLabel(self) -> Any:
        """getItemByLabel(string projectionType) - return specified Projection Item"""
        ...

    def getXYPosition(self) -> Any:
        """getXYPosition(string projectionType) - return the AutoDistribute position for specified Projection Item"""
        ...
