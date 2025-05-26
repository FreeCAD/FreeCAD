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
import uuid
import json
from typing import Mapping, List, Optional, Type
import FreeCAD
from ...assets import Asset, AssetUri, AssetSerializer
from ...toolbit import ToolBit
from ...toolbit.mixins import RotaryToolBitMixin
from ...shape import ToolBitShape, ToolBitShapeEndmill
from ..models.library import Library

SHAPEMAP = {
    "ballend": "Ballnose",
    "endmill": "Cylindrical",
    "v-bit": "Conical",
    "vbit": "Conical",
    "chamfer": "Snubnose",
}
SHAPEMAP_REVERSE = dict((v, k) for k, v in SHAPEMAP.items())

tooltemplate = {
    "units": "metric",
    "shape": "Cylindrical",
    "length": 10,
    "diameter": 3.125,
    "description": "",
}


class CamoticsLibrarySerializer(AssetSerializer):
    for_class: Type[Asset] = Library
    extensions: tuple[str] = (".json",)
    mime_type: str = "application/json"

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "Camotics Tool Library")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        if not isinstance(asset, Library):
            raise TypeError("Asset must be a Library instance")

        toollist = {}
        for tool_no, tool in asset._bit_nos.items():
            assert isinstance(tool, RotaryToolBitMixin)
            toolitem = tooltemplate.copy()

            diameter_value = tool.get_diameter()
            # Ensure diameter is a float, handle Quantity and other types
            diameter_serializable = 2.0  # Default value as float
            if isinstance(diameter_value, FreeCAD.Units.Quantity):
                try:
                    val_mm = diameter_value.getValueAs("mm")
                    if val_mm is not None:
                        diameter_serializable = float(val_mm)
                except ValueError:
                    # Fallback to raw value if unit conversion fails
                    raw_val = diameter_value.Value if hasattr(diameter_value, "Value") else None
                    if isinstance(raw_val, (int, float)):
                        diameter_serializable = float(raw_val)
            elif isinstance(diameter_value, (int, float)):
                diameter_serializable = float(diameter_value) if diameter_value is not None else 2.0

            toolitem["diameter"] = diameter_serializable

            toolitem["description"] = tool.label

            length_value = tool.get_length()
            # Ensure length is a float, handle Quantity and other types
            length_serializable = 10.0  # Default value as float
            if isinstance(length_value, FreeCAD.Units.Quantity):
                try:
                    val_mm = length_value.getValueAs("mm")
                    if val_mm is not None:
                        length_serializable = float(val_mm)
                except ValueError:
                    # Fallback to raw value if unit conversion fails
                    raw_val = length_value.Value if hasattr(length_value, "Value") else None
                    if isinstance(raw_val, (int, float)):
                        length_serializable = float(raw_val)
            elif isinstance(length_value, (int, float)):
                length_serializable = float(length_value) if length_value is not None else 10.0

            toolitem["length"] = length_serializable

            toolitem["shape"] = SHAPEMAP.get(tool._tool_bit_shape.name.lower(), "Cylindrical")
            toollist[str(tool_no)] = toolitem

        return json.dumps(toollist, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Library:
        try:
            data_dict = json.loads(data.decode("utf-8"))
        except json.JSONDecodeError as e:
            raise ValueError(f"Failed to decode JSON data: {e}") from e

        library = Library(id, id=id)
        for tool_no_str, toolitem in data_dict.items():
            try:
                tool_no = int(tool_no_str)
            except ValueError:
                print(f"Warning: Skipping invalid tool number: {tool_no_str}")
                continue

            # Find the shape class to use
            shape_name_str = SHAPEMAP_REVERSE.get(toolitem.get("shape", "Cylindrical"), "endmill")
            shape_class = ToolBitShape.get_subclass_by_name(shape_name_str)
            if not shape_class:
                print(f"Warning: Unknown shape name '{shape_name_str}', defaulting to endmill")
                shape_class = ToolBitShapeEndmill

            # Translate parameters to FreeCAD types
            params = {}
            try:
                diameter = float(toolitem.get("diameter", 2))
                params["Diameter"] = FreeCAD.Units.Quantity(f"{diameter} mm")
            except (ValueError, TypeError):
                print(f"Warning: Invalid diameter for tool {tool_no_str}, skipping.")

            try:
                length = float(toolitem.get("length", 10))
                params["Length"] = FreeCAD.Units.Quantity(f"{length} mm")
            except (ValueError, TypeError):
                print(f"Warning: Invalid length for tool {tool_no_str}, skipping.")

            # Create the shape
            shape_id = shape_name_str.lower()
            tool_bit_shape = shape_class(shape_id, **params)

            # Create the toolbit
            tool = ToolBit(tool_bit_shape, id=f"camotics_tool_{tool_no_str}")
            tool.label = toolitem.get("description", "")

            library.add_bit(tool, tool_no)

        return library

    @classmethod
    def deep_deserialize(cls, data: bytes) -> Library:
        # TODO: Build tools here
        return cls.deserialize(data, str(uuid.uuid4()), {})
