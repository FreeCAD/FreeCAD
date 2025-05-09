# -*- coding: utf-8 -*-
import pathlib

import Path
import Path.Log
from Path.Tool.assets import asset_manager
from Path.Preferences import addToolPreferenceObserver

from ..assets.store.flat import FlatFileStore

toolbitshape_store = FlatFileStore(
    name="shapestore",
    base_dir=pathlib.Path(Path.Preferences.getShapePath()),
    type_to_extension={
        "toolbitshape": ".fcstd",
        "toolbitshapepng": ".png",
        "toolbitshapesvg": ".svg",
    }
)
asset_manager.register_store(toolbitshape_store)


def on_tool_path_changed(group, key, value):
    if group == "Path" and key == "ShapePath":
        new_path = pathlib.Path(Path.Preferences.getShapePath())
        toolbitshape_store.set_dir(new_path)
        Path.Log.info(f"ToolBitShape store directory updated to {new_path}")

addToolPreferenceObserver(on_tool_path_changed)
