from .base import AssetStore
from ..uri import Uri
from typing import List, Optional
import pathlib

class FlatLocalStore(AssetStore):
    """
    A local file system asset store that ignores asset type and version,
    storing all assets directly in the base directory.
    """
    def __init__(self,
                 protocol: str,
                 base_dir: pathlib.Path,
                 asset_type: str,
                 file_extension: str):
        super().__init__(protocol)
        self.base_dir = base_dir
        self.asset_type = asset_type
        self.file_extension = file_extension

    def set_dir(self, new_dir: pathlib.Path):
        """
        Sets the base directory for the store.
        """
        self.base_dir = new_dir


    def _uri_to_path(self, uri: Uri) -> pathlib.Path:
        """
        Converts a URI to a file system path, ignoring asset type and version.
        The URI is expected to be in the format local://<domain>/<asset_type>/<asset>/<version>.
        Only the asset name is used to construct the path.
        """
        # Assuming uri.path is like /<asset_type>/<asset>/<version>
        if uri.asset_type != self.asset_type:
            raise ValueError(f"Invalid asset type in URI: {uri.asset_type} != {self.asset_type}")

        # The asset name is stored in the asset attribute for this store type
        asset_name = uri.asset
        if not asset_name:
             raise ValueError(f"Invalid URI format: missing asset name in {uri}")

        # Construct the path in the base directory
        file_name = f"{asset_name}{self.file_extension}"
        return self.base_dir / file_name

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> Uri:
        """
        Saves data to a file in the base directory.
        """
        if asset_type != self.asset_type:
            raise ValueError(f"Invalid asset type in URI: {asset_type} != {self.asset_type}")
        file_path = self.base_dir / f"{asset_id}{self.file_extension}"
        file_path.parent.mkdir(parents=True, exist_ok=True)
        with open(file_path, 'wb') as f:
            f.write(data)
        # Return a URI with a fixed version as it's ignored by this store
        return Uri.build(self.protocol, None, asset_type, asset_id, "1")

    async def update(self, uri: Uri, data: bytes) -> Uri:
        """
        Overwrites the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        with open(file_path, 'wb') as f:
            f.write(data)
        return uri

    async def get(self, uri: Uri) -> bytes:
        """
        Reads data from the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        with open(file_path, 'rb') as f:
            return f.read()

    async def delete(self, uri: Uri) -> None:
        """
        Deletes the file at the path derived from the URI.
        """
        file_path = self._uri_to_path(uri)
        file_path.unlink()

    async def list_assets(self,
                          asset_type: Optional[str] = None,
                          limit: Optional[int] = None,
                          offset: Optional[int] = None) -> List[Uri]:
        """
        Lists all assets in the base directory, ignoring asset_type.
        Returns a list of URIs. Pagination is not implemented for this store.
        """
        assets = []
        try:
            for entry in self.base_dir.iterdir():
                if entry.is_file() and entry.name.endswith(self.file_extension):
                    # Construct a URI for the flat file.
                    # The asset_type and version are not used for path construction
                    # but are included in the URI for consistency with AssetStore.
                    asset_name = entry.stem
                    uri = Uri.build(
                        self.protocol,
                        None,
                        self.asset_type,
                        asset_name,
                        "1"
                    )
                    assets.append(uri)
        except FileNotFoundError:
            pass  # Return empty list if base_dir doesn't exist

        # Basic pagination (not efficient for large directories)
        if offset is not None:
            assets = assets[offset:]
        if limit is not None:
            assets = assets[:limit]

        return assets

    async def list_versions(self, uri: Uri) -> List[str]:
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
        Checks if the store contains any assets.
        For FlatLocalStore, this checks if the base directory contains any
        files with the configured file extension. The asset_type parameter
        is ignored.
        """
        if asset_type and asset_type != self.asset_type:
            raise ValueError(f"Invalid asset type in URI: {asset_type} != {self.asset_type}")
        try:
            # Check if the base directory contains any files with the
            # specified extension.
            return not any(f.is_file() and f.name.endswith(self.file_extension)
                           for f in self.base_dir.iterdir())
        except FileNotFoundError:
            # If the base directory doesn't exist, it's empty.
            return True
