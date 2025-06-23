import unittest
import asyncio
from unittest.mock import Mock
import pathlib
import tempfile
from typing import Any, Mapping, List
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
    def extract_dependencies(cls, data: bytes, serializer: AssetSerializer) -> List[AssetUri]:
        # Mock implementation doesn't use data or format for dependencies
        return []

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Mapping[AssetUri, Asset] | None,
        serializer: AssetSerializer,
    ) -> "MockAsset":
        # Create instance with provided id
        return cls(data, id)

    def to_bytes(self, serializer: AssetSerializer) -> bytes:
        return self._data

    def get_id(self) -> str:
        return self._id


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
                dependencies: Mapping[AssetUri, Asset] | None,
                serializer: AssetSerializer,
            ) -> "AnotherMockAsset":
                return cls()

            def to_bytes(self, serializer: AssetSerializer) -> bytes:
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
            retrieved_object = manager.get(test_uri)

            # Assert the retrieved object is an instance of MockAsset
            self.assertIsInstance(retrieved_object, MockAsset)
            # Assert the data was passed to from_bytes
            self.assertEqual(retrieved_object._data, test_data)

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
            self.assertEqual(retrieved_data, test_obj.to_bytes(DummyAssetSerializer))

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
            obj = manager.get(updated_uri, store="local")
            self.assertEqual(updated_data, test_obj.to_bytes(DummyAssetSerializer))
            self.assertEqual(updated_data, obj.to_bytes(DummyAssetSerializer))

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
        retrieved_assets = manager.get_bulk(uris, store="memory_bulk")

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets), 3)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets[0], MockAsset)
        self.assertEqual(
            retrieved_assets[0].to_bytes(DummyAssetSerializer),
            data1,
        )

        self.assertIsInstance(retrieved_assets[1], MockAsset)
        self.assertEqual(
            retrieved_assets[1].to_bytes(DummyAssetSerializer),
            data2,
        )

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

        retrieved_assets_filtered = manager_filtered.fetch(
            asset_type=MockAsset.asset_type, store="memory_fetch_filtered"
        )

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets_filtered), 2)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets_filtered[0], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[0].to_bytes(DummyAssetSerializer).decode("utf-8"),
            data1.decode("utf-8"),
        )

        self.assertIsInstance(retrieved_assets_filtered[1], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[1].to_bytes(DummyAssetSerializer).decode("utf-8"),
            data2.decode("utf-8"),
        )

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.fetch(store="non_existent_store")
        self.assertIn("No store registered for name:", str(cm.exception))


if __name__ == "__main__":
    unittest.main()
