# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from typing import Optional, Union, Sequence
import Path
from Path import Preferences
from Path.Preferences import addToolPreferenceObserver
from .assets import AssetManager, AssetUri, Asset, FileStore


def ensure_library_assets_initialized(asset_manager: AssetManager, store_name: str = "local"):
    """
    Ensures the given store is initialized with built-in library
    if it is currently empty.
    """
    builtin_library_path = Preferences.getBuiltinLibraryPath()

    if asset_manager.is_empty("toolbitlibrary", store=store_name):
        for path in builtin_library_path.glob("*.fctl"):
            asset_manager.add_file("toolbitlibrary", path)


def ensure_toolbit_assets_initialized(asset_manager: AssetManager, store_name: str = "local"):
    """
    Ensures the given store is initialized with built-in bits
    if it is currently empty.
    """
    builtin_toolbit_path = Preferences.getBuiltinToolBitPath()

    if asset_manager.is_empty("toolbit", store=store_name):
        for path in builtin_toolbit_path.glob("*.fctb"):
            asset_manager.add_file("toolbit", path)


def ensure_toolbitshape_assets_initialized(asset_manager: AssetManager, store_name: str = "local"):
    """
    Ensures the given store is initialized with built-in shapes
    if it is currently empty.
    """
    builtin_shape_path = Preferences.getBuiltinShapePath()

    if asset_manager.is_empty("toolbitshape", store=store_name):
        for path in builtin_shape_path.glob("*.fcstd"):
            uri = AssetUri.build(
                asset_type="toolbitshape",
                asset_id=path.stem,
            )
            if not asset_manager.exists(uri, store=store_name):
                asset_manager.add_file("toolbitshape", path)

    if asset_manager.is_empty("toolbitshapesvg", store=store_name):
        for path in builtin_shape_path.glob("*.svg"):
            uri = AssetUri.build(
                asset_type="toolbitshapesvg",
                asset_id=path.stem + ".svg",
            )
            if not asset_manager.exists(uri, store=store_name):
                asset_manager.add_file("toolbitshapesvg", path, asset_id=path.stem + ".svg")

    if asset_manager.is_empty("toolbitshapepng", store=store_name):
        for path in builtin_shape_path.glob("*.png"):
            uri = AssetUri.build(
                asset_type="toolbitshapepng",
                asset_id=path.stem + ".png",
            )
            if not asset_manager.exists(uri, store=store_name):
                asset_manager.add_file("toolbitshapepng", path, asset_id=path.stem + ".png")


def ensure_assets_initialized(asset_manager: AssetManager, store="local"):
    """
    Ensures the given store is initialized with built-in assets.
    """
    ensure_library_assets_initialized(asset_manager, store)
    ensure_toolbit_assets_initialized(asset_manager, store)
    ensure_toolbitshape_assets_initialized(asset_manager, store)


def _on_asset_path_changed(group, key, value):
    Path.Log.info(f"CAM asset directory changed in preferences: {group} {key} {value}")
    user_asset_store.set_dir(Preferences.getAssetPath())
    ensure_assets_initialized(cam_assets)


# Set up the local CAM asset storage.
asset_mapping = {
    "toolbitlibrary": "Library/{asset_id}.fctl",
    "toolbit": "Bit/{asset_id}.fctb",
    "toolbitshape": "Shape/{asset_id}.fcstd",
    "toolbitshapesvg": "Shape/{asset_id}",  # Asset ID has ".svg" included
    "toolbitshapepng": "Shape/{asset_id}",  # Asset ID has ".png" included
    "machine": "Machine/{asset_id}.fcm",
}

user_asset_store = FileStore(
    name="local",
    base_dir=Preferences.getAssetPath(),
    mapping=asset_mapping,
)

builtin_asset_store = FileStore(
    name="builtin",
    base_dir=Preferences.getBuiltinAssetPath(),
    mapping=asset_mapping,
)

class CamAssetManager(AssetManager):
    """
    Custom CAM Asset Manager that extends the base AssetManager, such
    that the get methods return fallbacks: if the asset is not present
    in the "local" store, then it falls back to the builtin-asset store.
    """
    def __init__(self):
        super().__init__()
        self.register_store(user_asset_store)
        self.register_store(builtin_asset_store)

    def get(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = ("local", "builtin"),
        depth: Optional[int] = None,
    ) -> Asset:
        """
        Gets an asset from the "local" store, falling back to the "builtin"
        store if not found locally.
        """
        return super().get(uri, store=store, depth=depth)


# Set up the CAM asset manager.
Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
cam_assets = CamAssetManager()
try:
    ensure_assets_initialized(cam_assets)
except Exception as e:
    Path.Log.error(f"Failed to initialize CAM assets in {user_asset_store._base_dir}: {e}")
else:
    Path.Log.debug(f"Using CAM assets in {user_asset_store._base_dir}")
addToolPreferenceObserver(_on_asset_path_changed)
