# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Base.BaseClass import BaseClass
from Base.Metadata import constmethod
from typing import Final, List, Any


@export(
    Twin="Array2D",
    TwinPointer="Array2D",
    Namespace="Materials",
    Include="Mod/Material/App/MaterialValue.h",
    Delete=True,
    Constructor=True,
)
class Array2D(BaseClass):
    """
    2D Array of material properties.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Array: Final[List] = ...
    """The 2 dimensional array."""

    Dimensions: Final[int] = ...
    """The number of dimensions in the array, in this case 2."""

    Rows: int = ...
    """The number of rows in the array."""

    Columns: int = ...
    """The number of columns in the array."""

    @constmethod
    def getRow(self, value: Any, /) -> Any:
        """
        Get the row given the first column value
        """
        ...

    @constmethod
    def getValue(self, row: int, column: int, /) -> Any:
        """
        Get the value at the given row and column
        """
        ...

    def setValue(self, row: int, column: int, value: Any, /):
        """
        Set the value at the given row and column
        """
        ...
