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
import re
import pathlib
from typing import List, Dict, Tuple, Optional, cast
from ..uri import AssetUri
from .base import AssetStore


def _resolve_case_insensitive(path: pathlib.Path) -> pathlib.Path:
    if path.is_file():
        return path

    # pathlib in Python 2.10 (2.1x versions) does not support
    # 'case_sensitive' argument in glob. Instead, simulate
    # case-insensitive search manually
    pattern = path.name.lower()
    for p in path.parent.glob("*"):
        if p.name.lower() == pattern:
            return p
    return path

    # Use this starting Python 2.13:
    try:
        return next(path.parent.glob(path.name, case_sensitive=False))
    except StopIteration:
        return path


class FileStore(AssetStore):
    """
    Asset store implementation for the local filesystem with optional
    versioning.

    Maps URIs of the form <asset_type>://<asset_id>[/<version>]
    to paths within a base directory.

    The mapping to file system paths is configurable depending on the asset
    type. Example mapping:

        mapping = {
            "*": "{asset_type}/{asset_id}/{version}.dat",
            "model": "models_dir/{asset_id}-{version}.ml",
            "dataset": "data/{asset_id}.csv" # Unversioned (conceptual version "1")
        }

    Placeholders like {version} are matched greedily (.*), but for compatibility,
    versions are expected to be numeric strings for versioned assets.
    """

    DEFAULT_MAPPING = {
        # Default from original problem doc was "{asset_type}/{asset_id}/{id}/<version>"
        # Adjusted to a more common simple case:
        "*": "{asset_type}/{asset_id}/{version}"
    }

    KNOWN_PLACEHOLDERS = {"asset_type", "asset_id", "id", "version"}

    def __init__(
        self,
        name: str,
        base_dir: pathlib.Path,
        mapping: Optional[Dict[str, str]] = None,
    ):
        super().__init__(name)
        self._base_dir = base_dir.resolve()
        self._mapping = mapping if mapping is not None else self.DEFAULT_MAPPING.copy()
        self._validate_patterns_on_init()
        # For _path_to_uri: iterate specific keys before '*' to ensure correct pattern matching
        self._sorted_mapping_keys = sorted(self._mapping.keys(), key=lambda k: (k == "*", k))

    def _validate_patterns_on_init(self):
        if not self._mapping:
            raise ValueError("Asset store mapping cannot be empty.")

        for asset_type_key, path_format in self._mapping.items():
            if not isinstance(path_format, str):
                raise TypeError(f"Path format for key '{asset_type_key}' must be a string.")

            placeholders_in_format = set(re.findall(r"\{([^}]+)\}", path_format))
            for ph_name in placeholders_in_format:
                if ph_name not in self.KNOWN_PLACEHOLDERS:
                    raise ValueError(
                        f"Unknown placeholder {{{ph_name}}} in pattern: '{path_format}'. Allowed: {self.KNOWN_PLACEHOLDERS}"
                    )

            has_asset_id_ph = "asset_id" in placeholders_in_format or "id" in placeholders_in_format
            if not has_asset_id_ph:
                raise ValueError(
                    f"Pattern '{path_format}' for key '{asset_type_key}' must include {{asset_id}} or {{id}}."
                )

            # CORRECTED LINE: Check for the placeholder name "asset_type" not "{asset_type}"
            if asset_type_key == "*" and "asset_type" not in placeholders_in_format:
                raise ValueError(
                    f"Pattern '{path_format}' for wildcard key '*' must include {{asset_type}}."
                )

    @staticmethod
    def _match_path_to_format_string(format_str: str, path_str_posix: str) -> Dict[str, str]:
        """Matches a POSIX-style path string against a format string."""
        tokens = re.split(r"\{(.*?)\}", format_str)  # format_str uses /
        if len(tokens) == 1:  # No placeholders
            if format_str == path_str_posix:
                return {}
            raise ValueError(f"Path '{path_str_posix}' does not match pattern '{format_str}'")

        keywords = tokens[1::2]
        regex_parts = []
        for i, literal_part in enumerate(tokens[0::2]):
            # Literal parts from format_str (using /) are escaped.
            # The path_str_posix is already normalized to /, so direct matching works.
            regex_parts.append(re.escape(literal_part))
            if i < len(keywords):
                regex_parts.append(f"(?P<{keywords[i]}>.*)")

        pattern_regex_str = "".join(regex_parts)
        match_obj = re.fullmatch(pattern_regex_str, path_str_posix, re.I)

        if not match_obj:
            raise ValueError(
                f"Path '{path_str_posix}' does not match format '{format_str}' (regex: '{pattern_regex_str}')"
            )
        return {kw: match_obj.group(kw) for kw in keywords}

    def _get_path_format_for_uri_type(self, uri_asset_type: str) -> str:
        if uri_asset_type in self._mapping:
            return self._mapping[uri_asset_type]
        if "*" in self._mapping:
            return self._mapping["*"]
        raise ValueError(
            f"No mapping pattern for asset_type '{uri_asset_type}' and no '*' fallback."
        )

    def _path_to_uri(self, file_path: pathlib.Path) -> Optional[AssetUri]:
        """Converts a filesystem path to an AssetUri, if it matches a pattern."""
        if not file_path.is_file():
            return None

        try:
            # Convert to relative path object first, then to POSIX string for matching
            relative_path_obj = file_path.relative_to(self._base_dir)
            relative_path_posix = relative_path_obj.as_posix()
        except ValueError:
            return None  # Path not under base_dir

        for asset_type_key in self._sorted_mapping_keys:
            path_format_str = self._mapping[asset_type_key]  # Pattern uses /
            try:
                components = FileStore._match_path_to_format_string(
                    path_format_str, relative_path_posix
                )

                asset_id = components.get("asset_id", components.get("id"))
                if not asset_id:
                    continue

                current_asset_type: str
                if "{asset_type}" in path_format_str:
                    current_asset_type = components.get("asset_type", "")
                    if not current_asset_type:
                        continue
                else:
                    current_asset_type = asset_type_key
                    if current_asset_type == "*":
                        continue  # Invalid state, caught by validation

                version_str: str
                if "{version}" in path_format_str:
                    version_str = components.get("version", "")
                    if not version_str or not version_str.isdigit():
                        continue
                else:
                    version_str = "1"

                return AssetUri.build(
                    asset_type=current_asset_type,
                    asset_id=asset_id,
                    version=version_str,
                )
            except ValueError:  # No match
                continue
        return None

    def set_dir(self, new_dir: pathlib.Path):
        """Sets the base directory for the store."""
        self._base_dir = new_dir.resolve()

    def _uri_to_path(self, uri: AssetUri) -> pathlib.Path:
        """Converts an AssetUri to a filesystem path using mapping."""
        path_format_str = self._get_path_format_for_uri_type(uri.asset_type)

        format_values: Dict[str, str] = {
            "asset_type": uri.asset_type,
            "asset_id": uri.asset_id,
            "id": uri.asset_id,
        }

        # Only add 'version' to format_values if the pattern expects it AND uri.version is set.
        # uri.version must be a string for .format() (e.g. "1", not None).
        if "{version}" in path_format_str:
            if uri.version is None:
                # This state implies an issue: a versioned pattern is being used
                # but the URI hasn't had its version appropriately set (e.g. to "1" for create,
                # or resolved from "latest").
                raise ValueError(
                    f"URI version is None for versioned pattern '{path_format_str}'. URI: {uri}"
                )
            format_values["version"] = uri.version

        try:
            # Patterns use '/', pathlib handles OS-specific path construction.
            resolved_path_str = path_format_str.format(**format_values)
        except KeyError as e:
            raise ValueError(
                f"Pattern '{path_format_str}' placeholder {{{e}}} missing in URI data for {uri}."
            )

        return self._base_dir / resolved_path_str

    async def get(self, uri: AssetUri) -> bytes:
        """Retrieve the raw byte data for the asset at the given URI."""
        path_to_read: pathlib.Path

        if uri.version == "latest":
            query_uri = AssetUri.build(
                asset_type=uri.asset_type,
                asset_id=uri.asset_id,
                params=uri.params,
            )
            versions = await self.list_versions(query_uri)
            if not versions:
                raise FileNotFoundError(f"No versions found for {uri.asset_type}://{uri.asset_id}")
            latest_version_uri = versions[-1]  # list_versions now returns AssetUri with params
            path_to_read = self._uri_to_path(latest_version_uri)
        else:
            request_uri = uri
            path_format_str = self._get_path_format_for_uri_type(uri.asset_type)
            is_versioned_pattern = "{version}" in path_format_str

            if not is_versioned_pattern:
                if uri.version is not None and uri.version != "1":
                    raise FileNotFoundError(
                        f"Asset type '{uri.asset_type}' is unversioned. "
                        f"Version '{uri.version}' invalid for URI {uri}. Use '1' or no version."
                    )
                if uri.version is None:  # Conceptual "type://id" -> "type://id/1"
                    request_uri = AssetUri.build(
                        asset_type=uri.asset_type,
                        asset_id=uri.asset_id,
                        version="1",
                        params=uri.params,
                    )
            elif (
                uri.version is None
            ):  # Versioned pattern but URI has version=None (and not "latest")
                raise FileNotFoundError(
                    f"Version required for asset type '{uri.asset_type}' (pattern: '{path_format_str}'). URI: {uri}"
                )
            path_to_read = self._uri_to_path(request_uri)

        path_to_read = _resolve_case_insensitive(path_to_read)
        try:
            with open(path_to_read, mode="rb") as f:
                return f.read()
        except FileNotFoundError:
            raise FileNotFoundError(f"Asset for URI {uri} not found at path {path_to_read}")
        except IsADirectoryError:
            raise FileNotFoundError(f"Asset URI {uri} resolved to a directory: {path_to_read}")

    async def exists(self, uri: AssetUri) -> bool:
        """Check if the asset exists at the given URI."""
        try:
            await self.get(uri)
            return True
        except FileNotFoundError:
            return False
        except IsADirectoryError:
            # If the path is a directory, it means the asset exists but is not a file.
            return False

    async def delete(self, uri: AssetUri) -> None:
        """Delete the asset at the given URI."""
        paths_to_delete: List[pathlib.Path] = []
        parent_dirs_of_deleted_files = set()  # To track for cleanup

        path_format_str = self._get_path_format_for_uri_type(uri.asset_type)
        is_versioned_pattern = "{version}" in path_format_str

        if uri.version is None:  # Delete all versions or the single unversioned file
            for path_obj in self._base_dir.rglob("*"):
                parsed_uri = self._path_to_uri(path_obj)
                if (
                    parsed_uri
                    and parsed_uri.asset_type == uri.asset_type
                    and parsed_uri.asset_id == uri.asset_id
                ):
                    paths_to_delete.append(path_obj)
        else:  # Delete a specific version or an unversioned file (if version is "1")
            target_uri_for_path = uri
            if not is_versioned_pattern:
                if uri.version != "1":
                    return  # Idempotent: non-"1" version of unversioned asset "deleted"
                target_uri_for_path = AssetUri.build(
                    asset_type=uri.asset_type,
                    asset_id=uri.asset_id,
                    version="1",
                    params=uri.params,
                )

            path = self._uri_to_path(target_uri_for_path)
            path = _resolve_case_insensitive(path)
            if path.is_file():
                paths_to_delete.append(path)

        for p_del in paths_to_delete:
            try:
                p_del.unlink()
                parent_dirs_of_deleted_files.add(p_del.parent)
            except FileNotFoundError:
                pass

        # Clean up empty parent directories, from deepest first
        sorted_parents = sorted(
            list(parent_dirs_of_deleted_files),
            key=lambda p: len(p.parts),
            reverse=True,
        )
        for parent_dir in sorted_parents:
            current_cleanup_path = parent_dir
            while (
                current_cleanup_path.exists()
                and current_cleanup_path.is_dir()
                and current_cleanup_path != self._base_dir
                and current_cleanup_path.is_relative_to(self._base_dir)
                and not any(current_cleanup_path.iterdir())
            ):
                try:
                    current_cleanup_path.rmdir()
                    current_cleanup_path = current_cleanup_path.parent
                except OSError:
                    break

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> AssetUri:
        """Create a new asset in the store with the given data."""
        # New assets are conceptually version "1"
        uri_to_create = AssetUri.build(asset_type=asset_type, asset_id=asset_id, version="1")
        asset_path = self._uri_to_path(uri_to_create)

        if asset_path.exists():
            # More specific error messages based on what exists
            if asset_path.is_file():
                raise FileExistsError(f"Asset file already exists at {asset_path}")
            if asset_path.is_dir():
                raise IsADirectoryError(f"A directory exists at target path {asset_path}")
            raise FileExistsError(f"Path {asset_path} already exists (unknown type).")

        asset_path.parent.mkdir(parents=True, exist_ok=True)
        with open(asset_path, mode="wb") as f:
            f.write(data)
        return uri_to_create

    async def update(self, uri: AssetUri, data: bytes) -> AssetUri:
        """Update the asset at the given URI with new data, creating a new version."""
        # Get a Uri without the version number, use it to find all versions.
        query_uri = AssetUri.build(
            asset_type=uri.asset_type, asset_id=uri.asset_id, params=uri.params
        )
        existing_versions = await self.list_versions(query_uri)
        if not existing_versions:
            raise FileNotFoundError(
                f"No versions for asset {uri.asset_type}://{uri.asset_id} to update."
            )

        # Create a Uri for the NEXT version number.
        latest_version_uri = existing_versions[-1]
        latest_version_num = int(cast(str, latest_version_uri.version))
        next_version_str = str(latest_version_num + 1)
        next_uri = AssetUri.build(
            asset_type=uri.asset_type,
            asset_id=uri.asset_id,
            version=next_version_str,
            params=uri.params,
        )
        asset_path = self._uri_to_path(next_uri)
        asset_path = _resolve_case_insensitive(asset_path)

        # If the file is versioned, then the new version should not yet exist.
        # Double check to be sure.
        path_format_str = self._get_path_format_for_uri_type(uri.asset_type)
        is_versioned_pattern = "{version}" in path_format_str
        if asset_path.exists() and is_versioned_pattern:
            raise FileExistsError(f"Asset path for new version {asset_path} already exists.")

        # Done. Write to disk.
        asset_path.parent.mkdir(parents=True, exist_ok=True)
        with open(asset_path, mode="wb") as f:
            f.write(data)
        return next_uri

    async def list_assets(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
    ) -> List[AssetUri]:
        """
        List assets in the store, optionally filtered by asset type and
        with pagination. For versioned stores, this lists the latest
        version of each asset.
        """
        latest_asset_versions: Dict[Tuple[str, str], str] = {}

        for path_obj in self._base_dir.rglob("*"):
            parsed_uri = self._path_to_uri(path_obj)
            if parsed_uri:
                if asset_type is not None and parsed_uri.asset_type != asset_type:
                    continue

                key = (parsed_uri.asset_type, parsed_uri.asset_id)
                current_version_str = cast(str, parsed_uri.version)  # Is "1" or numeric string

                if key not in latest_asset_versions or int(current_version_str) > int(
                    latest_asset_versions[key]
                ):
                    latest_asset_versions[key] = current_version_str

        result_uris: List[AssetUri] = [
            AssetUri.build(
                asset_type=atype, asset_id=aid, version=vstr
            )  # Params not included in list_assets results
            for (atype, aid), vstr in latest_asset_versions.items()
        ]
        result_uris.sort(key=lambda u: (u.asset_type, u.asset_id, int(cast(str, u.version))))

        start = offset if offset is not None else 0
        end = start + limit if limit is not None else len(result_uris)
        return result_uris[start:end]

    async def count_assets(self, asset_type: Optional[str] = None) -> int:
        """
        Counts assets in the store, optionally filtered by asset type.
        """
        unique_assets: set[Tuple[str, str]] = set()

        for path_obj in self._base_dir.rglob("*"):
            parsed_uri = self._path_to_uri(path_obj)
            if parsed_uri:
                if asset_type is not None and parsed_uri.asset_type != asset_type:
                    continue
                unique_assets.add((parsed_uri.asset_type, parsed_uri.asset_id))

        return len(unique_assets)

    async def list_versions(self, uri: AssetUri) -> List[AssetUri]:
        """
        Lists available version identifiers for a specific asset URI.
        Args:
            uri: The URI of the asset (version component is ignored, params preserved).
        Returns:
            A list of AssetUri objects, sorted by version in ascending order.
        """
        if uri.asset_id is None:
            raise ValueError(f"Asset ID must be specified for listing versions: {uri}")

        path_format_str = self._get_path_format_for_uri_type(uri.asset_type)
        is_versioned_pattern = "{version}" in path_format_str

        if not is_versioned_pattern:
            # Check existence of the single unversioned file
            # Conceptual version is "1", params from input URI are preserved
            path_check_uri = AssetUri.build(
                asset_type=uri.asset_type,
                asset_id=uri.asset_id,
                version="1",
                params=uri.params,
            )
            path_to_asset = self._uri_to_path(path_check_uri)
            path_to_asset = _resolve_case_insensitive(path_to_asset)
            if path_to_asset.is_file():
                return [path_check_uri]  # Returns URI with version "1" and original params
            return []

        found_versions_strs: List[str] = []
        for path_obj in self._base_dir.rglob("*"):
            parsed_uri = self._path_to_uri(path_obj)  # This parsed_uri does not have params
            if (
                parsed_uri
                and parsed_uri.asset_type == uri.asset_type
                and parsed_uri.asset_id == uri.asset_id
            ):
                # Version from path is guaranteed numeric string by _path_to_uri for versioned patterns
                found_versions_strs.append(cast(str, parsed_uri.version))

        if not found_versions_strs:
            return []
        sorted_unique_versions = sorted(list(set(found_versions_strs)), key=int)

        return [
            AssetUri.build(
                asset_type=uri.asset_type,
                asset_id=uri.asset_id,
                version=v_str,
                params=uri.params,
            )
            for v_str in sorted_unique_versions
        ]

    async def is_empty(self, asset_type: Optional[str] = None) -> bool:
        """
        Checks if the store contains any assets, optionally filtered by asset
        type.
        """
        # Reuses list_assets which iterates files.
        # Limit=1 makes it stop after finding the first asset.
        assets = await self.list_assets(asset_type=asset_type, limit=1)
        return not bool(assets)
