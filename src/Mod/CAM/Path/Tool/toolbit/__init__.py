# -*- coding: utf-8 -*-
# This package aggregates tool bit classes.

# Import the base class and all concrete shape classes
from .models.base import ToolBit, Factory as ToolBitFactory
from .registry import TOOLBIT_REGISTRY

from .models.ballend import ToolBitBallend
from .models.chamfer import ToolBitChamfer
from .models.dovetail import ToolBitDovetail
from .models.drill import ToolBitDrill
from .models.endmill import ToolBitEndmill
from .models.probe import ToolBitProbe
from .models.reamer import ToolBitReamer
from .models.slittingsaw import ToolBitSlittingSaw
from .models.tap import ToolBitTap
from .models.threadmill import ToolBitThreadMill
from .models.bullnose import ToolBitBullnose
from .models.vbit import ToolBitVBit

# Define __all__ for explicit public interface
__all__ = [
    "TOOLBIT_REGISTRY",
    "ToolBit",
    "ToolBitFactory",
    "ToolBitBallend",
    "ToolBitChamfer",
    "ToolBitDovetail",
    "ToolBitDrill",
    "ToolBitEndmill",
    "ToolBitProbe",
    "ToolBitReamer",
    "ToolBitSlittingSaw",
    "ToolBitTap",
    "ToolBitThreadMill",
    "ToolBitBullnose",
    "ToolBitVBit",
]
