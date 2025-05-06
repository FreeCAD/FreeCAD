# -*- coding: utf-8 -*-
import sys
from .toolbit import ToolBit, ToolBitFactory

# For backward compatibility with files saved before the toolbit rename
# This makes the Path.Tool.toolbit.base module available as Path.Tool.Bit.
# Since C++ does not use the standard Python import mechanism and instead
# looks for Path.Tool.Bit in sys.modules, we need to update sys.modules here.
from .toolbit.models import base as Bit
sys.modules[__name__ + ".Bit"] = Bit

# Define __all__ for explicit public interface
__all__ = [
    "ToolBit",
    "ToolBitFactory",
]
