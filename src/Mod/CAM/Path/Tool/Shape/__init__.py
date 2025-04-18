# -*- coding: utf-8 -*-
# This package aggregates tool bit shape classes.

from typing import Dict, Optional, Type

# Import the base class and all concrete shape classes
from .base import ToolBitShape
from .ballend import ToolBitShapeBallEnd
from .chamfer import ToolBitShapeChamfer
from .dovetail import ToolBitShapeDovetail
from .drill import ToolBitShapeDrill
from .endmill import ToolBitShapeEndMill
from .probe import ToolBitShapeProbe
from .reamer import ToolBitShapeReamer
from .slittingsaw import ToolBitShapeSlittingSaw
from .tap import ToolBitShapeTap
from .threadmill import ToolBitShapeThreadMill
from .torus import ToolBitShapeTorus
from .vbit import ToolBitShapeVBit

# Recreate the dictionary mapping shape names (lowercase class names) to classes
# We gather all direct subclasses of ToolBitShape that are defined *within* this module's scope
# Note: This relies on the classes being imported above.
TOOL_BIT_SHAPE_CLASSES: Dict[str, Type[ToolBitShape]] = {
    cls.__name__.lower(): cls
    for cls in ToolBitShape.__subclasses__()
    if cls.__module__.startswith(__name__)
}

# Create a sorted list of user-friendly shape names (class names)
TOOL_BIT_SHAPE_NAMES = sorted(
    [cls.__name__ for cls in TOOL_BIT_SHAPE_CLASSES.values()]
)


def get_shape_class(name: str) -> Optional[Type[ToolBitShape]]:
    """Get the shape class corresponding to the given name (case-insensitive)."""
    return TOOL_BIT_SHAPE_CLASSES.get(name.lower())

# Define __all__ for explicit public interface
__all__ = [
    "ToolBitShape",
    "ToolBitShapeBallEnd",
    "ToolBitShapeChamfer",
    "ToolBitShapeDovetail",
    "ToolBitShapeDrill",
    "ToolBitShapeEndMill",
    "ToolBitShapeProbe",
    "ToolBitShapeReamer",
    "ToolBitShapeSlittingSaw",
    "ToolBitShapeTap",
    "ToolBitShapeThreadMill",
    "ToolBitShapeTorus",
    "ToolBitShapeVBit",
    "TOOL_BIT_SHAPE_CLASSES",
    "get_shape_class",
]
