# -*- coding: utf-8 -*-
# This package aggregates tool bit classes.

# Import the base class and all concrete shape classes
from .base import ToolBit, Factory as ToolBitFactory

from .ballend import ToolBitBallEnd
from .chamfer import ToolBitChamfer
from .dovetail import ToolBitDovetail
from .drill import ToolBitDrill
from .endmill import ToolBitEndMill
from .probe import ToolBitProbe
from .reamer import ToolBitReamer
from .slittingsaw import ToolBitSlittingSaw
from .tap import ToolBitTap
from .threadmill import ToolBitThreadMill
from .bullnose import ToolBitBullnose
from .vbit import ToolBitVBit

# A list of the name of each ToolBit
#TOOL_BIT_NAMES = sorted([cls.name for cls in ToolBit.__subclasses__()])

# Define __all__ for explicit public interface
__all__ = [
    "ToolBit",
    "ToolBitFactory",
    "ToolBitBallEnd",
    "ToolBitChamfer",
    "ToolBitDovetail",
    "ToolBitDrill",
    "ToolBitEndMill",
    "ToolBitProbe",
    "ToolBitReamer",
    "ToolBitSlittingSaw",
    "ToolBitTap",
    "ToolBitThreadMill",
    "ToolBitBullnose",
    "ToolBitVBit",
]
