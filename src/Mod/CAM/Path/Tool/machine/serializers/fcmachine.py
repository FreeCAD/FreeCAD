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
from ..models import Machine, Lathe, Mill, LinearAxis, AngularAxis


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

        # Serialize common base attributes.
        attrs = {
            "id": asset.get_id(),
            "label": asset.label,
            "max_feed": asset.max_feed.UserString,
            "post_processor": asset.post_processor,
            "post_processor_args": asset.post_processor_args,
            "spindles": [spindle.get_id() for spindle in asset.spindles],
        }

        # Serialize axis parameters.
        axis_data = {}
        for name, axis in asset.axes.items():
            if isinstance(axis, LinearAxis):
                axis_data[name] = {
                    "type": "linear",
                    "start": axis.start.UserString if axis.start is not None else "",
                    "end": axis.end.UserString if axis.end is not None else "",
                    "rigidity": axis.rigidity.UserString + "/N",
                }
            elif isinstance(axis, AngularAxis):
                axis_data[name] = {
                    "type": "angular",
                    "rigidity-x": axis.rigidity_x.UserString + "/N",
                    "rigidity-y": axis.rigidity_y.UserString + "/N",
                }
            else:
                raise TypeError(f"Unknown axis type for {name}")
        attrs["axes"] = axis_data

        # Serialize subclass-specific attributes.
        if isinstance(asset, Lathe):
            attrs["type"] = "Lathe"
            attrs["max_workpiece_diameter"] = asset.max_workpiece_diameter.UserString
        elif isinstance(asset, Mill):
            attrs["type"] = "Mill"
        else:
            raise AttributeError(f"unsupported machine type {asset.__class__}")

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

        axes = {}
        for name, axis_attrs in attrs.get("axes", {}).items():
            axis_type = axis_attrs.get("type", "linear")
            if axis_type == "linear":
                start = axis_attrs.get("start")
                end = axis_attrs.get("end")
                start = FreeCAD.Units.Quantity(start) if start else None
                end = FreeCAD.Units.Quantity(end) if end else None
                rigidity_str = axis_attrs.get("rigidity", "0.001 mm/N")
                if rigidity_str.endswith("/N"):
                    rigidity_str = rigidity_str[:-2]
                rigidity = FreeCAD.Units.Quantity(rigidity_str)
                axes[name] = LinearAxis(start=start, end=end, rigidity=rigidity)
            elif axis_type == "angular":
                rigidity_x_str = axis_attrs.get("rigidity-x", "0.001 °/N")
                rigidity_y_str = axis_attrs.get("rigidity-y", "0.001 °/N")
                if rigidity_x_str.endswith("/N"):
                    rigidity_x_str = rigidity_x_str[:-2]
                if rigidity_y_str.endswith("/N"):
                    rigidity_y_str = rigidity_y_str[:-2]
                rigidity_x = FreeCAD.Units.Quantity(rigidity_x_str)
                rigidity_y = FreeCAD.Units.Quantity(rigidity_y_str)
                print(f"Creating AngularAxis with rigidity_x={rigidity_x}, rigidity_y={rigidity_y}")
                axes[name] = AngularAxis(rigidity_x=rigidity_x, rigidity_y=rigidity_y)
            else:
                raise AttributeError(f"Unknown axis type: {axis_type}")

        if machine_type == "Lathe":
            max_workpiece_diameter = FreeCAD.Units.Quantity(attrs["max_workpiece_diameter"])
            instance = Lathe(
                label,
                axes=axes,
                max_feed=max_feed,
                max_workpiece_diameter=max_workpiece_diameter,
                post_processor=post_processor,
                post_processor_args=post_processor_args,
                id=id,
            )
        elif machine_type == "Mill":
            instance = Mill(
                label,
                axes=axes,
                max_feed=max_feed,
                post_processor=post_processor,
                post_processor_args=post_processor_args,
                id=id,
            )
        else:
            raise AttributeError(f"unsupported machine type {machine_type}")

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
