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
import yaml
from typing import List, Optional, Mapping, Type
from ...assets.serializer import AssetSerializer
from ...assets.uri import AssetUri
from ...shape import ToolBitShape
from ..models.base import ToolBit


class YamlToolBitSerializer(AssetSerializer):
    """
    Serializes and deserializes ToolBit instances to and from YAML.
    """

    for_class: Type[ToolBit] = ToolBit
    extensions: tuple[str, ...] = (".yaml", ".yml")
    mime_type: str = "application/x-yaml"
    can_import: bool = True
    can_export: bool = True

    @classmethod
    def get_label(cls) -> str:
        return "YAML ToolBit"

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        data_dict = yaml.safe_load(data)
        if isinstance(data_dict, dict):
            shape_id = data_dict.get("shape")
            if shape_id:
                # Assuming shape is identified by its ID/name
                return [ToolBitShape.resolve_name(str(shape_id))]
        return []

    @classmethod
    def serialize(cls, asset: ToolBit) -> bytes:
        """Serializes a ToolBit instance to bytes (shallow)."""
        # Shallow serialization: only serialize direct attributes and shape ID
        data = asset.to_dict()
        return yaml.dump(data, default_flow_style=False).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str | None = None,
        dependencies: Optional[Mapping[AssetUri, ToolBitShape]] = None,
    ) -> ToolBit:
        """
        Creates a ToolBit instance from serialized data and resolved
        dependencies (shallow).
        """
        data_dict = yaml.safe_load(data)
        if not isinstance(data_dict, dict):
            raise ValueError("Invalid YAML data for ToolBit")
        toolbit = ToolBit.from_dict(data_dict)
        if id:
            toolbit.id = id
        return toolbit

    @classmethod
    def deep_deserialize(cls, data: bytes) -> ToolBit:
        """
        Like deserialize(), but builds dependencies itself if they are
        sufficiently defined in the data.
        """
        raise NotImplementedError
