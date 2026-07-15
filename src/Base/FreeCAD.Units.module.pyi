# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCAD.Units`` helper module.

This source-adjacent stub file carries the callable surface together with the
small helper aliases and module data members those signatures need.
"""

from __future__ import annotations

from typing import Literal, TypeAlias, overload

from FreeCAD.Base import Quantity

_NumberFormat: TypeAlias = Literal["g", "f", "e"]
Radian: Quantity

# Schema helpers
@overload
def listSchemas() -> tuple[str, ...]:
    """Return the full ordered schema list."""
    ...

@overload
def listSchemas(index: int, /) -> str:
    """Return one schema name by numeric index."""
    ...

# Quantity formatting and parsing
@overload
def toNumber(value: Quantity, format: _NumberFormat = ..., decimals: int = ..., /) -> str:
    """Format an existing Quantity value using the current unit schema."""
    ...

@overload
def toNumber(value: float, format: _NumberFormat = ..., decimals: int = ..., /) -> str:
    """Format a plain numeric value using the current unit schema."""
    ...

def parseQuantity(expression: str, /) -> Quantity:
    """Parse one unit expression into a Quantity."""
    ...

def getSchema() -> int:
    """Return the active unit-schema index."""
    ...

def setSchema(index: int, /) -> None:
    """Set the active unit-schema index."""
    ...

def schemaTranslate(quantity: Quantity, schema: int, /) -> tuple[str, float, str]:
    """Translate one quantity into the textual pieces used by another schema."""
    ...
