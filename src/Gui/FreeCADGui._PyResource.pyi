# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._PyResource`` PyCXX type."""

from __future__ import annotations

from collections.abc import Callable
from typing import TypeAlias

_PyResourceValue: TypeAlias = str | int | float | bool | list[str]

class _PyResource:
    """Wrapper around a loaded Qt resource object tree."""

    def value(self, object_name: str, property_name: str, /) -> _PyResourceValue | None:
        """Return one property value from a named child object."""
        ...

    def setValue(
        self,
        object_name: str,
        property_name: str,
        value: _PyResourceValue,
        /,
    ) -> None:
        """Set one property value on a named child object."""
        ...

    def show(self) -> None:
        """Show the root resource widget."""
        ...

    def connect(self, sender: str, signal: str, callback: Callable[..., object], /) -> None:
        """Connect one named child signal to a Python callback."""
        ...
