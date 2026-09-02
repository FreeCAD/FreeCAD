# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.Quantity import Quantity
from Base.Persistence import Persistence
from typing import Final, overload

@export(
    Include="Mod/Sketcher/App/Constraint.h",
    Constructor=True,
    Delete=True,
)
class Constraint(Persistence):
    """
    With this object you can handle sketches

    Author: Juergen Riegel (FreeCAD@juergen-riegel.net)
    Licence: LGPL
    """

    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, constraint_type: str, elements: list[int], /) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        elements: list[int],
        text: str,
        font: str,
        is_text_height: bool = ...,
        /,
    ) -> None: ...
    @overload
    def __init__(self, constraint_type: str, first_index: int, /) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        first_index: int,
        second: int | float | Quantity,
        activated: bool = ...,
        driving: bool = ...,
        /,
    ) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        first_index: int,
        first_position: int,
        second: int | float | Quantity,
        activated: bool = ...,
        driving: bool = ...,
        /,
    ) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        first_index: int,
        first_position: int,
        second: int,
        value: int | float | Quantity,
        activated: bool = ...,
        driving: bool = ...,
        /,
    ) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        first_index: int,
        first_position: int,
        second: int,
        second_position: int,
        third: int | float | Quantity,
        activated: bool = ...,
        driving: bool = ...,
        /,
    ) -> None: ...
    @overload
    def __init__(
        self,
        constraint_type: str,
        first_index: int,
        first_position: int,
        second: int,
        second_position: int,
        third: int,
        third_position: int | float | Quantity,
        activated: bool = ...,
        driving: bool = ...,
        /,
    ) -> None: ...

    Type: Final[str] = ""
    """Get the constraint type"""

    First: int = 0
    """First geometry index the Constraint refers to"""

    FirstPos: int = 0
    """Position of first geometry index the Constraint refers to"""

    Second: int = 0
    """Second geometry index the Constraint refers to"""

    SecondPos: int = 0
    """Position of second geometry index the Constraint refers to"""

    Third: int = 0
    """Third geometry index the Constraint refers to"""

    ThirdPos: int = 0
    """Position of third geometry index the Constraint refers to"""

    Value: Final[float] = 0.0
    """Value of the Constraint"""

    Name: str = ""
    """Name of the constraint"""

    Driving: Final[bool] = False
    """Driving Constraint"""

    InVirtualSpace: Final[bool] = False
    """Constraint in virtual space"""

    IsActive: Final[bool] = False
    """Returns whether the constraint active (enforced) or not"""

    LabelDistance: Final[float] = 0.0
    """Label distance"""

    LabelPosition: Final[float] = 0.0
    """Label position"""
