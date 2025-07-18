from typing import (
    ClassVar,
    Final,
    List,
    Dict,
    Tuple,
    TypeVar,
    Any,
    Optional,
    Union,
    overload,
)
from App import DocumentObjectPy
from Metadata import export

@export(
    Father="DocumentObjectPy",
    Name="DrawPagePy",
    Twin="DrawPage",
    TwinPointer="DrawPage",
    Include="Mod/TechDraw/App/DrawPage.h",
    Namespace="TechDraw",
    FatherInclude="App/DocumentObjectPy.h",
    FatherNamespace="App",
)
class DrawPagePy(DocumentObjectPy):
    """
    Feature for creating and manipulating Technical Drawing Pages
    """

    def addView(self) -> Any:
        """addView(DrawView) - Add a View to this Page"""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView) - Remove a View to this Page"""
        ...

    def getViews(self, *args) -> Any:
        """getViews() - returns a list of all the views on page excluding Views inside Collections"""

        ...

    def getAllViews(self, *args) -> Any:
        """getAllViews() - returns a list of all the views on page including Views inside Collections"""
        ...

    def translateLabel(self) -> Any:
        """
        translateLabel(translationContext, objectBaseName, objectUniqueName).
        No return value.  Replace the current label with a translated version where possible.
        """
        ...

    def requestPaint(self) -> Any:
        """Ask the Gui to redraw this page"""
        ...
    
    PageWidth: Final[float]  # Read-only attribute
    """Returns the width of this page"""
    
    PageHeight: Final[float]  # Read-only attribute
    """Returns the height of this page"""
    
    PageOrientation: Final[str] # Read-only attribute
    """Returns the orientation of this page"""