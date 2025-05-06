import unittest
import pathlib
import asyncio
import tempfile
import aiofiles
from Path.Tool.assets.store.unversioned import UnversionedLocalStore
from Path.Tool.assets.store.versioned import VersionedLocalStore
from Path.Tool.assets.store.flat import FlatLocalStore
from Path.Tool.assets.uri import Uri


class TestPathToolUnversionedLocalStore(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.store_path = pathlib.Path(self.temp_dir.name) / "unversioned_store"
        self.store_path.mkdir()

        self.store = UnversionedLocalStore(
            protocol="test_unversioned",
            base_dir=self.store_path,
            file_extension=".txt"
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_unversioned_create_get_delete(self):
        async def async_test():
            asset_type = "test_type"
            asset_name = "test_asset"
            data = b"test data"

            # Create
            uri = await self.store.create(
                asset_type, asset_name, data
            )
            self.assertIsInstance(uri, Uri)
            self.assertEqual(uri.protocol, "test_unversioned")
            self.assertEqual(uri.asset_type, asset_type)
            self.assertEqual(uri.asset, asset_name)
            self.assertEqual(uri.version, "1") # Dummy version

            # Get
            print(f"Passing URI to get: {uri}")
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, data)

            # Delete
            print(f"Passing URI to delete: {uri}")
            await self.store.delete(uri)
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri)

        asyncio.run(async_test())

    def test_unversioned_update(self):
        async def async_test():
            asset_type = "test_type"
            asset_name = "test_asset_update"
            initial_data = b"initial data"
            updated_data = b"updated data"

            # Create
            uri = await self.store.create(
                asset_type, asset_name, initial_data
            )

            # Update
            print(f"Passing URI to update: {uri}")
            updated_uri = await self.store.update(uri, updated_data)
            self.assertEqual(updated_uri, uri) # URI should not change

            # Get updated data
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, updated_data)

        asyncio.run(async_test())

    def test_unversioned_list_assets(self):
        async def async_test():
            asset_type_1 = "type1"
            asset_type_2 = "type2"
            data = b"some data"

            await self.store.create(
                asset_type_1, "asset1", data
            )
            await self.store.create(
                asset_type_1, "asset2", data
            )
            await self.store.create(
                asset_type_2, "asset3", data
            )

            # List all
            all_assets = await self.store.list_assets()
            self.assertEqual(len(all_assets), 3)

            # List by type
            type1_assets = await self.store.list_assets(
                asset_type=asset_type_1
            )
            self.assertEqual(len(type1_assets), 2)
            self.assertTrue(
                any(uri.asset == "asset1" for uri in type1_assets)
            )
            self.assertTrue(
                any(uri.asset == "asset2" for uri in type1_assets)
            )

            # List with limit and offset
            paginated_assets = await self.store.list_assets(limit=1)
            self.assertEqual(len(paginated_assets), 1)

            paginated_assets_offset = await self.store.list_assets(
                limit=1, offset=1
            )
            self.assertEqual(len(paginated_assets_offset), 1)
            self.assertNotEqual(paginated_assets[0], paginated_assets_offset[0])

        asyncio.run(async_test())

    def test_unversioned_list_versions(self):
        async def async_test():
            asset_type = "version_test_type"
            asset_name = "version_test_asset"
            data = b"version data"

            uri = await self.store.create(
                asset_type, asset_name, data
            )

            print(f"Passing URI to list_versions: {uri}")
            versions = await self.store.list_versions(uri)
            self.assertEqual(versions, ["1"])

            # Non-existent asset
            dummy_uri = Uri.build(
                "test_unversioned", None, "non_existent_type", "non_existent_asset", "1"
            )
            versions = await self.store.list_versions(dummy_uri)
            self.assertEqual(versions, [])

        asyncio.run(async_test())


    def test_unversioned_set_dir(self):
        async def async_test():
            new_temp_dir = tempfile.TemporaryDirectory()
            new_store_path = pathlib.Path(new_temp_dir.name) / "new_unversioned_store"
            new_store_path.mkdir()

            self.store.set_dir(new_store_path)
            self.assertEqual(self.store._base_dir, new_store_path)

            # Clean up the new temporary directory
            new_temp_dir.cleanup()

        asyncio.run(async_test())


