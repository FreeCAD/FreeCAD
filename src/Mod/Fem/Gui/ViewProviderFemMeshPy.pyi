from typing import Any, Final

from Base.Metadata import export

from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject

@export(
    Father="ViewProviderGeometryObjectPy",
    Name="ViewProviderFemMeshPy",
    Twin="ViewProviderFemMesh",
    TwinPointer="ViewProviderFemMesh",
    Include="Mod/Fem/Gui/ViewProviderFemMesh.h",
    Namespace="FemGui",
    FatherInclude="Gui/ViewProviderGeometryObjectPy.h",
    FatherNamespace="Gui",
)
class ViewProviderFemMeshPy(ViewProviderGeometryObject):
    """
    ViewProviderFemMesh class
    """

    def applyDisplacement(self) -> Any:
        """"""
        ...

    def resetNodeColor(self) -> Any:
        """Reset color set by method setNodeColorByScalars."""
        ...

    def resetNodeDisplacement(self) -> Any:
        """Reset displacements set by method setNodeDisplacementByVectors."""
        ...

    def resetHighlightedNodes(self) -> Any:
        """Reset highlighted nodes."""
        ...

    def setNodeColorByScalars(self) -> Any:
        """Sets mesh node colors using element list and value list."""
        ...

    def setNodeDisplacementByVectors(self) -> Any:
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
