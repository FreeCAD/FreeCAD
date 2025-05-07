import unittest
from unittest.mock import Mock, AsyncMock
import pathlib
import tempfile
import asyncio
from Path.Tool.assets import AssetManager, VersionedLocalStore, AssetAdapter
from Path.Tool.assets.uri import AssetUri


# Mock classes for testing registration
class TestPathToolAssetManager(unittest.TestCase):
    def test_register_store(self):
        manager = AssetManager()
        mock_store_local = Mock()
        mock_store_local.protocol = "local"
        mock_store_remote = Mock()
        mock_store_remote.protocol = "remote"

        manager.register_store(mock_store_local)
        self.assertEqual(manager.stores["local"], mock_store_local)

        manager.register_store(mock_store_remote)
        self.assertEqual(manager.stores["remote"], mock_store_remote)

        # Test overwriting
        mock_store_local_new = Mock()
        mock_store_local_new.protocol = "local"
        manager.register_store(mock_store_local_new)
        self.assertEqual(manager.stores["local"], mock_store_local_new)

    def test_register_adapter(self):
        manager = AssetManager()
        mock_adapter_tool = Mock()
        mock_adapter_tool.asset_name = "tool"
        mock_adapter_tool.asset_class = type("Tool", (), {})
        mock_adapter_material = Mock()
        mock_adapter_material.asset_name = "material"
        mock_adapter_material.asset_class = type("Material", (), {})

        manager.register_adapter(mock_adapter_tool)
        self.assertEqual(manager.adapters["tool"], mock_adapter_tool)

        manager.register_adapter(mock_adapter_material)
        self.assertEqual(manager.adapters["material"], mock_adapter_material)

        # Test overwriting
        mock_adapter_tool_new = Mock()
        mock_adapter_tool_new.asset_name = "tool"
        mock_adapter_tool_new.asset_class = type("ToolNew", (), {})
        manager.register_adapter(mock_adapter_tool_new)
        self.assertEqual(manager.adapters["tool"], mock_adapter_tool_new)

    def test_get(self):
        # Setup AssetManager with a real LocalStore and a MockAssetAdapter
        # Use a temporary directory for the LocalStore
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = VersionedLocalStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Mock an adapter
            mock_adapter = Mock(spec=AssetAdapter)
            mock_adapter.asset_name = "test_asset"
            mock_adapter.asset_class = type("TestResource", (), {})
            manager.register_adapter(mock_adapter)

            # Create a test asset file via LocalStore
            test_data = b"test asset data"
            test_uri = asyncio.run(
                local_store.create("test_asset", "dummy_id_get", test_data)
            )

            # Mock adapter methods
            mock_adapter.dependencies.return_value = []
            mock_adapter.create.return_value = "mocked asset object"

            # Call AssetManager.get
            retrieved_object = asyncio.run(manager.get(test_uri))

            # Assert store/adapter methods called correctly
            # LocalStore.get is called internally by manager.get
            mock_adapter.dependencies.assert_called_once_with(test_data)
            mock_adapter.create.assert_called_once_with(test_data, {})

            # Assert final object matches adapter's create result
            self.assertEqual(retrieved_object, "mocked asset object")

            # Test error handling for non-existent URI
            non_existent_uri = AssetUri.build(
                "local", "", "test_asset", "non_existent", "1"
            )
            with self.assertRaises(FileNotFoundError):
                asyncio.run(manager.get(non_existent_uri))

    def test_delete(self):
        # Setup AssetManager with a real LocalStore
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = VersionedLocalStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Create a test asset file
            test_data = b"test asset data to delete"
            test_uri = asyncio.run(
                local_store.create("temp_asset", "dummy_id_delete", test_data)
            )
            test_path = (
                base_dir
                / "temp_asset"
                / test_uri.asset
                / test_uri.version
            )
            self.assertTrue(test_path.exists())

            # Call AssetManager.delete
            asyncio.run(manager.delete(test_uri))

            # Verify file deletion
            self.assertFalse(test_path.exists())

            # Test error handling for non-existent URI (should not raise error
            # as LocalStore.delete handles this)
            non_existent_uri = AssetUri.build(
                "local", "", "temp_asset", "non_existent", "1"
            )
            asyncio.run(manager.delete(non_existent_uri))  # Should not raise

    def test_create(self):
        # Setup AssetManager with LocalStore and MockAssetAdapter
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = VersionedLocalStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Mock an adapter
            mock_adapter = Mock(spec=AssetAdapter)
            mock_adapter.asset_name = "test_creatable_asset"
            mock_adapter.asset_class = type("TestCreatableResource", (), {})
            # Add id_of to the mock adapter spec
            mock_adapter.id_of = Mock(return_value="mocked_asset_id")
            manager.register_adapter(mock_adapter)

            # Mock adapter methods
            TestCreatableResource = mock_adapter.asset_class
            test_obj = TestCreatableResource()
            serialized_data = b"serialized object data"
            mock_adapter.serialize.return_value = serialized_data

            # Mock store create method to return a predictable URI
            expected_uri_str = (
                "local:///test_creatable_asset/mocked_asset_id/1"
            )
            expected_uri = AssetUri(expected_uri_str)
            local_store.create = AsyncMock(return_value=expected_uri)

            # Call manager.create
            created_uri = asyncio.run(manager.create("local", test_obj))

            # Assert adapter methods called
            mock_adapter.serialize.assert_called_once_with(test_obj)
            mock_adapter.id_of.assert_called_once_with(test_obj)

            # Assert store create called with correct arguments
            local_store.create.assert_called_once_with(
                mock_adapter.asset_name, "mocked_asset_id", serialized_data
            )

            # Assert returned URI matches store's result
            self.assertEqual(created_uri, expected_uri)

            # Test error handling (adapter not found)
            with self.assertRaises(ValueError) as cm:
                asyncio.run(manager.create(
                    "local", Mock()
                ))  # Use a different object type
            self.assertIn(
                "No adapter registered for object type:", str(cm.exception)
            )

            # Test error handling (store not found)
            with self.assertRaises(ValueError) as cm:
                asyncio.run(manager.create("non_existent_store", test_obj))
            self.assertIn(
                "No store registered for protocol:", str(cm.exception)
            )

    def test_update(self):
        # Setup AssetManager with LocalStore and MockAssetAdapter
        with tempfile.TemporaryDirectory() as tmpdir:
            base_dir = pathlib.Path(tmpdir)
            local_store = VersionedLocalStore("local", base_dir)
            manager = AssetManager()
            manager.register_store(local_store)

            # Mock an adapter
            mock_adapter = Mock(spec=AssetAdapter)
            mock_adapter.asset_name = "test_updatable_asset"
            mock_adapter.asset_class = type("TestUpdatableResource", (), {})
            manager.register_adapter(mock_adapter)

            # Mock adapter methods
            TestUpdatableResource = mock_adapter.asset_class
            test_obj = TestUpdatableResource()
            serialized_data = b"updated serialized object data"
            mock_adapter.serialize.return_value = serialized_data

            # Mock store update method
            test_uri_str = (
                "local:///test_updatable_asset/some_asset_id/1"
            )
            test_uri = AssetUri(test_uri_str)
            local_store.update = AsyncMock(
                return_value=AssetUri(
                    "local:///test_updatable_asset/some_asset_id/2"
                )
            )  # Simulate new version URI

            # Call manager.update
            asyncio.run(manager.update(test_uri, test_obj))

            # Assert adapter serialize called
            mock_adapter.serialize.assert_called_once_with(test_obj)

            # Assert store update called with correct URI and data
            local_store.update.assert_called_once_with(
                test_uri, serialized_data
            )

            # Test error handling (adapter not found)
            with self.assertRaises(ValueError) as cm:
                asyncio.run(manager.update(
                    test_uri, Mock()
                ))  # Use a different object type
            self.assertIn(
                "No adapter registered for object type:", str(cm.exception)
            )

            # Test error handling (store not found)
            non_existent_uri = AssetUri.build(
                "non_existent_store",
                "",
                "test_updatable_asset",
                "some_asset_id",
                "1",
            )
            with self.assertRaises(ValueError) as cm:
                asyncio.run(manager.update(non_existent_uri, test_obj))
            self.assertIn(
                "No store registered for protocol:", str(cm.exception)
            )

    def test_create_raw(self):
        # Setup AssetManager with a MockStore
        mock_store = AsyncMock()
        mock_store.protocol = "mock_raw"
        manager = AssetManager()
        manager.register_store(mock_store)

        asset_type = "raw_test_type"
        asset_id = "raw_test_id"
        data = b"raw test data"
        expected_uri = AssetUri.build(
            "mock_raw", "", asset_type, asset_id, "1"
        )
        mock_store.create.return_value = expected_uri

        # Call manager.create_raw
        created_uri = asyncio.run(manager.create_raw(
            "mock_raw", asset_type, asset_id, data
        ))

        # Assert store create called with correct arguments
        mock_store.create.assert_called_once_with(
            asset_type, asset_id, data
        )

        # Assert returned URI matches store's result
        self.assertEqual(created_uri, expected_uri)

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            asyncio.run(manager.create_raw(
                "non_existent_store", asset_type, asset_id, data
            ))
        self.assertIn(
            "No store registered for protocol:", str(cm.exception)
        )

    def test_get_raw(self):
        # Setup AssetManager with a MockStore
        mock_store = AsyncMock()
        mock_store.protocol = "mock_raw_get"
        manager = AssetManager()
        manager.register_store(mock_store)

        test_uri_str = "mock_raw_get:///test_type/test_id/1"
        test_uri = AssetUri(test_uri_str)
        expected_data = b"retrieved raw data"
        mock_store.get.return_value = expected_data

        # Call manager.get_raw
        retrieved_data = asyncio.run(manager.get_raw(test_uri))

        # Assert store get called with correct URI
        mock_store.get.assert_called_once_with(test_uri)

        # Assert returned data matches store's result
        self.assertEqual(retrieved_data, expected_data)

        # Test error handling (store not found)
        non_existent_uri = AssetUri("non_existent_store:///type/id/1")
        with self.assertRaises(ValueError) as cm:
            asyncio.run(manager.get_raw(non_existent_uri))
        self.assertIn(
            "No store registered for protocol:", str(cm.exception)
        )

    def test_is_empty(self):
        # Setup AssetManager with a MockStore
        mock_store = AsyncMock()
        mock_store.protocol = "mock_empty"
        manager = AssetManager()
        manager.register_store(mock_store)

        # Test when store is empty
        mock_store.is_empty.return_value = True
        self.assertTrue(asyncio.run(manager.is_empty("mock_empty")))
        mock_store.is_empty.assert_called_once_with(None)

        # Test when store is not empty
        mock_store.is_empty.reset_mock()
        mock_store.is_empty.return_value = False
        self.assertFalse(asyncio.run(manager.is_empty("mock_empty")))
        mock_store.is_empty.assert_called_once_with(None)

        # Test with asset type
        mock_store.is_empty.reset_mock()
        mock_store.is_empty.return_value = True
        self.assertTrue(asyncio.run(manager.is_empty("mock_empty", "test_type")))
        mock_store.is_empty.assert_called_once_with("test_type")

        # Test error handling (store not found)
        with self.assertRaises(ValueError) as cm:
            asyncio.run(manager.is_empty("non_existent_store"))
        self.assertIn(
            "No store registered for protocol:", str(cm.exception)
        )


if __name__ == "__main__":
    unittest.main()
