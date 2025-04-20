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
from .bullnose import ToolBitShapeBullnose
from .vbit import ToolBitShapeVBit

# A list of the name of each ToolBitShape
TOOL_BIT_SHAPE_NAMES = sorted(
    [cls.name for cls in ToolBitShape.__subclasses__()]
)

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
    "ToolBitShapeBullnose",
    "ToolBitShapeVBit",
]
