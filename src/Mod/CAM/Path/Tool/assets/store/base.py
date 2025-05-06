import abc
from typing import List
from ..uri import Uri, UriStr

class AssetStore(abc.ABC):
    """
    Abstract base class for storing and retrieving asset data as raw bytes.

    Stores are responsible for handling the low-level interaction with a
    specific storage backend (e.g., local filesystem, HTTP server) based
    on the URI protocol.
    """

    def __init__(self, protocol: str, *args, **kwargs):
        self.protocol = protocol

    @abc.abstractmethod
    async def get(self, uri: Uri | UriStr) -> bytes:
        """
        Retrieve the raw byte data for the asset at the given URI.

        Args:
            uri: The unique identifier for the asset.

        Returns:
            The raw byte data of the asset.

        Raises:
            FileNotFoundError: If the asset does not exist at the URI.
            # Other store-specific exceptions may be raised.
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def delete(self, uri: Uri | UriStr) -> None:
        """
        Delete the asset at the given URI.

        Args:
            uri: The unique identifier for the asset to delete.

        Raises:
            FileNotFoundError: If the asset does not exist at the URI.
            # Other store-specific exceptions may be raised.
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def create(self, asset_type: str, asset_id: str, data: bytes) -> Uri:
        """
        Create a new asset in the store with the given data.

        The store determines the final URI for the new asset. The
        `asset_type` can be used to influence the storage location
        or URI structure (e.g., as part of the path).

        Args:
            asset_type: The type of the asset (e.g., 'material',
                           'toolbitshape').
            asset_id: The unique identifier for the asset.
            data: The raw byte data of the asset to create.

        Returns:
            The URI of the newly created asset.

        Raises:
            # Store-specific exceptions may be raised (e.g., write errors).
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def update(self, uri: Uri | UriStr, data: bytes) -> Uri:
        """
        Update the asset at the given URI with new data, creating a new version.

        Args:
            uri: The unique identifier of the asset to update.
            data: The new raw byte data for the asset.

        Raises:
            FileNotFoundError: If the asset does not exist at the URI.
            # Other store-specific exceptions may be raised (e.g., write errors).
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def list_assets(self,
                          asset_type: str | None = None,
                          limit: int | None = None,
                          offset: int | None = None) -> List[Uri]:
        """
        List assets in the store, optionally filtered by asset type and
        with pagination. For versioned stores, this lists the latest
        version of each asset.

        Args:
            asset_type: Optional filter for asset type.
            limit: Maximum number of assets to return.
            offset: Number of assets to skip from the beginning.

        Returns:
            A list of URIs for the assets.
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def list_versions(self, uri: Uri | UriStr) -> List[str]:
        """
        Lists available version identifiers for a specific asset URI.

        Args:
            uri: The URI of the asset (version component is ignored).

        Returns:
            A list of version identifiers as strings, sorted in ascending
            order.
        """
        raise NotImplementedError