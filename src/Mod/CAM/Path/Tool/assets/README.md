# Asset Management Module

This module implements an asset manager that provides methods for storing,
updating, deleting, and reveiving assets. Assets are arbitrary data.

## API usage example

```python
import pathlib
from typing import Any, Mapping, List, Type
from Path.Tool.assets import AssetManager, VersionedLocalStore, AssetUri, Asset

# Define a simple Material class implementing the Asset interface
class Material(Asset):
    asset_type: str = "material"

    def __init__(self, name: str):
        self.name = name

    def get_id() -> str:
        return self.name.lower().replace(" ", "-")

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def from_bytes(cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]) -> Material:
        return cls(data.decode('utf-8'))

    def to_bytes(self) -> bytes:
        return self.name.encode('utf-8')

manager = AssetManager()

# Register VersionedLocalStore and the simple asset class
manager.register_store(VersionedLocalStore("local", pathlib.Path("/tmp/assets")))
manager.register_asset(Material)

# Create and get an asset
asset_uri = manager.create(Material("Copper"))
print(f"Stored with URI: {asset_uri}")
retrieved_asset = manager.get(asset_uri)
print(f"Retrieved: {retrieved_asset}")
```

## Class diagram

```mermaid
classDiagram
    direction LR

    %% -------------- Asset Manager Module --------------
    note for AssetManager "AssetUri structure:
        &lt;asset_type&gt;:\//&lt;asset_id&gt;[/&lt;version&gt;]<br/>
        Examples:
        material:\//1234567/1
        toolbitshape:\//endmill/1
        material:\//aluminium-6012/2"

    class AssetManager["AssetManager
    <small>Creates, assembles or deletes assets from URIs</small>"] {
        stores: Mapping[str, AssetStore]   // maps protocol to store
        _asset_classes: Mapping[str, Asset]   // maps asset type to Asset
        register_store(store: AssetStore)
        register_asset(asset: Asset) // Keyed by adapter.asset_name
        async get_async(uri: AssetUri, store: str = 'local') Any
        get(uri: AssetUri, store: str = 'local') Any
        async delete_async(uri: AssetUri, store: str = 'local')
        delete(uri: AssetUri, store: str = 'local')
        async create_async(store: str, obj: Any, store = 'local') AssetUri // Returns URI of created asset
        create(store: str, obj: Any, store = 'local') AssetUri // Returns URI of created asset
        async update_async(uri: AssetUri, obj: Any, store: str = 'local') AssetUri // Updates asset at URI
        update(uri: AssetUri, obj: Any, store: str = 'local') AssetUri // Updates asset at URI
        async create_raw_async(asset_type: str, asset_id: str, data: bytes, store: str = 'local') AssetUri
        create_raw(asset_type: str, asset_id: str, data: bytes, store: str = 'local') AssetUri
        async get_raw_async(uri: AssetUri | str, store: str = 'local') bytes
        get_raw(uri: AssetUri | str, store: str = 'local') bytes
        async is_empty_async(store: str | None = None, asset_type: str | None = None, store: str = 'local') bool
        is_empty(store: str | None = None, asset_type: str | None = None, store: str = 'local') bool
        async list_assets_async(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions_async(uri: AssetUri | AssetUriStr) List[AssetUri]
        list_versions(uri: AssetUri | AssetUriStr) List[AssetUri]
    }

    class AssetStore["AssetStore
    <small>Stores/Retrieves assets as raw bytes</small>"] {
        <<abstract>>
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions(uri: AssetUri | AssetUriStr) List[AssetUri]
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
        async list_versions(uri: AssetUri | AssetUriStr) List[AssetUri]
        async is_empty(asset_type: str | None = None) bool
    }
    VersionedLocalStore <|-- AssetStore: is

    class FlatLocalStore["FlatLocalStore
    <small>Stores/Retrieves assets in a flat local directory, using a mapping
    of asset types to file extensions.</small>"] {
        base_dir: pathlib.Path
        type_to_extension: Mapping[str, str]
        async get(uri: AssetUri) bytes
        async delete(uri: AssetUri)
        async create(asset_type: str, asset_id: str, data: bytes) AssetUri
        async update(uri: AssetUri, data: bytes) AssetUri
        async list_assets(asset_type: str | None = None, limit: int | None = None, offset: int | None = None) List[AssetUri]
        async list_versions(uri: AssetUri | AssetUriStr) List[AssetUri]
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
        async list_versions(uri: AssetUri | AssetUriStr) List[AssetUri]
    }
    HttpStore <|-- AssetStore: is

    class Asset["Asset<br/><small>Common interface for all asset types</small>"] {
        <<abstract>>
        asset_type: str  // type of the asset type, e.g., toolbit
        
        get_id() str  // Returns a unique ID of the asset
        dependencies(data: bytes) List[AssetUri]  // Finds dependency URIs in bytes
        from_bytes(data: bytes, id: str, dependencies: Dict[AssetUri, Any]) Any  // Creates object from bytes and resolved dependencies
        to_bytes(obj: Any) bytes // Converts object to bytes
    }
    Asset *-- AssetManager: creates

    namespace AssetManagerModule {
        class AssetManager
        class AssetStore
        class VersionedLocalStore
        class FlatLocalStore
        class HttpStore
        class Asset
    }

    %% -------------- CAM Module (as an example) --------------
    class ToolBitShape["ToolBitShape<br/><small>for assets with type toolbitshape</small>"] {
        <<Asset>>
        asset_type: str = "toolbitshape"

        get_id() str  // Returns a unique ID
        dependencies(data: bytes) List[AssetUri]
        from_bytes(data: bytes, id: str, dependencies: Dict[AssetUri, Any]) ToolBitShape
        to_bytes(obj: ToolBitShape) bytes
    }
    ToolBitShape ..|> Asset: is

    class ToolBit["ToolBit<br/><small>for assets with type toolbit</small>"] {
        <<Asset>>
        asset_type: str = "toolbit"

        get_id() str  // Returns a unique ID
        dependencies(data: bytes) List[AssetUri]
        from_bytes(data: bytes, id: str, dependencies: Dict[AssetUri, Any]) ToolBit
        to_bytes(obj: ToolBit) bytes
    }
    ToolBit ..|> Asset: is
    ToolBit --> ToolBitShape: has

    namespace CAMModule {
        class ToolBitShape
        class ToolBit
    }

    %% -------------- Materials Module (as an example) --------------
    class Material["Material<br/><small>for assets with type material</small>"] {
        <<Asset>>
        asset_type: str = "material"

        get_id() str  // Returns a unique ID
        dependencies(data: bytes) List[AssetUri]
        from_bytes(data: bytes, id: str, dependencies: Dict[AssetUri, Any]) Material
        to_bytes(obj: Material) bytes
    }
    Material ..|> Asset: is

    namespace MaterialModule {
        class Material
        class Material
    }
```
