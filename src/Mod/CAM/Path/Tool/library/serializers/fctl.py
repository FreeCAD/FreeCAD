# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

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
import uuid
import json
from typing import Mapping, List, Optional
import pathlib
import FreeCAD
import Path
from ...assets import Asset, AssetUri, AssetSerializer
from ...toolbit import ToolBit
from ..models.library import Library


class FCTLSerializer(AssetSerializer):
    for_class = Library
    extensions = (".fctl",)
    mime_type = "application/x-freecad-toolbit-library"

    @classmethod
    def get_label(cls) -> str:
        return FreeCAD.Qt.translate("CAM", "FreeCAD Tool Library")

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        data_dict = json.loads(data.decode("utf-8"))
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
    ) -> Library:
        """
        Creates a Library instance from serialized data and resolved
        dependencies.
        """

        data_dict = json.loads(data.decode("utf-8"))
        # The id parameter from the Asset.from_bytes method is the canonical ID
        # for the asset being deserialized. We should use this ID for the library
        # instance, overriding any 'id' that might be in the data_dict (which
        # is from an older version of the format).

        # For the label, prefer data_dict["label"], then "name", then fallback to using the id as filename
        # The id parameter often contains the filename stem when importing from files
        label = data_dict.get("label") or data_dict.get("name") or id or "Unnamed Library"
        library = Library(label, id=id)

        if dependencies is None:
            Path.Log.debug(
                f"FCTLSerializer.deserialize: Shallow load for library '{library.label}' (id: {id}). Tools not populated."
            )
            return library  # Only process tools if dependencies were resolved

        tools_list = data_dict.get("tools", [])
        for tool_data in tools_list:
            try:
                tool_no = int(tool_data["nr"])
            except ValueError:
                Path.Log.warning(f"Invalid tool ID in tool data: {tool_data}. Skipping.")
                continue
            tool_id = pathlib.Path(tool_data["path"]).stem  # Extract tool ID
            tool_uri = AssetUri(f"toolbit://{tool_id}")
            tool = dependencies.get(tool_uri)
            if tool:
                # Ensure the dependency is a ToolBit instance
                if not isinstance(tool, ToolBit):
                    Path.Log.warning(
                        f"Dependency for tool '{tool_id}' is not a ToolBit instance. Skipping."
                    )
                    continue
                library.add_bit(tool, bit_no=tool_no)
            else:
                # This should not happen if dependencies were resolved correctly,
                # but as a safeguard, log a warning and skip the tool.
                Path.Log.warning(
                    f"Tool with id {tool_id} not found in dependencies during deserialization."
                )
                # Create a placeholder toolbit with the original ID to preserve library structure
                from ...toolbit.models.custom import ToolBitCustom
                from ...shape.models.custom import ToolBitShapeCustom

                placeholder_shape = ToolBitShapeCustom(tool_id)
                placeholder_toolbit = ToolBitCustom(placeholder_shape, id=tool_id)
                placeholder_toolbit.label = f"Missing Tool ({tool_id})"
                library.add_bit(placeholder_toolbit, bit_no=tool_no)
                Path.Log.info(f"Created placeholder toolbit with original ID {tool_id}")
        return library

    @classmethod
    def deep_deserialize(cls, data: bytes) -> Library:
        """Deep deserialize a library by fetching all toolbit dependencies."""
        import uuid
        from ...camassets import cam_assets

        # Generate a unique ID for this library instance
        library_id = str(uuid.uuid4())

        Path.Log.info(
            f"FCTL DEEP_DESERIALIZE: Starting deep deserialization for library id='{library_id}'"
        )

        # Extract dependency URIs from the library data
        dependency_uris = cls.extract_dependencies(data)
        Path.Log.info(
            f"FCTL DEEP_DESERIALIZE: Found {len(dependency_uris)} toolbit dependencies: {[uri.asset_id for uri in dependency_uris]}"
        )

        # Fetch all toolbit dependencies
        resolved_dependencies = {}
        for dep_uri in dependency_uris:
            try:
                Path.Log.info(
                    f"FCTL DEEP_DESERIALIZE: Fetching toolbit '{dep_uri.asset_id}' from stores ['local', 'builtin']"
                )

                # Check if toolbit exists in each store individually for debugging
                exists_local = cam_assets.exists(dep_uri, store="local")
                exists_builtin = cam_assets.exists(dep_uri, store="builtin")
                Path.Log.info(
                    f"FCTL DEEP_DESERIALIZE: Toolbit '{dep_uri.asset_id}' exists - local: {exists_local}, builtin: {exists_builtin}"
                )

                toolbit = cam_assets.get(dep_uri, store=["local", "builtin"], depth=0)
                resolved_dependencies[dep_uri] = toolbit
                Path.Log.info(
                    f"FCTL DEEP_DESERIALIZE: Successfully fetched toolbit '{dep_uri.asset_id}'"
                )
            except Exception as e:
                Path.Log.warning(
                    f"FCTL DEEP_DESERIALIZE: Failed to fetch toolbit '{dep_uri.asset_id}': {e}"
                )

                # Try to get more detailed error information
                try:
                    # Check what's actually in the stores
                    local_toolbits = cam_assets.list_assets("toolbit", store="local")
                    local_ids = [uri.asset_id for uri in local_toolbits]
                    Path.Log.info(
                        f"FCTL DEBUG: Local store has {len(local_ids)} toolbits: {local_ids[:10]}{'...' if len(local_ids) > 10 else ''}"
                    )

                    if dep_uri.asset_id in local_ids:
                        Path.Log.warning(
                            f"FCTL DEBUG: Toolbit '{dep_uri.asset_id}' IS in local store list but get() failed!"
                        )
                except Exception as list_error:
                    Path.Log.error(f"FCTL DEBUG: Failed to list local toolbits: {list_error}")

        Path.Log.info(
            f"FCTL DEEP_DESERIALIZE: Resolved {len(resolved_dependencies)} of {len(dependency_uris)} dependencies"
        )

        # Now deserialize with the resolved dependencies
        return cls.deserialize(data, library_id, resolved_dependencies)

    @classmethod
    def deep_deserialize_with_context(cls, data: bytes, file_path: "pathlib.Path"):
        """Deep deserialize a library with file path context for external dependencies."""
        import uuid
        import pathlib
        from ...camassets import cam_assets
        from ...toolbit.serializers import all_serializers as toolbit_serializers

        # Use filename stem as library ID for meaningful names
        library_id = file_path.stem

        Path.Log.info(
            f"FCTL DEEP_DESERIALIZE_WITH_CONTEXT: Starting deep deserialization for library from {file_path}"
        )

        # Extract dependency URIs from the library data
        dependency_uris = cls.extract_dependencies(data)
        Path.Log.info(
            f"FCTL DEEP_DESERIALIZE_WITH_CONTEXT: Found {len(dependency_uris)} toolbit dependencies: {[uri.asset_id for uri in dependency_uris]}"
        )

        # Fetch all toolbit dependencies
        resolved_dependencies = {}
        for dep_uri in dependency_uris:
            try:
                # First try to get from asset manager stores
                Path.Log.info(
                    f"FCTL EXTERNAL: Trying to fetch toolbit '{dep_uri.asset_id}' from stores ['local', 'builtin']"
                )
                toolbit = cam_assets.get(dep_uri, store=["local", "builtin"], depth=0)
                resolved_dependencies[dep_uri] = toolbit
                Path.Log.info(
                    f"FCTL EXTERNAL: Successfully fetched toolbit '{dep_uri.asset_id}' from stores"
                )
            except Exception as e:
                # If not in stores, try to load from parallel Bit directory
                Path.Log.info(
                    f"FCTL EXTERNAL: Toolbit '{dep_uri.asset_id}' not in stores, trying external file: {e}"
                )

                # Look for toolbit files in parallel Bit directory
                library_dir = file_path.parent  # e.g., /path/to/Library/
                tools_dir = library_dir.parent  # e.g., /path/to/
                bit_dir = tools_dir / "Bit"  # e.g., /path/to/Bit/

                toolbit_loaded = False
                if bit_dir.exists():
                    possible_extensions = [".fctb", ".json", ".yaml", ".yml"]
                    for ext in possible_extensions:
                        toolbit_file = bit_dir / f"{dep_uri.asset_id}{ext}"
                        if toolbit_file.exists():
                            try:
                                # Find appropriate serializer for the file
                                from ...assets.ui.util import get_serializer_from_extension

                                serializer_class = get_serializer_from_extension(
                                    toolbit_serializers, ext, for_import=True
                                )
                                if serializer_class:
                                    # Load and deserialize the toolbit
                                    raw_toolbit_data = toolbit_file.read_bytes()
                                    toolbit = serializer_class.deep_deserialize(raw_toolbit_data)
                                    resolved_dependencies[dep_uri] = toolbit
                                    toolbit_loaded = True
                                    Path.Log.info(
                                        f"FCTL EXTERNAL: Successfully loaded toolbit '{dep_uri.asset_id}' from {toolbit_file}"
                                    )
                                    break
                            except Exception as load_error:
                                Path.Log.warning(
                                    f"FCTL EXTERNAL: Failed to load toolbit from {toolbit_file}: {load_error}"
                                )
                                continue

                if not toolbit_loaded:
                    Path.Log.warning(
                        f"FCTL EXTERNAL: Could not load toolbit '{dep_uri.asset_id}' from external files"
                    )

        Path.Log.info(
            f"FCTL EXTERNAL: Resolved {len(resolved_dependencies)} of {len(dependency_uris)} dependencies"
        )

        # Now deserialize with the resolved dependencies
        return cls.deserialize(data, library_id, resolved_dependencies)
