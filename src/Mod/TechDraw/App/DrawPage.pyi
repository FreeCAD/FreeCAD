# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final

from Base.Metadata import export
from App.DocumentObject import DocumentObject


@export(
    Include="Mod/TechDraw/App/DrawPage.h",
    Namespace="TechDraw",
)
class DrawPage(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Pages

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def addView(self) -> Any:
        """addView(DrawView) - Add a View to this Page"""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView) - Remove a View to this Page"""
        ...

    def getViews(self) -> Any:
        """getViews() - returns a list of all the views on page excluding Views inside Collections"""
        ...

    def getAllViews(self) -> Any:
        """getAllViews() - returns a list of all the views on page including Views inside Collections"""
        ...

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...

    def requestPaint(self) -> Any:
        """Ask the Gui to redraw this page"""
        ...

    PageWidth: Final[float]
    """Returns the width of this page"""

    PageHeight: Final[float]
    """Returns the height of this page"""

    PageOrientation: Final[str]
    """Returns the orientation of this page"""
