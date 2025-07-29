from typing import Any

from Base.Metadata import export
from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="DrawGeomHatchPy",
    Twin="DrawGeomHatch",
    TwinPointer="DrawGeomHatch",
    Include="Mod/TechDraw/App/DrawGeomHatch.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawGeomHatchPy(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing GeomHatch areas
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
