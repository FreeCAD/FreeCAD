from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Final, List

@export(
    PythonName="Sketcher.ExternalGeometryFacade",
    Include="Mod/Sketcher/App/ExternalGeometryFacade.h",
    Constructor=True,
)
class ExternalGeometryFacade(BaseClass):
    """
    Describes a GeometryFacade

    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Ref: str = ""
    """Returns the reference string of this external geometry."""

    Id: int = 0
    """Sets/returns the Internal Alignment Type of the Geometry."""

    Construction: bool = False
    """Sets/returns this geometry as a construction one, which will not be part of a later built shape."""

    GeometryLayerId: int = 0
    """Returns the Id of the geometry Layer in which the geometry is located."""

    InternalType: str = ""
    """Sets/returns the Internal Alignment Type of the Geometry."""

    Blocked: bool = False
    """Sets/returns whether the geometry is blocked or not."""

    Tag: Final[str] = ""
    """Gives the tag of the geometry as string."""

    Geometry: object = ...
    """Returns the underlying geometry object."""

    @constmethod
    def testFlag(self) -> bool:
        """
        Returns a boolean indicating whether the given bit is set.
        """
        ...

    def setFlag(self) -> None:
        """
        Sets the given bit to true/false.
        """
        ...

    def mirror(self) -> None:
        """
        Performs the symmetrical transformation of this geometric object
        """
        ...

    def rotate(self) -> None:
        """
        Rotates this geometric object at angle Ang (in radians) about axis
        """
        ...

    def scale(self) -> None:
        """
        Applies a scaling transformation on this geometric object with a center and scaling factor
        """
        ...

    def transform(self) -> None:
        """
        Applies a transformation to this geometric object
        """
        ...

    def translate(self) -> None:
        """
        Translates this geometric object
        """
        ...

    @constmethod
    def hasExtensionOfType(self) -> bool:
        """
        Returns a boolean indicating whether a geometry extension of the type indicated as a string exists.
        """
        ...

    @constmethod
    def hasExtensionOfName(self) -> bool:
        """
        Returns a boolean indicating whether a geometry extension with the name indicated as a string exists.
        """
        ...

    @constmethod
    def getExtensionOfType(self) -> object:
        """
        Gets the first geometry extension of the type indicated by the string.
        """
        ...

    @constmethod
    def getExtensionOfName(self) -> object:
        """
        Gets the first geometry extension of the name indicated by the string.
        """
        ...

    def setExtension(self) -> None:
        """
        Sets a geometry extension of the indicated type.
        """
        ...

    def deleteExtensionOfType(self) -> None:
        """
        Deletes all extensions of the indicated type.
        """
        ...

    def deleteExtensionOfName(self) -> None:
        """
        Deletes all extensions of the indicated name.
        """
        ...

    @constmethod
    def getExtensions(self) -> List[object]:
        """
        Returns a list with information about the geometry extensions.
        """
        ...
