# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Metadata import export
from PyObjectBase import PyObjectBase
from Quantity import Quantity
from typing import Final, overload

@export(
    NumberProtocol=True,
    RichCompare=True,
    Constructor=True,
    Delete=True,
)
class Unit(PyObjectBase):
    """
    Unit
    defines a unit type, calculate and compare.

    The following constructors are supported:
    Unit()                        -- empty constructor
    Unit(i1,i2,i3,i4,i5,i6,i7,i8) -- unit signature
    Unit(Quantity)                -- copy unit from Quantity
    Unit(Unit)                    -- copy constructor
    Unit(string)                  -- parse the string for units

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    @overload
    def __init__(
        self,
        i1: int = 0,
        i2: int = 0,
        i3: int = 0,
        i4: int = 0,
        i5: int = 0,
        i6: int = 0,
        i7: int = 0,
        i8: int = 0,
        /,
    ) -> None: ...
    @overload
    def __init__(self, quantity: Quantity, /) -> None: ...
    @overload
    def __init__(self, unit: Unit, /) -> None: ...
    @overload
    def __init__(self, string: str, /) -> None: ...

    Type: Final[str] = ...
    """holds the unit type as a string, e.g. 'Area'."""

    Signature: Final[tuple[int, ...]] = ...
    """Returns the signature."""
