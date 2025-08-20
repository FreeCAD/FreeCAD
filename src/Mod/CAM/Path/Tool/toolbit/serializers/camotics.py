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
from typing import Optional, Mapping
import FreeCAD
import Path
from ...camassets import cam_assets
from ..mixins import RotaryToolBitMixin
from ..models.base import ToolBit
from ...assets.serializer import AssetSerializer
from ...assets.uri import AssetUri
from ...assets.asset import Asset

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
    "diameter": 3,
    "description": "",
}


class CamoticsToolBitSerializer(AssetSerializer):
    for_class = ToolBit
    extensions = tuple()  # Camotics does not have tool files; tools are rows in tool tables
    mime_type = "application/json"
    can_import = False
    can_export = False

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "Camotics Tool")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> list[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        assert isinstance(asset, ToolBit)
        if not isinstance(asset, RotaryToolBitMixin):
            lbl = asset.label
            name = asset.get_shape_name()
            Path.Log.info(
                f"Skipping export of toolbit {lbl} ({name}) because it is not a rotary tool."
            )
            return b"{}"
        toolitem = tooltemplate.copy()
        toolitem["diameter"] = asset.get_diameter().Value or 2
        toolitem["description"] = asset.label
        toolitem["length"] = asset.get_length().Value or 10
        toolitem["shape"] = SHAPEMAP.get(asset.get_shape_name(), "Cylindrical")
        return json.dumps(toolitem).encode("ascii", "ignore")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> ToolBit:
        # Create an instance of the ToolBitShape class
        attrs: dict = json.loads(data.decode("ascii", "ignore"))
        shape = cam_assets.get("toolbitshape://endmill")

        # Create an instance of the ToolBit class
        bit = ToolBit.from_shape_id(shape.get_id())
        bit.label = attrs["description"]

        if not isinstance(bit, RotaryToolBitMixin):
            raise NotImplementedError(
                f"Only export of rotary tools is supported ({bit.label} ({bit.id})"
            )

        bit.set_diameter(FreeCAD.Units.Quantity(float(attrs["diameter"]), "mm"))
        bit.set_length(FreeCAD.Units.Quantity(float(attrs["length"]), "mm"))
        return bit
