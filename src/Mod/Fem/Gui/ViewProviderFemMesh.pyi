# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Final

from Base.Metadata import export
from Base.Vector import Vector

from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject

@export(
    Include="Mod/Fem/Gui/ViewProviderFemMesh.h",
    Namespace="FemGui",
)
class ViewProviderFemMesh(ViewProviderGeometryObject):
    """
    ViewProviderFemMesh class

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    def applyDisplacement(self, factor: float, /) -> None:
        """"""
        ...

    def resetNodeColor(self) -> None:
        """Reset color set by method setNodeColorByScalars."""
        ...

    def resetNodeDisplacement(self) -> None:
        """Reset displacements set by method setNodeDisplacementByVectors."""
        ...

    def resetHighlightedNodes(self) -> None:
        """Reset highlighted nodes."""
        ...

    def setNodeColorByScalars(self, node_ids: list[int], values: list[float], /) -> None:
        """Sets mesh node colors using element list and value list."""
        ...

    def setNodeDisplacementByVectors(
        self, node_ids: list[int], vectors: list[Vector | tuple[float, float, float]], /
    ) -> None:
        """"""
        ...
    NodeColor: dict
    """Postprocessing color of the nodes. The faces between the nodes get interpolated."""

    ElementColor: dict
    """Postprocessing color of the elements. All faces of the element get the same color."""

    NodeDisplacement: dict
    """Postprocessing color of the nodes. The faces between the nodes get interpolated."""

    HighlightedNodes: list
    """List of nodes which get highlighted."""

    VisibleElementFaces: Final[list]
    """List of elements and faces which are actually shown. These are all surface faces of the mesh."""
