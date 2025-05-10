# -*- coding: utf-8 -*-
import logging
import asyncio
import threading
from typing import Dict, Any, Type, Optional, List, Sequence, Union, Set, Tuple
from dataclasses import dataclass, field
from PySide2.QtWidgets import QApplication
from PySide2.QtCore import QThread

from .store.base import AssetStore
from .asset import Asset
from .uri import AssetUri


logger = logging.getLogger(__name__)


@dataclass
class _AssetConstructionData:
    """Holds raw data and type info needed to construct an asset instance."""
    uri: AssetUri
    raw_data: bytes
    asset_class_type: Type[Asset]
    asset_id: str
    # Stores AssetConstructionData for dependencies, keyed by their AssetUri
    dependencies_data: Dict[AssetUri, Optional["_AssetConstructionData"]] = (
        field(default_factory=dict)
    )


class AssetManager:
    def __init__(self):
        self.stores: Dict[str, AssetStore] = {}
        self._asset_classes: Dict[str, Type[Asset]] = {}
        logger.debug(
            f"AssetManager initialized (Thread: {threading.current_thread().name})"
        )

    def register_store(self, store: AssetStore):
        """Registers an AssetStore with the manager."""
        logger.debug(f"Registering store: {store.name}")
        self.stores[store.name] = store

    def register_asset(self, asset_class: Type[Asset]):
        """Registers an Asset class with the manager."""
        if not issubclass(asset_class, Asset):
            raise TypeError(
                f"Registered item '{asset_class.__name__}' must be a subclass of Asset."
            )

        asset_type_name = getattr(asset_class, "asset_type", None)
        if (
            not isinstance(asset_type_name, str) or not asset_type_name
        ):  # Ensure not empty
            raise TypeError(
                f"Asset class '{asset_class.__name__}' must have a non-empty string 'asset_type' attribute."
            )

        logger.debug(
            f"Registering asset type: '{asset_type_name}' -> {asset_class.__name__}"
        )
        self._asset_classes[asset_type_name] = asset_class

    async def _fetch_asset_construction_data_recursive_async(
        self, uri: AssetUri, store_name: str, visited_uris: Set[AssetUri]
    ) -> Optional[_AssetConstructionData]:
        """
        Asynchronously and recursively fetches all raw data and type information
        needed to construct an asset and its dependencies.
        Does NOT call Asset.from_bytes.
        Returns _AssetConstructionData or None if the primary asset is not found.
        """
        if uri in visited_uris:
            logger.error(f"Cyclic dependency detected for URI: {uri}")
            raise RuntimeError(f"Cyclic dependency encountered for URI: {uri}")

        logger.debug(f"AsyncFetchData: URI '{uri}', store '{store_name}'.")
        visited_uris.add(uri)

        try:
            logger.debug(
                f"AsyncFetchData: Looking up store '{store_name}'. Available stores: {list(self.stores.keys())}"
            )
            selected_store = self.stores[store_name]  # Can raise KeyError
            raw_data = await selected_store.get(
                uri
            )  # Can raise FileNotFoundError (async)
            logger.debug(
                f"AsyncFetchData: Looking up asset type '{uri.asset_type}'. Available types: {list(self._asset_classes.keys())}"
            )
            asset_class_type = self._asset_classes[
                uri.asset_type
            ]  # Can raise KeyError

            # Get dependency URIs from the raw data using the static/class method
            dependency_uris = asset_class_type.dependencies(raw_data)

            deps_construction_data_map: Dict[
                AssetUri, Optional[_AssetConstructionData]
            ] = {}
            if dependency_uris:
                # Create a list of tasks to fetch data for all dependencies concurrently
                dep_fetch_tasks = {
                    dep_uri: self._fetch_asset_construction_data_recursive_async(
                        dep_uri,
                        store_name,
                        visited_uris.copy(),  # Pass copy of visited set for each branch
                    )
                    for dep_uri in dependency_uris
                }

                # Execute tasks concurrently
                dependency_results = await asyncio.gather(
                    *dep_fetch_tasks.values(), return_exceptions=True
                )

                # Populate the map with results
                for original_dep_uri, result in zip(
                    dep_fetch_tasks.keys(), dependency_results
                ):
                    if isinstance(result, FileNotFoundError):
                        logger.warning(
                            f"Optional dependency '{original_dep_uri}' for '{uri}' not found."
                        )
                        deps_construction_data_map[original_dep_uri] = None
                    elif isinstance(
                        result, Exception
                    ):  # Other exceptions are re-raised
                        logger.error(
                            f"Error fetching data for dependency '{original_dep_uri}' of '{uri}'.",
                            exc_info=result,
                        )
                        raise result
                    else:  # result is AssetConstructionData or None
                        if isinstance(result, BaseException):
                            logger.error(
                                f"Error fetching data for dependency '{original_dep_uri}' of '{uri}'.",
                                exc_info=result,
                            )
                            raise result
                        deps_construction_data_map[original_dep_uri] = result
            return _AssetConstructionData(
                uri=uri,
                raw_data=raw_data,
                asset_class_type=asset_class_type,
                asset_id=uri.asset_id,
                dependencies_data=deps_construction_data_map,
            )
        except KeyError as e:  # For self.stores or self._asset_classes lookup
            if str(e) == f"'{store_name}'":
                raise ValueError(
                    f"No store registered for name: {store_name}"
                ) from e
            elif str(e) == f"'{uri.asset_type}'":
                raise ValueError(
                    f"No asset class registered for asset type: {uri.asset_type}"
                ) from e
            else:
                # Re-raise if it's a different KeyError
                raise
        except FileNotFoundError:
            # This is for the primary URI fetch. Dependencies are handled above.
            return None  # asset does not exist
        finally:
            if uri in visited_uris:
                visited_uris.remove(uri)

    def _build_asset_tree_from_data_sync(
        self, construction_data: Optional[_AssetConstructionData]
    ) -> Asset | None:
        """
        Synchronously and recursively builds an asset instance (and its dependencies)
        from the provided _AssetConstructionData.
        This method calls Asset.from_bytes and is intended to run in the
        thread where UI operations are safe (typically the main UI thread).
        """
        if not construction_data:
            return None
        logger.debug(
            f"BuildAssetTreeSync: Instantiating '{construction_data.uri.asset_id}' "
            f"of type '{construction_data.asset_class_type.__name__}' "
            f"(Thread: {threading.current_thread().name})."
        )

        # Recursively build instances for all dependencies first
        resolved_dependency_instances: Dict[AssetUri, Any] = {}
        for (
            dep_uri,
            dep_data_node,
        ) in construction_data.dependencies_data.items():
            resolved_dependency_instances[dep_uri] = (
                self._build_asset_tree_from_data_sync(dep_data_node)
            )

        # Now, instantiate the current asset using its data and resolved dependency instances
        asset_class = construction_data.asset_class_type
        return asset_class.from_bytes(
            construction_data.raw_data,
            construction_data.asset_id,
            resolved_dependency_instances,
        )

    def get(self, uri: Union[AssetUri, str], store: str = "local") -> Any:
        """
        Retrieves an asset by its URI (synchronous wrapper).
        IMPORTANT: Assumes this method is CALLED ONLY from the main UI thread
                   if Asset.from_bytes performs UI operations.
        """
        # Log entry with thread info for verification
        calling_thread_name = threading.current_thread().name
        logger.debug(
            f"AssetManager.get(uri='{uri}', store='{store}') called from thread: {calling_thread_name}"
        )
        if (
            QApplication.instance()
            and QThread.currentThread() is not QApplication.instance().thread()
        ):
            logger.warning(
                "AssetManager.get() called from a non-main thread! UI in from_bytes may fail!"
            )

        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        # Step 1: Fetch all data using asyncio.run (creates a new event loop for the async part)
        try:
            logger.debug(
                f"Get: Starting asyncio.run for data fetching of '{asset_uri_obj}'."
            )
            all_construction_data = asyncio.run(
                self._fetch_asset_construction_data_recursive_async(
                    asset_uri_obj, store, set()
                )
            )
            logger.debug(
                f"Get: asyncio.run for data fetching of '{asset_uri_obj}' completed."
            )
        except Exception as e:
            logger.error(
                f"Get: Error during asyncio.run data fetching for '{asset_uri_obj}': {e}",
                exc_info=False,
            )
            raise  # Re-raise the exception from the async part

        if all_construction_data is None:
            # This means the top-level asset itself was not found by _fetch_...
            raise FileNotFoundError(
                f"Asset '{asset_uri_obj}' not found in store '{store}'."
            )

        # Step 2: Synchronously build the asset tree (and call from_bytes)
        # This happens in the current thread (which is assumed to be the main UI thread)
        deps = all_construction_data.dependencies_data
        found_deps = sum(1 for d in deps.values() if d is None)
        logger.debug(
            f"Get: Starting synchronous asset tree build for '{asset_uri_obj}' "
            f"and {len(deps)} dependencies ({found_deps} found)"
        )
        final_asset = self._build_asset_tree_from_data_sync(
            all_construction_data
        )
        logger.debug(
            f"Get: Synchronous asset tree build for '{asset_uri_obj}' completed."
        )
        return final_asset

    async def get_async(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> Asset:
        """
        Retrieves an asset by its URI (asynchronous).
        NOTE: If Asset.from_bytes does UI work, this method should ideally be awaited
        from an asyncio loop that is integrated with the main UI thread (e.g., via QtAsyncio).
        If awaited from a plain worker thread's asyncio loop, from_bytes will run on that worker.
        """
        calling_thread_name = threading.current_thread().name
        logger.debug(
            f"AssetManager.get_async(uri='{uri}', store='{store}') called from thread: {calling_thread_name}"
        )

        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        all_construction_data = (
            await self._fetch_asset_construction_data_recursive_async(
                asset_uri_obj, store, set()
            )
        )

        if all_construction_data is None:
            raise FileNotFoundError(
                f"Asset '{asset_uri_obj}' not found in store '{store}' (async path)."
            )

        # Instantiation happens in the context of where this get_async was awaited.
        logger.debug(
            f"get_async: Building asset tree for '{asset_uri_obj}' in current async context."
        )
        return self._build_asset_tree_from_data_sync(all_construction_data)

    def get_raw(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> bytes:
        """Retrieves raw asset data by its URI (synchronous wrapper)."""
        logger.debug(
            f"AssetManager.get_raw(uri='{uri}', store='{store}') from T:{threading.current_thread().name}"
        )

        async def _fetch_raw_async():
            asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
            logger.debug(
                f"GetRawAsync (internal): Looking up store '{store}'. Available stores: {list(self.stores.keys())}"
            )
            try:
                selected_store = self.stores[store]
            except KeyError:
                raise ValueError(f"No store registered for name: {store}")
            return await selected_store.get(asset_uri_obj)

        try:
            return asyncio.run(_fetch_raw_async())
        except Exception as e:
            logger.error(
                f"GetRaw: Error during asyncio.run for '{uri}': {e}",
                exc_info=False,
            )
            raise

    async def get_raw_async(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> bytes:
        """Retrieves raw asset data by its URI (asynchronous)."""
        logger.debug(
            f"AssetManager.get_raw_async(uri='{uri}', store='{store}') from T:{threading.current_thread().name}"
        )
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
        selected_store = self.stores[store]
        return await selected_store.get(asset_uri_obj)

    def get_bulk(
        self, uris: Sequence[Union[AssetUri, str]], store: str = "local"
    ) -> List[Any]:
        """Retrieves multiple assets by their URIs (synchronous wrapper)."""
        logger.debug(
            f"AssetManager.get_bulk for {len(uris)} URIs from store '{store}'"
        )

        async def _fetch_all_construction_data_bulk_async():
            tasks = [
                self._fetch_asset_construction_data_recursive_async(
                    AssetUri(u) if isinstance(u, str) else u, store, set()
                )
                for u in uris
            ]
            # Gather all construction data concurrently
            # return_exceptions=True means results list can contain exceptions
            return await asyncio.gather(*tasks, return_exceptions=True)

        try:
            logger.debug("GetBulk: Starting bulk data fetching")
            all_construction_data_list = asyncio.run(
                _fetch_all_construction_data_bulk_async()
            )
            logger.debug("GetBulk: bulk data fetching completed")
        except (
            Exception
        ) as e:  # Should ideally not happen if gather returns exceptions
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
                assets.append(
                    self._build_asset_tree_from_data_sync(data_or_exc)
                )
            elif (
                data_or_exc is None
            ):  # From _fetch_... returning None for not found
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
        self, uris: Sequence[Union[AssetUri, str]], store: str = "local"
    ) -> List[Any]:
        """Retrieves multiple assets by their URIs (asynchronous)."""
        logger.debug(
            f"AssetManager.get_bulk_async for {len(uris)} URIs from store '{store}'"
        )
        tasks = [
            self._fetch_asset_construction_data_recursive_async(
                AssetUri(u) if isinstance(u, str) else u, store, set()
            )
            for u in uris
        ]
        all_construction_data_list = await asyncio.gather(
            *tasks, return_exceptions=True
        )

        assets = []
        for i, data_or_exc in enumerate(all_construction_data_list):
            if isinstance(data_or_exc, _AssetConstructionData):
                assets.append(
                    self._build_asset_tree_from_data_sync(data_or_exc)
                )
            elif (
                isinstance(data_or_exc, FileNotFoundError)
                or data_or_exc is None
            ):
                assets.append(None)
            elif isinstance(data_or_exc, Exception):
                assets.append(data_or_exc)  # Caller must check
        return assets

    def fetch(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: str = "local",
    ) -> List[Asset]:
        """Fetches asset instances based on type, limit, and offset (synchronous)."""
        logger.debug(f"Fetch(type='{asset_type}', store='{store}')")
        # list_assets is sync, uses asyncio.run internally for its async part
        asset_uris = self.list_assets(asset_type, limit, offset, store)
        # get_bulk is sync, uses asyncio.run internally for its async part
        results = self.get_bulk(asset_uris, store)
        # Filter out non-Asset objects (e.g., None for not found, or exceptions if collected)
        # Note: get_bulk should now re-raise exceptions, so we might not see them here.
        return [asset for asset in results if isinstance(asset, Asset)]

    async def fetch_async(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: str = "local",
    ) -> List[Asset]:
        """Fetches asset instances based on type, limit, and offset (asynchronous)."""
        logger.debug(f"FetchAsync(type='{asset_type}', store='{store}')")
        asset_uris = await self.list_assets_async(
            asset_type, limit, offset, store
        )
        results = await self.get_bulk_async(asset_uris, store)
        return [asset for asset in results if isinstance(asset, Asset)]

    def list_assets(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: str = "local",
    ) -> List[AssetUri]:
        logger.debug(f"ListAssets(type='{asset_type}', store='{store}')")
        return asyncio.run(
            self.list_assets_async(asset_type, limit, offset, store)
        )

    async def list_assets_async(
        self,
        asset_type: Optional[str] = None,
        limit: Optional[int] = None,
        offset: Optional[int] = None,
        store: str = "local",
    ) -> List[AssetUri]:
        logger.debug(
            f"ListAssetsAsync executing for type='{asset_type}', store='{store}'"
        )
        logger.debug(
            f"ListAssetsAsync: Looking up store '{store}'. Available stores: {list(self.stores.keys())}"
        )
        try:
            selected_store = self.stores[store]
        except KeyError:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.list_assets(asset_type, limit, offset)

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

        data = obj.to_bytes()
        return await self.add_raw_async(uri.asset_type, uri.asset_id, data, store)
    def add(self, obj: Asset, store: str = "local") -> AssetUri:
        """Synchronous wrapper for adding an asset to the store."""
        logger.debug(f"Add: Adding {type(obj).__name__} to store '{store}' from T:{threading.current_thread().name}")
        return asyncio.run(self.add_async(obj, store))

    async def add_raw_async(self, asset_type: str, asset_id: str, data: bytes, store: str = "local") -> AssetUri:
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
            logger.debug(f"AddRawAsync: Asset not found, creating new asset with {asset_type} and {asset_id}")
            uri = await selected_store.create(asset_type, asset_id, data)
        return uri

    def add_raw(self, asset_type: str, asset_id: str, data: bytes, store: str = "local") -> AssetUri:
        """Synchronous wrapper for adding raw asset data to the store."""
        logger.debug(f"AddRaw: type='{asset_type}', id='{asset_id}', store='{store}' from T:{threading.current_thread().name}")
        try:
            return asyncio.run(self.add_raw_async(asset_type, asset_id, data, store))
        except Exception as e:
            logger.error(f"AddRaw: Error for type='{asset_type}', id='{asset_id}': {e}", exc_info=False)
            raise

    def delete(self, uri: Union[AssetUri, str], store: str = "local") -> None:
        logger.debug(f"Delete URI '{uri}' from store '{store}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        async def _do_delete_async():
            selected_store = self.stores[store]
            await selected_store.delete(asset_uri_obj)

        asyncio.run(_do_delete_async())

    async def delete_async(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> None:
        logger.debug(f"DeleteAsync URI '{uri}' from store '{store}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri
        selected_store = self.stores[store]
        await selected_store.delete(asset_uri_obj)


    # --- Is Empty ---
    async def is_empty_async(
        self, asset_type: Optional[str] = None, store: str = "local"
    ) -> bool:
        """Checks if the asset store has any assets of a given type (asynchronous)."""
        logger.debug(f"IsEmptyAsync: type='{asset_type}', store='{store}'")
        logger.debug(
            f"IsEmptyAsync: Looking up store '{store}'. Available stores: {list(self.stores.keys())}"
        )
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.is_empty(asset_type)

    def is_empty(
        self, asset_type: Optional[str] = None, store: str = "local"
    ) -> bool:
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

    # --- List Versions ---
    async def list_versions_async(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> List[AssetUri]:
        """Lists available versions for a given asset URI (asynchronous)."""
        logger.debug(f"ListVersionsAsync: uri='{uri}', store='{store}'")
        asset_uri_obj = AssetUri(uri) if isinstance(uri, str) else uri

        logger.debug(
            f"ListVersionsAsync: Looking up store '{store}'. Available stores: {list(self.stores.keys())}"
        )
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.list_versions(asset_uri_obj)

    def list_versions(
        self, uri: Union[AssetUri, str], store: str = "local"
    ) -> List[AssetUri]:
        """Lists available versions for a given asset URI (synchronous wrapper)."""
        logger.debug(
            f"ListVersions: uri='{uri}', store='{store}' from T:{threading.current_thread().name}"
        )
        try:
            return asyncio.run(self.list_versions_async(uri, store))
        except Exception as e:
            logger.error(
                f"ListVersions: Error for uri='{uri}', store='{store}': {e}",
                exc_info=False,
            )  # Changed exc_info to False
            return []  # Return empty list on error to satisfy type hint

    def get_registered_asset_types(self) -> List[str]:
        """Returns a list of registered asset type names."""
        return list(self._asset_classes.keys())
