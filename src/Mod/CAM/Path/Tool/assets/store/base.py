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
import abc
from typing import List
from ..uri import AssetUri


class AssetStore(abc.ABC):
    """
    Abstract base class for storing and retrieving asset data as raw bytes.

    Stores are responsible for handling the low-level interaction with a
    specific storage backend (e.g., local filesystem, HTTP server) based
    on the URI protocol.
    """

    def __init__(self, name: str, *args, **kwargs):
        self.name = name

    @abc.abstractmethod
    async def get(self, uri: AssetUri) -> bytes:
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

    async def exists(self, uri: AssetUri) -> bool:
        """
        Check if the asset exists at the given URI.

        Args:
            uri: The unique identifier for the asset.

        Returns:
            True if the asset exists, False otherwise.
        """
        try:
            await self.get(uri)
            return True
        except FileNotFoundError:
            return False

    @abc.abstractmethod
    async def delete(self, uri: AssetUri) -> None:
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
    async def create(self, asset_type: str, asset_id: str, data: bytes) -> AssetUri:
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
    async def update(self, uri: AssetUri, data: bytes) -> AssetUri:
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
    async def list_assets(
        self, asset_type: str | None = None, limit: int | None = None, offset: int | None = None
    ) -> List[AssetUri]:
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
    async def count_assets(self, asset_type: str | None = None) -> int:
        """
        Counts assets in the store, optionally filtered by asset type.

        Args:
            asset_type: Optional filter for asset type.

        Returns:
            The number of assets.
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def list_versions(self, uri: AssetUri) -> List[AssetUri]:
        """
        Lists available version identifiers for a specific asset URI.

        Args:
            uri: The URI of the asset (version component is ignored).

        Returns:
            A list of URIs pointing to the specific versions of the asset.
        """
        raise NotImplementedError

    @abc.abstractmethod
    async def is_empty(self, asset_type: str | None = None) -> bool:
        """
        Checks if the store contains any assets, optionally filtered by asset
        type.

        Args:
            asset_type: Optional filter for asset type.

        Returns:
            True if the store is empty (or empty for the given asset type),
            False otherwise.
        """
        raise NotImplementedError
