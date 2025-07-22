from Base.Metadata import export, constmethod
from Base.PyObjectBase import PyObjectBase
from typing import Final


@export(
    Include="Mod/Part/App/GeometryExtension.h",
    Constructor=True,
    Delete=True,
)
class GeometryExtension(PyObjectBase):
    """
    The abstract class GeometryExtension enables to extend geometry objects with application specific data.
    Author: Abdullah Tahiri (abdullah.tahiri.yo@gmail.com)
    Licence: LGPL
    """

    Name: str = ""
    """Sets/returns the name of this extension."""

    @constmethod
    def copy(self) -> "GeometryExtension":
        """Create a copy of this geometry extension."""
        ...
