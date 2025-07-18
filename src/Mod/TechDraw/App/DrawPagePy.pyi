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
from App import object
from Base.Metadata import export
from Base.Metadata import constmethod

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
class DrawPagePy(object):
    """
    Feature for creating and manipulating Technical Drawing Pages
    """

    def addView(self) -> Any:
        """addView(DrawView) - Add a View to this Page"""
        ...

    def removeView(self) -> Any:
        """removeView(DrawView) - Remove a View to this Page"""
        ...

    def getViews(self) -> Any:
        """getViews() - returns a list of all the views on page excluding Views inside Collections"""
        ...

    def getAllViews(self) -> Any:
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
    PageWidth: Final[float]  # ReadOnly
    """Returns the width of this page"""

    PageHeight: Final[float]  # ReadOnly
    """Returns the height of this page"""

    PageOrientation: Final[str]  # ReadOnly
    """Returns the orientation of this page"""
