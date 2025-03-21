from Base.Metadata import export
from Base.BaseClass import BaseClass
from Base.Metadata import constmethod
from typing import Final, List, Any


@export(
    Twin="Material2DArray",
    TwinPointer="Material2DArray",
    Namespace="Materials",
    Include="Mod/Material/App/MaterialValue.h",
    Delete=True,
    Constructor=True
)
class Array2D(BaseClass):
    """
    2D Array of material properties.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Array: Final[List] = ...
    """The 2 dimensional array."""

    Rows: Final[int] = ...
    """The number of rows in the array."""

    Columns: Final[int] = ...
    """The number of columns in the array."""

    @constmethod
    def getRow(self, value: Any) -> Any:
        """
        Get the row given the first column value
        """
        ...

    @constmethod
    def getValue(self, row: int, column: int) -> Any:
        """
        Get the value at the given row and column
        """
        ...

