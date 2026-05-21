# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCAD._ParameterGrp`` PyCXX type."""

from __future__ import annotations

from typing import Literal, Protocol, TypeAlias

_ParameterValue: TypeAlias = str | int | float | bool
_ParameterContentTag: TypeAlias = Literal["String", "Integer", "Float", "Boolean", "Unsigned Long"]
_ParameterContent: TypeAlias = tuple[_ParameterContentTag, str, _ParameterValue]

class _ParameterObserver(Protocol):
    """Observer protocol for parameter changes inside one parameter group."""

    def onChange(
        self,
        group: _ParameterGrp,
        param_type: str,
        name: str,
        value: str,
        /,
    ) -> object:
        """Handle one parameter change notification."""
        ...

class _ParameterManagerObserver(Protocol):
    """Observer protocol for manager-level parameter change notifications."""

    def slotParamChanged(
        self,
        group: _ParameterGrp,
        param_type: str,
        name: str,
        value: str,
        /,
    ) -> object:
        """Handle one manager-level parameter change notification."""
        ...

class _ParameterGrp:
    """Hierarchical parameter-group wrapper used for FreeCAD preferences."""

    def GetGroup(self, name: str, /) -> _ParameterGrp:
        """Return one child parameter group by name."""
        ...

    def GetGroupName(self) -> str:
        """Return this group's local name."""
        ...

    def GetGroups(self) -> list[str]:
        """Return the child group names."""
        ...

    def RemGroup(self, name: str, /) -> None:
        """Remove one child parameter group."""
        ...

    def HasGroup(self, name: str, /) -> bool:
        """Return whether one child group exists."""
        ...

    def RenameGroup(self, old_name: str, new_name: str, /) -> bool:
        """Rename one child parameter group."""
        ...

    def CopyTo(self, group: _ParameterGrp, /) -> None:
        """Copy this group's contents into another parameter group."""
        ...

    def Manager(self) -> _ParameterGrp | None:
        """Return the manager group for this parameter group, if any."""
        ...

    def Parent(self) -> _ParameterGrp | None:
        """Return the parent group, if any."""
        ...

    def IsEmpty(self) -> bool:
        """Return whether the group has no stored values or child groups."""
        ...

    def Clear(self) -> None:
        """Remove all stored values and child groups."""
        ...

    def Attach(self, observer: _ParameterObserver, /) -> None:
        """Register one direct parameter observer."""
        ...

    def AttachManager(self, observer: _ParameterManagerObserver, /) -> None:
        """Register one manager-level parameter observer."""
        ...

    def Detach(self, observer: _ParameterObserver | _ParameterManagerObserver, /) -> None:
        """Unregister one direct or manager-level observer."""
        ...

    def Notify(self, name: str, /) -> None:
        """Notify observers that one named entry changed."""
        ...

    def NotifyAll(self) -> None:
        """Notify observers that the whole group changed."""
        ...

    def SetBool(self, name: str, value: bool | int, /) -> None:
        """Store one boolean parameter value."""
        ...

    def GetBool(self, name: str, default: bool | int = False, /) -> bool:
        """Return one boolean parameter value."""
        ...

    def GetBools(self, filter: str = "", /) -> list[str]:
        """Return the names of boolean parameters, optionally filtered."""
        ...

    def RemBool(self, name: str, /) -> None:
        """Remove one boolean parameter value."""
        ...

    def SetInt(self, name: str, value: int, /) -> None:
        """Store one integer parameter value."""
        ...

    def GetInt(self, name: str, default: int = 0, /) -> int:
        """Return one integer parameter value."""
        ...

    def GetInts(self, filter: str = "", /) -> list[str]:
        """Return the names of integer parameters, optionally filtered."""
        ...

    def RemInt(self, name: str, /) -> None:
        """Remove one integer parameter value."""
        ...

    def SetUnsigned(self, name: str, value: int, /) -> None:
        """Store one unsigned integer parameter value."""
        ...

    def GetUnsigned(self, name: str, default: int = 0, /) -> int:
        """Return one unsigned integer parameter value."""
        ...

    def GetUnsigneds(self, filter: str = "", /) -> list[str]:
        """Return the names of unsigned integer parameters, optionally filtered."""
        ...

    def RemUnsigned(self, name: str, /) -> None:
        """Remove one unsigned integer parameter value."""
        ...

    def SetFloat(self, name: str, value: float, /) -> None:
        """Store one floating-point parameter value."""
        ...

    def GetFloat(self, name: str, default: float = 0.0, /) -> float:
        """Return one floating-point parameter value."""
        ...

    def GetFloats(self, filter: str = "", /) -> list[str]:
        """Return the names of floating-point parameters, optionally filtered."""
        ...

    def RemFloat(self, name: str, /) -> None:
        """Remove one floating-point parameter value."""
        ...

    def SetString(self, name: str, value: str, /) -> None:
        """Store one string parameter value."""
        ...

    def GetString(self, name: str, default: str = "", /) -> str:
        """Return one string parameter value."""
        ...

    def GetStrings(self, filter: str = "", /) -> list[str]:
        """Return the names of string parameters, optionally filtered."""
        ...

    def RemString(self, name: str, /) -> None:
        """Remove one string parameter value."""
        ...

    def Import(self, path: str, /) -> None:
        """Import parameter values from one external file."""
        ...

    def Insert(self, path: str, /) -> None:
        """Insert parameter values from one external file."""
        ...

    def Export(self, path: str, /) -> None:
        """Export parameter values to one external file."""
        ...

    def GetContents(self) -> list[_ParameterContent] | None:
        """Return the stored parameter entries as tagged name-value tuples."""
        ...
