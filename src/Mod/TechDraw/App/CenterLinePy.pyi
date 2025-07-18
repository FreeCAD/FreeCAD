from typing import Any, Dict, Final, List

from Base import PyObjectBase
from Base.Metadata import constmethod, export

@export(
    Father="PyObjectBase",
    Name="CenterLinePy",
    Twin="CenterLine",
    TwinPointer="CenterLine",
    Include="Mod/TechDraw/App/CenterLine.h",
    Namespace="TechDraw",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    Constructor=True,  # Allow constructing this object
    Delete=True,
)
class CenterLinePy(PyObjectBase):
    """
    CenterLine specifies additional mark up edges in a View
    """

    @constmethod
    def clone(self) -> Any:
        """Create a clone of this centerline"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Create a copy of this centerline"""
        ...
    Tag: Final[str]  # Read-only attribute
    """Gives the tag of the CenterLine as string."""

    Type: Final[int]  # Read-only attribute
    """0 - face, 1 - 2 line, 2 - 2 point."""

    Mode: int  #
    """0 - vert/ 1 - horiz/ 2 - aligned."""

    Format: Dict[str, Any]  #
    """The appearance attributes (style, color, weight, visible) for this CenterLine."""

    HorizShift: float  #
    """The left/right offset for this CenterLine."""

    VertShift: float  #
    """The up/down offset for this CenterLine."""

    Rotation: float  #
    """The rotation of the Centerline in degrees."""

    Extension: float  #
    """The additional length to be added to this CenterLine."""

    Flip: bool  #
    """Reverse the order of points for 2 point CenterLine."""

    Edges: List[Any]  #
    """The names of source edges for this CenterLine."""

    Faces: List[Any]  #
    """The names of source Faces for this CenterLine."""

    Points: List[Any]  #
    """The names of source Points for this CenterLine."""
