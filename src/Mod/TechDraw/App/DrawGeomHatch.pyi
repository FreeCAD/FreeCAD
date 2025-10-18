# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any

from Base.Metadata import export
from App.DocumentObject import DocumentObject


@export(
    Include="Mod/TechDraw/App/DrawGeomHatch.h",
    Namespace="TechDraw",
)
class DrawGeomHatch(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing GeomHatch areas

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
