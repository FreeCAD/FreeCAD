from typing import ClassVar, Final, List, Dict, Tuple, TypeVar, Any, Optional, Union, overload
from App import DocumentObjectPy
from Metadata import export

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
class DrawHatchPy(DocumentObjectPy):
    """
        Feature for creating and manipulating Technical Drawing Hatch areas
    """
    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
                 No return value.  Replace the current label with a translated version where possible.
        """
        ...


