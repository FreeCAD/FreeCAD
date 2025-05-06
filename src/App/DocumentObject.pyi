from Base.Metadata import constmethod
from Base.Matrix import Matrix
from Document import Document
from DocumentObjectGroup import DocumentObjectGroup
from ExtensionContainer import ExtensionContainer
from typing import Any, Final, List, Optional, Union, Tuple


class DocumentObject(ExtensionContainer):
    """
    This is the father of all classes handled by the document
    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    OutList: Final[List["DocumentObject"]] = []
    """A list of all objects this object links to."""

    OutListRecursive: Final[List["DocumentObject"]] = []
    """A list of all objects this object links to recursively."""

    InList: Final[List["DocumentObject"]] = []
    """A list of all objects which link to this object."""

    InListRecursive: Final[List["DocumentObject"]] = []
    """A list of all objects which link to this object recursively."""

    FullName: Final[str] = ""
    """Return the document name and internal name of this object"""

    Name: Final[Optional[str]] = ""
    """Return the internal name of this object"""

    Document: Final[Document] = None
    """Return the document this object is part of"""

    State: Final[List[Any]] = []
    """State of the object in the document"""

    ViewObject: Final[Any] = None
    """
    If the GUI is loaded the associated view provider is returned
    or None if the GUI is not up
    """

    MustExecute: Final[bool] = False
    """Check if the object must be recomputed"""

    ID: Final[int] = 0
    """The unique identifier (among its document) of this object"""

    Removing: Final[bool] = False
    """Indicate if the object is being removed"""

    Parents: Final[List[Any]] = []
    """A List of tuple(parent,subname) holding all parents to this object"""

    OldLabel: Final[str] = ""
    """Contains the old label before change"""

    NoTouch: bool = False
    """Enable/disable no touch on any property change"""

    def addProperty(
        self,
        *,
        type: str,
        name: str,
        group: str = "",
        doc: str = "",
        attr: int = 0,
        read_only: bool = False,
        hidden: bool = False,
        locked: bool = False,
        enum_vals: list = []
    ) -> "DocumentObject":
        """
        addProperty(type: string, name: string, group="", doc="", attr=0, read_only=False, hidden=False, locked = False, enum_vals=[]) -- Add a generic property.
        """
        ...

    def removeProperty(self, string: str) -> None:
        """
        removeProperty(string) -- Remove a generic property.

        Note, you can only remove user-defined properties but not built-in ones.
        """
        ...

    def supportedProperties(self) -> list:
        """
        A list of supported property types
        """
        ...

    def touch(self) -> None:
        """
        Mark the object as changed (touched)
        """
        ...

    def purgeTouched(self) -> None:
        """
        Mark the object as unchanged
        """
        ...

    def enforceRecompute(self) -> None:
        """
        Mark the object for recompute
        """
        ...

    def setExpression(self, name: str, expression: str) -> None:
        """
        Register an expression for a property
        """
        ...

    def clearExpression(self, name: str) -> None:
        """
        Clear the expression for a property
        """
        ...

    @classmethod
    def evalExpression(cls, expression: str) -> Any:
        """
        Evaluate an expression
        """
        ...

    def recompute(self, recursive: bool = False) -> None:
        """
        recompute(recursive=False): Recomputes this object
        """
        ...

    @constmethod
    def getStatusString(self) -> str:
        """
        Returns the status of the object as string.
        If the object is invalid its error description will be returned.
        If the object is valid but touched then 'Touched' will be returned,
        'Valid' otherwise.
        """
        ...

    @constmethod
    def isValid(self) -> bool:
        """
        Returns True if the object is valid, False otherwise
        """
        ...

    def getSubObject(
        self,
        *,
        subname: Union[str, List[str], Tuple[str, ...]],
        retType: int = 0,
        matrix: Matrix = None,
        transform: bool = True,
        depth: int = 0,
    ) -> Any:
        """
        getSubObject(subname, retType=0, matrix=None, transform=True, depth=0)

        * subname(string|list|tuple): dot separated string or sequence of strings
        referencing subobject.

        * retType: return type, 0=PyObject, 1=DocObject, 2=DocAndPyObject, 3=Placement

            PyObject: return a python binding object for the (sub)object referenced in
            each 'subname' The actual type of 'PyObject' is implementation dependent.
            For Part::Feature compatible objects, this will be of type TopoShapePy and
            pre-transformed by accumulated transformation matrix along the object path.

            DocObject:  return the document object referenced in subname, if 'matrix' is
            None. Or, return a tuple (object, matrix) for each 'subname' and 'matrix' is
            the accumulated transformation matrix for the sub object.

            DocAndPyObject: return a tuple (object, matrix, pyobj) for each subname

            Placement: return a transformed placement of the sub-object

        * matrix: the initial transformation to be applied to the sub object.

        * transform: whether to transform the sub object using this object's placement

        * depth: current recursive depth
        """
        ...

    def getSubObjectList(self, subname: str) -> list:
        """
        getSubObjectList(subname)

        Return a list of objects referenced by a given subname including this object
        """
        ...

    def getSubObjects(self, reason: int = 0) -> list:
        """
        getSubObjects(reason=0): Return subname reference of all sub-objects
        """
        ...

    def getLinkedObject(
        self,
        *,
        recursive: bool = True,
        matrix: Matrix = None,
        transform: bool = True,
        depth: int = 0,
    ) -> Any:
        """
        getLinkedObject(recursive=True, matrix=None, transform=True, depth=0)
        Returns the linked object if there is one, or else return itself

        * recursive: whether to recursively resolve the links

        * transform: whether to transform the sub object using this object's placement

        * matrix: If not none, this specifies the initial transformation to be applied
        to the sub object. And cause the method to return a tuple (object, matrix)
        containing the accumulated transformation matrix

        * depth: current recursive depth
        """
        ...

    def setElementVisible(self, element: str, visible: bool) -> int:
        """
        setElementVisible(element,visible): Set the visibility of a child element
        Return -1 if element visibility is not supported, 0 if element not found, 1 if success
        """
        ...

    def isElementVisible(self, element: str) -> int:
        """
        isElementVisible(element): Check if a child element is visible
        Return -1 if element visibility is not supported or element not found, 0 if invisible, or else 1
        """
        ...

    def hasChildElement(self) -> bool:
        """
        Return true to indicate the object having child elements
        """
        ...

    def getParentGroup(self) -> DocumentObjectGroup:
        """
        Returns the group the object is in or None if it is not part of a group.

        Note that an object can only be in a single group, hence only a single return value.
        """
        ...

    def getParentGeoFeatureGroup(self) -> Any:
        """
        Returns the GeoFeatureGroup, and hence the local coordinate system, the object
        is in or None if it is not part of a group.

        Note that an object can only be in a single group, hence only a single return value.
        """
        ...

    def getParent(self) -> Any:
        """
        Returns the group the object is in or None if it is not part of a group.

        Note that an object can only be in a single group, hence only a single return value.
        The parent can be a simple group as with getParentGroup() or a GeoFeature group as
        with getParentGeoFeatureGroup().
        """
        ...

    def getPathsByOutList(self) -> list:
        """
        Get all paths from this object to another object following the OutList.
        """
        ...

    @constmethod
    def resolve(self, subname: str) -> tuple:
        """
        resolve(subname) -- resolve the sub object

        Returns a tuple (subobj,parent,elementName,subElement), where 'subobj' is the
        last object referenced in 'subname', and 'parent' is the direct parent of
        'subobj', and 'elementName' is the name of the subobj, which can be used
        to call parent.isElementVisible/setElementVisible(). 'subElement' is the
        non-object sub-element name if any.
        """
        ...

    @constmethod
    def resolveSubElement(self, subname: str, append: bool, type: int) -> tuple:
        """
        resolveSubElement(subname,append,type) -- resolve both new and old style sub element

        subname: subname reference containing object hierarchy
        append: Whether to append object hierarchy prefix inside subname to returned element name
        type: 0: normal, 1: for import, 2: for export

        Return tuple(obj,newElementName,oldElementName)
        """
        ...

    def adjustRelativeLinks(self, parent: DocumentObject, recursive: bool = True) -> bool:
        """
        adjustRelativeLinks(parent,recursive=True) -- auto correct potential cyclic dependencies
        """
        ...

    @constmethod
    def getElementMapVersion(self, property_name: str) -> str:
        """
        getElementMapVersion(property_name): return element map version of a given geometry property
        """
        ...

    @constmethod
    def isAttachedToDocument(self) -> bool:
        """
        isAttachedToDocument() -> bool

        Return true if the object is part of a document, false otherwise.
        """
        ...
