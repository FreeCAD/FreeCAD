from .asset import Asset
from .manager import AssetManager
from .uri import AssetUri
from .store.base import AssetStore
from .store.flat import FlatLocalStore
from .store.memory import MemoryStore
from .store.versioned import VersionedLocalStore

asset_manager = AssetManager()
memory_store = MemoryStore("test")
asset_manager.register_store(memory_store)

__all__ = [
    "Asset",
    "AssetUri",
    "asset_manager",
    "AssetStore",
    "FlatLocalStore",
    "MemoryStore",
    "VersionedLocalStore",
]
