# -*- coding: utf-8 -*-
# This package aggregates tool bit classes.

# Import the base class and all concrete shape classes
from .models.base import ToolBit
from .models.ballend import ToolBitBallend
from .models.bullnose import ToolBitBullnose
from .models.chamfer import ToolBitChamfer
from .models.custom import ToolBitCustom
from .models.dovetail import ToolBitDovetail
from .models.drill import ToolBitDrill
from .models.endmill import ToolBitEndmill
from .models.fillet import ToolBitFillet
from .models.probe import ToolBitProbe
from .models.reamer import ToolBitReamer
from .models.slittingsaw import ToolBitSlittingSaw
from .models.tap import ToolBitTap
from .models.threadmill import ToolBitThreadMill
from .models.vbit import ToolBitVBit

# Define __all__ for explicit public interface
__all__ = [
    "ToolBit",
    "ToolBitBallend",
    "ToolBitBullnose",
    "ToolBitChamfer",
    "ToolBitCustom",
    "ToolBitDovetail",
    "ToolBitDrill",
    "ToolBitEndmill",
    "ToolBitFillet",
    "ToolBitProbe",
    "ToolBitReamer",
    "ToolBitSlittingSaw",
    "ToolBitTap",
    "ToolBitThreadMill",
    "ToolBitVBit",
]
