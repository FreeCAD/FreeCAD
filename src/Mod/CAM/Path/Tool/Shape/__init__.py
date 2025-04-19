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

# Maps ToolBitShape.aliases to ToolBitShape
# Note: This relies on the classes being imported above.
TOOL_BIT_SHAPE_CLASSES: Dict[str, Type[ToolBitShape]] = {}

for cls in ToolBitShape.__subclasses__():
    if cls.__module__.startswith(__name__) and hasattr(cls, 'aliases') and cls.aliases:
        for alias in cls.aliases:
            if alias in TOOL_BIT_SHAPE_CLASSES:
                raise ValueError(
                    f"Duplicate alias '{alias}' found for shape class '{cls.__name__}'. "
                    f"It was already mapped to '{TOOL_BIT_SHAPE_CLASSES[alias].__name__}'. "
                    "Aliases must be unique."
                )
            TOOL_BIT_SHAPE_CLASSES[alias] = cls
            TOOL_BIT_SHAPE_CLASSES[alias+".fcstd"] = cls

# A list of the first alias of each ToolBitShape
TOOL_BIT_SHAPE_NAMES = sorted(
    [cls.aliases[0] for cls in set(TOOL_BIT_SHAPE_CLASSES.values())]
)

def get_shape_class_from_alias(alias: str) -> Optional[Type[ToolBitShape]]:
    """Get the shape class corresponding to the given name (case-insensitive)."""
    return TOOL_BIT_SHAPE_CLASSES.get(alias.lower())

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
