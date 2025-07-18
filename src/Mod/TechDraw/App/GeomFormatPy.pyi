from typing import (
    ClassVar,
    Final,
    List,
    Dict,
    Tuple,
    TypeVar,
    Any,
    Optional,
    Union,
    overload,
)
from Base import object
from Base.Metadata import export
from Base.Metadata import constmethod

@export(
    Father="PyObjectBase",
    Name="GeomFormatPy",
    Twin="GeomFormat",
    TwinPointer="GeomFormat",
    Include="Mod/TechDraw/App/Cosmetic.h",
    Namespace="TechDraw",
    FatherInclude="Base/PyObjectBase.h",
    FatherNamespace="Base",
    ReadOnly=["Tag"],
    Constructor=True,  # Allow constructing this object
    Delete=True,
)
class GeomFormatPy(object):
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
    Tag: Final[str]  # ReadOnly
    """Gives the tag of the GeomFormat as string."""
