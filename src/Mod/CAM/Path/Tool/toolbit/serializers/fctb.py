import json
import Path
from ...assets import asset_manager
from ...shape import ToolBitShape
from ..models.base import ToolBit
from .base import ToolBitSerializer


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
        shape_filename = attrs.get('shape', 'endmill.fcstd')
        uri = ToolBitShape.resolve_name(shape_filename)
        shape = asset_manager.get(uri, store='shapestore')

        # Update shape parameters from the JSON 'parameter' dictionary.
        shape.set_parameters(**attrs.get('parameter', {}))

        # Create an instance of the correct ToolBit subclass based on shape.
        registry = Path.Tool.toolbit.TOOLBIT_REGISTRY
        tool_bit_class = registry.get_bit_class_from_shape_name(shape.name)
        tool_bit = tool_bit_class(shape)

        # Populate attributes from the JSON data
        tool_bit.set_label(attrs.get('name', shape.name))

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
