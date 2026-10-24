# SPDX-License-Identifier: LGPL-2.1-or-later

from Base.Metadata import export, constmethod
from Base.Persistence import Persistence
from typing import Any, Final, Literal, overload


@export(DisableNotify=True, )
class PropertyContainer(Persistence):
    """
    App.PropertyContainer class.
    """

    PropertiesList: Final[list[str]] = []
    """A list of all property names."""

    @overload
    def getPropertyByName(self, name: str, checkOwner: Literal[0, 1] = 0, /) -> Any:
        ...

    @overload
    def getPropertyByName(self, name: str, checkOwner: Literal[2] = 0, /) -> tuple[PropertyContainer, Any]:
        ...

    @overload
    def getPropertyByName(self, name: str, checkOwner: int = 0, /) -> Any | tuple[PropertyContainer, Any]:
        """
        Returns the value of a named property. Note that the returned property may not
        always belong to this container (e.g. from a linked object).

        name : str
            Name of the property.
        checkOwner : int
            0: just return the property.
            1: raise exception if not found or the property does not belong to this container.
            2: return a tuple (owner, propertyValue).
        """

    def getPropertyTouchList(self, name: str, /) -> tuple[int, ...]:
        """
        Returns a list of index of touched values for list type properties.

        name : str
            Property name.
        """

    def getTypeOfProperty(self, name: str, /) -> list[str]:
        """
        Returns the type of a named property. This can be a list conformed by elements in
        (Hidden, NoRecompute, NoPersist, Output, ReadOnly, Transient, Input).

        name : str
            Property name.
        """

    def getTypeIdOfProperty(self, name: str, /) -> str:
        """
        Returns the C++ class name of a named property.

        name : str
            Property name.
        """

    def setEditorMode(self, name: str, type: int | list[str] | tuple[str, ...], /) -> None:
        """
        Set the behaviour of the property in the property editor.

        name : str
            Property name.
        type : int, sequence of str
            Property type.
            0: default behaviour. 1: item is ready-only. 2: item is hidden. 3: item is hidden and read-only.
            If sequence, the available items are 'ReadOnly' and 'Hidden'.
        """

    def getEditorMode(self, name: str, /) -> list[str]:
        """
        Get the behaviour of the property in the property editor.
        It returns a list of strings with the current mode. If the list is empty there are no
        special restrictions.
        If the list contains 'ReadOnly' then the item appears in the property editor but is
        disabled.
        If the list contains 'Hidden' then the item even doesn't appear in the property editor.

        name : str
            Property name.
        """

    def getGroupOfProperty(self, name: str, /) -> str:
        """
        Returns the name of the group which the property belongs to in this class.
        The properties are sorted in different named groups for convenience.

        name : str
            Property name.
        """

    def setGroupOfProperty(self, name: str, group: str, /) -> None:
        """
        Set the name of the group of a dynamic property.

        name : str
            Property name.
        group : str
            Group name.
        """

    def setPropertyStatus(self, name: str, val: int | str | list[str | int] | tuple[str | int], /) -> None:
        """
        Set property status.

        name : str
            Property name.
        val : int, str, sequence of str or int
            Call getPropertyStatus() to get a list of supported text value.
            If the text start with '-' or the integer value is negative, then the status is cleared.
        """

    def getPropertyStatus(self, name: str = "", /) -> list[str | int]:
        """
        Get property status.

        name : str
            Property name. If empty, returns a list of supported text names of the status.
        """

    def getDocumentationOfProperty(self, name: str, /) -> str:
        """
        Returns the documentation string of the property of this class.

        name : str
            Property name.
        """

    def setDocumentationOfProperty(self, name: str, docstring: str, /) -> None:
        """
        Set the documentation string of a dynamic property of this class.

        name : str
            Property name.
        docstring : str
            Documentation string.
        """

    def getEnumerationsOfProperty(self, name: str, /) -> list[str] | None:
        """
        Return all enumeration strings of the property of this class or None if not a
        PropertyEnumeration.

        name : str
            Property name.
        """

    @constmethod
    def dumpPropertyContent(self, Property: str, Compression: int = 3) -> bytearray:
        """
        Dumps the content of the property, both the XML representation and the additional
        data files required, into a byte representation.

        Property : str
            Property Name.
        Compression : int
            Set the data compression level in the range [0, 9]. Set to 0 for no compression.
        """

    def restorePropertyContent(self, name: str, obj: object, /) -> None:
        """
        Restore the content of the object from a byte representation as stored by `dumpPropertyContent`.
        It could be restored from any Python object implementing the buffer protocol.

        name : str
            Property name.
        obj : buffer
            Object with buffer protocol support.
        """

    @constmethod
    def renameProperty(self, oldName: str, newName: str, /) -> None:
        """
        Rename a property.

        oldName : str
            Old property name.
        newName : str
            New property name.
        """
