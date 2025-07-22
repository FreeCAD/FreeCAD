from DocumentObject import DocumentObject
from Base import Placement
from typing import Any, Final, Optional


class GeoFeature(DocumentObject):
    """
    App.GeoFeature class.

    Base class of all geometric document objects.
    This class does the whole placement and position handling.
    With the method `getPropertyOfGeometry` is possible to obtain
    the main geometric property in general form, without reference
    to any particular property name.
    """

    ElementMapVersion: Final[str] = ""
    """Element map version"""

    def getPaths(self) -> Any:
        """
        getPaths()

        Returns all possible paths to the root of the document.
        Note: Not implemented.
        """
        ...

    def getGlobalPlacement(self) -> Placement:
        """
        getGlobalPlacement() -> Base.Placement
        Deprecated: This function does not handle Links correctly. Use getGlobalPlacementOf instead.

        Returns the placement of the object in the global coordinate space, respecting all stacked
        relationships.
        Note: This function is not available during recompute, as there the placements of parents
        can change after the execution of this object, rendering the result wrong.
        """
        ...

    @staticmethod
    def getGlobalPlacementOf(targetObj: Any, rootObj: Any, subname: str) -> Placement:
        """
        getGlobalPlacementOf(targetObj, rootObj, subname) -> Base.Placement
        Selection example: obj = "part1" sub = "linkToPart2.LinkToBody.Pad.face1"

        Global placement of Pad in this context :
        getGlobalPlacementOf(pad, part1, "linkToPart2.LinkToBody.Pad.face1")

        Global placement of linkToPart2 in this context :
        getGlobalPlacementOf(linkToPart2, part1, "linkToPart2.LinkToBody.Pad.face1")

        Returns the placement of the object in the global coordinate space, respecting all stacked
        relationships.
        """
        ...

    def getPropertyNameOfGeometry(self) -> Optional[str]:
        """
        getPropertyNameOfGeometry() -> str or None

        Returns the property name of the actual geometry.
        For example for a Part feature it returns the value 'Shape', for a mesh feature the value
        'Mesh' and so on.
        If an object has no such property then None is returned.
        """
        ...

    def getPropertyOfGeometry(self) -> Optional[Any]:
        """
        getPropertyOfGeometry() -> object or None

        Returns the property of the actual geometry.
        For example for a Part feature it returns its Shape property, for a Mesh feature its
        Mesh property and so on.
        If an object has no such property then None is returned.
        Unlike to getPropertyNameOfGeometry this function returns the geometry, not its name.
        """
        ...
