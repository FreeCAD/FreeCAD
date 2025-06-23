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
import json
import pathlib
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


def ensure_toolbits_have_shape_type(asset_manager: AssetManager, store_name: str = "local"):
    from .shape import ToolBitShape

    toolbit_uris = asset_manager.list_assets(
        asset_type="toolbit",
        store=store_name,
    )

    for uri in toolbit_uris:
        data = asset_manager.get_raw(uri, store=store_name)
        attrs = json.loads(data)
        if "shape-type" in attrs:
            continue

        shape_id = pathlib.Path(
            str(attrs.get("shape", ""))
        ).stem  # backward compatibility. used to be a filename
        if not shape_id:
            Path.Log.error(f"ToolBit {uri} missing shape ID")
            continue

        try:
            shape_class = ToolBitShape.get_shape_class_from_id(shape_id)
        except Exception as e:
            Path.Log.error(f"Failed to load toolbit {uri}: {e}. Skipping")
            continue
        if not shape_class:
            Path.Log.error(f"Toolbit {uri} has no shape-type attribute, and failed to infer it")
            continue
        attrs["shape-type"] = shape_class.name
        Path.Log.info(f"Migrating toolbit {uri}: Adding shape-type attribute '{shape_class.name}'")
        data = json.dumps(attrs, sort_keys=True, indent=2).encode("utf-8")
        asset_manager.add_raw("toolbit", uri.asset_id, data, store=store_name)


def ensure_toolbit_assets_initialized(asset_manager: AssetManager, store_name: str = "local"):
    """
    Ensures the given store is initialized with built-in bits
    if it is currently empty.
    """
    builtin_toolbit_path = Preferences.getBuiltinToolBitPath()

    if asset_manager.is_empty("toolbit", store=store_name):
        for path in builtin_toolbit_path.glob("*.fctb"):
            asset_manager.add_file("toolbit", path)

    ensure_toolbits_have_shape_type(asset_manager, store_name)


def ensure_toolbitshape_assets_present(asset_manager: AssetManager, store_name: str = "local"):
    """
    Ensures the given store is initialized with built-in shapes
    if it is currently empty. This copies all built-in shapes,
    which is generally not recommended, but is useful for
    testing.

    In practice, the built-in tools don't need to be copied,
    because the CamAssetManager will automatically fall back to
    fetching them from the builtin store if they are not
    present in the local store (=the user's Shape directory).
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

        for path in builtin_shape_path.glob("*.svg"):
            uri = AssetUri.build(
                asset_type="toolbitshapesvg",
                asset_id=path.stem + ".svg",
            )
            if not asset_manager.exists(uri, store=store_name):
                asset_manager.add_file("toolbitshapesvg", path, asset_id=path.stem + ".svg")

        for path in builtin_shape_path.glob("*.png"):
            uri = AssetUri.build(
                asset_type="toolbitshapepng",
                asset_id=path.stem + ".png",
            )
            if not asset_manager.exists(uri, store=store_name):
                asset_manager.add_file("toolbitshapepng", path, asset_id=path.stem + ".png")


def ensure_toolbitshape_assets_initialized(asset_manager: AssetManager, store_name: str = "local"):
    """
    Copies an example shape to the given store if it is currently empty.
    """
    builtin_shape_path = Preferences.getBuiltinShapePath()

    if asset_manager.is_empty("toolbitshape", store=store_name):
        path = builtin_shape_path / "endmill.fcstd"
        asset_manager.add_file("toolbitshape", path, store=store_name, asset_id="example")


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

    def setup(self):
        try:
            ensure_assets_initialized(cam_assets)
        except Exception as e:
            Path.Log.error(f"Failed to initialize CAM assets in {user_asset_store._base_dir}: {e}")
        else:
            Path.Log.debug(f"Using CAM assets in {user_asset_store._base_dir}")

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

    def get_or_none(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = ("local", "builtin"),
        depth: Optional[int] = None,
    ) -> Optional[Asset]:
        """
        Gets an asset from the "local" store, falling back to the "builtin"
        store if not found locally.
        """
        return super().get_or_none(uri, store=store, depth=depth)


# Set up the CAM asset manager.
Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
cam_assets = CamAssetManager()
addToolPreferenceObserver(_on_asset_path_changed)
