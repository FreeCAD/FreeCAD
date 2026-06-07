# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from TechDraw.Drawview import DrawView


@export(
    Namespace="TechDraw",
)
class DrawViewAnnotation(DrawView):
    """
    Feature for creating and manipulating Technical Drawing Annotation Views

    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """
