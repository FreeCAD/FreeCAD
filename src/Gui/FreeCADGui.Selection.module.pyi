# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCADGui.Selection`` module.

This file keeps both the function surface and the lightweight helper types
close to the GUI selection implementation.
"""

from __future__ import annotations

from collections.abc import Sequence
from enum import IntEnum
from typing import Protocol, overload

from FreeCAD import DocumentObject
from FreeCADGui import SelectionObject

_Point3 = tuple[float, float, float]

class ResolveMode(IntEnum):
    """How selection queries should resolve linked or mapped elements."""

    NoResolve = 0
    OldStyleElement = 1
    NewStyleElement = 2
    FollowLink = 3

class SelectionStyle(IntEnum):
    """High-level selection behavior modes exposed by the GUI."""

    NormalSelection = 0
    GreedySelection = 1

class _SelectionGate(Protocol):
    """Protocol for custom selection-gate objects."""

    def allow(self, doc: object, obj: DocumentObject, sub: str, /) -> bool:
        """Return whether one candidate selection should be accepted."""
        ...

class Filter:
    """Selection filter helper that wraps the GUI filter expression language."""

    def __init__(self, filter: str, /) -> None:
        """Create one selection filter from its expression string."""
        ...

    def match(self) -> bool:
        """Return whether the current filter matches the current selection state."""
        ...

    def test(self, obj: DocumentObject, sub_name: str = "", /) -> bool:
        """Return whether one object and optional subname matches the filter."""
        ...

    def result(self) -> list[tuple[SelectionObject, ...]]:
        """Return the current filter match result set."""
        ...

    def setFilter(self, filter: str, /) -> None:
        """Replace the filter expression string."""
        ...

    def getFilter(self) -> str:
        """Return the current filter expression string."""
        ...

# Selection mutation
@overload
def addSelection(
    doc_name: str,
    obj_name: str,
    sub_name: str = "",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    clear: bool = True,
    /,
) -> None:
    """Add a selection by document name, object name, and optional picked point."""
    ...

@overload
def addSelection(
    obj: DocumentObject,
    sub_name: str = "",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    clear: bool = True,
    /,
) -> None:
    """Add a selection from an object reference plus one optional subname."""
    ...

@overload
def addSelection(
    obj: DocumentObject,
    sub_names: Sequence[str],
    clear: bool = True,
    /,
) -> None:
    """Add one object with a batch of subnames in a single call."""
    ...

def updateSelection(show: bool, obj: DocumentObject, sub_name: str = "", /) -> None:
    """Update one object's selected state explicitly."""
    ...

@overload
def removeSelection(doc_name: str, obj_name: str, sub_name: str = "", /) -> None:
    """Remove a selection by document and object names."""
    ...

@overload
def removeSelection(obj: DocumentObject, sub_name: str = "", /) -> None:
    """Remove a selection by object reference."""
    ...

@overload
def clearSelection(clear_preselect: bool = True, /) -> None:
    """Clear the active selection globally."""
    ...

@overload
def clearSelection(doc_name: str | None, clear_preselect: bool = True, /) -> None:
    """Clear the active selection only for one document name."""
    ...

# Selection state
def isSelected(obj: DocumentObject, sub_name: str = "", resolve: ResolveMode | int = 1, /) -> bool:
    """Return whether one object or subelement is currently selected."""
    ...

# Preselection
def setPreselection(
    obj: DocumentObject,
    subname: str = "",
    x: float = 0.0,
    y: float = 0.0,
    z: float = 0.0,
    tp: int = 1,
) -> None:
    """Set the current preselection target and optional picked point."""
    ...

def getPreselection() -> SelectionObject:
    """Return the current preselection object."""
    ...

def clearPreselection() -> None:
    """Clear the current preselection target."""
    ...

def countObjectsOfType(
    type_name: str, doc_name: str | None = None, resolve: ResolveMode | int = 1, /
) -> int:
    """Count selected objects of one type, optionally in one document."""
    ...

def getSelection(
    doc_name: str | None = None,
    resolve: ResolveMode | int = 1,
    single: bool = False,
    /,
) -> list[DocumentObject]:
    """Return the current object selection."""
    ...

def getPickedList(doc_name: str | None = None, /) -> list[SelectionObject]:
    """Return the picked selection entries for one document or globally."""
    ...

def enablePickedList(enable: bool = True, /) -> None:
    """Enable or disable picked-list collection."""
    ...

def getCompleteSelection(resolve: ResolveMode | int = 1, /) -> list[SelectionObject]:
    """Return the complete resolved selection object list."""
    ...

def getSelectionEx(
    doc_name: str | None = None,
    resolve: ResolveMode | int = 1,
    single: bool = False,
    /,
) -> list[SelectionObject]:
    """Return the extended selection objects with subelement details."""
    ...

def getSelectionObject(
    doc_name: str,
    obj_name: str,
    sub_name: str,
    point: _Point3 = ...,
    /,
) -> SelectionObject:
    """Build one SelectionObject wrapper from explicit selection components."""
    ...

def hasSelection(doc_name: str | None = None, resolve: ResolveMode | int = 0, /) -> bool:
    """Return whether any selection exists."""
    ...

def hasSubSelection(doc_name: str | None = None, sub_element: bool = False, /) -> bool:
    """Return whether any subelement selection exists."""
    ...

# Observers and filters
def setSelectionStyle(selection_style: SelectionStyle | int, /) -> None:
    """Set the active selection interaction style."""
    ...

def addObserver(observer: object, resolve: ResolveMode | int = 1, /) -> None:
    """Register one selection observer."""
    ...

def removeObserver(observer: object, /) -> None:
    """Unregister one selection observer."""
    ...

def addSelectionGate(
    filter: str | Filter | _SelectionGate,
    resolve: ResolveMode | int = 1,
    /,
) -> None:
    """Install one selection gate or filter object."""
    ...

def removeSelectionGate(doc_name: str = "", /) -> None:
    """Remove the active selection gate, optionally for one document."""
    ...

# Visibility and selection history
def setVisible(visible: bool | None = None, /) -> None:
    """Set or toggle selection visibility helpers."""
    ...

def pushSelStack(clear_forward: bool = True, overwrite: bool = False, /) -> None:
    """Push the current selection onto the history stack."""
    ...

def getSelectionFromStack(
    doc_name: str | None = None,
    resolve: ResolveMode | int = 1,
    index: int = 0,
    /,
) -> list[SelectionObject]:
    """Return one stored selection state from the history stack."""
    ...
