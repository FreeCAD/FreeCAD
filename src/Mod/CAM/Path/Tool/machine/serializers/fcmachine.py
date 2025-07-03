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
from typing import Mapping, Optional, cast
import FreeCAD
import yaml
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
    def serialize(cls, asset: Asset) -> bytes:
        """Serializes a Machine, Lathe, or Mill object into bytes."""
        if not isinstance(asset, Machine):
            raise TypeError(f"Expected Machine instance, got {type(asset).__name__}")

        attrs = asset.to_dict()

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
        machine_type = attrs.get("type")
        if machine_type == "Lathe":
            instance = cast(Lathe, Lathe.from_dict(attrs))
        elif machine_type == "Mill":
            instance = cast(Mill, Mill.from_dict(attrs))
        else:
            raise ValueError(f"Unknown machine type: {machine_type}")
        return instance

    @classmethod
    def deep_deserialize(cls, data: bytes) -> Asset:
        """
        deep_deserialize is not implemented as it requires a mechanism
        to recursively load and resolve dependencies, which is typically
        handled by an asset manager.
        """
        raise NotImplementedError
