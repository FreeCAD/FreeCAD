# -*- coding: utf-8 -*-
# ***************************************************************************
# * Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# * *
# * This program is free software; you can redistribute it and/or modify  *
# * it under the terms of the GNU Lesser General Public License (LGPL)    *
# * as published by the Free Software Foundation; either version 2 of     *
# * the License, or (at your option) any later version.                   *
# * for detail see the LICENCE text file.                                 *
# * *
# * This program is distributed in the hope that it will be useful,       *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# * GNU Library General Public License for more details.                  *
# * *
# * You should have received a copy of the GNU Library General Public     *
# * License along with this program; if not, write to the Free Software   *
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# * USA                                                                   *
# * *
# ***************************************************************************
import yaml
from typing import Mapping, List, Optional
import FreeCAD
import Path
from ...spindle import Spindle
from ...assets import Asset, AssetUri, AssetSerializer
from ..models import Machine, Lathe, Mill


class MachineSerializer(AssetSerializer):
    """
    Serializes and deserializes Machine, Lathe, and Mill objects.
    """

    for_class = Machine
    mime_type = "application/x-freecad-machine"
    extensions = (".fcmachine",)

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "FreeCAD Machine")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        data_dict = yaml.safe_load(data.decode("utf-8"))
        spindle_uris = [
            AssetUri(f"spindle://{spindle_id}") for spindle_id in data_dict.get("spindles", [])
        ]
        return spindle_uris

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        """Serializes a Machine, Lathe, or Mill object into bytes."""
        if not isinstance(asset, Machine):
            raise TypeError(f"Expected Machine instance, got {type(asset).__name__}")

        attrs = {
            "id": asset.get_id(),
            "label": asset.label,
            "max_feed": asset.max_feed.UserString,
            "post_processor": asset.post_processor,
            "post_processor_args": asset.post_processor_args,
            "spindles": [spindle.get_id() for spindle in asset.spindles],
        }

        if isinstance(asset, Lathe):
            attrs["type"] = "Lathe"
            attrs["x_extent"] = [asset.x_min.UserString, asset.x_max.UserString]
            attrs["z_extent"] = [asset.z_min.UserString, asset.z_max.UserString]
            attrs["max_workpiece_diameter"] = asset.max_workpiece_diameter.UserString
            rigidity = asset.rigidity
            attrs["rigidity"] = {
                "x": rigidity[0].UserString,
                "z": rigidity[1].UserString,
                "rotational": rigidity[2].UserString,
            }
        elif isinstance(asset, Mill):
            attrs["type"] = "Mill"
            attrs["x_extent"] = [asset.x_min.UserString, asset.x_max.UserString]
            attrs["y_extent"] = [asset.y_min.UserString, asset.y_max.UserString]
            attrs["z_extent"] = [asset.z_min.UserString, asset.z_max.UserString]
            rigidity = asset.rigidity
            attrs["rigidity"] = {
                "x": rigidity[0].UserString,
                "y": rigidity[1].UserString,
                "z": rigidity[2].UserString,
                "rotational": rigidity[3].UserString,
            }
        else:
            attrs["type"] = "Machine"

        return yaml.dump(attrs, sort_keys=True, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Machine:
        """Creates a Machine object from serialized data and resolved dependencies."""
        attrs = yaml.safe_load(data.decode("utf-8", "ignore"))
        attrs["id"] = id

        # Resolve spindle dependencies
        spindles = []
        if dependencies:
            for spindle_id in attrs.get("spindles", []):
                spindle_uri = AssetUri(f"spindle://{spindle_id}")
                spindle = dependencies.get(spindle_uri)
                if spindle and isinstance(spindle, Spindle):
                    spindles.append(spindle)
                else:
                    Path.Log.warning(f"Could not resolve spindle dependency: {spindle_uri}")

        machine_type = attrs.get("type", "Machine")
        label = attrs["label"]
        max_feed = FreeCAD.Units.Quantity(attrs["max_feed"])
        post_processor = attrs.get("post_processor", "")
        post_processor_args = attrs.get("post_processor_args", "")

        if machine_type == "Lathe":
            x_extent = (
                FreeCAD.Units.Quantity(attrs["x_extent"][0]),
                FreeCAD.Units.Quantity(attrs["x_extent"][1]),
            )
            z_extent = (
                FreeCAD.Units.Quantity(attrs["z_extent"][0]),
                FreeCAD.Units.Quantity(attrs["z_extent"][1]),
            )
            max_workpiece_diameter = FreeCAD.Units.Quantity(attrs["max_workpiece_diameter"])
            rigidity = (
                FreeCAD.Units.Quantity(attrs["rigidity"]["x"]),
                FreeCAD.Units.Quantity(attrs["rigidity"]["z"]),
                FreeCAD.Units.Quantity(attrs["rigidity"]["rotational"]),
            )
            instance = Lathe(
                label,
                x_extent,
                z_extent,
                max_feed,
                max_workpiece_diameter,
                rigidity,
                post_processor,
                post_processor_args,
                id,
            )
        elif machine_type == "Mill":
            x_extent = (
                FreeCAD.Units.Quantity(attrs["x_extent"][0]),
                FreeCAD.Units.Quantity(attrs["x_extent"][1]),
            )
            y_extent = (
                FreeCAD.Units.Quantity(attrs["y_extent"][0]),
                FreeCAD.Units.Quantity(attrs["y_extent"][1]),
            )
            z_extent = (
                FreeCAD.Units.Quantity(attrs["z_extent"][0]),
                FreeCAD.Units.Quantity(attrs["z_extent"][1]),
            )
            rigidity = (
                FreeCAD.Units.Quantity(attrs["rigidity"]["x"]),
                FreeCAD.Units.Quantity(attrs["rigidity"]["y"]),
                FreeCAD.Units.Quantity(attrs["rigidity"]["z"]),
                FreeCAD.Units.Quantity(attrs["rigidity"]["rotational"]),
            )
            instance = Mill(
                label,
                x_extent,
                y_extent,
                z_extent,
                max_feed,
                rigidity,
                post_processor,
                post_processor_args,
                id,
            )
        else:
            instance = Machine(label, max_feed, post_processor, post_processor_args, id)

        instance.spindles = spindles
        return instance

    @classmethod
    def deep_deserialize(cls, data: bytes) -> Asset:
        """
        deep_deserialize is not implemented as it requires a mechanism
        to recursively load and resolve dependencies, which is typically
        handled by an asset manager.
        """
        raise NotImplementedError
