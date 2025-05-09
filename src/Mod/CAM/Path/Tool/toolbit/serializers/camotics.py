import json
from typing import Optional
import FreeCAD
import Path
from ...assets import asset_manager
from ...shape import ToolBitShape
from ..mixins import RotaryToolBitMixin
from ..models.base import ToolBit
from .base import ToolBitSerializer

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


class CamoticsToolBitSerializer(ToolBitSerializer):
    NAME = "Camotics"
    EXTENSIONS = ()  # Camotics does not have tool files; tools are rows in tool tables

    def serialize_toolbit(self, bit: ToolBit) -> bytes:
        if not isinstance(bit, RotaryToolBitMixin):
            lbl = bit.get_label()
            name = bit.get_shape_name()
            Path.Log.info(
                f"Skipping export of toolbit {lbl} ({name}) because it is not a rotary tool."
            )
        toolitem = tooltemplate.copy()
        toolitem["diameter"] = bit.get_diameter().Value or 2
        toolitem["description"] = bit.get_label()
        toolitem["length"] = bit.get_length().Value or 10
        toolitem["shape"] = SHAPEMAP.get(bit.get_shape_name(), "Cylindrical")
        return json.dumps(toolitem).encode("ascii", "ignore")

    def deserialize_toolbit(self, data: bytes) -> Optional[ToolBit]:
        # Create an instance of the ToolBitShape class
        data = json.loads(data.decode("ascii", "ignore"))
        uri = ToolBitShape.resolve_name("endmill")
        shape = asset_manager.get(uri, store='shapestore')

        # Create an instance of the ToolBit class
        registry = Path.Tool.toolbit.TOOLBIT_REGISTRY
        bit_class = registry.get_bit_class_from_shape_name(shape.get_id())

        bit = bit_class(shape)
        bit.set_label(data["description"])
        bit.set_diameter(FreeCAD.Units.Quantity(float(data["diameter"]), "mm"))
        bit.set_length(FreeCAD.Units.Quantity(float(data["length"]), "mm"))
        return bit
