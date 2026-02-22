# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

from __future__ import annotations

from typing import Any

from Base.Metadata import export, constmethod

from App.DocumentObject import DocumentObject


@export(
    Include="Mod/TechDraw/App/DrawView.h",
    Namespace="TechDraw",
)
class DrawView(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
    @constmethod
    def getScale(self) -> Any:
        """
        float scale = getScale().  Returns the correct scale for this view.  Handles whether to
        use this view's scale property or a parent's view (as in a projection group).
        """
        ...

    @constmethod
    def findParentPage(self) -> Any:
        """
        DrawPage parent = findParentPage().  Returns the parent page that contains this view.
        """
        ...


