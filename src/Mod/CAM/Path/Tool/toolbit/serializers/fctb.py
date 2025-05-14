import json
import Path
from typing import Mapping, List, Optional
from ...assets import Asset, AssetUri, AssetSerializer
from ...shape import ToolBitShape
from ..models.base import ToolBit
from Path.Base import Util as PathUtil


class FCTBSerializer(AssetSerializer):
    for_class = ToolBit
    mime_type = "application/x-freecad-toolbit"
    extensions = ('.fctb',)

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        Path.Log.info(f"FCTBSerializer.extract_dependencies: raw data = {data!r}")
        data_dict = json.loads(data.decode('utf-8'))
        shape = data_dict["shape"]
        return [ToolBitShape.resolve_name(shape)]

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        # Ensure the asset is a ToolBit instance before serializing
        if not isinstance(asset, ToolBit):
            raise TypeError(f"Expected ToolBit instance, got {type(asset).__name__}")
        attrs = asset.to_dict()
        return json.dumps(attrs, sort_keys=True, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Asset:
        """
        Creates a ToolBit instance from serialized data and resolved
        dependencies.
        """
        attrs = json.loads(data.decode("utf-8", "ignore"))
        attrs['id'] = id # Ensure id is available for from_dict

        if dependencies is None:
            # Shallow load: dependencies are not resolved.
            # Delegate to from_dict with shallow=True.
            return ToolBit.from_dict(attrs, shallow=True)

        # Full load: dependencies are resolved.
        # Proceed with existing logic to use the resolved shape.
        shape_id = attrs.get("shape")
        if not shape_id:
            Path.Log.warning("ToolBit data is missing 'shape' key, defaulting to 'endmill'")
            shape_id = 'endmill'

        shape_uri = ToolBitShape.resolve_name(shape_id)
        shape = dependencies.get(shape_uri)

        if shape is None:
            raise ValueError(
                f"Dependency for shape '{shape_id}' not found by uri {shape_uri}"
                f" {dependencies}"
            )
        elif not isinstance(shape, ToolBitShape):
            raise ValueError(
                f"Dependency for shape '{shape_id}' found by uri {shape_uri} "
                f"is not a ToolBitShape instance. {dependencies}"
            )

        # Find the correct ToolBit subclass for the shape
        selected_toolbit_subclass = ToolBit._find_subclass_for_shape(shape)

        # Create the tool bit instance
        toolbit = selected_toolbit_subclass(shape, id=id)

        # Populate properties from the data dictionary
        toolbit.set_label(attrs.get("name") or shape.label)

        for param_name, param_value in attrs.get("parameter", {}).items():
            if hasattr(toolbit.obj, param_name):
                PathUtil.setProperty(toolbit.obj, param_name, param_value)
            else:
                Path.Log.warning(
                    f"Parameter '{param_name}' not found on tool bit "
                    f"'{toolbit.obj.Label}'. Skipping."
                )

        for attr_name, attr_value in attrs.get("attribute", {}).items():
            if hasattr(toolbit.obj, attr_name):
                PathUtil.setProperty(toolbit.obj, attr_name, attr_value)
            else:
                Path.Log.warning(
                    f"Attribute '{attr_name}' not found on tool bit "
                    f"'{toolbit.obj.Label}'. Skipping."
                )

        return toolbit
