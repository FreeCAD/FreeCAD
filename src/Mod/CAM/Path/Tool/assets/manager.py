from typing import Dict, Any
from .store.base import AssetStore
from .adapter import AssetAdapter
from .uri import Uri


class AssetManager:
    def __init__(self):
        self.stores: Dict[str, AssetStore] = {}
        self.adapters: Dict[str, AssetAdapter] = {}

    def register_store(self, store: AssetStore):
        """Registers an AssetStore with the manager."""
        self.stores[store.protocol] = store

    def register_adapter(self, adapter: AssetAdapter):
        """Registers an AssetAdapter with the manager."""
        self.adapters[adapter.asset_name] = adapter

    async def get(self, uri: Uri | str) -> Any:
        """Retrieves an asset by its URI."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        store = self.stores.get(parsed_uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {parsed_uri.protocol}")

        data = await store.get(parsed_uri)

        adapter = self.adapters.get(parsed_uri.asset_type)
        if not adapter:
            raise ValueError(f"No adapter registered for asset type: {parsed_uri.asset_type}")

        dep_uris = adapter.dependencies(data)
        resolved_deps = {}
        for dep_uri in dep_uris:
            resolved_deps[dep_uri] = await self.get(dep_uri) # Recursive get

        return adapter.create(data, resolved_deps)

    async def delete(self, uri: Uri | str):
        """Deletes an asset by its URI."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        store = self.stores.get(parsed_uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {parsed_uri.protocol}")

        await store.delete(parsed_uri)

    async def create(self, store_protocol: str, obj: Any) -> Uri:
        """Creates a new asset from an object."""
        adapter = None
        for adpt in self.adapters.values():
            if isinstance(obj, adpt.asset_class):
                adapter = adpt
                break

        if not adapter:
            raise ValueError(
                f"No adapter registered for object type: {type(obj)}"
            )

        serialized_data = adapter.serialize(obj)

        store = self.stores.get(store_protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {store_protocol}")

        # Get the asset_id from the adapter
        asset_id = adapter.id_of(obj)

        return await store.create(adapter.asset_name, asset_id, serialized_data)

    async def update(self, uri: Uri | str, obj) -> Uri:
        """Updates an existing asset."""
        if isinstance(uri, str):
            parsed_uri = Uri(uri)
        else:
            parsed_uri = uri

        adapter = None
        for adpt in self.adapters.values():
            if isinstance(obj, adpt.asset_class):
                adapter = adpt
                break

        if not adapter:
            raise ValueError(
                f"No adapter registered for object type: {type(obj)}"
            )

        serialized_data = adapter.serialize(obj)

        store = self.stores.get(parsed_uri.protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {parsed_uri.protocol}")

        return await store.update(parsed_uri, serialized_data)

    async def create_raw(self,
                         store_protocol: str,
                         asset_type: str,
                         asset_id: str,
                         data: bytes) -> Uri:
        """Creates a new asset with raw data."""
        store = self.stores.get(store_protocol)
        if not store:
            raise ValueError(f"No store registered for protocol: {store_protocol}")
        return await store.create(asset_type, asset_id, data)

    async def get_raw(self, uri: Uri | str) -> bytes:
        """Retrieves raw asset data by its URI."""
        if isinstance(uri, str):
            uri = Uri(uri)

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
