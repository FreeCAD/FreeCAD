# -*- coding: utf-8 -*-
# This package aggregates tool bit shape classes.

# Import the base class and all concrete shape classes
from .base import ToolBitShape
from .ballend import ToolBitShapeBallend
from .chamfer import ToolBitShapeChamfer
from .dovetail import ToolBitShapeDovetail
from .drill import ToolBitShapeDrill
from .endmill import ToolBitShapeEndmill
from .probe import ToolBitShapeProbe
from .reamer import ToolBitShapeReamer
from .slittingsaw import ToolBitShapeSlittingSaw
from .tap import ToolBitShapeTap
from .threadmill import ToolBitShapeThreadMill
from .bullnose import ToolBitShapeBullnose
from .vbit import ToolBitShapeVBit

# A list of the name of each ToolBitShape
TOOL_BIT_SHAPE_NAMES = sorted([cls.name for cls in ToolBitShape.__subclasses__()])

# Define __all__ for explicit public interface
__all__ = [
    "ToolBitShape",
    "ToolBitShapeBallend",
    "ToolBitShapeChamfer",
    "ToolBitShapeDovetail",
    "ToolBitShapeDrill",
    "ToolBitShapeEndmill",
    "ToolBitShapeProbe",
    "ToolBitShapeReamer",
    "ToolBitShapeSlittingSaw",
    "ToolBitShapeTap",
    "ToolBitShapeThreadMill",
    "ToolBitShapeBullnose",
    "ToolBitShapeVBit",
]
