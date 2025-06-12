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
from typing import Mapping, Optional
import FreeCAD
from ...assets import Asset, AssetUri, AssetSerializer
from ...spindle import Spindle


class SpindleSerializer(AssetSerializer):
    """
    Serializes and deserializes Spindle objects.
    """

    for_class = Spindle
    mime_type = "application/x-freecad-spindle"
    extensions = (".fcspindle",)

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "FreeCAD Spindle")

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        """Serializes a Spindle object into bytes."""
        if not isinstance(asset, Spindle):
            raise TypeError(f"Expected Spindle instance, got {type(asset).__name__}")

        attrs = {
            "id": asset.id,
            "label": asset.label,
            "min_rpm": asset.min_rpm,
            "max_rpm": asset.max_rpm,
            "max_power": asset.max_power.UserString,
            "max_torque": asset.max_torque,
            "peak_torque_rpm": asset.peak_torque_rpm,
        }

        return yaml.dump(attrs, sort_keys=True, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Spindle:
        """Creates a Spindle object from serialized data."""
        attrs = yaml.safe_load(data.decode("utf-8", "ignore"))

        min_rpm = attrs["min_rpm"]
        max_rpm = attrs["max_rpm"]
        max_power = FreeCAD.Units.Quantity(attrs["max_power"])
        max_torque = attrs["max_torque"]
        peak_torque_rpm = attrs["peak_torque_rpm"]

        instance = Spindle(
            id=attrs["id"],
            label=attrs["label"],
            min_rpm=min_rpm,
            max_rpm=max_rpm,
            max_power=max_power,
            max_torque=max_torque,
            peak_torque_rpm=peak_torque_rpm,
        )
        return instance
