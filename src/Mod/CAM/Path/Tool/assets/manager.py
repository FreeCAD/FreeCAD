import asyncio
from typing import Dict, Any, Type, Optional, List, Sequence
from .store.base import AssetStore
from .asset import Asset
from .uri import AssetUri


class AssetManager:
    def __init__(self):
        self.stores: Dict[str, AssetStore] = {}
        self._asset_classes: Dict[str, Type[Asset]] = {}

    def register_store(self, store: AssetStore):
        """Registers an AssetStore with the manager."""
        self.stores[store.name] = store

    def register_asset(self, asset_class: Type[Asset]):
        """Registers an Asset class with the manager."""
        if not issubclass(asset_class, Asset):
             raise TypeError(f"Registered item must be a subclass of Asset, not {asset_class.__name__}")
        if not hasattr(asset_class, "asset_type") \
            or not isinstance(asset_class.asset_type, str):
            raise TypeError(
                f"Asset must have a class attribute 'asset_type: str', {asset_class.__name__} does not."
            )
        self._asset_classes[asset_class.asset_type] = asset_class

    async def get_async(self, uri: AssetUri | str, store: str = 'local') -> Any:
        """Retrieves an asset by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        data = await selected_store.get(uri)

        asset_class = self._asset_classes.get(uri.asset_type)
        if not asset_class:
            raise ValueError(f"No asset class registered for asset type: {uri.asset_type}")

        dep_uris = asset_class.dependencies(data)
        resolved_deps = {}
        for dep_uri in dep_uris:
            try:
                dep = await self.get_async(dep_uri, store=store)
            except FileNotFoundError:
                dep = None

            resolved_deps[dep_uri] = dep

        # Pass the asset ID from the URI to the from_bytes method
        return asset_class.from_bytes(data, uri.asset_id, resolved_deps)

    def get(self, uri: AssetUri | str, store: str = 'local') -> Any:
        """Retrieves an asset by its URI (synchronous wrapper)."""
        return asyncio.run(self.get_async(uri, store))

    async def get_bulk_async(self, uris: Sequence[AssetUri | str], store: str = 'local') -> List[Any]:
        """Retrieves multiple assets by their URIs."""
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        # Use asyncio.Semaphore to limit concurrency
        limit = 3
        semaphore = asyncio.Semaphore(limit)

        async def get_with_semaphore(uri):
            async with semaphore:
                try:
                    return await self.get_async(uri, store=store)
                except FileNotFoundError:
                    return None # Handle FileNotFoundError specifically

        tasks = [get_with_semaphore(uri) for uri in uris]
        results = await asyncio.gather(*tasks, return_exceptions=True)

        # Handle potential exceptions other than FileNotFoundError
        assets = []
        for result in results:
            if isinstance(result, Exception) and not isinstance(result, FileNotFoundError):
                 # Re-raise other exceptions
                 raise result
            else:
                assets.append(result)
        return assets

    def get_bulk(self, uris: Sequence[AssetUri | str], store: str = 'local') -> List[Any]:
        """Retrieves multiple assets by their URIs (synchronous wrapper)."""
        return asyncio.run(self.get_bulk_async(uris, store))

    async def fetch_async(self,
                          asset_type: Optional[str] = None,
                          limit: Optional[int] = None,
                          offset: Optional[int] = None,
                          store: str = 'local') -> List[Asset]:
        """Fetches asset instances based on type, limit, and offset."""
        # First, list the asset URIs
        uris = await self.list_assets_async(asset_type, limit, offset, store)

        # Then, get the actual asset instances using get_bulk_async
        assets = await self.get_bulk_async(uris, store)

        # Filter out None values if any assets were not found
        return [asset for asset in assets if asset is not None]

    def fetch(self,
              asset_type: Optional[str] = None,
              limit: Optional[int] = None,
              offset: Optional[int] = None,
              store: str = 'local') -> List[Asset]:
        """Fetches asset instances (synchronous wrapper)."""
        return asyncio.run(self.fetch_async(asset_type, limit, offset, store))

    async def delete_async(self, uri: AssetUri | str, store: str = 'local'):
        """Deletes an asset by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        await selected_store.delete(uri)

    def delete(self, uri: AssetUri | str, store: str = 'local'):
        """Deletes an asset by its URI (synchronous wrapper)."""
        return asyncio.run(self.delete_async(uri, store))

    async def create_async(self, obj: Any, store: str = 'local') -> AssetUri:
        """Creates a new asset from an object."""
        # Find a registered asset class that the object is an instance of
        registered_asset_class = None
        for registered_cls in self._asset_classes.values():
            if isinstance(obj, registered_cls):
                registered_asset_class = registered_cls
                break # Found a matching registered class

        if not registered_asset_class:
            raise ValueError(
                f"Object of type {type(obj)} is not an instance of any registered asset class."
            )

        serialized_data = obj.to_bytes()

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        asset_id = obj.get_id() # Call instance method

        # Use the asset_type from the *registered* asset class found via isinstance
        return await selected_store.create(registered_asset_class.asset_type, asset_id, serialized_data)

    def create(self, obj: Any, store: str = 'local') -> AssetUri:
        """Creates a new asset from an object (synchronous wrapper)."""
        return asyncio.run(self.create_async(obj, store))

    async def update_async(self, uri: AssetUri | str, obj, store: str) -> AssetUri:
        """Updates an existing asset."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        asset_class = obj.__class__
        if asset_class not in self._asset_classes.values():
            raise ValueError(
                f"No asset class registered for object type: {type(obj)}"
            )

        serialized_data = obj.to_bytes() # Call instance method

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        return await selected_store.update(uri, serialized_data)

    def update(self, uri: AssetUri | str, obj, store: str) -> AssetUri:
        """Updates an existing asset (synchronous wrapper)."""
        return asyncio.run(self.update_async(uri, obj, store))

    async def create_raw_async(self,
                               asset_type: str,
                               asset_id: str,
                               data: bytes,
                               store: str = 'local') -> AssetUri:
        """Creates a new asset with raw data."""
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.create(asset_type, asset_id, data)

    def create_raw(self,
                   asset_type: str,
                   asset_id: str,
                   data: bytes,
                   store: str = 'local') -> AssetUri:
        """Creates a new asset with raw data (synchronous wrapper)."""
        return asyncio.run(self.create_raw_async(asset_type, asset_id, data, store))

    async def get_raw_async(self, uri: AssetUri | str, store: str = 'local') -> bytes:
        """Retrieves raw asset data by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")

        return await selected_store.get(uri)

    def get_raw(self, uri: AssetUri | str, store: str = 'local') -> bytes:
        """Retrieves raw asset data by its URI (synchronous wrapper)."""
        return asyncio.run(self.get_raw_async(uri, store))

    async def is_empty_async(self, asset_type: str | None = None, store: str = 'local') -> bool:
        """Checks if the asset manager has any assets of a given type."""
        # Log to diagnose store registration error
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        return await selected_store.is_empty(asset_type)

    def is_empty(self, asset_type: str | None = None, store: str = 'local') -> bool:
        """Checks if the asset manager has any assets of a given type (synchronous wrapper)."""
        return asyncio.run(self.is_empty_async(asset_type, store))

    async def list_assets_async(self,
                                asset_type: Optional[str] = None,
                                limit: Optional[int] = None,
                                offset: Optional[int] = None,
                                store: str = 'local') -> List[AssetUri]:
        """Lists assets in a specific store, optionally filtered by type."""
        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        # Delegate the call to the store's list_assets method
        return await selected_store.list_assets(asset_type, limit, offset)

    def list_assets(self,
                    asset_type: Optional[str] = None,
                    limit: Optional[int] = None,
                    offset: Optional[int] = None,
                    store: str = 'local') -> List[AssetUri]:
        """Lists assets in a specific store, optionally filtered by type (synchronous wrapper)."""
        return asyncio.run(self.list_assets_async(asset_type, limit, offset, store))

    async def list_versions_async(self, uri: AssetUri | str, store: str = 'local') -> List[AssetUri]:
        """Lists available versions for a given asset URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        selected_store = self.stores.get(store)
        if not selected_store:
            raise ValueError(f"No store registered for name: {store}")
        # Delegate the call to the store's list_versions method
        return await selected_store.list_versions(uri)

    def list_versions(self, uri: AssetUri | str, store: str = 'local') -> List[AssetUri]:
        """Lists available versions for a given asset URI (synchronous wrapper)."""
        return asyncio.run(self.list_versions_async(uri, store))

    def get_registered_asset_types(self) -> list[str]:
        """Returns a list of registered asset types."""
        return list(self._asset_classes.keys())