class TestPathToolVersionedLocalStore(unittest.TestCase):
    def setUp(self):
        # Create a temporary directory for testing
        self.tmp_dir = tempfile.TemporaryDirectory()
        self.tmp_path = pathlib.Path(self.tmp_dir.name)
        self.store = VersionedLocalStore("local", self.tmp_path)

    def tearDown(self):
        # Clean up the temporary directory
        self.tmp_dir.cleanup()

    def test_protocol(self):
        async def async_test():
            self.assertEqual(self.store.protocol, "local")
        asyncio.run(async_test())

    def test_create(self):
        async def async_test():
            data = b"test data"
            asset_type = "test_type"
            uri = await self.store.create(asset_type, "dummy_id", data)

            # Assert URI format (assuming Uri class handles parsing)
            self.assertTrue(str(uri).startswith(f"local:/{asset_type}/"))
            self.assertEqual(uri.asset_type, asset_type)
            self.assertIsNotNone(uri.asset)
            self.assertEqual(uri.version, "1")

            # Assert file created with correct content
            # This part assumes the URI maps to a path as per the plan's assumption
            # base_dir / type / asset / version
            expected_path = self.tmp_path / uri.asset_type / uri.asset / uri.version
            self.assertTrue(expected_path.exists())
            self.assertEqual(expected_path.read_bytes(), data)

        asyncio.run(async_test())

    def test_get(self):
        async def async_test():
            data = b"initial data"
            asset_type = "get_test"
            uri = await self.store.create(asset_type, "dummy_id", data)

            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, data)

            # Test get with non-existent URI
            non_existent_uri = Uri("local:///non_existent/asset/version")
            with self.assertRaises(FileNotFoundError):
                await self.store.get(non_existent_uri)

        asyncio.run(async_test())

    def test_update(self):
        async def async_test():
            initial_data = b"initial data for update"
            updated_data = b"updated data"
            asset_type = "update_test"
            uri = await self.store.create(asset_type, "dummy_id", initial_data)

            updated_uri = await self.store.update(uri, updated_data)

            retrieved_data = await self.store.get(updated_uri)
            self.assertEqual(retrieved_data, updated_data)

            # Test with version "latest"
            uri.version = "latest"
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, updated_data)

            # Test update with non-existent URI
            non_existent_uri = Uri("local:///non_existent/asset/version")
            with self.assertRaises(FileNotFoundError):
                await self.store.update(non_existent_uri, b"some data")

        asyncio.run(async_test())

    def test_delete(self):
        async def async_test():
            data = b"data to delete"
            asset_type = "delete_test"
            uri = await self.store.create(asset_type, "dummy_id", data)

            # This part assumes the URI maps to a path as per the plan's assumption
            # base_dir / type / asset / version
            expected_path = self.tmp_path / uri.asset_type / uri.asset / uri.version
            self.assertTrue(expected_path.exists())

            await self.store.delete(uri)
            self.assertFalse(expected_path.exists())

            # Test delete with non-existent URI (should not raise error)
            non_existent_uri = Uri("local:///non_existent/asset/version")
            await self.store.delete(non_existent_uri) # Should not raise

        asyncio.run(async_test())

    def test_get_latest_version(self):
        async def async_test():
            data1 = b"version 1"
            data2 = b"version 2"
            data3 = b"version 3"
            asset_type = "latest_test_type"
            # Use a fixed asset name for easier path construction in test
            asset = "test_asset_latest"

            # Manually create versions for testing 'latest'
            path1 = self.tmp_path / asset_type / asset / "1"
            path1.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path1, mode='wb') as f:
                await f.write(data1)

            path2 = self.tmp_path / asset_type / asset / "2"
            path2.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path2, mode='wb') as f:
                await f.write(data2)

            path3 = self.tmp_path / asset_type / asset / "3"
            path3.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path3, mode='wb') as f:
                await f.write(data3)

            # Get latest version
            latest_uri_str = f"local:///{asset_type}/{asset}/latest"
            retrieved_data = await self.store.get(latest_uri_str)
            self.assertEqual(retrieved_data, data3)

            # Get a specific version
            uri1_str = f"local:///{asset_type}/{asset}/1"
            retrieved_data_v1 = await self.store.get(uri1_str)
            self.assertEqual(retrieved_data_v1, data1)

        asyncio.run(async_test())

    def test_update_increments_version(self):
        async def async_test():
            initial_data = b"initial data for update"
            updated_data = b"updated data"
            asset_type = "update_increment_test"

            # Create initial version
            uri1 = await self.store.create(asset_type, "dummy_id", initial_data)

            # Update the asset
            uri2 = await self.store.update(uri1, updated_data)

            # Verify new version is created and contains updated data
            expected_path2 = self.tmp_path / uri2.asset_type / uri2.asset / uri2.version
            self.assertTrue(expected_path2.exists())
            async with aiofiles.open(expected_path2, mode='rb') as f:
                content2 = await f.read()
                self.assertEqual(content2, updated_data)

            # Verify old version still exists
            expected_path1 = self.tmp_path / uri1.asset_type / uri1.asset / uri1.version
            self.assertTrue(expected_path1.exists())
            async with aiofiles.open(expected_path1, mode='rb') as f:
                content1_after_update = await f.read()
                self.assertEqual(content1_after_update, initial_data)

            # Verify the version number is incremented
            self.assertEqual(int(uri2.version), int(uri1.version) + 1)
            self.assertEqual(uri1.asset, uri2.asset)
            self.assertEqual(uri1.asset_type, uri2.asset_type)

        asyncio.run(async_test())

    def test_delete_all_versions(self):
        async def async_test():
            data1 = b"version 1"
            data2 = b"version 2"
            asset_type = "delete_all_test_type"
            asset = "test_asset_delete_all"

            # Create version 1
            path1 = self.tmp_path / asset_type / asset / "1"
            path1.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path1, mode='wb') as f:
                await f.write(data1)

            # Create version 2
            path2 = self.tmp_path / asset_type / asset / "2"
            path2.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path2, mode='wb') as f:
                await f.write(data2)

            # Verify versions exist
            self.assertTrue(path1.exists())
            self.assertTrue(path2.exists())

            # Delete all versions using "latest"
            latest_uri_str = f"local:///{asset_type}/{asset}/latest"
            await self.store.delete(latest_uri_str)

            # Verify asset directory is removed
            asset_path = self.tmp_path / asset_type / asset
            self.assertFalse(asset_path.exists())

        asyncio.run(async_test())

    def test_delete_specific_version(self):
        async def async_test():
            data1 = b"version 1"
            data2 = b"version 2"
            asset_type = "delete_specific_test_type"
            asset = "test_asset_delete_specific"

            # Create version 1
            uri1_str = f"local:///{asset_type}/{asset}/1"
            path1 = self.tmp_path / asset_type / asset / "1"
            path1.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path1, mode='wb') as f:
                await f.write(data1)

            # Create version 2
            uri2_str = f"local:///{asset_type}/{asset}/2"
            path2 = self.tmp_path / asset_type / asset / "2"
            path2.parent.mkdir(parents=True, exist_ok=True)
            async with aiofiles.open(path2, mode='wb') as f:
                await f.write(data2)

            # Verify versions exist
            self.assertTrue(path1.exists())
            self.assertTrue(path2.exists())

            # Delete version 1
            await self.store.delete(uri1_str)

            # Verify version 1 is removed and version 2 still exists
            self.assertFalse(path1.exists())
            self.assertTrue(path2.exists())

            # Delete version 2
            await self.store.delete(uri2_str)

            # Verify version 2 is removed and asset directory is empty
            self.assertFalse(path2.exists())
            asset_path = self.tmp_path / asset_type / asset
            self.assertTrue(asset_path.exists()) # Directory should still exist
            self.assertFalse(any(asset_path.iterdir())) # But should be empty

        asyncio.run(async_test())

    def test_versioned_set_dir(self):
        async def async_test():
            new_temp_dir = tempfile.TemporaryDirectory()
            new_store_path = pathlib.Path(new_temp_dir.name) / "new_versioned_store"
            new_store_path.mkdir()

            self.store.set_dir(new_store_path)
            self.assertEqual(self.store._base_dir, new_store_path)

            # Clean up the new temporary directory
            new_temp_dir.cleanup()

        asyncio.run(async_test())

