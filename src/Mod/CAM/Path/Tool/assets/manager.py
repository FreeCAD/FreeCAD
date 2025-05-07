from typing import Dict, Any, Type
from .store.base import AssetStore
from .asset import Asset
from .uri import AssetUri


class AssetManager:
    def __init__(self):
        self.stores: Dict[str, AssetStore] = {}
        self._asset_classes: Dict[str, Type[Asset]] = {}

    def register_store(self, store: AssetStore):
        """Registers an AssetStore with the manager."""
        self.stores[store.protocol] = store

    def register_asset(self, asset_class: Type[Asset]):
        """Registers an Asset class with the manager."""
        if not issubclass(asset_class, Asset):
             raise TypeError(f"Registered item must be a subclass of Asset, not {asset_class.__name__}")
        self._asset_classes[asset_class.asset_type] = asset_class

    async def get(self, uri: AssetUri | str) -> Any:
        """Retrieves an asset by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        store = self.stores.get(uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {uri.protocol}")

        data = await store.get(uri)

        asset_class = self._asset_classes.get(uri.asset_type)
        if not asset_class:
            raise ValueError(f"No asset class registered for asset type: {uri.asset_type}")

        dep_uris = asset_class.dependencies(data)
        resolved_deps = {}
        for dep_uri in dep_uris:
            resolved_deps[dep_uri] = await self.get(dep_uri) # Recursive get

        return asset_class.from_bytes(data, resolved_deps)

    async def delete(self, uri: AssetUri | str):
        """Deletes an asset by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        store = self.stores.get(uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {uri.protocol}")

        await store.delete(uri)

    async def create(self, store_protocol: str, obj: Any) -> AssetUri:
        """Creates a new asset from an object."""
        asset_class = obj.__class__
        if asset_class not in self._asset_classes.values():
            raise ValueError(
                f"No asset class registered for object type: {type(obj)}"
            )

        serialized_data = obj.to_bytes() # Call instance method

        store = self.stores.get(store_protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {store_protocol}")

        asset_id = obj.get_id() # Call instance method

        return await store.create(asset_class.asset_type, asset_id, serialized_data)

    async def update(self, uri: AssetUri | str, obj) -> AssetUri:
        """Updates an existing asset."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        asset_class = obj.__class__
        if asset_class not in self._asset_classes.values():
            raise ValueError(
                f"No asset class registered for object type: {type(obj)}"
            )

        serialized_data = obj.to_bytes() # Call instance method

        store = self.stores.get(uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {uri.protocol}")

        return await store.update(uri, serialized_data)

    async def create_raw(self,
                         store_protocol: str,
                         asset_type: str,
                         asset_id: str,
                         data: bytes) -> AssetUri:
        """Creates a new asset with raw data."""
        store = self.stores.get(store_protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {store_protocol}")
        return await store.create(asset_type, asset_id, data)

    async def get_raw(self, uri: AssetUri | str) -> bytes:
        """Retrieves raw asset data by its URI."""
        if isinstance(uri, str):
            uri = AssetUri(uri)

        store = self.stores.get(uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {uri.protocol}")

        return await store.get(uri)

    async def is_empty(self, store_protocol: str, asset_type: str | None = None) -> bool:
        """Checks if the asset manager has any assets of a given type."""
        store = self.stores.get(store_protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {store_protocol}")
        return await store.is_empty(asset_type)
