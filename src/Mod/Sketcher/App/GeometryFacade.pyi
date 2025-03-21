from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from Base.Axis import Axis
from Base.CoordinateSystem import CoordinateSystem
from Base.Placement import Placement
from Base.Vector import Vector
from App.DocumentObjectExtension import DocumentObjectExtension
from typing import Final, List

@export(
    PythonName="Sketcher.GeometryFacade",
    Include="Mod/Sketcher/App/GeometryFacade.h",
    Constructor=True,
    Delete=True,
)
class GeometryFacade(BaseClass):
    """
    Describes a GeometryFacade

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Id: int = ...
    """Sets/returns the Id of the SketchGeometryExtension."""

    InternalType: str = ...
    """Sets/returns the Internal Alignment Type of the Geometry."""

    Blocked: bool = ...
    """Sets/returns whether the geometry is blocked or not."""

    Construction: bool = ...
    """Sets/returns this geometry as a construction one, which will not be part of a later built shape."""

    GeometryLayerId: int = ...
    """Returns the Id of the geometry Layer in which the geometry is located."""

    Tag: Final[str] = ...
    """Gives the tag of the geometry as string."""

    Geometry: object = ...
    """Returns the underlying geometry object."""

    @constmethod
    def testGeometryMode(self) -> bool:
        """
        Returns a boolean indicating whether the given bit is set.
        """
        ...

    def setGeometryMode(self) -> None:
        """
        Sets the given bit to true/false.
        """
        ...

    def mirror(self) -> None:
        """
        Performs the symmetrical transformation of this geometric object
        """
        ...

    def rotate(self, Ang: float, axis: Axis) -> None:
        """
        Rotates this geometric object at angle Ang (in radians) about axis
        """
        ...

    def scale(self, center: CoordinateSystem, factor: float) -> None:
        """
        Applies a scaling transformation on this geometric object with a center and scaling factor
        """
        ...

    def transform(self, transformation: Placement) -> None:
        """
        Applies a transformation to this geometric object
        """
        ...

    def translate(self, offset: Vector) -> None:
        """
        Translates this geometric object
        """
        ...

    @constmethod
    def hasExtensionOfType(self, type_str: str) -> bool:
        """
        Returns a boolean indicating whether a geometry extension of the type indicated as a string exists.
        """
        ...

    @constmethod
    def hasExtensionOfName(self, name: str) -> bool:
        """
        Returns a boolean indicating whether a geometry extension with the name indicated as a string exists.
        """
        ...

    @constmethod
    def getExtensionOfType(self, type_str: str) -> DocumentObjectExtension:
        """
        Gets the first geometry extension of the type indicated by the string.
        """
        ...

    @constmethod
    def getExtensionOfName(self, name: str) -> DocumentObjectExtension:
        """
        Gets the first geometry extension of the name indicated by the string.
        """
        ...

    def setExtension(self, extension: DocumentObjectExtension) -> None:
        """
        Sets a geometry extension of the indicated type.
        """
        ...

    def deleteExtensionOfType(self, type_str: str) -> None:
        """
        Deletes all extensions of the indicated type.
        """
        ...

    def deleteExtensionOfName(self, name: str) -> None:
        """
        Deletes all extensions of the indicated name.
        """
        ...

    @constmethod
    def getExtensions(self) -> List[DocumentObjectExtension]:
        """
        Returns a list with information about the geometry extensions.
        """
        ...
