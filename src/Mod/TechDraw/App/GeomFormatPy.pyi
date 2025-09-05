from typing import Any, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import constmethod, export

@export(
    Father="PyObjectBase",
    Name="GeomFormatPy",
    Twin="GeomFormat",
    TwinPointer="GeomFormat",
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class GeomFormatPy(PyObjectBase):
    """
    GeomFormat specifies appearance parameters for TechDraw Geometry objects
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this geomformat"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this geomformat"""
        ...
    Tag: Final[str]
    """Gives the tag of the GeomFormat as string."""
