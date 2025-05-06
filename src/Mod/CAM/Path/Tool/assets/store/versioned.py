import pathlib
import aiofiles
import shutil
from typing import List
from ..uri import Uri, UriStr
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

    def _uri_to_path(self, uri: Uri | UriStr) -> pathlib.Path:
        """Converts a local URI to a filesystem path."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri
        if parsed_uri.protocol != self.protocol:
            raise ValueError(f"Invalid protocol for VersionedLocalStore: {parsed_uri.protocol}")
        # Ignore domain for local paths
        return self._base_dir / parsed_uri.asset_type / parsed_uri.asset / parsed_uri.version

    def _asset_path(self, uri: Uri) -> pathlib.Path:
        """Gets the path to the asset directory (excluding version)."""
        return self._base_dir / uri.asset_type / uri.asset

    async def get(self, uri: Uri | UriStr) -> bytes:
        """Retrieve the raw byte data for the asset at the given URI."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        if parsed_uri.version == "latest":
            versions = await self.list_versions(parsed_uri)
            if not versions:
                raise FileNotFoundError(f"No versions found for {uri}")
            latest_version = versions[-1]
            # Construct a new URI with the latest version
            uri_with_version = Uri.build(
                parsed_uri.protocol,
                parsed_uri.domain,
                parsed_uri.asset_type,
                parsed_uri.asset,
                latest_version,
                parsed_uri.params,
            )
            path = self._uri_to_path(uri_with_version)
        else:
            path = self._uri_to_path(parsed_uri)

        try:
            async with aiofiles.open(path, mode='rb') as f:
                return await f.read()
        except FileNotFoundError:
            raise FileNotFoundError(f"Asset not found at {uri}")

    async def delete(self, uri: Uri | UriStr) -> None:
        """Delete the asset at the given URI."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        if parsed_uri.version == "latest":
            # Delete the entire asset directory
            asset_path = self._asset_path(parsed_uri)
            if asset_path.exists():
                shutil.rmtree(asset_path)
            else:
                # If the asset directory doesn't exist, consider it deleted.
                pass
        else:
            # Delete the specific version file
            path = self._uri_to_path(parsed_uri)
            try:
                path.unlink()
            except FileNotFoundError:
                # If the file doesn't exist, consider it already deleted.
                pass

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> Uri:
        """Create a new asset in the store with the given data."""
        version = "1"

        # Construct the path and ensure directories exist
        asset_path = self._base_dir / asset_type / asset_id / version
        asset_path.parent.mkdir(parents=True, exist_ok=True)

        async with aiofiles.open(asset_path, mode='wb') as f:
            await f.write(data)

        return Uri.build(
            self.protocol,
            None, # Domain is ignored for local store
            asset_type,
            asset_id,
            version
        )

    async def update(self, uri: Uri | UriStr, data: bytes) -> Uri:
        """Update the asset at the given URI with new data, creating a new version."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        versions = await self.list_versions(parsed_uri)
        if not versions:
            # If no versions exist, this is effectively a create operation
            # We can reuse the create logic or raise an error.
            # Let's raise an error for now, as update implies the asset exists.
            raise FileNotFoundError(f"No existing versions found for update at {uri}")

        latest_version = int(versions[-1])
        next_version = str(latest_version + 1)

        # Construct the path for the new version
        asset_path = self._base_dir / parsed_uri.asset_type / parsed_uri.asset / next_version
        asset_path.parent.mkdir(parents=True, exist_ok=True) # Ensure asset directory exists

        async with aiofiles.open(asset_path, mode='wb') as f:
            await f.write(data)

        uri_string = f"{self.protocol}:///{parsed_uri.asset_type}/{parsed_uri.asset}/{next_version}"
        return Uri(uri_string)

    async def list_assets(self,
                          asset_type: str | None = None,
                          limit: int | None = None,
                          offset: int | None = None) -> List[Uri]:
        """
        List assets in the store, optionally filtered by asset type and
        with pagination. For versioned stores, this lists the latest
        version of each asset.
        """
        all_assets: List[Uri] = []
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
                dummy_uri = Uri.build(
                    self.protocol,
                    None, # Domain is ignored for local store
                    current_asset_type,
                    asset_name,
                    "latest" # Version is ignored for listing versions
                )
                versions = await self.list_versions(dummy_uri)
                if versions:
                    latest_version = versions[-1]
                    asset_uri = Uri.build(
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

    async def list_versions(self, uri: Uri | UriStr) -> List[str]:
        """
        Lists available version identifiers for a specific asset URI.

        Args:
            uri: The URI of the asset (version component is ignored).

        Returns:
            A list of version identifiers as strings, sorted in ascending order.
        """
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
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
