import pathlib
from typing import List, Optional, Mapping
from .base import AssetStore
from ..uri import AssetUri


class FlatLocalStore(AssetStore):
    """
    A local file system asset store that ignores asset type and version,
    storing all assets directly in the base directory.
    """
    def __init__(self,
                 protocol: str,
                 base_dir: pathlib.Path,
                 type_to_extension: Mapping[str, str]):
        super().__init__(protocol)
        self.base_dir = base_dir
        self.type_to_extension = type_to_extension

    def set_dir(self, new_dir: pathlib.Path):
        """
        Sets the base directory for the store.
        """
        self.base_dir = new_dir


    def _uri_to_path(self, uri: AssetUri) -> pathlib.Path:
        """
        Converts a URI to a file system path, ignoring asset type and version.
        The URI is expected to be in the format local://<domain>/<asset_type>/<asset>/<version>.
        Only the asset name is used to construct the path.
        """
        # Assuming uri.path is like /<asset_type>/<asset>/<version>
        if uri.asset_type not in self.type_to_extension:
            raise ValueError(f"Unsupported asset type: {uri.asset_type}")

        # The asset name is stored in the asset attribute for this store type
        asset_name = uri.asset
        if not asset_name:
            raise ValueError(f"Invalid URI format: missing asset name in {uri}")

        # Construct the path in the base directory
        file_extension = self.type_to_extension[uri.asset_type]
        file_name = f"{asset_name}{file_extension}"
        return self.base_dir / file_name

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> AssetUri:
        """
        Saves data to a file in the base directory.
        """
        if asset_type not in self.type_to_extension:
            raise ValueError(f"Unsupported asset type: {asset_type}")

        file_extension = self.type_to_extension[asset_type]
        file_path = self.base_dir / f"{asset_id}{file_extension}"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        with open(file_path, 'wb') as f:
            f.write(data)
        # Return a URI with a fixed version as it's ignored by this store
        return AssetUri.build(self.protocol, None, asset_type, asset_id, "1")

    async def update(self, uri: AssetUri, data: bytes) -> AssetUri:
        """
        Overwrites the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        with open(file_path, 'wb') as f:
            f.write(data)
        return uri

    async def get(self, uri: AssetUri) -> bytes:
        """
        Reads data from the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        with open(file_path, 'rb') as f:
            return f.read()

    async def delete(self, uri: AssetUri) -> None:
        """
        Deletes the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        file_path.unlink()

    async def list_assets(self,
                          asset_type: Optional[str] = None,
                          limit: Optional[int] = None,
                          offset: Optional[int] = None) -> List[AssetUri]:
        """
        Lists all assets in the base directory, optionally filtered by
        asset_type.
        Returns a list of URIs. Pagination is not implemented for this store.
        """
        if asset_type is None:
            # List all files matching any configured extension
            types = self.type_to_extension.keys()
        elif asset_type not in self.type_to_extension:
            # Return empty list for unsupported asset type
            return []
        else:
            types = [asset_type]

        assets = []
        for asset_type in types:
            extension = self.type_to_extension[asset_type]
            pattern = f"*{extension}"
            for file in self.base_dir.glob(pattern):
                if not file.is_file():
                    continue
                asset_name = file.stem
                uri = AssetUri.build(
                    self.protocol,
                    None,
                    asset_type,
                    asset_name,
                    "1"
                )
                assets.append(uri)

        # Basic pagination (not efficient for large directories)
        if offset is not None:
            assets = assets[offset:]
        if limit is not None:
            assets = assets[:limit]

        return assets

    async def list_versions(self, uri: AssetUri) -> List[str]:
        """
        Returns a list of versions for a given asset.
        Since this store is unversioned, it returns ['1'] if the asset exists,
        otherwise an empty list.
        """
        try:
            file_path = self._uri_to_path(uri)
            if file_path.exists():
                return ["1"]
            else:
                return []
        except ValueError:
            # Invalid URI format means the asset doesn't exist in this store
            return []

    async def is_empty(self, asset_type: str | None = None) -> bool:
        """
        Checks if the store contains any assets, optionally filtered by
        asset_type.
        For FlatLocalStore, this checks if the base directory contains any
        files with the configured file extension(s).
        """
        if asset_type:
            extensions = self.type_to_extension[asset_type]
        else:
            extensions = self.type_to_extension.values()

        # Check if any file matches any configured extension
        for ext in extensions:
            pattern = f"*{ext}"
            if next(self.base_dir.glob(pattern), None) is not None:
                return False  # Found a matching file

        return True  # No matching files found
