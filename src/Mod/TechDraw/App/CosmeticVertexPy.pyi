from typing import Any, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import constmethod, export

@export(
    Father="PyObjectBase",
    Name="CosmeticVertexPy",
    Twin="CosmeticVertex",
    TwinPointer="CosmeticVertex",
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,
    Delete=True,
)
class CosmeticVertexPy(PyObjectBase):
    """
    CosmeticVertex specifies an extra (cosmetic) vertex in Views
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this CosmeticVertex"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this CosmeticVertex"""
        ...
    Tag: Final[str]
    """Gives the tag of the CosmeticVertex as string."""

    Point: Any
    """Gives the position of this CosmeticVertex as vector."""

    Show: bool
    """Show/hide the vertex."""

    Color: Any # type: tuple[float, float, float, float]]
    """set/return the vertex's colour using a tuple (rgba)."""

    Size: Any
    """set/return the vertex's radius in mm."""

    Style: Any
    """set/return the vertex's style as integer."""
