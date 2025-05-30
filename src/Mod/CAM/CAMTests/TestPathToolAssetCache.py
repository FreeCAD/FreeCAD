# -*- coding: utf-8 -*-
import unittest
import asyncio
import hashlib
from typing import Any, Type, Optional, List, Mapping
from Path.Tool.assets.cache import AssetCache, CacheKey
from Path.Tool.assets import (
    AssetManager,
    Asset,
    AssetUri,
    AssetSerializer,
    DummyAssetSerializer,
    MemoryStore,
)


class MockAsset(Asset):
    asset_type: str = "mock_asset"
    _build_counter = 0

    def __init__(
        self,
        asset_id: str,
        raw_data: bytes,
        dependencies: Optional[Mapping[AssetUri, Any]] = None,
    ):
        super().__init__()  # Initialize Asset ABC
        self._asset_id = asset_id  # Store id internally
        self.raw_data_content = raw_data
        self.resolved_dependencies = dependencies or {}
        MockAsset._build_counter += 1
        self.build_id = MockAsset._build_counter

    def get_id(self) -> str:  # Implement abstract method
        return self._asset_id

    # get_uri() is inherited from Asset and uses self.asset_type and self.get_id()

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: AssetSerializer) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        # This mock implementation handles the simple "dep:" format
        data_str = data.decode()
        if data_str.startswith("dep:"):
            try:
                # Get content after the first "dep:"
                dep_content = data_str.split(":", 1)[1]
            except IndexError:
                # This case should ideally not be reached if startswith("dep:") is true
                # and there's content after "dep:", but good for robustness.
                return []

            dep_uri_strings = dep_content.split(",")
            uris = []
            for uri_string in dep_uri_strings:
                uri_string = uri_string.strip()  # Remove leading/trailing whitespace
                if not uri_string:
                    continue
                try:
                    uris.append(AssetUri(uri_string))
                except ValueError:
                    # This print will now show the full problematic uri_string
                    print(f"Warning: Could not parse mock dependency URI: '{uri_string}'")
            return uris
        return []

    @classmethod
    def from_bytes(
        cls: Type["MockAsset"],
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Any]],
        serializer: AssetSerializer,
    ) -> "MockAsset":
        return cls(asset_id=id, raw_data=data, dependencies=dependencies)

    def to_bytes(self, serializer: AssetSerializer) -> bytes:  # Implement abstract method
        return self.raw_data_content


class MockAssetB(Asset):  # New mock asset class for type 'mock_asset_b'
    asset_type: str = "mock_asset_b"
    _build_counter = 0  # Separate counter if needed, or share MockAsset's

    def __init__(
        self,
        asset_id: str,
        raw_data: bytes,
        dependencies: Optional[Mapping[AssetUri, Any]] = None,
    ):
        super().__init__()
        self._asset_id = asset_id
        self.raw_data_content = raw_data
        self.resolved_dependencies = dependencies or {}
        MockAssetB._build_counter += 1
        self.build_id = MockAssetB._build_counter

    def get_id(self) -> str:
        return self._asset_id

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: AssetSerializer) -> List[AssetUri]:
        # Keep simple, or adapt if MockAssetB has different dep logic
        return []

    @classmethod
    def from_bytes(
        cls: Type["MockAssetB"],
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Any]],
        serializer: AssetSerializer,
    ) -> "MockAssetB":
        return cls(asset_id=id, raw_data=data, dependencies=dependencies)

    def to_bytes(self, serializer: AssetSerializer) -> bytes:
        return self.raw_data_content


def _get_raw_data_hash(raw_data: bytes) -> int:
    return int(hashlib.sha256(raw_data).hexdigest(), 16)


