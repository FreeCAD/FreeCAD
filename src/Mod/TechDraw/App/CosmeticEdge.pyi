from typing import Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import export

@export(
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    FatherInclude="Base/GeometryPyCXX.h",
    Constructor=True,
    Delete=True,
)
class CosmeticEdge(PyObjectBase):
    """
    CosmeticEdge specifies an extra (cosmetic) edge in Views
    
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
    """

    Tag: Final[str]
    """Gives the tag of the CosmeticEdge as string."""

    Start: PyCXXVector
    """Gives the position of one end of this CosmeticEdge as vector."""

    End: PyCXXVector
    """Gives the position of one end of this CosmeticEdge as vector."""

    Center: PyCXXVector
    """Gives the position of center point of this CosmeticEdge as vector."""

    Radius: float
    """Gives the radius of CosmeticEdge in mm."""

    Format: dict
    """The appearance attributes (style, weight, color, visible) for this CosmeticEdge."""
