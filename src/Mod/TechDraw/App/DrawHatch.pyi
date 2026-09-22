# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from App.DocumentObject import DocumentObject


@export(
    Include="Mod/TechDraw/App/DrawHatch.h",
    Namespace="TechDraw",
)
class DrawHatch(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Hatch areas

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def translateLabel(self, context: str, base_name: str, unique_name: str, /) -> None:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
