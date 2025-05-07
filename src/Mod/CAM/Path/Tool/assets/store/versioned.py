import pathlib
import aiofiles
import shutil
from typing import List
from ..uri import AssetUri
from .base import AssetStore

class VersionedLocalStore(AssetStore):
    """
    Asset store implementation for the local filesystem with versioning.

    Maps URIs of the form local://<domain>/<asset_type>/<asset>/<version>
    to paths within a base directory: <base_dir>/<asset_type>/<asset>/<version>.
    """

    def __init__(self, protocol: str, base_dir: pathlib.Path):
        super().__init__(protocol)
        self._base_dir = base_dir

    def set_dir(self, new_dir: pathlib.Path):
        """
        Sets the base directory for the store.
        """
        self._base_dir = new_dir

    def _uri_to_path(self, uri: AssetUri) -> pathlib.Path:
        """Converts a local URI to a filesystem path."""
        if uri.protocol != self.protocol:
            raise ValueError(f"Invalid protocol for VersionedLocalStore: {uri.protocol}")
        # Ignore domain for local paths
        return self._base_dir / uri.asset_type / uri.asset / uri.version

    def _asset_path(self, uri: AssetUri) -> pathlib.Path:
        """Gets the path to the asset directory (excluding version)."""
        return self._base_dir / uri.asset_type / uri.asset

    async def get(self, uri: AssetUri) -> bytes:
        """Retrieve the raw byte data for the asset at the given URI."""
        if uri.version == "latest":
            versions = await self.list_versions(uri)
            if not versions:
                raise FileNotFoundError(f"No versions found for {uri}")
            latest_version = versions[-1]
            # Construct a new URI with the latest version
            uri_with_version = AssetUri.build(
                uri.protocol,
                uri.domain,
                uri.asset_type,
                uri.asset,
                latest_version,
                uri.params,
            )
            path = self._uri_to_path(uri_with_version)
        else:
            path = self._uri_to_path(uri)

        try:
            async with aiofiles.open(path, mode='rb') as f:
                return await f.read()
        except FileNotFoundError:
            raise FileNotFoundError(f"Asset not found at {uri}")

    async def delete(self, uri: AssetUri) -> None:
        """Delete the asset at the given URI."""
        if uri.version is None:
            # Delete the entire asset directory
            asset_path = self._asset_path(uri)
            try:
                shutil.rmtree(asset_path)
            except FileNotFoundError:
                pass
            return

        # Delete the specific version file
        path = self._uri_to_path(uri)
        try:
            path.unlink()
            # Check if the asset directory is empty and remove it
            asset_dir = path.parent
            if not any(asset_dir.iterdir()):
                asset_dir.rmdir()
                # Check if the asset type directory is empty and remove it
                asset_type_dir = asset_dir.parent
                if not any(asset_type_dir.iterdir()):
                    asset_type_dir.rmdir()
        except FileNotFoundError:
            # If the file doesn't exist, consider it already deleted.
            pass

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> AssetUri:
        """Create a new asset in the store with the given data."""
        version = "1"

        # Construct the path and ensure directories exist
        asset_path = self._base_dir / asset_type / asset_id / version
        asset_path.parent.mkdir(parents=True, exist_ok=True)

        async with aiofiles.open(asset_path, mode='wb') as f:
            await f.write(data)

        return AssetUri.build(
            self.protocol,
            None, # Domain is ignored for local store
            asset_type,
            asset_id,
            version
        )

    async def update(self, uri: AssetUri, data: bytes) -> AssetUri:
        """Update the asset at the given URI with new data, creating a new version."""
        versions = await self.list_versions(uri)
        if not versions:
            # If no versions exist, this is effectively a create operation
            # We can reuse the create logic or raise an error.
            # Let's raise an error for now, as update implies the asset exists.
            raise FileNotFoundError(f"No existing versions found for update at {uri}")

        latest_version = int(versions[-1])
        next_version = str(latest_version + 1)

        # Construct the path for the new version
        asset_path = self._base_dir / uri.asset_type / uri.asset / next_version
        asset_path.parent.mkdir(parents=True, exist_ok=True) # Ensure asset directory exists

        async with aiofiles.open(asset_path, mode='wb') as f:
            await f.write(data)

        uri_string = f"{self.protocol}:///{uri.asset_type}/{uri.asset}/{next_version}"
        return AssetUri(uri_string)

    async def list_assets(self,
                          asset_type: str | None = None,
                          limit: int | None = None,
                          offset: int | None = None) -> List[AssetUri]:
        """
        List assets in the store, optionally filtered by asset type and
        with pagination. For versioned stores, this lists the latest
        version of each asset.
        """
        all_assets: List[AssetUri] = []
        base_path = self._base_dir

        type_dirs = [d for d in base_path.iterdir() if d.is_dir()]

        for type_dir in type_dirs:
            current_asset_type = type_dir.name
            if asset_type is not None and \
               current_asset_type != asset_type:
                continue

            asset_dirs = [d for d in type_dir.iterdir() if d.is_dir()]

            for asset_dir in asset_dirs:
                asset_name = asset_dir.name
                # Use a dummy URI to find versions
                dummy_uri = AssetUri.build(
                    self.protocol,
                    None, # Domain is ignored for local store
                    current_asset_type,
                    asset_name,
                    "latest" # Version is ignored for listing versions
                )
                versions = await self.list_versions(dummy_uri)
                if versions:
                    latest_version = versions[-1]
                    asset_uri = AssetUri.build(
                        self.protocol,
                        None, # Domain is ignored for local store
                        current_asset_type,
                        asset_name,
                        latest_version
                    )
                    all_assets.append(asset_uri)

        # Apply pagination
        start = offset if offset is not None else 0
        end = start + limit if limit is not None else len(all_assets)
        return all_assets[start:end]

    async def list_versions(self, uri: AssetUri) -> List[str]:
        """
        Lists available version identifiers for a specific asset URI.

        Args:
            uri: The URI of the asset (version component is ignored).

        Returns:
            A list of version identifiers as strings, sorted in ascending order.
        """
        if isinstance(uri, str):
            parsed_uri = AssetUri(uri)
        else:
            parsed_uri = uri

        asset_path = self._asset_path(parsed_uri)
        if not asset_path.exists():
            return []
        # List files that are valid version numbers (integers)
        versions = [
            f.name
            for f in asset_path.iterdir()
            if f.is_file() and f.name.isdigit()
        ]
        return sorted(versions, key=int)

    async def is_empty(self, asset_type: str | None = None) -> bool:
        """
        Checks if the store contains any assets, optionally filtered by asset
        type.
        """
        path = self._base_dir
        if asset_type:
            path /= asset_type
            if not path.exists() or not path.is_dir():
                return True

        # Check if there are any asset type directories or asset directories
        # within the specified path (or base_dir if no asset_type)
        has_content = any(d.is_dir() and any(d.iterdir()) for d in path.iterdir())
        return not has_content
