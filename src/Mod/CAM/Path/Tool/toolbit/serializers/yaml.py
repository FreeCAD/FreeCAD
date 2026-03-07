# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
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
        """Deep deserialize preserving the original toolbit ID."""
        data_dict = yaml.safe_load(data)
        if not isinstance(data_dict, dict):
            raise ValueError("Invalid YAML data for ToolBit")

        original_id = data_dict.get("id")  # Extract the original ID
        toolbit = ToolBit.from_dict(data_dict)
        if original_id:
            toolbit.id = original_id  # Preserve the original ID
        return toolbit
