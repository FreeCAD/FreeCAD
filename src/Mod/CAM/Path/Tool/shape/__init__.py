# -*- coding: utf-8 -*-
# This package aggregates tool bit shape classes.

# Import the base class and all concrete shape classes
from .models.base import ToolBitShape
from .models.ballend import ToolBitShapeBallend
from .models.bullnose import ToolBitShapeBullnose
from .models.chamfer import ToolBitShapeChamfer
from .models.custom import ToolBitShapeCustom
from .models.dovetail import ToolBitShapeDovetail
from .models.drill import ToolBitShapeDrill
from .models.endmill import ToolBitShapeEndmill
from .models.fillet import ToolBitShapeFillet
from .models.probe import ToolBitShapeProbe
from .models.reamer import ToolBitShapeReamer
from .models.slittingsaw import ToolBitShapeSlittingSaw
from .models.tap import ToolBitShapeTap
from .models.threadmill import ToolBitShapeThreadMill
from .models.vbit import ToolBitShapeVBit
from .models.icon import (
    ToolBitShapeIcon,
    ToolBitShapePngIcon,
    ToolBitShapeSvgIcon,
)

# A list of the name of each ToolBitShape
TOOL_BIT_SHAPE_NAMES = sorted([cls.name for cls in ToolBitShape.__subclasses__()])

# Define __all__ for explicit public interface
__all__ = [
    "ToolBitShape",
    "ToolBitShapeBallend",
    "ToolBitShapeBullnose",
    "ToolBitShapeChamfer",
    "ToolBitShapeCustom",
    "ToolBitShapeDovetail",
    "ToolBitShapeDrill",
    "ToolBitShapeEndmill",
    "ToolBitShapeFillet",
    "ToolBitShapeProbe",
    "ToolBitShapeReamer",
    "ToolBitShapeSlittingSaw",
    "ToolBitShapeTap",
    "ToolBitShapeThreadMill",
    "ToolBitShapeVBit",
    "TOOL_BIT_SHAPE_NAMES",
    "ToolBitShapeIcon",
    "ToolBitShapeSvgIcon",
    "ToolBitShapePngIcon",
]
