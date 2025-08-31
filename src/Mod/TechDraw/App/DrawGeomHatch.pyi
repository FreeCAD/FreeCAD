from typing import Any

from Base.Metadata import export
from App.DocumentObject import DocumentObject

@export(
    Include="Mod/TechDraw/App/DrawGeomHatch.h",
    Namespace="TechDraw",
)
class DrawGeomHatch(DocumentObject):
    """
    Feature for creating and manipulating Technical Drawing GeomHatch areas
    """

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
