from typing import Final

from Base import PyObjectBase
from Base.Metadata import export
from Base.Vector import Vector

@export(
    Father="PyObjectBase",
    Name="CosmeticEdgePy",
    Twin="CosmeticEdge",
    TwinPointer="CosmeticEdge",
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    FatherInclude="Base/GeometryPyCXX.h",
    FatherNamespace="Base",
    ReadOnly=["Tag"],
    Constructor=True,
    Delete=True,
    ExtraIncludes=["Base/Vector3D.h"],  # Add Vector3D.h include
)
class CosmeticEdgePy(PyObjectBase):
    """
    CosmeticEdge specifies an extra (cosmetic) edge in Views
    """

    Tag: Final[str]  # Only Tag is read-only
    """Gives the tag of the CosmeticEdge as string."""

    # Remove @con decorator which is causing issues
    Start: Vector
    """Gives the position of one end of this CosmeticEdge as vector."""

    End: Vector
    """Gives the position of one end of this CosmeticEdge as vector."""

    Center: Vector
    """Gives the position of center point of this CosmeticEdge as vector."""

    Radius: float
    """Gives the radius of CosmeticEdge in mm."""

    Format: dict
    """The appearance attributes (style, weight, color, visible) for this CosmeticEdge."""
