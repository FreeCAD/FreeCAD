import json
from typing import Mapping, List, Optional
import pathlib
import Path
from ...assets import Asset, AssetUri, AssetSerializer
from ...toolbit import ToolBit
from ..models.library import Library


class FCTLSerializer(AssetSerializer):
    for_class = Library
    extensions = (".fctl",)
    mime_type = "application/x-freecad-toolbit-library"

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        data_dict = json.loads(data.decode('utf-8'))
        tools_list = data_dict.get("tools", [])
        tool_ids = [pathlib.Path(tool["path"]).stem for tool in tools_list]
        return [AssetUri(f"toolbit://{tool_id}") for tool_id in tool_ids]

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        """Serializes a Library object into bytes."""
        if not isinstance(asset, Library):
            raise TypeError(f"Expected Library instance, got {type(asset).__name__}")
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
        Creates a Library instance from serialized data and resolved
        dependencies.
        """
        data_dict = json.loads(data.decode('utf-8'))
        # The id parameter from the Asset.from_bytes method is the canonical ID
        # for the asset being deserialized. We should use this ID for the library
        # instance, overriding any 'id' that might be in the data_dict (which
        # is from an older version of the format).
        library = Library(data_dict.get("label", id or "Unnamed Library"), id=id)

        if dependencies is None:
            Path.Log.debug(f"FCTLSerializer.deserialize: Shallow load for library '{library.label}' (id: {id}). Tools not populated.")
            return library  # Only process tools if dependencies were resolved

        tools_list = data_dict.get("tools", [])
        for tool_data in tools_list:
            tool_no = tool_data["nr"]
            tool_id = pathlib.Path(tool_data["path"]).stem  # Extract tool ID
            tool_uri = AssetUri(f"toolbit://{tool_id}")
            tool = dependencies.get(tool_uri)
            if tool:
                # Ensure the dependency is a ToolBit instance
                if not isinstance(tool, ToolBit):
                     Path.Log.warning(f"Dependency for tool '{tool_id}' is not a ToolBit instance. Skipping.")
                     continue
                library.add_tool(tool, tool_no=tool_no)
            else:
                # This should not happen if dependencies were resolved correctly,
                # but as a safeguard, log a warning and skip the tool.
                Path.Log.warning(f"Tool with id {tool_id} not found in dependencies during deserialization.")
        return library
