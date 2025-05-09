import unittest
import asyncio
from unittest.mock import Mock, AsyncMock
import pathlib
import tempfile
from typing import Any, Mapping, List
from Path.Tool.assets import (
    AssetManager,
    FileStore,
    Asset,
    AssetUri,
    MemoryStore,
)


# Mock Asset class for testing
class MockAsset(Asset):
    asset_type: str = "mock_asset"

    def __init__(self, data: Any = None):
        self._data = data
        self._id = "mock_id"

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def from_bytes(cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]) -> "MockAsset":
        # Mock the classmethod creation
        mock_instance = cls(data.decode('utf-8'))
        # Attach mock methods to the instance for assertion
        mock_instance.to_bytes = Mock(return_value=str(mock_instance._data).encode('utf-8'))
        mock_instance.get_id = Mock(return_value=mock_instance._id)
        return mock_instance

    def to_bytes(self) -> bytes:
        return str(self._data).encode('utf-8')

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
        manager.register_asset(MockAsset)
        self.assertEqual(manager._asset_classes[MockAsset.asset_type], MockAsset)

        # Test registering a different actual Asset class
        class AnotherMockAsset(Asset):
            asset_type: str = "another_mock_asset"

            @classmethod
            def dependencies(cls, data: bytes) -> List[AssetUri]:
                return []

            @classmethod
            def from_bytes(
                cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]
            ) -> "AnotherMockAsset":
                return cls()

            def to_bytes(self) -> bytes:
                return b""

            def get_id(self) -> str:
                return "another_mock_id"

        manager.register_asset(AnotherMockAsset)
        self.assertEqual(manager._asset_classes[AnotherMockAsset.asset_type], AnotherMockAsset)

        # Test overwriting
        manager.register_asset(MockAsset) # Registering again should overwrite
        self.assertEqual(manager._asset_classes[MockAsset.asset_type], MockAsset)

        # Test registering non-Asset class
        with self.assertRaises(TypeError):
            class NotAnAsset(Asset): # Inherit from Asset
                pass
            manager.register_asset(NotAnAsset)

    def test_get(self):
        # Setup AssetManager with a real LocalStore and the MockAsset class
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Register the MockAsset class
            manager.register_asset(MockAsset)

            # Create a test asset file via AssetManager
            test_data = b"test asset data"
            test_uri = manager.create_raw(asset_type=MockAsset.asset_type,
                                          asset_id="dummy_id_get",
                                          data=test_data,
                                          store="local")

            # Call AssetManager.get
            retrieved_object = manager.get(test_uri)

            # Assert the retrieved object is an instance of MockAsset
            self.assertIsInstance(retrieved_object, MockAsset)
            # Assert the data was passed to from_bytes (checked in MockAsset.from_bytes)
            self.assertEqual(retrieved_object._data, test_data.decode('utf-8'))

            # Test error handling for non-existent URI
            non_existent_uri = AssetUri.build(
                MockAsset.asset_type, "non_existent", "1"
            )
            with self.assertRaises(FileNotFoundError):
                manager.get(non_existent_uri)

            # Test error handling for no asset class registered
            non_registered_uri = AssetUri.build(
                "non_existent_type", "dummy_id", "1"
            )
            # Need to create a dummy file for the store to find, otherwise
            # it will raise FileNotFoundError first.
            dummy_data = b"dummy"
            manager.create_raw(asset_type="non_existent_type",
                               asset_id="dummy_id",
                               data=dummy_data,
                               store="local")

            with self.assertRaises(ValueError) as cm:
                 manager.get(non_registered_uri)
            self.assertIn("No asset class registered for asset type:", str(cm.exception))


    def test_delete(self):
        # Setup AssetManager with a real LocalStore
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Create a test asset file
            test_data = b"test asset data to delete"
            test_uri = manager.create_raw(asset_type="temp_asset",
                                          asset_id="dummy_id_delete",
                                          data=test_data,
                                          store="local")
            test_path = (
                base_dir
                / "temp_asset"
                / str(test_uri.asset_id)
                / str(test_uri.version)
            )
            self.assertTrue(test_path.exists())

            # Call AssetManager.delete
            manager.delete(test_uri)

            # Verify file deletion
            self.assertFalse(test_path.exists())

            # Test error handling for non-existent URI (should not raise error
            # as LocalStore.delete handles this)
            non_existent_uri = AssetUri.build(
                "temp_asset", "non_existent", "1" # Keep original for logging
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
            manager.register_asset(MockAsset)

            # Create a MockAsset instance
            test_obj = MockAsset("object data")
            # Mock the instance methods that AssetManager will call
            test_obj.to_bytes = Mock(return_value=b"serialized object data")
            test_obj.get_id = Mock(return_value="mocked_asset_id")


            # Mock store create method to return a predictable URI
            expected_uri_str = (
                f"{MockAsset.asset_type}://mocked_asset_id/1"
            )
            expected_uri = AssetUri(expected_uri_str)
            local_store.create = AsyncMock(return_value=expected_uri)

            # Call manager.create
            created_uri = manager.create(test_obj, store="local")

            # Assert instance methods called
            test_obj.to_bytes.assert_called_once_with()
            test_obj.get_id.assert_called_once_with()

            # Assert store create called with correct arguments
            local_store.create.assert_called_once_with(
                MockAsset.asset_type, "mocked_asset_id", b"serialized object data"
            )

            # Assert returned URI matches store's result
            self.assertEqual(created_uri, expected_uri)

            # Test error handling (asset class not found for object type)
            # Mock asset class for unittest.mock.Mock
            class MockAsset2(Asset):
                asset_type: str = "mock_asset2"

                @classmethod
                def dependencies(cls, data: bytes) -> List[AssetUri]:
                    return []

                @classmethod
                def from_bytes(cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]) -> Any:
                    return Mock() # Return a Mock object

                def to_bytes(self) -> bytes:
                    return b"mocked bytes"

                def get_id(self) -> str:
                    return "mocked_id"

            manager.register_asset(MockAsset2)
            with self.assertRaises(ValueError) as cm:
                manager.create(Mock(), store="local")  # different object type
            self.assertIn(
                "Object of type", str(cm.exception)
            )

            # Test error handling (store not found)
            with self.assertRaises(ValueError) as cm:
                manager.create(test_obj, store="non_existent_store")
            self.assertIn(
                "No store registered for name:", str(cm.exception)
            )

    def test_update(self):
        # Setup AssetManager with LocalStore and MockAsset class
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = FileStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Register the MockAsset class
            manager.register_asset(MockAsset)

            # Create a MockAsset instance
            test_obj = MockAsset("updated object data")
            # Mock the instance method that AssetManager will call
            test_obj.to_bytes = Mock(return_value=b"updated serialized object data")

            # Mock store update method
            test_uri_str = (
                f"{MockAsset.asset_type}://some_asset_id/1"
            )
            test_uri = AssetUri(test_uri_str)
            local_store.update = AsyncMock(
                return_value=AssetUri(
                    f"{MockAsset.asset_type}://some_asset_id/1"
                )
            )

            # Call manager.update
            manager.update(test_uri, test_obj, store="local")

            # Assert instance method called
            test_obj.to_bytes.assert_called_once_with()

            # Assert store update called with correct URI and data
            local_store.update.assert_called_once_with(
                test_uri, b"updated serialized object data"
            )

            # Test error handling (asset class not found for object type)
            with self.assertRaises(ValueError) as cm:
                manager.update(test_uri, Mock(), store="local")  # different object type
            self.assertIn(
                "No asset class registered for object type:", str(cm.exception)
            )

            # Test error handling (store not found)
            non_existent_uri = AssetUri.build(
                MockAsset.asset_type,
                "some_asset_id",
                "1",
            )
            with self.assertRaises(ValueError) as cm:
                manager.update(non_existent_uri, test_obj, store="non_existent_store")
            self.assertIn(
                "No store registered for name:", str(cm.exception)
            )

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

        # Call manager.create_raw
        created_uri = manager.create_raw(
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
            manager.create_raw(
                asset_type=asset_type, asset_id=asset_id, data=data, store="non_existent_store"
            )
        self.assertIn(
            "No store registered for name:", str(cm.exception)
        )

    def test_get_raw(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_raw_get")
        manager = AssetManager()
        manager.register_store(memory_store)

        test_uri_str = "test_type://test_id/1"
        test_uri = AssetUri(test_uri_str)
        expected_data = b"retrieved raw data"

        # Manually put data into the memory store
        manager.create_raw("test_type", "test_id", expected_data, "memory_raw_get")


        # Call manager.get_raw using the URI returned by create_raw
        retrieved_data = manager.get_raw(test_uri, store="memory_raw_get")

        # Assert returned data matches store's result
        self.assertEqual(retrieved_data, expected_data)

        # Test error handling (store not found)
        non_existent_uri = AssetUri("type://id/1")
        with self.assertRaises(ValueError) as cm:
            manager.get_raw(non_existent_uri, store="non_existent_store")
        self.assertIn(
            "No store registered for name:", str(cm.exception)
        )

    def test_is_empty(self):
        # Setup AssetManager with a real MemoryStore
        memory_store = MemoryStore("memory_empty")
        manager = AssetManager()
        manager.register_store(memory_store)

        # Test when store is empty
        self.assertTrue(manager.is_empty(store="memory_empty"))

        # Add an asset and test again
        manager.create_raw("test_type", "test_id", b"data", "memory_empty")
        self.assertFalse(manager.is_empty(store="memory_empty"))

        # Test with asset type
        self.assertTrue(manager.is_empty(store="memory_empty", asset_type="another_type"))
        self.assertFalse(manager.is_empty(store="memory_empty", asset_type="test_type"))


        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.is_empty(store="non_existent_store")
        self.assertIn(
            "No store registered for name:", str(cm.exception)
        )


    def test_get_bulk(self):
        # Setup AssetManager with a real MemoryStore and MockAsset class
        memory_store = MemoryStore("memory_bulk")
        manager = AssetManager()
        manager.register_store(memory_store)
        manager.register_asset(MockAsset)

        # Create some assets in the memory store
        data1 = b"data for id1"
        data2 = b"data for id2"
        uri1 = manager.create_raw(MockAsset.asset_type, "id1", data1, "memory_bulk")
        uri2 = manager.create_raw(MockAsset.asset_type, "id2", data2, "memory_bulk")
        uri3 = AssetUri.build(MockAsset.asset_type, "non_existent", "1")
        uris = [uri1, uri2, uri3]

        # Call manager.get_bulk
        retrieved_assets = manager.get_bulk(uris, store="memory_bulk")

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets), 3)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets[0], MockAsset)
        self.assertEqual(
            retrieved_assets[0].to_bytes().decode("utf-8"),
            data1.decode("utf-8"),
        )

        self.assertIsInstance(retrieved_assets[1], MockAsset)
        self.assertEqual(
            retrieved_assets[1].to_bytes().decode("utf-8"),
            data2.decode("utf-8"),
        )

        # Assert the non-existent asset is None
        self.assertIsNone(retrieved_assets[2])

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.get_bulk(uris, store="non_existent_store")
        self.assertIn(
            "No store registered for name:", str(cm.exception)
        )

    def test_fetch(self):
        # Setup AssetManager with a real MemoryStore and MockAsset class
        memory_store = MemoryStore("memory_fetch")
        manager = AssetManager()
        manager.register_store(memory_store)
        manager.register_asset(MockAsset)

        # Create some assets in the memory store
        data1 = b"data for id1"
        data2 = b"data for id2"
        manager.create_raw(MockAsset.asset_type, "id1", data1, "memory_fetch")
        manager.create_raw(MockAsset.asset_type, "id2", data2, "memory_fetch")
        # Create an asset of a different type
        manager.create_raw("another_type", "id3", b"data for id3", "memory_fetch")
        AssetUri.build(MockAsset.asset_type, "non_existent", "1")


        # Call manager.fetch without filters
        # This should raise ValueError because uri3 has an unregistered type
        with self.assertRaises(ValueError) as cm:
            manager.fetch(store="memory_fetch")
        self.assertIn("No asset class registered for asset type:", str(cm.exception))


        # Now test fetching with a registered asset type filter
        # Setup a new manager and store to avoid state from previous test
        memory_store_filtered = MemoryStore("memory_fetch_filtered")
        manager_filtered = AssetManager()
        manager_filtered.register_store(memory_store_filtered)
        manager_filtered.register_asset(MockAsset)

        # Create assets again
        manager_filtered.create_raw(MockAsset.asset_type, "id1", data1, "memory_fetch_filtered")
        manager_filtered.create_raw(MockAsset.asset_type, "id2", data2, "memory_fetch_filtered")
        manager_filtered.create_raw("another_type", "id3", b"data for id3", "memory_fetch_filtered")

        retrieved_assets_filtered = manager_filtered.fetch(
            asset_type=MockAsset.asset_type, store="memory_fetch_filtered"
        )

        # Assert the correct number of assets were returned
        self.assertEqual(len(retrieved_assets_filtered), 2)

        # Assert the retrieved assets are MockAsset instances with correct data
        self.assertIsInstance(retrieved_assets_filtered[0], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[0].to_bytes().decode("utf-8"),
            data1.decode("utf-8"),
        )

        self.assertIsInstance(retrieved_assets_filtered[1], MockAsset)
        self.assertEqual(
            retrieved_assets_filtered[1].to_bytes().decode("utf-8"),
            data2.decode("utf-8"),
        )

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            manager.fetch(store="non_existent_store")
        self.assertIn(
            "No store registered for name:", str(cm.exception)
        )


if __name__ == "__main__":
    unittest.main()
