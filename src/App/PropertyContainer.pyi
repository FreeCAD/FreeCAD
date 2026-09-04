# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.Persistence import Persistence
from typing import Any, Final, Union, List, Optional


@export(DisableNotify=True, )
class PropertyContainer(Persistence):
    """
    App.PropertyContainer class.
    """

    PropertiesList: Final[list] = []
    """A list of all property names."""

    def getPropertyByName(self, name: str, checkOwner: int = 0, /) -> Any:
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
        ...

    def getPropertyTouchList(self, name: str, /) -> tuple:
        """
        Returns a list of index of touched values for list type properties.

        name : str
            Property name.
        """
        ...

    def getTypeOfProperty(self, name: str, /) -> list:
        """
        Returns the type of a named property. This can be a list conformed by elements in
        (Hidden, NoRecompute, NoPersist, Output, ReadOnly, Transient, Input).

        name : str
            Property name.
        """
        ...

    def getTypeIdOfProperty(self, name: str, /) -> str:
        """
        Returns the C++ class name of a named property.

        name : str
            Property name.
        """
        ...

    def setEditorMode(self, name: str, type: Union[int, List[str]], /) -> None:
        """
        Set the behaviour of the property in the property editor.

        name : str
            Property name.
        type : int, sequence of str
            Property type.
            0: default behaviour. 1: item is ready-only. 2: item is hidden. 3: item is hidden and read-only.
            If sequence, the available items are 'ReadOnly' and 'Hidden'.
        """
        ...

    def getEditorMode(self, name: str, /) -> list:
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
        ...

    def getGroupOfProperty(self, name: str, /) -> str:
        """
        Returns the name of the group which the property belongs to in this class.
        The properties are sorted in different named groups for convenience.

        name : str
            Property name.
        """
        ...

    def setGroupOfProperty(self, name: str, group: str, /) -> None:
        """
        Set the name of the group of a dynamic property.

        name : str
            Property name.
        group : str
            Group name.
        """
        ...

    def setPropertyStatus(self, name: str, val: Union[int, str, List[Union[str, int]]], /) -> None:
        """
        Set property status.

        name : str
            Property name.
        val : int, str, sequence of str or int
            Call getPropertyStatus() to get a list of supported text value.
            If the text start with '-' or the integer value is negative, then the status is cleared.
        """
        ...

    def getPropertyStatus(self, name: str = "", /) -> list:
        """
        Get property status.

        name : str
            Property name. If empty, returns a list of supported text names of the status.
        """
        ...

    def getDocumentationOfProperty(self, name: str, /) -> str:
        """
        Returns the documentation string of the property of this class.

        name : str
            Property name.
        """
        ...

    def setDocumentationOfProperty(self, name: str, docstring: str, /) -> None:
        """
        Set the documentation string of a dynamic property of this class.

        name : str
            Property name.
        docstring : str
            Documentation string.
        """
        ...

    def getEnumerationsOfProperty(self, name: str, /) -> Optional[list]:
        """
        Return all enumeration strings of the property of this class or None if not a
        PropertyEnumeration.

        name : str
            Property name.
        """
        ...

    @constmethod
    def dumpPropertyContent(self, Property: str, *, Compression: int = 3) -> bytearray:
        """
        Dumps the content of the property, both the XML representation and the additional
        data files required, into a byte representation.

        Property : str
            Property Name.
        Compression : int
            Set the data compression level in the range [0, 9]. Set to 0 for no compression.
        """
        ...

    def restorePropertyContent(self, name: str, obj: object, /) -> None:
        """
        Restore the content of the object from a byte representation as stored by `dumpPropertyContent`.
        It could be restored from any Python object implementing the buffer protocol.

        name : str
            Property name.
        obj : buffer
            Object with buffer protocol support.
        """
        ...

    @constmethod
    def renameProperty(self, oldName: str, newName: str, /) -> None:
        """
        Rename a property.

        oldName : str
            Old property name.
        newName : str
            New property name.
        """
        ...

    def addPropertyAlias(
        self, property: str, alias: str, deprecated: bool = False, since: str = ""
    ) -> None:
        """
        Register an alias for a property name.

        After this call, getPropertyByName(alias) and Python attribute access via
        the alias name will transparently return the same property as the canonical name.
        This allows renaming a property without breaking add-ons and macros that still
        use the old name.

        If a dynamic property named `alias` exists and no property named `property`
        does, it is renamed to `property` in place, preserving its value. This is how
        documents saved under the old name are migrated, so the call is safe to make
        unconditionally on every load.

        property : str
            The current, authoritative name of the property.
        alias : str
            The old or alternative name to also accept.
        deprecated : bool
            If True, a developer warning is emitted the first time the alias is resolved
            on each object, encouraging migration to the canonical name.
        since : str
            The FreeCAD version in which the alias was introduced, e.g. "1.1". Included in
            the deprecation warning.
        """
        ...

    @no_args
    @constmethod
    def getPropertyAliases(self) -> dict[str, dict[str, object]]:
        """
        Get every property alias visible on this object.

        Merges aliases declared by the C++ class with aliases registered at runtime on this
        object. Aliases are deliberately absent from PropertiesList, so that deprecated names
        do not appear in the property editor or in dir().

        Returns a dict mapping each alias name to a dict with keys:
            canonical  : str  -- the current name of the property
            deprecated : bool -- whether using the alias emits a warning
            since      : str  -- the FreeCAD version introducing the alias, may be empty
        """
        ...
