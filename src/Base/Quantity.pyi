# SPDX-License-Identifier: LGPL-2.1-or-later

from Metadata import export, constmethod
from PyObjectBase import PyObjectBase
from typing import overload, Final, Any
from Unit import Unit as UnitPy

@export(
    NumberProtocol=True,
    RichCompare=True,
    Constructor=True,
    Delete=True,
)
class Quantity(PyObjectBase):
    """
    Quantity
    defined by a value and a unit.

    The following constructors are supported:
    Quantity() -- empty constructor
    Quantity(Value) -- empty constructor
    Quantity(Value,Unit) -- empty constructor
    Quantity(Quantity) -- copy constructor
    Quantity(string) -- arbitrary mixture of numbers and chars defining a Quantity

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    Value: float = ...
    """Numeric Value of the Quantity (in internal system mm,kg,s)"""

    Unit: UnitPy = ...
    """Unit of the Quantity"""

    UserString: Final[str] = ...
    """Unit of the Quantity"""

    Format: dict[str, int | str] = ...
    """Format of the Quantity"""

    # fmt: off
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, value: float, /) -> None: ...
    @overload
    def __init__(self, value: float, unit: UnitPy, /) -> None: ...
    @overload
    def __init__(self, quantity: Quantity, /) -> None: ...
    @overload
    def __init__(self, string: str, /) -> None: ...
    @overload
    def __init__(self, value: float, unit: str, /) -> None: ...
    @overload
    def __init__(
        self,
        value: float,  # technically value is optional, but if-branch wont set self without it.
        i1: int = 0, i2: int = 0, i3: int = 0, i4: int = 0,
        i5: int = 0, i6: int = 0, i7: int = 0, i8: int = 0,
        /,
    ) -> None: ...
    # fmt: on

    @constmethod
    def toStr(self, decimals: int = ..., /) -> str:
        """
        Returns a string representation rounded to number of decimals. If no decimals are specified then
        the internal precision is used
        """

    @constmethod
    def getUserPreferred(self) -> tuple[str, float, str]:
        """
        Returns tuple with (quantity incl. unit, translation factor, unit)
        """

    @overload
    def getValueAs(self, unit: str, /) -> float: ...
    @overload
    @constmethod
    def getValueAs(
        self,
        translation: float,
        i1: int = 0,
        i2: int = 0,
        i3: int = 0,
        i4: int = 0,
        i5: int = 0,
        i6: int = 0,
        i7: int = 0,
        i8: int = 0,
        /,
    ) -> float: ...
    @overload
    @constmethod
    def getValueAs(self, translation: float, unit: UnitPy, /) -> float: ...
    @overload
    @constmethod
    def getValueAs(self, unit: UnitPy, /) -> float: ...
    @overload
    @constmethod
    def getValueAs(self, quantity: Quantity, /) -> float: ...
    @overload
    @constmethod
    def getValueAs(self, /, *args: Any) -> float:
        """
        Returns a floating point value as the provided unit

        Following parameters are allowed:
        getValueAs('m/s')  # unit string to parse
        getValueAs(2.45,1) # translation value and unit signature
        getValueAs(FreeCAD.Units.Pascal) # predefined standard units
        getValueAs(Qantity('N/m^2')) # a quantity
        getValueAs(Unit(0,1,0,0,0,0,0,0)) # a unit
        """

    @overload
    @constmethod
    def __round__(self, /) -> int: ...
    @overload
    @constmethod
    def __round__(self, ndigits: int, /) -> float: ...
    @overload
    @constmethod
    def __round__(self, ndigits: int = ..., /) -> int | float:
        """
        Returns the Integral closest to x, rounding half toward even.
        When an argument is passed, work like built-in round(x, ndigits).
        """
