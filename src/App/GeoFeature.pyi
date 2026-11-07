# SPDX-License-Identifier: LGPL-2.1-or-later

# 'Placement' will be shadowed by auto-generated property from cpp.
# Using "PlacementPy" won't help because replace happens before cpp insert.
from FreeCAD import Base
from App.ComplexGeoData import ComplexGeoData
from DocumentObject import DocumentObject
from Base.Metadata import deprecated
from typing import Any, Final


class GeoFeature(DocumentObject):
    """
    App.GeoFeature class.

    Base class of all geometric document objects.
    This class does the whole placement and position handling.
    With the method `getPropertyOfGeometry` is possible to obtain
    the main geometric property in general form, without reference
    to any particular property name.
    """

    ElementMapVersion: Final[str]
    """Element map version"""

    def getPaths(self) -> Any:
        """
        Returns all possible paths to the root of the document.
        Note: Not implemented.
        """

    @deprecated(
        deprecated_in="26.3",
        removed_in="27.2",
        replacement="getGlobalPlacementOf",
        details="This method does not handle Links correctly.",
    )
    def getGlobalPlacement(self) -> Base.Placement:
        """
        Deprecated: This function does not handle Links correctly. Use getGlobalPlacementOf instead.

        Returns the placement of the object in the global coordinate space, respecting all stacked
        relationships.
        Note: This function is not available during recompute, as there the placements of parents
        can change after the execution of this object, rendering the result wrong.
        """

    @staticmethod
    def getGlobalPlacementOf(targetObj: DocumentObject, rootObj: DocumentObject, subname: str, /) -> Base.Placement:
        """
        Examples:
            obj = "part1"
            sub = "linkToPart2.LinkToBody.Pad.face1"

            Global placement of Pad in this context:
            getGlobalPlacementOf(pad, part1, "linkToPart2.LinkToBody.Pad.face1")


            Global placement of linkToPart2 in this context:
            getGlobalPlacementOf(linkToPart2, part1, "linkToPart2.LinkToBody.Pad.face1")

        Returns the placement of the object in the global coordinate space, respecting all stacked
        relationships.
        """

    def getPropertyNameOfGeometry(self) -> str | None:
        """
        Returns the property name of the actual geometry.
        For example for a Part feature it returns the value 'Shape', for a mesh feature the value
        'Mesh' and so on.
        If an object has no such property then None is returned.
        """

    def getPropertyOfGeometry(self) -> ComplexGeoData | None:
        """
        Returns the property of the actual geometry.
        For example for a Part feature it returns its Shape property, for a Mesh feature its
        Mesh property and so on.
        If an object has no such property then None is returned.
        Unlike to getPropertyNameOfGeometry this function returns the geometry, not its name.
        """
