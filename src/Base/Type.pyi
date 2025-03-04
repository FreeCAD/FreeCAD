from Metadata import export, forward_declarations, constmethod
from PyObjectBase import PyObjectBase
from typing import List, Final

@export(
    Twin="BaseType",
    TwinPointer="BaseType",
    Delete=True,
)
@forward_declarations(
    """
namespace Base {
    using BaseType = Type;
}"""
)
class Type(PyObjectBase):
    """
    BaseTypePy class.

    This class provides functionality related to type management in the Base module. It's not intended for direct instantiation but for accessing type information and creating instances of various types. Instantiation is possible for classes that inherit from the Base::BaseClass class and are not abstract.

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Name: Final[str] = ""
    """The name of the type id."""

    Key: Final[int] = 0
    """The key of the type id."""

    Module: Final[str] = ""
    """Module in which this class is defined."""

    @staticmethod
    def fromName(name: str) -> "Type":
        """
        fromName(name) -> Base.BaseType

        Returns a type object by name.

        name : str
        """
        ...

    @staticmethod
    def fromKey(key: int) -> "Type":
        """
        fromKey(key) -> Base.BaseType

        Returns a type id object by key.

        key : int
        """
        ...

    @staticmethod
    def getNumTypes() -> int:
        """
        getNumTypes() -> int

        Returns the number of type ids created so far.
        """
        ...

    @staticmethod
    def getBadType() -> "Type":
        """
        getBadType() -> Base.BaseType

        Returns an invalid type id.
        """
        ...

    @staticmethod
    def getAllDerivedFrom(type: str) -> List[str]:
        """
        getAllDerivedFrom(type) -> list

        Returns all descendants from the given type id.

        type : str, Base.BaseType
        """
        ...

    @constmethod
    def getParent(self) -> "Type":
        """
        getParent() -> Base.BaseType

        Returns the parent type id.
        """
        ...

    @constmethod
    def isBad(self) -> bool:
        """
        isBad() -> bool

        Checks if the type id is invalid.
        """
        ...

    @constmethod
    def isDerivedFrom(self, type: str) -> bool:
        """
        isDerivedFrom(type) -> bool

        Returns true if given type id is a father of this type id.

        type : str, Base.BaseType
        """
        ...

    @constmethod
    def getAllDerived(self) -> List[object]:
        """
        getAllDerived() -> list

        Returns all descendants from this type id.
        """
        ...

    def createInstance(self) -> object:
        """
        createInstance() -> object

        Creates an instance of this type id.
        """
        ...

    @staticmethod
    def createInstanceByName(name: str, load: bool = False) -> object:
        """
        createInstanceByName(name, load=False) -> object

        Creates an instance of the named type id.

        name : str
        load : bool
            Load named type id module.
        """
        ...
