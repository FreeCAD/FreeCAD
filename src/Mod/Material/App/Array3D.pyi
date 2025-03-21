from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Any, Final, List


@export(
    Twin="Material3DArray",
    TwinPointer="Material3DArray",
    Namespace="Materials",
    Include="Mod/Material/App/MaterialValue.h",
    Delete=True,
    Constructor=True
)
class Array3D(BaseClass):
    """
    3D Array of material properties.

    Author: DavidCarter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    Array: Final[List] = ...
    """The 3 dimensional array."""

    Columns: Final[int] = ...
    """The number of columns in the array."""

    Depth: Final[int] = ...
    """The depth of the array (3rd dimension)."""

    @constmethod
    def getRows(self) -> int:
        """
        Get the number of rows in the array at the specified depth.
        """
        ...

    @constmethod
    def getValue(self) -> Any:
        """
        Get the value at the given row and column
        """
        ...

    @constmethod
    def getDepthValue(self) -> Any:
        """
        Get the column value at the given depth
        """
        ...
