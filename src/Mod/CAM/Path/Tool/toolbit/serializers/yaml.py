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
from ..util import format_value


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
        try:
            data_dict = yaml.safe_load(data)
            if isinstance(data_dict, dict):
                shape_id = data_dict.get("shape")
                if shape_id:
                    # Assuming shape is identified by its ID/name
                    return [ToolBitShape.resolve_name(str(shape_id))]
        except yaml.YAMLError:
            pass
        return []

    @classmethod
    def serialize(cls, asset: ToolBit) -> bytes:
        """Serializes a ToolBit instance to bytes (shallow)."""
        # Shallow serialization: only serialize direct attributes and shape ID
        data = {
            "id": asset.id,
            "name": asset.label,
            "shape": asset.get_shape_name(),  # Serialize shape ID/name
            "shape-type": asset.obj.ShapeType,  # Include shape type for deserialization
            "parameter": {},
            "attribute": {},
        }

        # Include parameters and attributes from the FreeCAD object
        for prop_name in asset.obj.PropertiesList:
            group = asset.obj.getGroupOfProperty(prop_name)
            if group == "Shape":
                # Parameters are part of the shape schema
                data["parameter"][prop_name] = format_value(asset.get_property(prop_name))
            elif group == "Attributes":
                # Attributes are general toolbit properties
                data["attribute"][prop_name] = format_value(asset.get_property(prop_name))

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

        # Ensure required keys are present
        if "shape" not in data_dict:
            raise ValueError("YAML data is missing attribute 'shape'")
        if "shape-type" not in data_dict:
            raise ValueError("YAML data is missing attribute 'shape-type'")

        # Deserialize shallowly: use the provided dependencies or create a dummy
        tool_bit_shape = None
        shape_id = str(data_dict["shape"])
        shape_uri = ToolBitShape.resolve_name(shape_id)

        if dependencies and shape_uri in dependencies:
            tool_bit_shape = dependencies[shape_uri]
        else:
            # If dependency not provided, create a dummy shape instance
            shape_type = data_dict["shape-type"]
            shape_class = ToolBitShape.get_subclass_by_name(shape_type or shape_id)
            if not shape_class:
                raise ValueError(f"Unknown tool shape type: {shape_type or shape_id}")
            tool_bit_shape = shape_class(shape_id)

        # Create the ToolBit instance using from_shape
        # Pass the full data_dict to from_shape to handle parameters/attributes
        id = id or data_dict.get("id")
        return ToolBit.from_shape(tool_bit_shape, data_dict, id=id)

    @classmethod
    def deep_deserialize(cls, data: bytes) -> ToolBit:
        """
        Like deserialize(), but builds dependencies itself if they are
        sufficiently defined in the data.
        """
        raise NotImplementedError
