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
import pprint
from typing import Dict, List, Optional
from ..uri import AssetUri
from .base import AssetStore


class MemoryStore(AssetStore):
    """
    An in-memory implementation of the AssetStore.

    This store keeps all asset data in memory and is primarily intended for
    testing and demonstration purposes. It does not provide persistence.
    """

    def __init__(self, name: str, *args, **kwargs):
        super().__init__(name, *args, **kwargs)
        self._data: Dict[str, Dict[str, Dict[str, bytes]]] = {}
        self._versions: Dict[str, Dict[str, List[str]]] = {}

    async def get(self, uri: AssetUri) -> bytes:
        asset_type = uri.asset_type
        asset_id = uri.asset_id
        version = uri.version or self._get_latest_version(asset_type, asset_id)

        if (
            asset_type not in self._data
            or asset_id not in self._data[asset_type]
            or version not in self._data[asset_type][asset_id]
        ):
            raise FileNotFoundError(f"Asset not found: {uri}")

        return self._data[asset_type][asset_id][version]

    async def exists(self, uri: AssetUri) -> bool:
        asset_type = uri.asset_type
        asset_id = uri.asset_id
        version = uri.version or self._get_latest_version(asset_type, asset_id)

        return (
            asset_type in self._data
            and asset_id in self._data[asset_type]
            and version in self._data[asset_type][asset_id]
        )

    async def delete(self, uri: AssetUri) -> None:
        asset_type = uri.asset_type
        asset_id = uri.asset_id
        version = uri.version  # Capture the version from the URI

        if asset_type not in self._data or asset_id not in self._data[asset_type]:
            # Deleting non-existent asset should not raise an error
            return

        if version:
            # If a version is specified, try to delete only that version
            if version in self._data[asset_type][asset_id]:
                del self._data[asset_type][asset_id][version]
                # Remove version from the versions list
                if (
                    asset_type in self._versions
                    and asset_id in self._versions[asset_type]
                    and version in self._versions[asset_type][asset_id]
                ):
                    self._versions[asset_type][asset_id].remove(version)

                # If no versions left for this asset_id, clean up
                if not self._data[asset_type][asset_id]:
                    del self._data[asset_type][asset_id]
                    if asset_type in self._versions and asset_id in self._versions[asset_type]:
                        del self._versions[asset_type][asset_id]
        else:
            # If no version is specified, delete the entire asset
            del self._data[asset_type][asset_id]
            if asset_type in self._versions and asset_id in self._versions[asset_type]:
                del self._versions[asset_type][asset_id]

        # Clean up empty asset types
        if asset_type in self._data and not self._data[asset_type]:
            del self._data[asset_type]
        if asset_type in self._versions and not self._versions[asset_type]:
            del self._versions[asset_type]

    async def create(self, asset_type: str, asset_id: str, data: bytes) -> AssetUri:
        if asset_type not in self._data:
            self._data[asset_type] = {}
            self._versions[asset_type] = {}

        if asset_id in self._data[asset_type]:
            # For simplicity, create overwrites existing in this memory store
            # A real store might handle this differently or raise an error
            pass

        if asset_id not in self._data[asset_type]:
            self._data[asset_type][asset_id] = {}
            self._versions[asset_type][asset_id] = []

        version = "1"
        self._data[asset_type][asset_id][version] = data
        self._versions[asset_type][asset_id].append(version)

        return AssetUri(f"{asset_type}://{asset_id}/{version}")

    async def update(self, uri: AssetUri, data: bytes) -> AssetUri:
        asset_type = uri.asset_type
        asset_id = uri.asset_id

        if asset_type not in self._data or asset_id not in self._data[asset_type]:
            raise FileNotFoundError(f"Asset not found for update: {uri}")

        # Update should create a new version
        latest_version = self._get_latest_version(asset_type, asset_id)
        version = str(int(latest_version or 0) + 1)

        self._data[asset_type][asset_id][version] = data
        self._versions[asset_type][asset_id].append(version)

        return AssetUri(f"{asset_type}://{asset_id}/{version}")

    async def list_assets(
        self, asset_type: str | None = None, limit: int | None = None, offset: int | None = None
    ) -> List[AssetUri]:
        all_uris: List[AssetUri] = []
        for current_type, assets in self._data.items():
            if asset_type is None or current_type == asset_type:
                for asset_id in assets:
                    latest_version = self._get_latest_version(current_type, asset_id)
                    if latest_version:
                        all_uris.append(AssetUri(f"{current_type}://{asset_id}/{latest_version}"))

        # Apply offset and limit
        start = offset if offset is not None else 0
        end = start + limit if limit is not None else len(all_uris)
        return all_uris[start:end]

    async def count_assets(self, asset_type: str | None = None) -> int:
        """
        Counts assets in the store, optionally filtered by asset type.
        """
        if asset_type is None:
            count = 0
            for assets_by_id in self._data.values():
                count += len(assets_by_id)
            return count
        else:
            if asset_type in self._data:
                return len(self._data[asset_type])
            return 0

    async def list_versions(self, uri: AssetUri) -> List[AssetUri]:
        asset_type = uri.asset_type
        asset_id = uri.asset_id

        if asset_type not in self._versions or asset_id not in self._versions[asset_type]:
            return []

        version_uris: List[AssetUri] = []
        for version in self._versions[asset_type][asset_id]:
            version_uris.append(AssetUri(f"{asset_type}://{asset_id}/{version}"))
        return version_uris

    async def is_empty(self, asset_type: str | None = None) -> bool:
        if asset_type is None:
            return not bool(self._data)
        else:
            return asset_type not in self._data or not bool(self._data[asset_type])

    def _get_latest_version(self, asset_type: str, asset_id: str) -> Optional[str]:
        if (
            asset_type in self._versions
            and asset_id in self._versions[asset_type]
            and self._versions[asset_type][asset_id]
        ):
            return self._versions[asset_type][asset_id][-1]
        return None

    def dump(self, print: bool = False) -> Dict[str, Dict[str, Dict[str, bytes]]] | None:
        """
        Dumps the entire content of the memory store.

        Args:
            print (bool): If True, pretty-prints the data to the console,
                          excluding the asset data itself.

        Returns:
            Dict[str, Dict[str, Dict[str, bytes]]] | None: The stored data as a
            dictionary, or None if print is True.
        """
        if not print:
            return self._data

        printable_data = {}
        for asset_type, assets in self._data.items():
            printable_data[asset_type] = {}
            for asset_id, versions in assets.items():
                printable_data[asset_type][asset_id] = {}
                for version, data_bytes in versions.items():
                    printable_data[asset_type][asset_id][
                        version
                    ] = f"<data skipped, {len(data_bytes)} bytes>"

        pprint.pprint(printable_data, indent=4)
        return self._data