class TestPathToolAssetCache(unittest.TestCase):
    def setUp(self):
        self.cache = AssetCache(max_size_bytes=1000)

    def test_put_and_get_simple(self):
        key = CacheKey("store1", "mock_asset://id1", _get_raw_data_hash(b"data1"), ("dep1",))
        asset_obj = MockAsset(asset_id="id1", raw_data=b"data1")
        self.cache.put(key, asset_obj, len(b"data1"), {"dep_uri_str"})

        retrieved = self.cache.get(key)
        self.assertIsNotNone(retrieved)
        # Assuming retrieved is MockAsset, it will have get_id()
        self.assertEqual(retrieved.get_id(), "id1")

    def test_get_miss(self):
        key = CacheKey("store1", "mock_asset://id1", _get_raw_data_hash(b"data1"), tuple())
        self.assertIsNone(self.cache.get(key))

    def test_lru_eviction(self):
        asset_data_size = 300
        asset1_data = b"a" * asset_data_size
        asset2_data = b"b" * asset_data_size
        asset3_data = b"c" * asset_data_size
        asset4_data = b"d" * asset_data_size

        key1 = CacheKey("s", "mock_asset://id1", _get_raw_data_hash(asset1_data), tuple())
        key2 = CacheKey("s", "mock_asset://id2", _get_raw_data_hash(asset2_data), tuple())
        key3 = CacheKey("s", "mock_asset://id3", _get_raw_data_hash(asset3_data), tuple())
        key4 = CacheKey("s", "mock_asset://id4", _get_raw_data_hash(asset4_data), tuple())

        self.cache.put(key1, MockAsset("id1", asset1_data), asset_data_size, set())
        self.cache.put(key2, MockAsset("id2", asset2_data), asset_data_size, set())
        self.cache.put(key3, MockAsset("id3", asset3_data), asset_data_size, set())

        self.assertEqual(self.cache.current_size_bytes, 3 * asset_data_size)
        self.assertIsNotNone(self.cache.get(key1))  # Access key1 to make it MRU

        # Adding key4 should evict key2 (oldest after key1 accessed)
        self.cache.put(key4, MockAsset("id4", asset4_data), asset_data_size, set())
        self.assertEqual(self.cache.current_size_bytes, 3 * asset_data_size)
        self.assertIsNotNone(self.cache.get(key1))
        self.assertIsNone(self.cache.get(key2))  # Evicted
        self.assertIsNotNone(self.cache.get(key3))
        self.assertIsNotNone(self.cache.get(key4))

    def test_invalidate_direct(self):
        key = CacheKey("s", "mock_asset://id1", _get_raw_data_hash(b"data"), tuple())
        self.cache.put(key, MockAsset("id1", b"data"), 4, set())
        retrieved = self.cache.get(key)  # Ensure it's there
        self.assertIsNotNone(retrieved)

        self.cache.invalidate_for_uri("mock_asset://id1")
        self.assertIsNone(self.cache.get(key))
        self.assertEqual(self.cache.current_size_bytes, 0)

    def test_invalidate_recursive(self):
        data_a = b"data_a_dep:mock_asset_b://idB"  # A depends on B
        data_b = b"data_b"
        data_c = b"data_c_dep:mock_asset_a://idA"  # C depends on A

        uri_a_str = "mock_asset_a://idA"
        uri_b_str = "mock_asset_b://idB"
        uri_c_str = "mock_asset_c://idC"

        key_b = CacheKey("s", uri_b_str, _get_raw_data_hash(data_b), tuple())
        key_a = CacheKey("s", uri_a_str, _get_raw_data_hash(data_a), (uri_b_str,))
        key_c = CacheKey("s", uri_c_str, _get_raw_data_hash(data_c), (uri_a_str,))

        self.cache.put(key_b, MockAsset("idB", data_b), len(data_b), set())
        self.cache.put(key_a, MockAsset("idA", data_a), len(data_a), {uri_b_str})
        self.cache.put(key_c, MockAsset("idC", data_c), len(data_c), {uri_a_str})

        self.assertIsNotNone(self.cache.get(key_a))
        self.assertIsNotNone(self.cache.get(key_b))
        self.assertIsNotNone(self.cache.get(key_c))

        self.cache.invalidate_for_uri(uri_b_str)  # Invalidate B

        self.assertIsNone(self.cache.get(key_a), "Asset A should be invalidated")
        self.assertIsNone(self.cache.get(key_b), "Asset B should be invalidated")
        self.assertIsNone(self.cache.get(key_c), "Asset C should be invalidated")
        self.assertEqual(self.cache.current_size_bytes, 0)

    def test_clear_cache(self):
        key = CacheKey("s", "mock_asset://id1", _get_raw_data_hash(b"data"), tuple())
        self.cache.put(key, MockAsset("id1", b"data"), 4, set())
        self.assertNotEqual(self.cache.current_size_bytes, 0)

        self.cache.clear()
        self.assertIsNone(self.cache.get(key))
        self.assertEqual(self.cache.current_size_bytes, 0)
        self.assertEqual(len(self.cache._cache_dependencies_map), 0)
        self.assertEqual(len(self.cache._cache_dependents_map), 0)


