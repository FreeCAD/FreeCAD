# -*- coding: utf-8 -*-
import Path
from Path import Preferences
from Path.Preferences import addToolPreferenceObserver
from .assets import AssetManager, FileStore


def ensure_library_assets_initialized(
    asset_manager: AssetManager,
    store_name: str = "local"
):
    """
    Ensures the given store is initialized with built-in library
    if it is currently empty.
    """
    builtin_library_path = Preferences.getBuiltinLibraryPath()

    if asset_manager.is_empty("toolbitlibrary", store=store_name):
        for path in builtin_library_path.glob("*.fctl"):
            asset_manager.add_file("toolbitlibrary", path)

def ensure_toolbit_assets_initialized(
    asset_manager: AssetManager,
    store_name: str = "local"
):
    """
    Ensures the given store is initialized with built-in bits
    if it is currently empty.
    """
    builtin_toolbit_path = Preferences.getBuiltinToolBitPath()

    if asset_manager.is_empty("toolbit", store=store_name):
        for path in builtin_toolbit_path.glob("*.fctb"):
            asset_manager.add_file("toolbit", path)


def ensure_toolbitshape_assets_initialized(
    asset_manager: AssetManager,
    store_name: str = "local"
):
    """
    Ensures the given store is initialized with built-in shapes
    if it is currently empty.
    """
    builtin_shape_path = Preferences.getBuiltinShapePath()

    if asset_manager.is_empty("toolbitshape", store=store_name):
        for path in builtin_shape_path.glob("*.fcstd"):
            asset_manager.add_file("toolbitshape", path)

    if asset_manager.is_empty("toolbitshapesvg", store=store_name):
        for path in builtin_shape_path.glob("*.svg"):
            asset_manager.add_file("toolbitshapesvg", path, asset_id=path.stem+".svg")

    if asset_manager.is_empty("toolbitshapepng", store=store_name):
        for path in builtin_shape_path.glob("*.png"):
            asset_manager.add_file("toolbitshapepng", path, asset_id=path.stem+".png")


def ensure_assets_initialized(asset_manager: AssetManager, store = "local"):
    """
    Ensures the given store is initialized with built-in assets.
    """
    ensure_library_assets_initialized(asset_manager, store)
    ensure_toolbit_assets_initialized(asset_manager, store)
    ensure_toolbitshape_assets_initialized(asset_manager, store)


def _on_asset_path_changed(group, key, value):
    Path.Log.info(f"CAM asset directory changed in preferences: {group} {key} {value}")
    cam_asset_store.set_dir(Preferences.getAssetPath())
    ensure_assets_initialized(cam_assets)


# Set up the local CAM asset storage.
cam_asset_store = FileStore(
    name="local",
    base_dir=Preferences.getAssetPath(),
    mapping={
        "toolbitlibrary": "Library/{asset_id}.fctl",
        "toolbit": "Bit/{asset_id}.fctb",
        "toolbitshape": "Shape/{asset_id}.fcstd",
        "toolbitshapesvg": "Shape/{asset_id}",  # Asset ID has ".svg" included
        "toolbitshapepng": "Shape/{asset_id}",  # Asset ID has ".png" included
        "machine": "Machine/{asset_id}.fcm",
    },
)

# Set up the CAM asset manager.
Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
cam_assets = AssetManager()
cam_assets.register_store(cam_asset_store)
try:
    ensure_assets_initialized(cam_assets)
except Exception as e:
    Path.Log.error(f"Failed to initialize CAM assets in {cam_asset_store._base_dir}: {e}")
else:
    Path.Log.debug(f"Using CAM assets in {cam_asset_store._base_dir}")
addToolPreferenceObserver(_on_asset_path_changed)
