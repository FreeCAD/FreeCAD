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
import logging
import asyncio
import threading
import pathlib
import hashlib
from typing import (
    Dict,
    Any,
    Type,
    Optional,
    List,
    Sequence,
    Union,
    Set,
    Mapping,
    Tuple,
)
from dataclasses import dataclass
from PySide import QtCore, QtGui
from .store.base import AssetStore
from .asset import Asset
from .serializer import AssetSerializer
from .uri import AssetUri
from .cache import AssetCache, CacheKey


logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)


@dataclass
class _AssetConstructionData:
    """Holds raw data and type info needed to construct an asset instance."""

    store: str  # Name of the store where this asset was found
    uri: AssetUri
    raw_data: bytes
    asset_class: Type[Asset]
    # Stores AssetConstructionData for dependencies, keyed by their AssetUri
    dependencies_data: Optional[Dict[AssetUri, Optional["_AssetConstructionData"]]] = None


class AssetManager:
    def __init__(self, cache_max_size_bytes: int = 100 * 1024 * 1024):
        self.stores: Dict[str, AssetStore] = {}
        self._serializers: List[Tuple[Type[AssetSerializer], Type[Asset]]] = []
        self._asset_classes: Dict[str, Type[Asset]] = {}
        self.asset_cache = AssetCache(max_size_bytes=cache_max_size_bytes)
        self._cacheable_stores: Set[str] = set()
        logger.debug(f"AssetManager initialized (Thread: {threading.current_thread().name})")

    def register_store(self, store: AssetStore, cacheable: bool = False):
        """Registers an AssetStore with the manager."""
        logger.debug(f"Registering store: {store.name}, cacheable: {cacheable}")
        self.stores[store.name] = store
        if cacheable:
            self._cacheable_stores.add(store.name)

    def get_serializer_for_class(self, asset_class: Type[Asset]):
        for serializer, theasset_class in self._serializers:
            if issubclass(asset_class, theasset_class):
                return serializer
        raise ValueError(f"No serializer found for class {asset_class}")

    def register_asset(self, asset_class: Type[Asset], serializer: Type[AssetSerializer]):
        """Registers an Asset class with the manager."""
        if not issubclass(asset_class, Asset):
            raise TypeError(f"Item '{asset_class.__name__}' must be a subclass of Asset.")
        if not issubclass(serializer, AssetSerializer):
            raise TypeError(f"Item '{serializer.__name__}' must be a subclass of AssetSerializer.")
        self._serializers.append((serializer, asset_class))

        asset_type_name = getattr(asset_class, "asset_type", None)
        if not isinstance(asset_type_name, str) or not asset_type_name:  # Ensure not empty
            raise TypeError(
                f"Asset class '{asset_class.__name__}' must have a non-empty string 'asset_type' attribute."
            )

        logger.debug(f"Registering asset type: '{asset_type_name}' -> {asset_class.__name__}")
        self._asset_classes[asset_type_name] = asset_class

    async def _fetch_asset_construction_data_recursive_async(
        self,
        uri: AssetUri,
        store_names: Sequence[str],
        visited_uris: Set[AssetUri],
        depth: Optional[int] = None,
    ) -> Optional[_AssetConstructionData]:
        logger.debug(
            f"_fetch_asset_construction_data_recursive_async called {store_names} {uri} {depth}"
        )

        if uri in visited_uris:
            logger.error(f"Cyclic dependency detected for URI: {uri}")
            raise RuntimeError(f"Cyclic dependency encountered for URI: {uri}")

        # Check arguments
        if not store_names:
            raise ValueError("At least one store name must be provided.")

        asset_class = self._asset_classes.get(uri.asset_type)
        if not asset_class:
            raise ValueError(f"No asset class registered for URI: {uri}")

        # Fetch the requested asset, trying each store in order
        raw_data = None
        found_store_name = None
        for current_store_name in store_names:
            store = self.stores.get(current_store_name)
            if not store:
                logger.warning(f"Store '{current_store_name}' not registered. Skipping.")
                continue

            try:
                raw_data = await store.get(uri)
                found_store_name = current_store_name
                logger.debug(
                    f"_fetch_asset_construction_data_recursive_async: Asset {uri} found in store {found_store_name}"
                )
                break  # Asset found, no need to check other stores
            except FileNotFoundError:
                logger.debug(
                    f"_fetch_asset_construction_data_recursive_async: Asset {uri} not found in store {current_store_name}"
                )
                continue  # Try next store

        if raw_data is None:
            return None  # Asset not found in any store

        if depth == 0:
            return _AssetConstructionData(
                store=found_store_name,
                uri=uri,
                raw_data=raw_data,
                asset_class=asset_class,
                dependencies_data=None,  # Indicates that no attempt was made to fetch deps
            )

        # Extract the list of dependencies (non-recursive)
        serializer = self.get_serializer_for_class(asset_class)
        dependency_uris = asset_class.extract_dependencies(raw_data, serializer)

        # Initialize deps_construction_data_map. Any dependencies mapped to None
        # indicate that dependencies were intentionally not fetched.
        deps_construction_data: Dict[AssetUri, Optional[_AssetConstructionData]] = {}

        for dep_uri in dependency_uris:
            visited_uris.add(uri)
            try:
                # For dependencies, use the same list of stores for fallback
                dep_data = await self._fetch_asset_construction_data_recursive_async(
                    dep_uri,
                    store_names,  # Pass the list of stores for dependency fallback
                    visited_uris,
                    None if depth is None else depth - 1,
                )
            finally:
                visited_uris.remove(uri)
            deps_construction_data[dep_uri] = dep_data

        logger.debug(
            f"ToolBitShape '{uri.asset_id}' dependencies_data: {deps_construction_data is None}"
        )

        return _AssetConstructionData(
            store=found_store_name,
            uri=uri,
            raw_data=raw_data,
            asset_class=asset_class,
            dependencies_data=deps_construction_data,  # Can be None or Dict
        )

    def _calculate_cache_key_from_construction_data(
        self,
        construction_data: _AssetConstructionData,
        store_name_for_cache: str,
    ) -> Optional[CacheKey]:
        if not construction_data or not construction_data.raw_data:
            return None

        if construction_data.dependencies_data is None:
            deps_signature_tuple: Tuple = ("shallow_children",)
        else:
            deps_signature_tuple = tuple(
                sorted(str(uri) for uri in construction_data.dependencies_data.keys())
            )

        raw_data_hash = int(hashlib.sha256(construction_data.raw_data).hexdigest(), 16)

        return CacheKey(
            store_name=store_name_for_cache,
            asset_uri_str=str(construction_data.uri),
            raw_data_hash=raw_data_hash,
            dependency_signature=deps_signature_tuple,
        )

    def _build_asset_tree_from_data_sync(
        self,
        construction_data: Optional[_AssetConstructionData],
        store_name_for_cache: str,
    ) -> Asset | None:
        """
        Synchronously and recursively builds an asset instance.
        Integrates caching logic.
        """
        if not construction_data:
            return None

        cache_key: Optional[CacheKey] = None
        if store_name_for_cache in self._cacheable_stores:
            cache_key = self._calculate_cache_key_from_construction_data(
                construction_data, store_name_for_cache
            )
            if cache_key:
                cached_asset = self.asset_cache.get(cache_key)
                if cached_asset is not None:
                    return cached_asset

        logger.debug(
            f"BuildAssetTreeSync: Instantiating '{construction_data.uri}' "
            f"of type '{construction_data.asset_class.__name__}'"
        )

        resolved_dependencies: Optional[Mapping[AssetUri, Any]] = None
        if construction_data.dependencies_data is not None:
            resolved_dependencies = {}
            for (
                dep_uri,
                dep_data_node,
            ) in construction_data.dependencies_data.items():
                # Assuming dependencies are fetched from the same store context
                # for caching purposes. If a dependency *could* be from a
                # different store and that store has different cacheability,
                # this would need more complex store_name propagation.
                # For now, use the parent's store_name_for_cache.
                try:
                    dep = self._build_asset_tree_from_data_sync(dep_data_node, store_name_for_cache)
                except Exception as e:
                    logger.error(
                        f"Error building dependency '{dep_uri}' for asset '{construction_data.uri}': {e}",
                        exc_info=True,
                    )
                else:
                    resolved_dependencies[dep_uri] = dep

        asset_class = construction_data.asset_class
        serializer = self.get_serializer_for_class(asset_class)
        try:
            final_asset = asset_class.from_bytes(
                construction_data.raw_data,
                construction_data.uri.asset_id,
                resolved_dependencies,
                serializer,
            )
        except Exception as e:
            logger.error(
                f"Error instantiating asset '{construction_data.uri}' of type '{asset_class.__name__}': {e}",
                exc_info=True,
            )
            return None

        if final_asset is not None and cache_key:
            # This check implies store_name_for_cache was in _cacheable_stores
            direct_deps_uris_strs: Set[str] = set()
            if construction_data.dependencies_data is not None:
                direct_deps_uris_strs = {
                    str(uri) for uri in construction_data.dependencies_data.keys()
                }
            raw_data_size = len(construction_data.raw_data)
            self.asset_cache.put(
                cache_key,
                final_asset,
                raw_data_size,
                direct_deps_uris_strs,
            )
        return final_asset

    def get(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> Asset:
        """
        Retrieves an asset by its URI (synchronous wrapper), to a specified depth.
        IMPORTANT: Assumes this method is CALLED ONLY from the main UI thread
                   if Asset.from_bytes performs UI operations.
        Depth None means infinite depth. Depth 0 means only this asset, no dependencies.
        """
        # Log entry with thread info for verification
        calling_thread_name = threading.current_thread().name
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get(uri='{uri}', stores='{stores_list}', depth='{depth}') called from thread: {calling_thread_name}"
        )
        if (
            QtGui.QApplication.instance()
            and QtCore.QThread.currentThread() is not QtGui.QApplication.instance().thread()
        ):
            logger.warning(
                "AssetManager.get() called from a non-main thread! UI in from_bytes may fail!"
            )

        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        # Step 1: Fetch all data using asyncio.run
        try:
            logger.debug(
                f"Get: Starting asyncio.run for data fetching of '{asset_uri_obj}', depth {depth}."
            )
            all_construction_data = asyncio.run(
                self._fetch_asset_construction_data_recursive_async(
                    asset_uri_obj, stores_list, set(), depth
                )
            )
            logger.debug(
                f"Get: asyncio.run for data fetching of '{asset_uri_obj}', depth {depth} completed."
            )
        except Exception as e:
            logger.error(
                f"Get: Error during asyncio.run data fetching for '{asset_uri_obj}': {e}",
                exc_info=False,
            )
            raise  # Re-raise the exception from the async part

        if all_construction_data is None:
            # This means the top-level asset itself was not found by _fetch_...
            raise FileNotFoundError(f"Asset '{asset_uri_obj}' not found in stores '{stores_list}'")

        # Step 2: Synchronously build the asset tree (and call from_bytes)
        # This happens in the current thread (which is assumed to be the main UI thread)
        deps_count = 0
        found_deps_count = 0
        if all_construction_data.dependencies_data is not None:
            deps_count = len(all_construction_data.dependencies_data)
            found_deps_count = sum(
                1
                for d in all_construction_data.dependencies_data.values()
                if d is not None  # Count actual data, not None placeholders
            )

        logger.debug(
            f"Get: Starting synchronous asset tree build for '{asset_uri_obj}' "
            f"and {deps_count} dependencies ({found_deps_count} resolved)."
        )
        # Use the first store from the list for caching purposes
        store_name_for_cache = stores_list[0] if stores_list else "local"
        final_asset = self._build_asset_tree_from_data_sync(
            all_construction_data, store_name_for_cache=store_name_for_cache
        )
        logger.debug(f"Get: Synchronous asset tree build for '{asset_uri_obj}' completed.")
        return final_asset

    def get_or_none(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> Asset | None:
        """
        Convenience wrapper for get() that does not raise FileNotFoundError; returns
        None instead
        """
        try:
            return self.get(uri, store, depth)
        except FileNotFoundError:
            return None

    async def get_async(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> Optional[Asset]:
        """
        Retrieves an asset by its URI (asynchronous), to a specified depth.
        NOTE: If Asset.from_bytes does UI work, this method should ideally be awaited
        from an asyncio loop that is integrated with the main UI thread (e.g., via QtAsyncio).
        If awaited from a plain worker thread's asyncio loop, from_bytes will run on that worker.
        """
        calling_thread_name = threading.current_thread().name
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get_async(uri='{uri}', stores='{stores_list}', depth='{depth}') called from thread: {calling_thread_name}"
        )

        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        all_construction_data = await self._fetch_asset_construction_data_recursive_async(
            asset_uri_obj, stores_list, set(), depth
        )

        if all_construction_data is None:
            # Consistent with get(), if the top-level asset is not found,
            # raise FileNotFoundError.
            raise FileNotFoundError(
                f"Asset '{asset_uri_obj}' not found in stores '{stores_list}' (async path)"
            )
            # return None # Alternative: if Optional[Asset] means asset might not exist

        # Instantiation happens in the context of where this get_async was awaited.
        logger.debug(
            f"get_async: Building asset tree for '{asset_uri_obj}', depth {depth} in current async context."
        )
        return self._build_asset_tree_from_data_sync(
            all_construction_data, store_name_for_cache=store
        )

    def get_raw(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
    ) -> bytes:
        """Retrieves raw asset data by its URI (synchronous wrapper)."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get_raw(uri='{uri}', stores='{stores_list}') from T:{threading.current_thread().name}"
        )

        async def _fetch_raw_async(stores_list: Sequence[str]):
            asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
            logger.debug(
                f"GetRawAsync (internal): Trying stores '{stores_list}'. Available stores: {list(self.stores.keys())}"
            )
            for current_store_name in stores_list:
                store = self.stores.get(current_store_name)
                if not store:
                    logger.warning(f"Store '{current_store_name}' not registered. Skipping.")
                    continue
                try:
                    raw_data = await store.get(asset_uri_obj)
                    logger.debug(
                        f"GetRawAsync: Asset {asset_uri_obj} found in store {current_store_name}"
                    )
                    return raw_data
                except FileNotFoundError:
                    logger.debug(
                        f"GetRawAsync: Asset {asset_uri_obj} not found in store {current_store_name}"
                    )
                    continue
            raise FileNotFoundError(f"Asset '{asset_uri_obj}' not found in stores '{stores_list}'")

        try:
            return asyncio.run(_fetch_raw_async(stores_list))
        except Exception as e:
            logger.error(
                f"GetRaw: Error during asyncio.run for '{uri}': {e}",
                exc_info=False,
            )
            raise

    async def get_raw_async(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
    ) -> bytes:
        """Retrieves raw asset data by its URI (asynchronous)."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get_raw_async(uri='{uri}', stores='{stores_list}') from T:{threading.current_thread().name}"
        )
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        for current_store_name in stores_list:
            store = self.stores.get(current_store_name)
            if not store:
                logger.warning(f"Store '{current_store_name}' not registered. Skipping.")
                continue
            try:
                raw_data = await store.get(asset_uri_obj)
                logger.debug(
                    f"GetRawAsync: Asset {asset_uri_obj} found in store {current_store_name}"
                )
                return raw_data
            except FileNotFoundError:
                logger.debug(
                    f"GetRawAsync: Asset {asset_uri_obj} not found in store {current_store_name}"
                )
                continue

        raise FileNotFoundError(f"Asset '{asset_uri_obj}' not found in stores '{stores_list}'")

    def get_bulk(
        self,
        uris: Sequence[Union[AssetUri, str]],
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> List[Any]:
        """Retrieves multiple assets by their URIs (synchronous wrapper), to a specified depth."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get_bulk for {len(uris)} URIs from stores '{stores_list}', depth '{depth}'"
        )

        async def _fetch_all_construction_data_bulk_async():
            tasks = [
                self._fetch_asset_construction_data_recursive_async(
                    AssetUri(u) if isinstance(u, str) else u,
                    stores_list,
                    set(),
                    depth,
                )
                for u in uris
            ]
            # Gather all construction data concurrently
            # return_exceptions=True means results list can contain exceptions
            return await asyncio.gather(*tasks, return_exceptions=True)

        try:
            logger.debug("GetBulk: Starting bulk data fetching")
            all_construction_data_list = asyncio.run(_fetch_all_construction_data_bulk_async())
            logger.debug("GetBulk: bulk data fetching completed")
        except Exception as e:  # Should ideally not happen if gather returns exceptions
            logger.error(
                f"GetBulk: Unexpected error during asyncio.run for bulk data: {e}",
                exc_info=False,
            )
            raise

        assets = []
        for i, data_or_exc in enumerate(all_construction_data_list):
            original_uri_input = uris[i]
            # Explicitly re-raise exceptions found in the results list
            if isinstance(data_or_exc, Exception):
                logger.error(
                    f"GetBulk: Re-raising exception for '{original_uri_input}': {data_or_exc}",
                    exc_info=False,
                )
                raise data_or_exc
            elif isinstance(data_or_exc, _AssetConstructionData):
                # Build asset instance synchronously. Exceptions during build should propagate.
                # Use the first store from the list for caching purposes in build_asset_tree
                store_name_for_cache = stores_list[0] if stores_list else "local"
                assets.append(
                    self._build_asset_tree_from_data_sync(
                        data_or_exc, store_name_for_cache=store_name_for_cache
                    )
                )
            elif data_or_exc is None:  # From _fetch_... returning None for not found
                logger.debug(f"GetBulk: Asset '{original_uri_input}' not found")
                assets.append(None)
            else:  # Should not happen
                logger.error(
                    f"GetBulk: Unexpected item in construction data list for '{original_uri_input}': {type(data_or_exc)}"
                )
                # Raise an exception for unexpected data types
                raise RuntimeError(
                    f"Unexpected data type for {original_uri_input}: {type(data_or_exc)}"
                )
        return assets

    async def get_bulk_async(
        self,
        uris: Sequence[Union[AssetUri, str]],
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> List[Any]:
        """Retrieves multiple assets by their URIs (asynchronous), to a specified depth."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"AssetManager.get_bulk_async for {len(uris)} URIs from stores '{stores_list}', depth '{depth}'"
        )
        tasks = [
            self._fetch_asset_construction_data_recursive_async(
                AssetUri(u) if isinstance(u, str) else u,
                stores_list,
                set(),
                depth,
            )
            for u in uris
        ]
        all_construction_data_list = await asyncio.gather(*tasks, return_exceptions=True)

        assets = []
        for i, data_or_exc in enumerate(all_construction_data_list):
            if isinstance(data_or_exc, _AssetConstructionData):
                # Use the first store from the list for caching purposes in build_asset_tree
                store_name_for_cache = stores_list[0] if stores_list else "local"
                assets.append(
                    self._build_asset_tree_from_data_sync(
                        data_or_exc, store_name_for_cache=store_name_for_cache
                    )
                )
            elif isinstance(data_or_exc, FileNotFoundError) or data_or_exc is None:
                assets.append(None)
            elif isinstance(data_or_exc, Exception):
                assets.append(data_or_exc)  # Caller must check
        return assets

    def exists(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
    ) -> bool:
        """
        Returns True if the asset exists in any of the specified stores, False otherwise.
        """

        async def _exists_async(stores_list: Sequence[str]):
            asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
            logger.debug(
                f"ExistsAsync (internal): Trying stores '{stores_list}'. Available stores: {list(self.stores.keys())}"
            )
            for current_store_name in stores_list:
                store = self.stores.get(current_store_name)
                if not store:
                    logger.warning(f"Store '{current_store_name}' not registered. Skipping.")
                    continue
                try:
                    exists = await store.exists(asset_uri_obj)
                    if exists:
                        logger.debug(
                            f"ExistsAsync: Asset {asset_uri_obj} found in store {current_store_name}"
                        )
                        return True
                    else:
                        logger.debug(
                            f"ExistsAsync: Asset {asset_uri_obj} not found in store {current_store_name}"
                        )
                        continue
                except Exception as e:
                    logger.error(
                        f"ExistsAsync: Error checking store '{current_store_name}': {e}",
                        exc_info=False,
                    )
                    continue  # Try next store
            return False  # Not found in any store

        stores_list = [store] if isinstance(store, str) else store
        try:
            return asyncio.run(_exists_async(stores_list))
        except Exception as e:
            logger.error(
                f"AssetManager.exists: Error during asyncio.run for '{uri}': {e}",
                exc_info=False,
            )
            raise

    def fetch(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> List[Asset]:
        """Fetches asset instances based on type, limit, and offset (synchronous), to a specified depth."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"Fetch(type='{asset_type}', stores='{stores_list}', depth='{depth}')")
        # Note: list_assets currently only supports a single store.
        # If fetching from multiple stores is needed for listing, this needs
        # to be updated. For now, list from the first store.
        list_store = stores_list[0] if stores_list else "local"
        asset_uris = self.list_assets(asset_type, limit, offset, list_store)
        results = self.get_bulk(asset_uris, stores_list, depth)  # Pass stores_list to get_bulk
        # Filter out non-Asset objects (e.g., None for not found, or exceptions if collected)
        return [asset for asset in results if isinstance(asset, Asset)]

    async def fetch_async(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: Union[str, Sequence[str]] = "local",
        depth: Optional[int] = None,
    ) -> List[Asset]:
        """Fetches asset instances based on type, limit, and offset (asynchronous), to a specified depth."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"FetchAsync(type='{asset_type}', stores='{stores_list}', depth='{depth}')")
        # Note: list_assets_async currently only supports a single store.
        # If fetching from multiple stores is needed for listing, this needs
        # to be updated. For now, list from the first store.
        list_store = stores_list[0] if stores_list else "local"
        asset_uris = await self.list_assets_async(asset_type, limit, offset, list_store)
        results = await self.get_bulk_async(
            asset_uris, stores_list, depth
        )  # Pass stores_list to get_bulk_async
        return [asset for asset in results if isinstance(asset, Asset)]

    def list_assets(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: Union[str, Sequence[str]] = "local",
    ) -> List[AssetUri]:
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"ListAssets(type='{asset_type}', stores='{stores_list}')")
        # Note: list_assets_async currently only supports a single store.
        # If listing from multiple stores is needed, this needs to be updated.
        # For now, list from the first store.
        list_store = stores_list[0] if stores_list else "local"
        return asyncio.run(self.list_assets_async(asset_type, limit, offset, list_store))

    async def list_assets_async(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: Union[str, Sequence[str]] = "local",
    ) -> List[AssetUri]:
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"ListAssetsAsync executing for type='{asset_type}', stores='{stores_list}'")
        # Note: list_assets_async currently only supports a single store.
        # If listing from multiple stores is needed, this needs to be updated.
        # For now, list from the first store.
        list_store = stores_list[0] if stores_list else "local"
        logger.debug(
            f"ListAssetsAsync: Looking up store '{list_store}'. Available stores: {list(self.stores.keys())}"
        )
        try:
            selected_store = self.stores[list_store]
        except KeyError:
            raise ValueError(f"No store registered for name: {list_store}")
        return await selected_store.list_assets(asset_type, limit, offset)

    def count_assets(
        self,
        asset_type: Optional[str] = None,
        store: Union[str, Sequence[str]] = "local",
    ) -> int:
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"CountAssets(type='{asset_type}', stores='{stores_list}')")
        # Note: count_assets_async currently only supports a single store.
        # If counting across multiple stores is needed, this needs to be updated.
        # For now, count from the first store.
        count_store = stores_list[0] if stores_list else "local"
        return asyncio.run(self.count_assets_async(asset_type, count_store))

    async def count_assets_async(
        self,
        asset_type: Optional[str] = None,
        store: Union[str, Sequence[str]] = "local",
    ) -> int:
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"CountAssetsAsync executing for type='{asset_type}', stores='{stores_list}'")
        # Note: count_assets_async currently only supports a single store.
        # If counting across multiple stores is needed, this needs to be updated.
        # For now, count from the first store.
        count_store = stores_list[0] if stores_list else "local"
        logger.debug(
            f"CountAssetsAsync: Looking up store '{count_store}'. Available stores: {list(self.stores.keys())}"
        )
        try:
            selected_store = self.stores[count_store]
        except KeyError:
            raise ValueError(f"No store registered for name: {count_store}")
        return await selected_store.count_assets(asset_type)

    def _is_registered_type(self, obj: Asset) -> bool:
        """Helper to extract asset_type, id, and data from an object instance."""
        for registered_class_type in self._asset_classes.values():
            if isinstance(obj, registered_class_type):
                return True
        return False

    async def add_async(self, obj: Asset, store: str = "local") -> AssetUri:
        """
        Adds an asset to the store, either creating a new one or updating an existing one.
        Uses obj.get_url() to determine if the asset exists.
        """
        logger.debug(f"AddAsync: Adding {type(obj).__name__} to store '{store}'")
        uri = obj.get_uri()
        if not self._is_registered_type(obj):
            logger.warning(f"Asset has unregistered type '{uri.asset_type}' ({type(obj).__name__})")

        serializer = self.get_serializer_for_class(obj.__class__)
        data = obj.to_bytes(serializer)
        return await self.add_raw_async(uri.asset_type, uri.asset_id, data, store)

    def add(self, obj: Asset, store: str = "local") -> AssetUri:
        """Synchronous wrapper for adding an asset to the store."""
        logger.debug(
            f"Add: Adding {type(obj).__name__} to store '{store}' from T:{threading.current_thread().name}"
        )
        return asyncio.run(self.add_async(obj, store))

    async def add_raw_async(
        self, asset_type: str, asset_id: str, data: bytes, store: str = "local"
    ) -> AssetUri:
        """
        Adds raw asset data to the store, either creating a new asset or updating an existing one.
        """
        logger.debug(f"AddRawAsync: type='{asset_type}', id='{asset_id}', store='{store}'")
        if not asset_type or not asset_id:
            raise ValueError("asset_type and asset_id must be provided for add_raw.")
        if not isinstance(data, bytes):
            raise TypeError("Data for add_raw must be bytes.")
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        uri = AssetUri.build(asset_type=asset_type, asset_id=asset_id)
        try:
            uri = await selected_store.update(uri, data)
            logger.debug(f"AddRawAsync: Updated existing asset at {uri}")
        except FileNotFoundError:
            logger.debug(
                f"AddRawAsync: Asset not found, creating new asset with {asset_type} and {asset_id}"
            )
            uri = await selected_store.create(asset_type, asset_id, data)

        if store in self._cacheable_stores:
            self.asset_cache.invalidate_for_uri(str(uri))  # Invalidate after add/update
        return uri

    def add_raw(
        self, asset_type: str, asset_id: str, data: bytes, store: str = "local"
    ) -> AssetUri:
        """Synchronous wrapper for adding raw asset data to the store."""
        logger.debug(
            f"AddRaw: type='{asset_type}', id='{asset_id}', store='{store}' from T:{threading.current_thread().name}"
        )
        try:
            return asyncio.run(self.add_raw_async(asset_type, asset_id, data, store))
        except Exception as e:
            logger.error(
                f"AddRaw: Error for type='{asset_type}', id='{asset_id}': {e}",
                exc_info=False,
            )
            raise

    def add_file(
        self,
        asset_type: str,
        path: pathlib.Path,
        store: str = "local",
        asset_id: str | None = None,
    ) -> AssetUri:
        """
        Convenience wrapper around add_raw().
        If asset_id is None, the path.stem is used as the id.
        """
        return self.add_raw(asset_type, asset_id or path.stem, path.read_bytes(), store=store)

    def delete(self, uri: Union[AssetUri, str], store: str = "local") -> None:
        logger.debug(f"Delete URI '{uri}' from store '{store}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        async def _do_delete_async():
            selected_store = self.stores[store]
            await selected_store.delete(asset_uri_obj)
            if store in self._cacheable_stores:
                self.asset_cache.invalidate_for_uri(str(asset_uri_obj))

        asyncio.run(_do_delete_async())

    async def delete_async(self, uri: Union[AssetUri, str], store: str = "local") -> None:
        logger.debug(f"DeleteAsync URI '{uri}' from store '{store}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
        selected_store = self.stores[store]
        await selected_store.delete(asset_uri_obj)
        if store in self._cacheable_stores:
            self.asset_cache.invalidate_for_uri(str(asset_uri_obj))

    async def is_empty_async(self, asset_type: Optional[str] = None, store: str = "local") -> bool:
        """Checks if the asset store has any assets of a given type (asynchronous)."""
        logger.debug(f"IsEmptyAsync: type='{asset_type}', store='{store}'")
        logger.debug(
            f"IsEmptyAsync: Looking up store '{store}'. Available stores: {list(self.stores.keys())}"
        )
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.is_empty(asset_type)

    def is_empty(self, asset_type: Optional[str] = None, store: str = "local") -> bool:
        """Checks if the asset store has any assets of a given type (synchronous wrapper)."""
        logger.debug(
            f"IsEmpty: type='{asset_type}', store='{store}' from T:{threading.current_thread().name}"
        )
        try:
            return asyncio.run(self.is_empty_async(asset_type, store))
        except Exception as e:
            logger.error(
                f"IsEmpty: Error for type='{asset_type}', store='{store}': {e}",
                exc_info=False,
            )  # Changed exc_info to False
            raise

    async def list_versions_async(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
    ) -> List[AssetUri]:
        """Lists available versions for a given asset URI (asynchronous)."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(f"ListVersionsAsync: uri='{uri}', stores='{stores_list}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        # Note: list_versions_async currently only supports a single store.
        # If listing versions across multiple stores is needed, this needs to be updated.
        # For now, list from the first store.
        list_store = stores_list[0] if stores_list else "local"
        logger.debug(
            f"ListVersionsAsync: Looking up store '{list_store}'. Available stores: {list(self.stores.keys())}"
        )
        selected_store = self.stores.get(list_store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {list_store}")
        return await selected_store.list_versions(asset_uri_obj)

    def list_versions(
        self,
        uri: Union[AssetUri, str],
        store: Union[str, Sequence[str]] = "local",
    ) -> List[AssetUri]:
        """Lists available versions for a given asset URI (synchronous wrapper)."""
        stores_list = [store] if isinstance(store, str) else store
        logger.debug(
            f"ListVersions: uri='{uri}', stores='{stores_list}' from T:{threading.current_thread().name}"
        )
        try:
            return asyncio.run(self.list_versions_async(uri, stores_list))
        except Exception as e:
            logger.error(
                f"ListVersions: Error for uri='{uri}', stores='{stores_list}': {e}",
                exc_info=False,
            )
            return []  # Return empty list on error to satisfy type hint

    def get_registered_asset_types(self) -> List[str]:
        """Returns a list of registered asset type names."""
        return list(self._asset_classes.keys())
