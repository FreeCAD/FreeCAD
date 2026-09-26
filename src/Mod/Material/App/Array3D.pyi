# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from Base.Quantity import Quantity
from typing import Final, List


@export(
    Twin="Array3D",
    TwinPointer="Array3D",
    Namespace="Materials",
    Include="Mod/Material/App/MaterialValue.h",
    Delete=True,
    Constructor=True,
)
class Array3D(BaseClass):
    """
    3D Array of material properties.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Array: Final[List] = ...
    """The 3 dimensional array."""

    Dimensions: Final[int] = ...
    """The number of dimensions in the array, in this case 3."""

    Columns: int = ...
    """The number of columns in the array."""

    Depth: int = ...
    """The depth of the array (3rd dimension)."""

    @constmethod
    def getRows(self, depth: int = ..., /) -> int:
        """
        Get the number of rows in the array at the specified depth.
        """
        ...

    @constmethod
    def getValue(self, depth: int, row: int, column: int, /) -> Quantity:
        """
        Get the value at the given row and column
        """
        ...

    @constmethod
    def getDepthValue(self, depth: int, /) -> Quantity:
        """
        Get the column value at the given depth
        """
        ...

    def setDepthValue(self, depth: int, value: str, /) -> None:
        """
        Set the column value at the given depth
        """
        ...

    def setValue(self, depth: int, row: int, column: int, value: str, /) -> None:
        """
        Set the value at the given depth, row, and column
        """
        ...

    def setRows(self, depth: int, rows: int, /) -> None:
        """
        Set the number of rows at the given depth
        """
        ...
