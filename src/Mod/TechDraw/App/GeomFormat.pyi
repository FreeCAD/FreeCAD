from typing import Any, Final

from Base.PyObjectBase import PyObjectBase
from Base.Metadata import constmethod, export

@export(
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    Constructor=True,
    Delete=True,
)
class GeomFormat(PyObjectBase):
    """
    Author: WandererFan (wandererfan@gmail.com)
    License: LGPL-2.1-or-later
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
