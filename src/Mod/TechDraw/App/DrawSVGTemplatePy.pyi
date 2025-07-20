from typing import Any

from Base.Metadata import export
from DrawTemplate import DrawTemplate

@export(
    Father="DrawTemplatePy",
    Name="DrawSVGTemplatePy",
    Twin="DrawSVGTemplate",
    TwinPointer="DrawSVGTemplate",
    Include="Mod/TechDraw/App/DrawSVGTemplate.h",
    Namespace="TechDraw",
    FatherInclude="DrawTemplatePy.h",
    FatherNamespace="TechDraw",
)
class DrawSVGTemplatePy(DrawTemplate):
    """
    Feature for creating and manipulating Technical Drawing SVG Templates
    """

    def getEditFieldContent(self) -> Any:
        """getEditFieldContent(EditFieldName) - returns the content of a specific Editable Text Field"""
        ...

    def setEditFieldContent(self) -> Any:
        """setEditFieldContent(EditFieldName, NewContent) - sets a specific Editable Text Field to a new value"""
        ...

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...
