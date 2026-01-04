# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export

from App.DocumentObject import DocumentObject


@export(
    Include="Mod/TechDraw/App/DrawTile.h",
    Namespace="TechDraw",
)
class DrawTile(DocumentObject):
    """
    Feature for adding tiles to leader lines

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
