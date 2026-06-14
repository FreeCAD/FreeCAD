# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any
from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject
from Base.Metadata import export

@export(
    Include="Mod/Mesh/Gui/ViewProvider.h",
    Namespace="MeshGui",
)
class ViewProviderMesh(ViewProviderGeometryObject):
    """
    This is the ViewProvider base class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    License: LGPL-2.1-or-later
    """

    def setSelection(self) -> Any:
        """Select list of facets"""
        ...

    def addSelection(self) -> Any:
        """Add list of facets to selection"""
        ...

    def removeSelection(self) -> Any:
        """Remove list of facets from selection"""
        ...

    def invertSelection(self) -> Any:
        """Invert the selection"""
        ...

    def clearSelection(self) -> Any:
        """Clear the selection"""
        ...

    def highlightSegments(self) -> Any:
        """Highlights the segments of a mesh with a given list of colors.
        The number of elements of this list must be equal to the number of mesh segments.
        """
        ...
