# -*- coding: utf-8 -*-
import sys
from .assets import DummyAssetSerializer
from .camassets import cam_assets
from .library import Library
from .library.serializers import FCTLSerializer
from .toolbit import ToolBit
from .toolbit.serializers import FCTBSerializer
from .shape import ToolBitShape, ToolBitShapePngIcon, ToolBitShapeSvgIcon
from .machine import Machine

# Register asset classes and serializers.
cam_assets.register_asset(Library, FCTLSerializer)
cam_assets.register_asset(ToolBit, FCTBSerializer)
cam_assets.register_asset(ToolBitShape, DummyAssetSerializer)
cam_assets.register_asset(ToolBitShapePngIcon, DummyAssetSerializer)
cam_assets.register_asset(ToolBitShapeSvgIcon, DummyAssetSerializer)
cam_assets.register_asset(Machine, DummyAssetSerializer)

# For backward compatibility with files saved before the toolbit rename
# This makes the Path.Tool.toolbit.base module available as Path.Tool.Bit.
# Since C++ does not use the standard Python import mechanism and instead
# looks for Path.Tool.Bit in sys.modules, we need to update sys.modules here.
from .toolbit.models import base as Bit
sys.modules[__name__ + ".Bit"] = Bit

# Define __all__ for explicit public interface
__all__ = [
    "ToolBit",
    "cam_assets",
]
