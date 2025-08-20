from typing import Any
from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject
from Base.Metadata import export

@export(
    Father="ViewProviderGeometryObjectPy",
    Name="ViewProviderMeshPy",
    Twin="ViewProviderMesh",
    TwinPointer="ViewProviderMesh",
    Include="Mod/Mesh/Gui/ViewProvider.h",
    Namespace="MeshGui",
    FatherInclude="Gui/ViewProviderGeometryObjectPy.h",
    FatherNamespace="Gui",
)
class ViewProviderMeshPy(ViewProviderGeometryObject):
    """
    This is the ViewProvider base class
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
