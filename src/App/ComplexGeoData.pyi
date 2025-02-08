from Base.Metadata import export, constmethod
from Base.Persistence import Persistence
from Base.BoundBox import BoundBox as BoundBoxPy
from Base.Vector import Vector
from Base.Placement import Placement as PlacementPy
from Base.Rotation import Rotation
from Base.Matrix import Matrix
from StringHasher import StringHasher
from typing import Any, Final, List, Dict


@export(
    Namespace="Data",
    Reference=True,
)
class ComplexGeoData(Persistence):
    """
    Data.ComplexGeoData class.

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    Licence: LGPL
    UserDocu: Father of all complex geometric data types
    """

    @constmethod
    def getElementTypes(self) -> List[str]:
        """
        Return a list of element types present in the complex geometric data
        """
        ...

    @constmethod
    def countSubElements(self) -> int:
        """
        Return the number of elements of a type
        """
        ...

    @constmethod
    def getFacesFromSubElement(self) -> Any:
        """
        Return vertexes and faces from a sub-element
        """
        ...

    @constmethod
    def getLinesFromSubElement(self) -> Any:
        """
        Return vertexes and lines from a sub-element
        """
        ...

    @constmethod
    def getPoints(self) -> Any:
        """
        Return a tuple of points and normals with a given accuracy
        """
        ...

    @constmethod
    def getLines(self) -> Any:
        """
        Return a tuple of points and lines with a given accuracy
        """
        ...

    @constmethod
    def getFaces(self) -> Any:
        """
        Return a tuple of points and triangles with a given accuracy
        """
        ...

    def applyTranslation(self, translation: Vector) -> None:
        """
        Apply an additional translation to the placement
        """
        ...

    def applyRotation(self, rotation: Rotation) -> None:
        """
        Apply an additional rotation to the placement
        """
        ...

    def transformGeometry(self, transformation: Matrix) -> None:
        """
        Apply a transformation to the underlying geometry
        """
        ...

    def setElementName(self, *, element: str, name: str = None, postfix: str = None, overwrite: bool = False, sid: Any = None) -> None:
        """
        setElementName(element,name=None,postfix=None,overwrite=False,sid=None), Set an element name

        element  : the original element name, e.g. Edge1, Vertex2
        name     : the new name for the element, None to remove the mapping
        postfix  : postfix of the name that will not be hashed
        overwrite: if true, it will overwrite exiting name
        sid      : to hash the name any way you want, provide your own string id(s) in this parameter

        An element can have multiple mapped names. However, a name can only be mapped
        to one element
        """
        ...

    @constmethod
    def getElementName(self, name: str, direction: int = 0) -> Any:
        """
        getElementName(name,direction=0) - Return a mapped element name or reverse
        """
        ...

    @constmethod
    def getElementIndexedName(self, name: str) -> Any:
        """
        getElementIndexedName(name) - Return the indexed element name
        """
        ...

    @constmethod
    def getElementMappedName(self, name: str) -> Any:
        """
        getElementMappedName(name) - Return the mapped element name
        """
        ...

    BoundBox: Final[BoundBoxPy] = ...
    """Get the bounding box (BoundBox) of the complex geometric data."""

    CenterOfGravity: Final[Vector] = ...
    """Get the center of gravity"""

    Placement: PlacementPy = ...
    """Get the current transformation of the object as placement"""

    Tag: int = 0
    """Geometry Tag"""

    Hasher: StringHasher = ...
    """Get/Set the string hasher of this object"""

    ElementMapSize: Final[int] = 0
    """Get the current element map size"""

    ElementMap: Dict[Any, Any] = {}
    """Get/Set a dict of element mapping"""

    ElementReverseMap: Final[Dict[Any, Any]] = {}
    """Get a dict of element reverse mapping"""

    ElementMapVersion: Final[str] = ""
    """Element map version"""
