# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

"""
AssetManager tests.
"""

import unittest
import asyncio
from unittest.mock import Mock
import pathlib
import tempfile
from typing import Any, Mapping, List, Type, Optional, cast
from Path.Tool.assets import (
    AssetManager,
    FileStore,
    Asset,
    AssetUri,
    MemoryStore,
    AssetSerializer,
    DummyAssetSerializer,
)


# Mock Asset class for testing
class MockAsset(Asset):
    asset_type: str = "mock_asset"

    def __init__(self, data: Any = None, id: str = "mock_id"):
        self._data = data
        self._id = id

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: Type[AssetSerializer]) -> List[AssetUri]:
        # Mock implementation doesn't use data or format for dependencies
        return []

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: Type[AssetSerializer],
    ) -> "MockAsset":
        # Create instance with provided id
        return cls(data, id)

    def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
        return self._data

    def get_id(self) -> str:
        return self._id

    def get_data(self) -> bytes:
        """Returns the raw data stored in the mock asset."""
        return self._data


# Mock Asset class with dependencies for testing deepcopy
class MockAssetWithDeps(Asset):
    asset_type: str = "mock_asset_with_deps"

    def __init__(
        self,
        data: Any = None,
        id: str = "mock_id",
        dependencies: Optional[Mapping[AssetUri, Asset]] = None,
    ):
        self._data = data
        self._id = id
        self._dependencies = dependencies or {}

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: Type[AssetSerializer]) -> List[AssetUri]:
        # Assuming data is a simple JSON string like '{"deps": ["uri1", "uri2"]}'
        try:
            import json

            data_str = data.decode("utf-8")
            data_dict = json.loads(data_str)
            dep_uris_str = data_dict.get("deps", [])
            return [AssetUri(uri_str) for uri_str in dep_uris_str]
        except Exception:
            return []

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: Type[AssetSerializer],
    ) -> "MockAssetWithDeps":
        # Create instance with provided id and resolved dependencies
        return cls(data, id, dependencies)

    def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
        # Serialize data and dependency URIs into a simple format
        try:
            import json

            dep_uri_strs = [str(uri) for uri in self._dependencies.keys()]
            data_dict = {"data": self._data.decode("utf-8"), "deps": dep_uri_strs}
            return json.dumps(data_dict).encode("utf-8")
        except Exception:
            return self._data  # Fallback if serialization fails

    def get_id(self) -> str:
        return self._id

    def get_data(self) -> bytes:
        """Returns the raw data stored in the mock asset."""
        return self._data

    def get_dependencies(self) -> Mapping[AssetUri, Asset]:
        """Returns the resolved dependencies."""
        return self._dependencies


