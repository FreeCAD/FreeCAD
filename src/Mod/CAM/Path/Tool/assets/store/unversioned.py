import os
import pathlib
import aiofiles
from typing import List
from ..uri import Uri, UriStr
from .base import AssetStore

class UnversionedLocalStore(AssetStore):
    """
    Asset store implementation for the local filesystem without versioning.

    Maps URIs of the form local://<domain>/<asset_type>/<asset>
    to paths within a base directory: <base_dir>/<asset_type>/<asset>.<file_extension>
    """
    def __init__(self, protocol: str, base_dir: pathlib.Path, file_extension: str):
        super().__init__(protocol)
        self._base_dir = base_dir
        self._file_extension = file_extension

    def set_dir(self, new_dir: pathlib.Path):
        """
        Sets the base directory for the store.
        """
        self._base_dir = new_dir

    def _uri_to_path(self, uri: Uri | UriStr) -> pathlib.Path:
        """Converts a local URI to a filesystem path for unversioned store."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        if parsed_uri.protocol != self.protocol:
            raise ValueError(f"Invalid protocol for UnversionedLocalStore: {parsed_uri.protocol}")

        # Reconstruct path components from Uri object attributes
        # Expected path format: <asset_type>/<asset>.<file_extension>
        asset_type = parsed_uri.asset_type
        asset_name = parsed_uri.asset

        return self._base_dir / asset_type / f"{asset_name}{self._file_extension}"

    async def get(self, uri: Uri | UriStr) -> bytes:
        """Retrieve the raw byte data for the asset at the given URI."""
        path = self._uri_to_path(uri)
        try:
            async with aiofiles.open(path, mode='rb') as f:
                return await f.read()
        except FileNotFoundError:
            raise FileNotFoundError(f"Asset not found at {uri}")

    async def delete(self, uri: Uri | UriStr) -> None:
        """Delete the asset at the given URI."""
        path = self._uri_to_path(uri)
        try:
            path.unlink()
        except FileNotFoundError:
            # If the file doesn't exist, consider it already deleted.
            pass

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> Uri:
        """Create a new asset with a specified ID in the unversioned store."""
        # Construct the directory path and ensure it exists
        asset_dir = self._base_dir / asset_type
        asset_dir.mkdir(parents=True, exist_ok=True)

        # Construct the full file path
        file_path = asset_dir / f"{asset_id}{self._file_extension}"

        async with aiofiles.open(file_path, mode='wb') as f:
            await f.write(data)

        # Return a dummy URI with version '1' as it's ignored by this store
        # Use Uri.build for consistent URI creation
        return Uri.build(
            self.protocol,
            None, # Domain is ignored for local store
            asset_type,
            asset_id,
            "1" # Dummy version for unversioned store
        )


    async def update(self, uri: Uri | UriStr, data: bytes) -> Uri:
        """Update the asset at the given URI with new data."""
        path = self._uri_to_path(uri)
        if not path.exists():
             raise FileNotFoundError(f"Asset not found for update at {uri}")

        async with aiofiles.open(path, mode='wb') as f:
            await f.write(data)

        return uri # Return the original URI as no new version is created

    async def list_assets(self,
                          asset_type: str | None = None,
                          limit: int | None = None,
                          offset: int | None = None) -> List[Uri]:
        """
        List assets in the unversioned store, optionally filtered by asset type
        and with pagination.
        """
        all_uris = []
        search_dir = self._base_dir / asset_type if asset_type else self._base_dir

        if not search_dir.exists():
            return []

        for root, _, files in os.walk(search_dir):
            for file in files:
                if file.endswith(self._file_extension):
                    # Construct URI from path
                    relative_path = pathlib.Path(root) / file
                    try:
                        relative_path = relative_path.relative_to(self._base_dir)
                    except ValueError:
                        # Should not happen if os.walk is within _base_dir
                        continue

                    # Extract asset_type and asset_name
                    current_asset_type = relative_path.parent.name
                    asset_name = relative_path.stem # Get filename without extension

                    uri = Uri.build(
                        self.protocol,
                        None, # Domain is ignored for local store
                        current_asset_type,
                        asset_name,
                        "1" # Dummy version for unversioned store
                    )
                    all_uris.append(uri)

        # Implement pagination
        start_index = offset if offset is not None else 0
        end_index = start_index + limit if limit is not None else len(all_uris)
        return all_uris[start_index:end_index]

    async def list_versions(self, uri: Uri | UriStr) -> List[str]:
        """
        Lists available version identifiers for a specific asset URI.
        For unversioned store, this always returns ['1'] if the asset exists.
        """
        path = self._uri_to_path(uri)
        if path.exists() and path.is_file():
            return ["1"]
        return []