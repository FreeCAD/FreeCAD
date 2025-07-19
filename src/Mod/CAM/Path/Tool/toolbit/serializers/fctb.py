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
import Path
from typing import Mapping, List, Optional, cast
import FreeCAD
from ...assets import Asset, AssetUri, AssetSerializer
from ...shape import ToolBitShape
from ..models.base import ToolBit


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class FCTBSerializer(AssetSerializer):
    for_class = ToolBit
    mime_type = "application/x-freecad-toolbit"
    extensions = (".fctb",)

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "FreeCAD Tool")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        Path.Log.debug(f"FCTBSerializer.extract_dependencies: raw data = {data!r}")
        data_dict = json.loads(data.decode("utf-8"))
        shape = data_dict["shape"]
        return [ToolBitShape.resolve_name(shape)]

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        # Ensure the asset is a ToolBit instance before serializing
        if not isinstance(asset, ToolBit):
            raise TypeError(f"Expected ToolBit instance, got {type(asset).__name__}")
        attrs = asset.to_dict()
        return json.dumps(attrs, sort_keys=True, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> ToolBit:
        """
        Creates a ToolBit instance from serialized data and resolved
        dependencies.
        """
        attrs = json.loads(data.decode("utf-8", "ignore"))
        attrs["id"] = id  # Ensure id is available for from_dict

        if dependencies is None:
            # Shallow load: dependencies are not resolved.
            # Delegate to from_dict with shallow=True.
            Path.Log.debug(f"FCTBSerializer.deserialize: shallow. id = {id!r}, attrs = {attrs!r}")
            return ToolBit.from_dict(attrs, shallow=True)

        # Full load: dependencies are resolved.
        # Proceed with existing logic to use the resolved shape.
        shape_id = attrs.get("shape")
        if not shape_id:
            Path.Log.warning("ToolBit data is missing 'shape' key, defaulting to 'endmill'")
            shape_id = "endmill"

        shape_uri = ToolBitShape.resolve_name(shape_id)
        shape = dependencies.get(shape_uri)

        if shape is None:
            raise ValueError(
                f"Dependency for shape '{shape_id}' not found by uri {shape_uri}" f" {dependencies}"
            )
        elif not isinstance(shape, ToolBitShape):
            raise ValueError(
                f"Dependency for shape '{shape_id}' found by uri {shape_uri} "
                f"is not a ToolBitShape instance. {dependencies}"
            )

        # Find the correct ToolBit subclass for the shape
        Path.Log.debug(
            f"FCTBSerializer.deserialize: shape = {shape!r}, id = {id!r},"
            f" params = {shape.get_parameters()}, attrs = {attrs!r}"
        )
        return ToolBit.from_shape(shape, attrs, id)

    @classmethod
    def deep_deserialize(cls, data: bytes) -> ToolBit:
        attrs_map = json.loads(data)
        asset_class = cast(ToolBit, cls.for_class)
        return asset_class.from_dict(attrs_map)
