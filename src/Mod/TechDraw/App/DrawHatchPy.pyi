from typing import Any
from Base.Metadata import export
from App.DocumentObject import DocumentObject

@export(
    Father="DocumentObjectPy",
    Name="DrawHatchPy",
    Twin="DrawHatch",
    TwinPointer="DrawHatch",
    Include="Mod/TechDraw/App/DrawHatch.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawHatchPy(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing Hatch areas
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