class TestPathToolFlatLocalStore(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.store_path = pathlib.Path(self.temp_dir.name) / "flat_store"
        self.store_path.mkdir()

        self.store = FlatLocalStore(
            protocol="test_flat",
            base_dir=self.store_path,
            file_extension=".dat"
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_flat_create_get_delete(self):
        async def async_test():
            asset_type = "ignored_type" # Should be ignored by FlatLocalStore
            asset_name = "flat_asset"
            data = b"flat test data"

            # Create
            uri = await self.store.create(
                asset_type, asset_name, data
            )
            self.assertIsInstance(uri, Uri)
            self.assertEqual(uri.protocol, "test_flat")
            # FlatLocalStore ignores asset_type and version in URI path construction
            # but the URI object itself might still hold these values from creation
            # We primarily test the path mapping and file operations
            self.assertEqual(uri.asset, asset_name)
            # The version is fixed to '1' in FlatLocalStore create
            self.assertEqual(uri.version, "1")

            # Verify file path
            expected_path = self.store_path / f"{asset_name}{self.store.file_extension}"
            self.assertTrue(expected_path.exists())
            self.assertEqual(expected_path.read_bytes(), data)

            # Get
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, data)

            # Delete
            await self.store.delete(uri)
            self.assertFalse(expected_path.exists())
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri)

        asyncio.run(async_test())

    def test_flat_update(self):
        async def async_test():
            asset_type = "ignored_type"
            asset_name = "flat_asset_update"
            initial_data = b"initial flat data"
            updated_data = b"updated flat data"

            # Create
            uri = await self.store.create(
                asset_type, asset_name, initial_data
            )

            # Update
            updated_uri = await self.store.update(uri, updated_data)
            self.assertEqual(updated_uri, uri) # URI should not change

            # Get updated data
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, updated_data)

            # Verify file content
            expected_path = self.store_path / f"{asset_name}{self.store.file_extension}"
            self.assertEqual(expected_path.read_bytes(), updated_data)

        asyncio.run(async_test())

    # FlatLocalStore does not support listing assets or versions in a meaningful way
    # based on the current implementation which ignores asset_type and version in path.
    # The list_assets and list_versions methods from the base class might need
    # to be overridden or adapted if listing is required for this store type.
    # For now, we will not add tests for list_assets or list_versions as they
    # are not explicitly required by step 2.2 for FlatLocalStore.
