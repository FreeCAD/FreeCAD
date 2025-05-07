# Asset Management Module

This module implements an asset manager that provides methods for storing,
updating, deleting, and reveiving assets. Assets are arbitrary data.

## API usage example

```python
import asyncio
import pathlib
from typing import Any, Dict, List, Type
from Path.Tool.assets import AssetManager, AssetAdapter, VersionedLocalStore, AssetUri

# Define a simple Material class for the example
class Material:
    def __init__(self, name: str):
        self.name = name

# Define a simple MaterialAdapter for the example
class MaterialAdapter(AssetAdapter):
    asset_name: str = "material"
    asset_class: Type[Material] = Material

    def serialize(self, obj: Material) -> bytes:
        return obj.name.encode('utf-8')

    def dependencies(self, data: bytes) -> List[AssetUri]:
        return []

    def create(self, data: bytes, dependencies: Dict[AssetUri, Any]) -> Material:
        return Material(data.decode('utf-8'))

    def id_of(self, obj: Material) -> str:
        return obj.name.lower().replace(" ", "-")


async def main():
    manager = AssetManager()

    # Register VersionedLocalStore and the simple adapter
    manager.register_store(VersionedLocalStore("local", pathlib.Path("/tmp/assets")))
    manager.register_adapter(MaterialAdapter())

    # Create and get an asset
    asset_uri = await manager.create("local", Material("Copper"))
    print(f"Stored with URI: {asset_uri}")
    retrieved_asset = await manager.get(asset_uri)
    print(f"Retrieved: {retrieved_asset}")

# To run: asyncio.run(main())
```

## Class diagram

```mermaid
classDiagram
    direction LR

    %% -------------- Asset Manager Module --------------
    note for AssetManager "AssetUri structure:
        <protocol>://<domain>/<asset_type>/<asset>/<version>\?<params><br/>
        Examples:
        local://material/1234567/v1
        local://toolbitshape/endmill/v1
        https:\//assets.freecad.org/material/aluminium-6012/v2"

    class AssetManager["AssetManager
    <small>Creates, assembles or deletes assets from URIs</small>"] {
        stores: Mapping[str, AssetStore]   // maps protocol to store
        adapters: Mapping[str, AssetAdapter]   // maps asset type to adapter
        register_store(store: AssetStore)
        register_adapter(adapter: AssetAdapter) // Keyed by adapter.asset_name
        async get(uri: AssetUri) Any
        async delete(uri: AssetUri)
        async create(store_protocol: str, obj: Any) AssetUri // Returns URI of created asset
        async update(uri: AssetUri, obj: Any) AssetUri // Updates asset at URI
        async create_raw(store_protocol: str, asset_type: str, asset_id: str, data: bytes) AssetUri
        async get_raw(uri: AssetUri | str) bytes
        async is_empty(store_protocol: str, asset_type: str | None = None) bool
    }

    class AssetStore["AssetStore
    <small>Stores/Retrieves assets as raw bytes</small>"] {
        <<abstract>>
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions(uri: AssetUri | AssetUriStr) List[str]
        async is_empty(asset_type: str | None = None) bool
    }
    AssetStore *-- AssetManager: has many

    class VersionedLocalStore["VersionedLocalStore
    <small>Stores/Retrieves versioned assets locally (local://)</small>"] {
        _base_dir: pathlib.Path
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions(uri: AssetUri | AssetUriStr) List[str]
        async is_empty(asset_type: str | None = None) bool
    }
    VersionedLocalStore <|-- AssetStore: is

    class UnversionedLocalStore["UnversionedLocalStore
    <small>Stores/Retrieves unversioned assets locally</small>"] {
        _base_dir: pathlib.Path
        _file_extension: str
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async is_empty(asset_type: str | None = None) bool
    }
    UnversionedLocalStore <|-- AssetStore: is

    class FlatLocalStore["FlatLocalStore
    <small>Stores/Retrieves assets in a flat local directory</small>"] {
        _base_dir: pathlib.Path
        _file_extension: str
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async is_empty(asset_type: str | None = None) bool
    }
    FlatLocalStore <|-- AssetStore: is

    class HttpStore["HttpStore
    <small>Stores/Retrieves bytes for URIs starting with https://</small>"] {
        auth_data: Mapping
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions(uri: AssetUri | AssetUriStr) List[str]
    }
    HttpStore <|-- AssetStore: is

    class AssetAdapter["AssetAdapter<br/><small>Handles serialization/deserialization for a asset type</small>"] {
        <<abstract>>
        asset_name: str   // name of the asset type, e.g., toolbit
        asset_class: Type[Any]  // class of the asset type, e.g., ToolBit
        
        serialize(obj: Any) bytes // Converts object to bytes
        dependencies(data: bytes) List[AssetUri] // Finds dependency URIs in bytes
        create(data: bytes, dependencies: Dict[AssetUri, Any]) Any // Creates object from bytes and resolved dependencies
        id_of(obj: Any) str // Returns the unique ID of an asset object
    }
    AssetAdapter *-- AssetManager: has many

    namespace AssetManagerModule {
        class AssetManager
        class AssetStore
        class VersionedLocalStore
        class UnversionedLocalStore
        class FlatLocalStore
        class HttpStore
        class AssetAdapter
    }

    %% -------------- CAM Module (as an example) --------------
    class ToolBitShapeAdapter["ToolBitShapeAdapter<br/><small>for assets with type toolbitshape</small>"] {
        <<AssetAdapter>>
        asset_name: str = "toolbitshape"
        asset_class: Type = ToolBitShape
        serialize(obj: ToolBitShape) bytes
        dependencies(data: bytes) List[AssetUri]
        create(data: bytes, dependencies: Dict[AssetUri, Any]) ToolBitShape
        id_of(obj: ToolBitShape) str
    }
    ToolBitShapeAdapter --|> ToolBitShape: creates
    ToolBitShapeAdapter ..|> AssetAdapter: is

    class ToolBitAdapter["ToolBitAdapter<br/><small>for assets with type toolbit</small>"] {
        <<AssetAdapter>>
        asset_name: str = "toolbit"
        asset_class: Type = ToolBit
        serialize(obj: ToolBit) bytes
        dependencies(data: bytes) List[AssetUri]
        create(data: bytes, dependencies: Dict[AssetUri, Any]) ToolBit
        id_of(obj: ToolBit) str
    }
    ToolBitAdapter --|> ToolBit: creates
    ToolBitAdapter ..|> AssetAdapter: is
    ToolBit --> ToolBitShape: has

    namespace CAMModule {
        class ToolBitShapeAdapter
        class ToolBitAdapter
        class ToolBitShape
        class ToolBit
    }

    %% -------------- Materials Module (as an example) --------------
    class MaterialAdapter["MaterialAdapter<br/><small>for assets with type material</small>"] {
        <<AssetAdapter>>
        asset_name: str = "material"
        asset_class: Type = Material
        serialize(obj: Material) bytes
        dependencies(data: bytes) List[AssetUri]
        create(data: bytes, dependencies: Dict[AssetUri, Any]) Material
        id_of(obj: Material) str
    }
    MaterialAdapter --|> Material: creates
    MaterialAdapter ..|> AssetAdapter: is

    namespace MaterialModule {
        class MaterialAdapter
        class Material
    }
```