class TestPathToolAssetCacheIntegration(unittest.TestCase):
    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

        self.manager = AssetManager(cache_max_size_bytes=10 * 1024)  # 10KB cache
        self.store_name = "test_store"
        self.store = MemoryStore(name=self.store_name)
        self.manager.register_store(self.store, cacheable=True)
        self.manager.register_asset(MockAsset, DummyAssetSerializer)
        self.manager.register_asset(MockAssetB, DummyAssetSerializer)  # Register the new mock type
        MockAsset._build_counter = 0
        MockAssetB._build_counter = 0

    def tearDown(self):
        self.loop.close()

    def _run_async(self, coro):
        return self.loop.run_until_complete(coro)

    def test_get_caches_asset(self):
        uri_str = "mock_asset://asset1"
        raw_data = b"asset1_data"
        self._run_async(self.store.create("mock_asset", "asset1", raw_data))

        # First get - should build and cache
        asset1 = self.manager.get(uri_str, store=self.store_name)
        self.assertIsInstance(asset1, MockAsset)
        self.assertEqual(asset1.get_id(), "asset1")
        self.assertEqual(MockAsset._build_counter, 1)  # Built once

        # Second get - should hit cache
        asset2 = self.manager.get(uri_str, store=self.store_name)
        self.assertIsInstance(asset2, MockAsset)
        self.assertEqual(asset2.get_id(), "asset1")
        self.assertEqual(MockAsset._build_counter, 1)  # Still 1, not rebuilt
        self.assertIs(asset1, asset2)  # Should be the same instance from cache

    def test_get_respects_depth_in_cache_key(self):
        uri_str = "mock_asset://asset_depth"
        # A depends on B (mock_asset_b://dep_b)
        raw_data_a = b"dep:mock_asset_b://dep_b"
        raw_data_b = b"dep_b_data"

        self._run_async(self.store.create("mock_asset", "asset_depth", raw_data_a))
        self._run_async(self.store.create("mock_asset_b", "dep_b", raw_data_b))

        # Get with depth=0 (shallow)
        asset_shallow = self.manager.get(uri_str, store=self.store_name, depth=0)
        self.assertIsInstance(asset_shallow, MockAsset)
        self.assertEqual(len(asset_shallow.resolved_dependencies), 0)
        self.assertEqual(MockAsset._build_counter, 1)  # asset_depth built

        # Get with depth=None (full)
        asset_full = self.manager.get(uri_str, store=self.store_name, depth=None)
        self.assertIsInstance(asset_full, MockAsset)
        self.assertEqual(len(asset_full.resolved_dependencies), 1)
        # asset_depth (MockAsset) built twice (once shallow, once full)
        self.assertEqual(MockAsset._build_counter, 2)
        # dep_b (MockAssetB) built once as a dependency of the full asset_depth
        self.assertEqual(MockAssetB._build_counter, 1)

        # Get shallow again - should hit shallow cache
        asset_shallow_2 = self.manager.get(uri_str, store=self.store_name, depth=0)
        self.assertIs(asset_shallow, asset_shallow_2)
        self.assertEqual(MockAsset._build_counter, 2)  # No new MockAsset builds
        self.assertEqual(MockAssetB._build_counter, 1)  # No new MockAssetB builds

        # Get full again - should hit full cache
        asset_full_2 = self.manager.get(uri_str, store=self.store_name, depth=None)
        self.assertIs(asset_full, asset_full_2)
        self.assertEqual(MockAsset._build_counter, 2)  # No new MockAsset builds
        self.assertEqual(MockAssetB._build_counter, 1)  # No new MockAssetB builds

    def test_update_invalidates_cache(self):
        uri_str = "mock_asset://asset_upd"
        raw_data_v1 = b"version1"
        raw_data_v2 = b"version2"
        asset_uri = AssetUri(uri_str)  # Use real AssetUri

        self._run_async(self.store.create(asset_uri.asset_type, asset_uri.asset_id, raw_data_v1))

        asset_v1 = self.manager.get(asset_uri, store=self.store_name)
        self.assertEqual(asset_v1.raw_data_content, raw_data_v1)
        self.assertEqual(MockAsset._build_counter, 1)

        # Update the asset in the store (MemoryStore creates new version)
        # For this test, let's simulate an update by re-adding with add_raw
        # which should trigger invalidation.
        # Note: MemoryStore's update creates a new version, so get() would get latest.
        # To test invalidation of the *exact* cached object, we'd need to ensure
        # the cache key changes (e.g. raw_data_hash).
        # Let's use add_raw which calls invalidate.
        self.manager.add_raw(
            asset_uri.asset_type, asset_uri.asset_id, raw_data_v2, store=self.store_name
        )

        # Get again - should rebuild because v1 was invalidated
        # And MemoryStore's get() for a URI without version gets latest.
        # The add_raw invalidates based on URI (type+id), so all versions of it.
        asset_v2 = self.manager.get(asset_uri, store=self.store_name)
        self.assertEqual(asset_v2.raw_data_content, raw_data_v2)
        self.assertEqual(MockAsset._build_counter, 2)  # Rebuilt

    def test_delete_invalidates_cache(self):
        uri_str = "mock_asset://asset_del"
        raw_data = b"delete_me"
        asset_uri = AssetUri(uri_str)
        self._run_async(self.store.create(asset_uri.asset_type, asset_uri.asset_id, raw_data))

        asset1 = self.manager.get(asset_uri, store=self.store_name)
        self.assertIsNotNone(asset1)
        self.assertEqual(MockAsset._build_counter, 1)

        self.manager.delete(asset_uri, store=self.store_name)

        with self.assertRaises(FileNotFoundError):
            self.manager.get(asset_uri, store=self.store_name)
        # Check build counter didn't increase due to trying to get deleted asset
        self.assertEqual(MockAsset._build_counter, 1)


if __name__ == "__main__":
    unittest.main()