class TestPathToolAssetManager(unittest.TestCase):
    def test_register_store(self):
        manager = AssetManager()
        mock_store_local = Mock()
        mock_store_local.name = "local"
        mock_store_remote = Mock()
        mock_store_remote.name = "remote"

        manager.register_store(mock_store_local)
        self.assertEqual(manager.stores["local"], mock_store_local)

        manager.register_store(mock_store_remote)
        self.assertEqual(manager.stores["remote"], mock_store_remote)

        # Test overwriting
        mock_store_local_new = Mock()
        mock_store_local_new.name = "local"
        manager.register_store(mock_store_local_new)
        self.assertEqual(manager.stores["local"], mock_store_local_new)

    def test_register_asset(self):
        manager = AssetManager()
        # Register the actual MockAsset class
        manager.register_asset(MockAsset, DummyAssetSerializer)
        self.assertEqual(manager._asset_classes[MockAsset.asset_type], MockAsset)

        # Test registering a different actual Asset class
        class AnotherMockAsset(Asset):
            asset_type: str = "another_mock_asset"

            @classmethod
            def dependencies(cls, data: bytes) -> List[AssetUri]:
                return []

            @classmethod
            def from_bytes(
                cls,
                data: bytes,
                id: str,
                dependencies: Optional[Mapping[AssetUri, Asset]],
                serializer: Type[AssetSerializer],
            ) -> "AnotherMockAsset":
                return cls()

            def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
                return b""

            def get_id(self) -> str:
                return "another_mock_id"

        manager.register_asset(AnotherMockAsset, DummyAssetSerializer)
        self.assertEqual(manager._asset_classes[AnotherMockAsset.asset_type], AnotherMockAsset)

        # Test overwriting
        manager.register_asset(
            MockAsset, DummyAssetSerializer
        )  # Registering again should overwrite
        self.assertEqual(manager._asset_classes[MockAsset.asset_type], MockAsset)

        # Test registering non-Asset class
        with self.assertRaises(TypeError):

            class NotAnAsset(Asset):  # Inherit from Asset
                pass

            manager.register_asset(NotAnAsset, DummyAssetSerializer)

    def test_get(self):
        # Setup AssetManager with a real LocalStore and the MockAsset class
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Register the MockAsset class
            manager.register_asset(MockAsset, DummyAssetSerializer)

            # Create a test asset file via AssetManager
            test_data = b"test asset data"
            test_uri = manager.add_raw(
                asset_type=MockAsset.asset_type,
                asset_id="dummy_id_get",
                data=test_data,
                store="local",
            )

            # Call AssetManager.get
            retrieved_object = cast(MockAsset, manager.get(test_uri))

            # Assert the retrieved object is an instance of MockAsset
            self.assertIsInstance(retrieved_object, MockAsset)
            self.assertEqual(retrieved_object.get_data(), test_data)

            # Test error handling for non-existent URI
            non_existent_uri = AssetUri.build(MockAsset.asset_type, "non_existent", "1")
            with self.assertRaises(FileNotFoundError):
                manager.get(non_existent_uri)

            # Test error handling for no asset class registered
            non_registered_uri = AssetUri.build("non_existent_type", "dummy_id", "1")
            # Need to create a dummy file for the store to find
            dummy_data = b"dummy"
            manager.add_raw(
                asset_type="non_existent_type", asset_id="dummy_id", data=dummy_data, store="local"
            )

            with self.assertRaises(ValueError) as cm:
                manager.get(non_registered_uri)
            self.assertIn("No asset class registered for URI:", str(cm.exception))

    def test_delete(self):
        # Setup AssetManager with a real LocalStore
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Create a test asset file
            test_data = b"test asset data to delete"
            test_uri = manager.add_raw(
                asset_type="temp_asset", asset_id="dummy_id_delete", data=test_data, store="local"
            )
            test_path = base_dir / "temp_asset" / str(test_uri.asset_id) / str(test_uri.version)
            self.assertTrue(test_path.exists())

            # Call AssetManager.delete
            manager.delete(test_uri)

            # Verify file deletion
            self.assertFalse(test_path.exists())

            # Test error handling for non-existent URI (should not raise error
            # as LocalStore.delete handles this)
            non_existent_uri = AssetUri.build(
                "temp_asset", "non_existent", "1"  # Keep original for logging
            )
            manager.delete(non_existent_uri)  # Should not raise

    def test_create(self):
        # Setup AssetManager with LocalStore and MockAsset class
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Register the MockAsset class
            manager.register_asset(MockAsset, DummyAssetSerializer)

            # Create a MockAsset instance with a specific id
            test_obj = MockAsset(b"object data", id="mocked_asset_id")

            # Call manager.add to create
            created_uri = manager.add(test_obj, store="local")

            # Assert returned URI is as expected
            expected_uri = AssetUri.build(MockAsset.asset_type, "mocked_asset_id", "1")
            self.assertEqual(created_uri, expected_uri)

            # Verify the asset was created
            retrieved_data = asyncio.run(local_store.get(created_uri))
            self.assertEqual(retrieved_data, test_obj.get_data())

            # Test error handling (store not found)
            with self.assertRaises(ValueError) as cm:
                manager.add(test_obj, store="non_existent_store")
            self.assertIn("No store registered for name:", str(cm.exception))

        with tempfile.TemporaryDirectory() as tmpdir:
            local_store = MemoryStore("local")
            manager = AssetManager()
            manager.register_store(local_store)

            # Register the MockAsset class
            manager.register_asset(MockAsset, DummyAssetSerializer)

            # First, create an asset
            initial_data = b"initial data"
            asset_id = "some_asset_id"
            test_uri = manager.add_raw(MockAsset.asset_type, asset_id, initial_data, "local")
            self.assertEqual(test_uri.version, "1")

            # Create a MockAsset instance with the same id for update
            updated_data = b"updated object data"
            test_obj = MockAsset(updated_data, id=asset_id)

            # Call manager.add to update
            updated_uri = manager.add(test_obj, store="local")

            # Assert returned URI matches the original except for version
            self.assertEqual(updated_uri.asset_type, test_uri.asset_type)
            self.assertEqual(updated_uri.asset_id, test_uri.asset_id)
            self.assertEqual(updated_uri.version, "2")

            # Verify the asset was updated
            obj = cast(MockAsset, manager.get(updated_uri, store="local"))
            self.assertEqual(updated_data, test_obj.get_data())
            self.assertIsInstance(obj, MockAsset)
            self.assertEqual(updated_data, obj.get_data())

            # Test error handling (store not found)
            with self.assertRaises(ValueError) as cm:
                manager.add(test_obj, store="non_existent_store")
            self.assertIn("No store registered for name:", str(cm.exception))

    def test_create_raw(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_raw")
        manager = AssetManager()
        manager.register_store(memory_store)

        asset_type = "raw_test_type"
        asset_id = "raw_test_id"
        data = b"raw test data"

        # Expected URI with version 1 (assuming MemoryStore uses integer versions)
        expected_uri = AssetUri.build(asset_type, asset_id, "1")

        # Call manager.add_raw
        created_uri = manager.add_raw(
            asset_type=asset_type, asset_id=asset_id, data=data, store="memory_raw"
        )

        # Assert returned URI is correct (check asset_type and asset_id)
        self.assertEqual(created_uri.asset_type, asset_type)
        self.assertEqual(created_uri.asset_id, asset_id)
        self.assertEqual(created_uri, expected_uri)

        # Verify data was stored using the actual created_uri
        # Await the async get method using asyncio.run
        retrieved_data = asyncio.run(memory_store.get(created_uri))
        self.assertEqual(retrieved_data, data)

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.add_raw(
                asset_type=asset_type, asset_id=asset_id, data=data, store="non_existent_store"
            )
        self.assertIn("No store registered for name:", str(cm.exception))

    def test_get_raw(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_raw_get")
        manager = AssetManager()
        manager.register_store(memory_store)

        test_uri_str = "test_type://test_id/1"
        test_uri = AssetUri(test_uri_str)
        expected_data = b"retrieved raw data"

        # Manually put data into the memory store
        manager.add_raw("test_type", "test_id", expected_data, "memory_raw_get")

        # Call manager.get_raw using the URI returned by add_raw
        retrieved_data = manager.get_raw(test_uri, store="memory_raw_get")

        # Assert returned data matches store's result
        self.assertEqual(retrieved_data, expected_data)

        # Test error handling (store not found)
        non_existent_uri = AssetUri("type://id/1")
        # Test error handling (asset not found in any store, including non-existent ones)
        non_existent_uri = AssetUri("type://id/1")
        with self.assertRaises(FileNotFoundError) as cm:
            manager.get_raw(non_existent_uri, store="non_existent_store")
        self.assertIn(
            "Asset 'type://id/1' not found in stores '['non_existent_store']'", str(cm.exception)
        )

    def test_is_empty(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_empty")
        manager = AssetManager()
        manager.register_store(memory_store)

        # Test when store is empty
        self.assertTrue(manager.is_empty(store="memory_empty"))

        # Add an asset and test again
        manager.add_raw("test_type", "test_id", b"data", "memory_empty")
        self.assertFalse(manager.is_empty(store="memory_empty"))

        # Test with asset type
        self.assertTrue(manager.is_empty(store="memory_empty", asset_type="another_type"))
        self.assertFalse(manager.is_empty(store="memory_empty", asset_type="test_type"))

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.is_empty(store="non_existent_store")
        self.assertIn("No store registered for name:", str(cm.exception))

    def test_count_assets(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_count")
        manager = AssetManager()
        manager.register_store(memory_store)

        # Test when store is empty
        self.assertEqual(manager.count_assets(store="memory_count"), 0)
        self.assertEqual(manager.count_assets(store="memory_count", asset_type="type1"), 0)

        # Add assets and test counts
        manager.add_raw("type1", "asset1", b"data1", "memory_count")
        self.assertEqual(manager.count_assets(store="memory_count"), 1)
        self.assertEqual(manager.count_assets(store="memory_count", asset_type="type1"), 1)
        self.assertEqual(manager.count_assets(store="memory_count", asset_type="type2"), 0)

        manager.add_raw("type2", "asset2", b"data2", "memory_count")
        manager.add_raw("type1", "asset3", b"data3", "memory_count")
        self.assertEqual(manager.count_assets(store="memory_count"), 3)
        self.assertEqual(manager.count_assets(store="memory_count", asset_type="type1"), 2)
        self.assertEqual(manager.count_assets(store="memory_count", asset_type="type2"), 1)

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.count_assets(store="non_existent_store")
        self.assertIn("No store registered for name:", str(cm.exception))

    def test_get_bulk(self):
        # Setup AssetManager with a real MemoryStore and MockAsset class
        memory_store = MemoryStore("memory_bulk")
        manager = AssetManager()
        manager.register_store(memory_store)
        manager.register_asset(MockAsset, DummyAssetSerializer)

        # Create some assets in the memory store
        data1 = b"data for id1"
        data2 = b"data for id2"
        uri1 = manager.add_raw(MockAsset.asset_type, "id1", data1, "memory_bulk")
        uri2 = manager.add_raw(MockAsset.asset_type, "id2", data2, "memory_bulk")
        uri3 = AssetUri.build(MockAsset.asset_type, "non_existent", "1")
        uris = [uri1, uri2, uri3]

        # Call manager.get_bulk
        retrieved_assets = cast(List[MockAsset], manager.get_bulk(uris, store="memory_bulk"))

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets), 3)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets[0], MockAsset)
        self.assertEqual(retrieved_assets[0].get_data(), data1)

        self.assertIsInstance(retrieved_assets[1], MockAsset)
        self.assertEqual(retrieved_assets[1].get_data(), data2)

        # Assert the non-existent asset is None
        self.assertIsNone(retrieved_assets[2])

        # Test error handling (store not found)
        # Test handling of non-existent store (should skip and not raise ValueError)
        # The test already asserts that the non-existent asset is None, which is the expected behavior.
        manager.get_bulk(uris, store="non_existent_store")

    def test_fetch(self):
        # Setup AssetManager with a real MemoryStore and MockAsset class
        memory_store = MemoryStore("memory_fetch")
        manager = AssetManager()
        manager.register_store(memory_store)
        manager.register_asset(MockAsset, DummyAssetSerializer)

        # Create some assets in the memory store
        data1 = b"data for id1"
        data2 = b"data for id2"
        manager.add_raw(MockAsset.asset_type, "id1", data1, "memory_fetch")
        manager.add_raw(MockAsset.asset_type, "id2", data2, "memory_fetch")
        # Create an asset of a different type
        manager.add_raw("another_type", "id3", b"data for id3", "memory_fetch")
        AssetUri.build(MockAsset.asset_type, "non_existent", "1")

        # Call manager.fetch without filters
        # This should raise ValueError because uri3 has an unregistered type
        with self.assertRaises(ValueError) as cm:
            manager.fetch(store="memory_fetch")
        self.assertIn("No asset class registered for URI:", str(cm.exception))

        # Now test fetching with a registered asset type filter
        # Setup a new manager and store to avoid state from previous test
        memory_store_filtered = MemoryStore("memory_fetch_filtered")
        manager_filtered = AssetManager()
        manager_filtered.register_store(memory_store_filtered)
        manager_filtered.register_asset(MockAsset, DummyAssetSerializer)

        # Create assets again
        manager_filtered.add_raw(MockAsset.asset_type, "id1", data1, "memory_fetch_filtered")
        manager_filtered.add_raw(MockAsset.asset_type, "id2", data2, "memory_fetch_filtered")
        manager_filtered.add_raw("another_type", "id3", b"data for id3", "memory_fetch_filtered")

        retrieved_assets_filtered = cast(
            List[MockAsset],
            manager_filtered.fetch(asset_type=MockAsset.asset_type, store="memory_fetch_filtered"),
        )

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets_filtered), 2)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets_filtered[0], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[0].get_data().decode("utf-8"),
            data1.decode("utf-8"),
        )

        self.assertIsInstance(retrieved_assets_filtered[1], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[1].get_data().decode("utf-8"),
            data2.decode("utf-8"),
        )

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.fetch(store="non_existent_store")
        self.assertIn("No store registered for name:", str(cm.exception))

    def test_copy(self):
        # Setup AssetManager with two MemoryStores and MockAsset class
        memory_store_src = MemoryStore("memory_copy_src")
        memory_store_dest = MemoryStore("memory_copy_dest")
        manager = AssetManager()
        manager.register_store(memory_store_src)
        manager.register_store(memory_store_dest)
        manager.register_asset(MockAsset, DummyAssetSerializer)

        # Create a source asset
        src_data = b"source asset data"
        src_uri = manager.add_raw(MockAsset.asset_type, "source_id", src_data, "memory_copy_src")

        # Test copying to a different store with default destination URI
        copied_uri_default_dest = manager.copy(
            src_uri, dest_store="memory_copy_dest", store="memory_copy_src"
        )
        self.assertEqual(copied_uri_default_dest.asset_type, src_uri.asset_type)
        self.assertEqual(copied_uri_default_dest.asset_id, src_uri.asset_id)
        self.assertEqual(copied_uri_default_dest.version, "1")  # First version in dest

        # Verify the copied asset exists in the destination store
        copied_data_default_dest = manager.get_raw(
            copied_uri_default_dest, store="memory_copy_dest"
        )
        self.assertEqual(copied_data_default_dest, src_data)

        # Test copying to a different store with a specified destination URI
        dest_uri_specified = AssetUri.build(MockAsset.asset_type, "specified_dest_id", "1")
        copied_uri_specified_dest = manager.copy(
            src_uri,
            dest_store="memory_copy_dest",
            store="memory_copy_src",
            dest=dest_uri_specified,
        )
        self.assertEqual(copied_uri_specified_dest, dest_uri_specified)

        # Verify the copied asset exists at the specified destination URI
        copied_data_specified_dest = manager.get_raw(
            copied_uri_specified_dest, store="memory_copy_dest"
        )
        self.assertEqual(copied_data_specified_dest, src_data)

        # Test copying to the same store with a different destination URI
        dest_uri_same_store = AssetUri.build(MockAsset.asset_type, "same_store_dest", "1")
        copied_uri_same_store = manager.copy(
            src_uri, dest_store="memory_copy_src", store="memory_copy_src", dest=dest_uri_same_store
        )
        self.assertEqual(copied_uri_same_store, dest_uri_same_store)

        # Verify the copied asset exists in the same store at the new URI
        copied_data_same_store = manager.get_raw(copied_uri_same_store, store="memory_copy_src")
        self.assertEqual(copied_data_same_store, src_data)

        # Test assertion for source and destination being the same
        with self.assertRaises(ValueError) as cm:
            manager.copy(
                src_uri, dest_store="memory_copy_src", store="memory_copy_src", dest=src_uri
            )
        self.assertIn(
            "Source and destination cannot be the same asset in the same store.",
            str(cm.exception),
        )

        # Test overwriting existing destination (add a different asset at specified_dest_id)
        overwrite_data = b"data to be overwritten"
        overwrite_uri = manager.add_raw(
            MockAsset.asset_type, "specified_dest_id", overwrite_data, "memory_copy_dest"
        )
        self.assertEqual(overwrite_uri.version, "2")  # Should be version 2 now

        # Copy the original src_uri to the existing destination
        copied_uri_overwrite = manager.copy(
            src_uri,
            dest_store="memory_copy_dest",
            store="memory_copy_src",
            dest=dest_uri_specified,
        )
        # The version should be incremented again due to overwrite
        self.assertEqual(copied_uri_overwrite.version, "3")

        # Verify the destination now contains the source data
        retrieved_overwritten_data = manager.get_raw(copied_uri_overwrite, store="memory_copy_dest")
        self.assertEqual(retrieved_overwritten_data, src_data)

    def test_deepcopy(self):
        # Setup AssetManager with two MemoryStores and MockAssetWithDeps class
        memory_store_src = MemoryStore("memory_deepcopy_src")
        memory_store_dest = MemoryStore("memory_deepcopy_dest")
        manager = AssetManager()
        manager.register_store(memory_store_src)
        manager.register_store(memory_store_dest)
        manager.register_asset(MockAssetWithDeps, DummyAssetSerializer)

        # Create dependency assets in the source store
        dep1_data = b'{"data": "dependency 1 data", "deps": []}'
        dep2_data = b'{"data": "dependency 2 data", "deps": []}'
        dep1_uri = manager.add_raw(
            MockAssetWithDeps.asset_type, "dep1_id", dep1_data, "memory_deepcopy_src"
        )
        dep2_uri = manager.add_raw(
            MockAssetWithDeps.asset_type, "dep2_id", dep2_data, "memory_deepcopy_src"
        )

        # Create a source asset with dependencies
        src_data = (
            b'{"data": "source asset data", "deps": ["'
            + str(dep1_uri).encode("utf-8")
            + b'", "'
            + str(dep2_uri).encode("utf-8")
            + b'"]}'
        )
        src_uri = manager.add_raw(
            MockAssetWithDeps.asset_type, "source_id", src_data, "memory_deepcopy_src"
        )

        # Test deep copying to a different store with default destination URI
        copied_uri_default_dest = manager.deepcopy(
            src_uri, dest_store="memory_deepcopy_dest", store="memory_deepcopy_src"
        )
        self.assertEqual(copied_uri_default_dest.asset_type, src_uri.asset_type)
        self.assertEqual(copied_uri_default_dest.asset_id, src_uri.asset_id)
        self.assertEqual(copied_uri_default_dest.version, "1")  # First version in dest

        # Verify the copied top-level asset exists in the destination store
        copied_asset_default_dest = cast(
            MockAssetWithDeps, manager.get(copied_uri_default_dest, store="memory_deepcopy_dest")
        )
        self.assertIsInstance(copied_asset_default_dest, MockAssetWithDeps)
        # The copied asset's data should be the serialized form including dependencies
        expected_data = b'{"data": "source asset data", "deps": ["mock_asset_with_deps://dep1_id/1", "mock_asset_with_deps://dep2_id/1"]}'
        self.assertEqual(copied_asset_default_dest.get_data(), expected_data)

        # Verify dependencies were also copied and resolved correctly
        copied_deps_default_dest = copied_asset_default_dest.get_dependencies()
        self.assertEqual(len(copied_deps_default_dest), 2)
        self.assertIn(dep1_uri, copied_deps_default_dest)
        self.assertIn(dep2_uri, copied_deps_default_dest)

        copied_dep1 = cast(MockAssetWithDeps, copied_deps_default_dest[dep1_uri])
        self.assertIsInstance(copied_dep1, MockAssetWithDeps)
        self.assertEqual(copied_dep1.get_data(), b'{"data": "dependency 1 data", "deps": []}')

        copied_dep2 = cast(MockAssetWithDeps, copied_deps_default_dest[dep2_uri])
        self.assertIsInstance(copied_dep2, MockAssetWithDeps)
        self.assertEqual(copied_dep2.get_data(), b'{"data": "dependency 2 data", "deps": []}')

        # Test deep copying with a specified destination URI for the top-level asset
        dest_uri_specified = AssetUri.build(MockAssetWithDeps.asset_type, "specified_dest_id", "1")
        copied_uri_specified_dest = manager.deepcopy(
            src_uri,
            dest_store="memory_deepcopy_dest",
            store="memory_deepcopy_src",
            dest=dest_uri_specified,
        )
        self.assertEqual(copied_uri_specified_dest, dest_uri_specified)

        # Verify the copied asset exists at the specified destination URI
        copied_asset_specified_dest = cast(
            MockAssetWithDeps, manager.get(copied_uri_specified_dest, store="memory_deepcopy_dest")
        )
        self.assertIsInstance(copied_asset_specified_dest, MockAssetWithDeps)
        self.assertEqual(
            copied_asset_specified_dest.get_data(),
            b'{"data": "source asset data", "deps": ["mock_asset_with_deps://dep1_id/1", "mock_asset_with_deps://dep2_id/1"]}',
        )

        # Verify dependencies were copied and resolved correctly (their URIs should be
        # in the destination store, but their asset_type and asset_id should be the same)
        copied_deps_specified_dest = copied_asset_specified_dest.get_dependencies()
        self.assertEqual(len(copied_deps_specified_dest), 2)

        # The keys in the dependencies mapping should be the *original* URIs,
        # but the values should be the *copied* dependency assets.
        self.assertIn(dep1_uri, copied_deps_specified_dest)
        self.assertIn(dep2_uri, copied_deps_specified_dest)

        copied_dep1_specified = cast(MockAssetWithDeps, copied_deps_specified_dest[dep1_uri])
        self.assertIsInstance(copied_dep1_specified, MockAssetWithDeps)
        self.assertEqual(
            copied_dep1_specified.get_data(), b'{"data": "dependency 1 data", "deps": []}'
        )
        # Check the URI of the copied dependency in the destination store
        self.assertIsNotNone(
            manager.get_or_none(copied_dep1_specified.get_uri(), store="memory_deepcopy_dest")
        )
        self.assertEqual(copied_dep1_specified.get_uri().asset_type, dep1_uri.asset_type)
        self.assertEqual(copied_dep1_specified.get_uri().asset_id, dep1_uri.asset_id)

        copied_dep2_specified = cast(MockAssetWithDeps, copied_deps_specified_dest[dep2_uri])
        self.assertIsInstance(copied_dep2_specified, MockAssetWithDeps)
        self.assertEqual(
            copied_dep2_specified.get_data(), b'{"data": "dependency 2 data", "deps": []}'
        )
        # Check the URI of the copied dependency in the destination store
        self.assertIsNotNone(
            manager.get_or_none(copied_dep2_specified.get_uri(), store="memory_deepcopy_dest")
        )
        self.assertEqual(copied_dep2_specified.get_uri().asset_type, dep2_uri.asset_type)
        self.assertEqual(copied_dep2_specified.get_uri().asset_id, dep2_uri.asset_id)

        # Test handling of existing dependencies in the destination store (should be skipped)
        # Add a dependency with the same URI as dep1_uri to the destination store
        existing_dep1_data = b'{"data": "existing dependency 1 data", "deps": []}'
        existing_dep1_uri_in_dest = manager.add_raw(
            dep1_uri.asset_type, dep1_uri.asset_id, existing_dep1_data, "memory_deepcopy_dest"
        )
        self.assertEqual(existing_dep1_uri_in_dest.version, "2")  # Should be version 2 now

        # Deep copy the source asset again
        copied_uri_existing_dep = manager.deepcopy(
            src_uri, dest_store="memory_deepcopy_dest", store="memory_deepcopy_src"
        )
        # The top-level asset should be overwritten, incrementing its version
        self.assertEqual(copied_uri_existing_dep.version, "2")

        # Verify the top-level asset was overwritten
        copied_asset_existing_dep = cast(
            MockAssetWithDeps, manager.get(copied_uri_existing_dep, store="memory_deepcopy_dest")
        )
        self.assertIsInstance(copied_asset_existing_dep, MockAssetWithDeps)
        self.assertEqual(
            copied_asset_existing_dep.get_data(),
            b'{"data": "source asset data", "deps": ["mock_asset_with_deps://dep1_id/1", "mock_asset_with_deps://dep2_id/1"]}',
        )

        # Verify that the existing dependency was *not* overwritten
        retrieved_existing_dep1 = manager.get_raw(
            existing_dep1_uri_in_dest, store="memory_deepcopy_dest"
        )
        self.assertEqual(retrieved_existing_dep1, existing_dep1_data)

        # Verify the dependencies in the copied asset still point to the correct
        # (existing) dependency in the destination store.
        copied_deps_existing_dep = copied_asset_existing_dep.get_dependencies()
        self.assertEqual(len(copied_deps_existing_dep), 2)
        self.assertIn(dep1_uri, copied_deps_existing_dep)
        self.assertIn(dep2_uri, copied_deps_existing_dep)

        copied_dep1_existing = cast(MockAssetWithDeps, copied_deps_existing_dep[dep1_uri])
        self.assertIsInstance(copied_dep1_existing, MockAssetWithDeps)
        self.assertEqual(
            copied_dep1_existing.get_data(), b'{"data": "dependency 1 data", "deps": []}'
        )  # Should be the original data from source

        copied_dep2_existing = cast(MockAssetWithDeps, copied_deps_existing_dep[dep2_uri])
        self.assertIsInstance(copied_dep2_existing, MockAssetWithDeps)
        self.assertEqual(
            copied_dep2_existing.get_data(), b'{"data": "dependency 2 data", "deps": []}'
        )  # Should be the newly copied raw data

        # Test handling of existing top-level asset in the destination store (should be overwritten)
        # This was implicitly tested in the previous step where the top-level asset's
        # version was incremented. Let's add a more explicit test.
        overwrite_src_data = b'{"data": "overwrite source data", "deps": []}'
        overwrite_src_uri = manager.add_raw(
            MockAssetWithDeps.asset_type,
            "overwrite_source_id",
            overwrite_src_data,
            "memory_deepcopy_src",
        )

        # Add an asset to the destination store with the same URI as overwrite_src_uri
        existing_dest_data = b'{"data": "existing destination data", "deps": []}'
        existing_dest_uri = manager.add_raw(
            overwrite_src_uri.asset_type,
            overwrite_src_uri.asset_id,
            existing_dest_data,
            "memory_deepcopy_dest",
        )
        self.assertEqual(existing_dest_uri.version, "1")

        # Deep copy overwrite_src_uri to the existing destination URI
        copied_uri_overwrite_top = manager.deepcopy(
            overwrite_src_uri,
            dest_store="memory_deepcopy_dest",
            store="memory_deepcopy_src",
            dest=existing_dest_uri,
        )
        # The version should be incremented
        self.assertEqual(copied_uri_overwrite_top.version, "2")

        # Verify the destination now contains the source data
        retrieved_overwritten_top = manager.get_raw(
            copied_uri_overwrite_top, store="memory_deepcopy_dest"
        )
        # Need to parse the data to get the actual content
        import json

        retrieved_data_dict = json.loads(retrieved_overwritten_top.decode("utf-8"))
        self.assertEqual(retrieved_data_dict.get("data"), b"overwrite source data".decode("utf-8"))

        # Test error handling for non-existent source asset
        non_existent_src_uri = AssetUri.build(MockAssetWithDeps.asset_type, "non_existent_src", "1")
        with self.assertRaises(FileNotFoundError) as cm:
            manager.deepcopy(
                non_existent_src_uri, dest_store="memory_deepcopy_dest", store="memory_deepcopy_src"
            )
        self.assertIn("Source asset", str(cm.exception))
        self.assertIn("not found", str(cm.exception))

        # Test error handling for non-existent source store
        with self.assertRaises(ValueError) as cm:
            manager.deepcopy(src_uri, dest_store="memory_deepcopy_dest", store="non_existent_store")
        self.assertIn("Source store", str(cm.exception))
        self.assertIn("not registered", str(cm.exception))

        # Test error handling for non-existent destination store
        with self.assertRaises(ValueError) as cm:
            manager.deepcopy(src_uri, dest_store="non_existent_store", store="memory_deepcopy_src")
        self.assertIn("Destination store", str(cm.exception))
        self.assertIn("not registered", str(cm.exception))

    def test_exists(self):
        # Setup AssetManager with a MemoryStore
        memory_store = MemoryStore("memory_exists")
        manager = AssetManager()
        manager.register_store(memory_store)

        # Create an asset
        test_uri = manager.add_raw("test_type", "test_id", b"data", "memory_exists")

        # Test exists for an existing asset
        self.assertTrue(manager.exists(test_uri, store="memory_exists"))

        # Test exists for a non-existent asset
        non_existent_uri = AssetUri.build("test_type", "non_existent_id", "1")
        self.assertFalse(manager.exists(non_existent_uri, store="memory_exists"))

        # Test exists for a non-existent store (should raise ValueError)
        with self.assertRaises(ValueError) as cm:
            manager.exists(test_uri, store="non_existent_store")
        self.assertIn("No store registered for name:", str(cm.exception))


if __name__ == "__main__":
    unittest.main()
