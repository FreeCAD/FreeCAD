from Base.Metadata import export
from DocumentObjectExtension import DocumentObjectExtension
from typing import Any, List


@export(
    Include="App/DocumentObjectGroup.h",
)
class GroupExtension(DocumentObjectExtension):
    """
    Extension class which allows grouping of document objects
    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def newObject(self, type: str, name: str) -> Any:
        """
        Create and add an object with given type and name to the group
        """
        ...

    def addObject(self, obj: Any) -> List[Any]:
        """
        Add an object to the group. Returns all objects that have been added.
        """
        ...

    def addObjects(self, objects: List[Any]) -> List[Any]:
        """
        Adds multiple objects to the group. Expects a list and returns all objects that have been added.
        """
        ...

    def setObjects(self, objects: List[Any]) -> List[Any]:
        """
        Sets the objects of the group. Expects a list and returns all objects that are now in the group.
        """
        ...

    def removeObject(self, obj: Any) -> List[Any]:
        """
        Remove an object from the group and returns all objects that have been removed.
        """
        ...

    def removeObjects(self, objects: List[Any]) -> List[Any]:
        """
        Remove multiple objects from the group. Expects a list and returns all objects that have been removed.
        """
        ...

    def removeObjectsFromDocument(self) -> None:
        """
        Remove all child objects from the group and document
        """
        ...

    def getObject(self, name: str) -> Any:
        """
        Return the object with the given name
        """
        ...

    def getObjectsOfType(self, typename: str) -> List[Any]:
        """
        Returns all object in the group of given type
        @param typename     The Freecad type identifier
        """
        ...

    def hasObject(self, obj: Any, recursive: bool = False) -> bool:
        """
        hasObject(obj, recursive=false)

        Checks if the group has a given object
        @param obj        the object to check for.
        @param recursive  if true check also if the obj is child of some sub group (default is false).
        """
        ...

    def allowObject(self, obj: Any) -> bool:
        """
        Returns true if obj is allowed in the group extension.
        """
        ...
