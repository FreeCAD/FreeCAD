# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

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

    def setSelection(self, indices: list[int], /) -> None:
        """Select list of facets"""
        ...

    def addSelection(self, indices: list[int], /) -> None:
        """Add list of facets to selection"""
        ...

    def removeSelection(self, indices: list[int], /) -> None:
        """Remove list of facets from selection"""
        ...

    def invertSelection(self) -> None:
        """Invert the selection"""
        ...

    def clearSelection(self) -> None:
        """Clear the selection"""
        ...

    def highlightSegments(self, colors: list, /) -> None:
        """Highlights the segments of a mesh with a given list of colors.
        The number of elements of this list must be equal to the number of mesh segments.
        """
        ...
