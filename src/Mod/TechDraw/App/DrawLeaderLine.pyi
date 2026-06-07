# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from DrawView import DrawView


@export(
    Namespace="TechDraw",
)
class DrawLeaderLine(DrawView):
    """
    Feature for adding leaders to Technical Drawings

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
