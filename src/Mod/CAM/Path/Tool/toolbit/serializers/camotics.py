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
    extensions = ()  # Camotics does not have tool files; tools are rows in tool tables
    mime_type = "application/json"

    @classmethod
    def extract_dependencies(cls, data: bytes) -> list[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        if not isinstance(asset, RotaryToolBitMixin):
            lbl = asset.get_label()
            name = asset.get_shape_name()
            Path.Log.info(
                f"Skipping export of toolbit {lbl} ({name}) because it is not a rotary tool."
            )
        toolitem = tooltemplate.copy()
        toolitem["diameter"] = asset.get_diameter().Value or 2
        toolitem["description"] = asset.get_label()
        toolitem["length"] = asset.get_length().Value or 10
        toolitem["shape"] = SHAPEMAP.get(asset.get_shape_name(), "Cylindrical")
        return json.dumps(toolitem).encode("ascii", "ignore")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Asset:
        # Create an instance of the ToolBitShape class
        data = json.loads(data.decode("ascii", "ignore"))
        shape = cam_assets.get("toolbitshape://endmill")

        # Create an instance of the ToolBit class
        bit = ToolBit.from_shape_id(shape.get_id())
        bit.set_label(data["description"])
        bit.set_diameter(FreeCAD.Units.Quantity(float(data["diameter"]), "mm"))
        bit.set_length(FreeCAD.Units.Quantity(float(data["length"]), "mm"))
        return bit
