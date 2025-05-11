import json
import Path
from ...camassets import cam_assets
from ..models.base import ToolBit
from .base import ToolBitSerializer
from Path.Base import Util as PathUtil


class FCTBSerializer(ToolBitSerializer):
    NAME = "FreeCAD"
    EXTENSIONS = ('.fctb',)

    def serialize_toolbit(self, bit: ToolBit) -> bytes:
        attrs = bit.to_dict()
        return json.dumps(attrs, sort_keys=True, indent=2).encode("utf-8")

    def deserialize_toolbit(self, data: bytes) -> ToolBit:
        # Create the ToolBitShape instance. This loads default parameters
        # from the shape file.
        attrs = json.loads(data.decode("utf-8", "ignore"))
        shape = cam_assets.get("toolbitshape://endmill")

        # Update shape parameters from the JSON 'parameter' dictionary.
        shape.set_parameters(**attrs.get('parameter', {}))

        # Populate attributes from the JSON data
        tool_bit = ToolBit.from_shape_id(shape.get_id())
        tool_bit.set_label(attrs.get('name', shape.name))

        # Set parameters from the JSON 'parameter' dictionary using PathUtil.setProperty
        for param_name, param_value in attrs.get("parameter", {}).items():
            if hasattr(tool_bit.obj, param_name):
                PathUtil.setProperty(
                    tool_bit.obj, param_name, param_value
                )
            else:
                Path.Log.warning(
                    f"Parameter '{param_name}' not found on tool bit "
                    f"'{tool_bit.obj.Label}'. Skipping."
                )

        # Set additional attributes from the JSON 'attribute' dictionary.
        # These are stored as properties on the ToolBit's obj.
        for key, value in attrs.get('attribute', {}).items():
            # Check if the property exists on the object before setting.
            if not hasattr(tool_bit.obj, key):
                Path.Log.warning(
                    f"Attribute '{key}' not found on tool bit "
                    f"'{tool_bit.obj.Label}'. Skipping."
                )
                continue

            try:
                setattr(tool_bit.obj, key, value)
            except Exception as e:
                Path.Log.warning(
                    f"Failed to set attribute '{key}' with value '{value}' "
                    f"on tool bit '{tool_bit.obj.Label}': {e}"
                )

        return tool_bit
