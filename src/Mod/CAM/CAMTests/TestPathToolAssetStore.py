import unittest
import pathlib
import asyncio
import tempfile
from uuid import uuid4
from Path.Tool.assets import (
    AssetUri,
    AssetStore,
    MemoryStore,
    FlatLocalStore,
    VersionedLocalStore,
)


class BaseTestPathToolAssetStore(unittest.TestCase):
    """
    Base test suite for Path Tool Asset Stores assuming full versioning support.
    Store-agnostic tests without direct file system access.
    """
    store: AssetStore

    def setUp(self):
        self.tmp_dir = tempfile.TemporaryDirectory()
        self.tmp_path = pathlib.Path(self.tmp_dir.name)

    def tearDown(self):
        self.tmp_dir.cleanup()

    def test_name(self):
        self.assertIsNotNone(self.store)
        self.assertIsInstance(self.store.name, str)
        self.assertTrue(len(self.store.name) > 0)

    def test_create_and_get(self):
        async def async_test():
            data = b"test data"
            asset_type = f"type_{uuid4()}"
            asset_id = f"asset_{uuid4()}"
            uri = await self.store.create(asset_type, asset_id, data)

            self.assertIsInstance(uri, AssetUri)
            self.assertEqual(uri.asset_type, asset_type)
            self.assertEqual(uri.asset_id, asset_id)
            self.assertIsNotNone(uri.version)

            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, data)

            # Test non-existent URI
            non_existent_uri = AssetUri.build(
                asset_type="non_existent",
                asset_id="missing",
                version="1"
            )
            with self.assertRaises(FileNotFoundError):
                await self.store.get(non_existent_uri)

        asyncio.run(async_test())

    def test_delete(self):
        async def async_test():
            data = b"data to delete"
            asset_type = "delete_type"
            asset_id = f"asset_{uuid4()}"
            uri = await self.store.create(asset_type, asset_id, data)

            await self.store.delete(uri)

            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri)

            # Deleting non-existent URI should not raise
            non_existent_uri = AssetUri.build(
                asset_type="non_existent",
                asset_id="missing",
                version="1"
            )
            await self.store.delete(non_existent_uri)

        asyncio.run(async_test())

    def test_is_empty(self):
        async def async_test():
            self.assertTrue(await self.store.is_empty())
            self.assertTrue(await self.store.is_empty("type1"))

            uri1 = await self.store.create("type1", f"asset_{uuid4()}", b"data")
            self.assertFalse(await self.store.is_empty())
            self.assertFalse(await self.store.is_empty("type1"))
            self.assertTrue(await self.store.is_empty("type2"))

            uri2 = await self.store.create("type2", f"asset_{uuid4()}", b"data")
            await self.store.delete(uri1)
            self.assertFalse(await self.store.is_empty())
            self.assertTrue(await self.store.is_empty("type1"))
            self.assertFalse(await self.store.is_empty("type2"))

            await self.store.delete(uri2)
            self.assertTrue(await self.store.is_empty())

        asyncio.run(async_test())

    def test_list_assets(self):
        async def async_test():
            asset_typedata = [
                ("type1", f"asset1_{uuid4()}", b"data1"),
                ("type1", f"asset2_{uuid4()}", b"data2"),
                ("type2", f"asset3_{uuid4()}", b"data3")
            ]
            
            uris = []
            for asset_type, asset_id, data in asset_typedata:
                uri = await self.store.create(asset_type, asset_id, data)
                uris.append(uri)

            all_assets = await self.store.list_assets()
            self.assertEqual(len(all_assets), 3)
            for uri in uris:
                self.assertTrue(any(u.asset_id == uri.asset_id for u in all_assets))

            type1_assets = await self.store.list_assets(asset_type="type1") 
            self.assertEqual(len(type1_assets), 2)
            self.assertTrue(any(u.asset_id == uris[0].asset_id for u in type1_assets))
            self.assertTrue(any(u.asset_id == uris[1].asset_id for u in type1_assets))

            paginated = await self.store.list_assets(limit=2)
            self.assertEqual(len(paginated), 2)

        asyncio.run(async_test())

    def test_update_versioning(self):
        async def async_test():
            initial_data = b"initial data"
            updated_data = b"updated data"
            asset_type = f"update_{uuid4()}"
            asset_id = f"asset_{uuid4()}"

            uri1 = await self.store.create(asset_type, asset_id, initial_data)
            uri2 = await self.store.update(uri1, updated_data)

            self.assertEqual(uri1.asset_type, uri2.asset_type)
            self.assertEqual(uri1.asset_id, uri2.asset_id)
            self.assertNotEqual(uri1.version, uri2.version)

            self.assertEqual(await self.store.get(uri1), initial_data)
            self.assertEqual(await self.store.get(uri2), updated_data)

            with self.assertRaises(FileNotFoundError):
                non_existent_uri = AssetUri.build(
                    asset_type="non_existent",
                    asset_id="missing",
                    version="1"
                )
                await self.store.update(non_existent_uri, b"data")

        asyncio.run(async_test())

    def test_list_versions(self):
        async def async_test():
            asset_type = f"version_{uuid4()}"
            asset_id = f"asset_{uuid4()}"
            data1 = b"version1"
            data2 = b"version2"

            uri1 = await self.store.create(asset_type, asset_id, data1)
            uri2 = await self.store.update(uri1, data2)

            versions = await self.store.list_versions(uri1)
            self.assertEqual(len(versions), 2)
            version_ids = {v.version for v in versions if v.version}
            self.assertEqual(version_ids, {uri1.version, uri2.version})

            non_existent_uri = AssetUri.build(
                asset_type="non_existent",
                asset_id="missing",
                version="1"
            )
            self.assertEqual(await self.store.list_versions(non_existent_uri), [])

        asyncio.run(async_test())


class TestPathToolVersionedLocalStore(BaseTestPathToolAssetStore):
    """Test suite for VersionedLocalStore with full versioning support."""

    def setUp(self):
        super().setUp()
        self.store = VersionedLocalStore("versioned", self.tmp_path)

    def test_get_latest_version(self):
        async def async_test():
            asset_type = f"latest_{uuid4()}"
            asset_id = f"asset_{uuid4()}"
            
            uri1 = await self.store.create(asset_type, asset_id, b"v1")
            await self.store.update(uri1, b"v2")

            latest_uri = AssetUri.build(
                asset_type=asset_type,
                asset_id=asset_id,
                version="latest"
            )
            self.assertEqual(await self.store.get(latest_uri), b"v2")

        asyncio.run(async_test())

    def test_delete_all_versions(self):
        async def async_test():
            asset_type = f"delete_{uuid4()}"
            asset_id = f"asset_{uuid4()}"
            
            uri1 = await self.store.create(asset_type, asset_id, b"v1")
            await self.store.update(uri1, b"v2")
            
            uri = AssetUri.build(asset_type=asset_type, asset_id=asset_id)
            await self.store.delete(uri)
            
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri1)

        asyncio.run(async_test())


class TestPathToolFlatLocalStore(BaseTestPathToolAssetStore):
    """Test suite for FlatLocalStore with limited versioning support."""

    def setUp(self):
        super().setUp()
        self.expected_store_name = "flat_test"
        self.type_to_extension = {
            "shape": ".fcs",
            "delete_type": ".del",
            "type1": ".tp1",
            "type2": ".tp2",
            "type3": ".tp3"
        }
        self.store = FlatLocalStore(
            name=self.expected_store_name,
            base_dir=self.tmp_path,
            type_to_extension=self.type_to_extension
        )

    def test_create_and_get(self):  # Override due to fixed version
        async def async_test():
            for asset_type in self.type_to_extension:
                asset_id = f"asset_{uuid4()}"
                data = f"data_{asset_type}".encode('utf-8')
                
                uri = await self.store.create(asset_type, asset_id, data)
                self.assertIsInstance(uri, AssetUri)
                self.assertEqual(uri.asset_type, asset_type)
                self.assertEqual(uri.asset_id, asset_id)
                self.assertEqual(uri.version, "1")  # Fixed version
                
                self.assertEqual(await self.store.get(uri), data)

        asyncio.run(async_test())

    def test_update_versioning(self):  # Override due to no version increment
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            initial_data = b"initial"
            updated_data = b"updated"

            uri1 = await self.store.create(asset_type, asset_id, initial_data)
            uri2 = await self.store.update(uri1, updated_data)

            self.assertEqual(uri1, uri2)  # URI remains identical
            self.assertEqual(uri1.version, "1")
            self.assertEqual(await self.store.get(uri1), updated_data)

        asyncio.run(async_test())

    def test_list_versions(self):  # Override due to single version
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            data = b"data"

            uri = await self.store.create(asset_type, asset_id, data)
            versions = await self.store.list_versions(uri)
            
            self.assertEqual(len(versions), 1)
            self.assertEqual(versions[0].version, "1")
            self.assertEqual(versions[0].asset_id, asset_id)
            self.assertEqual(versions[0].asset_type, asset_type)

            non_existent_uri = AssetUri.build(
                asset_type="non_existent",
                asset_id="missing"
            )
            self.assertEqual(await self.store.list_versions(non_existent_uri), [])

        asyncio.run(async_test())

    def test_get_latest_version(self):  # Override to ensure fixed version
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            data = b"data"

            uri = await self.store.create(asset_type, asset_id, data)
            latest_uri = AssetUri.build(
                asset_type=asset_type,
                asset_id=asset_id,
                version="latest"
            )
            
            self.assertEqual(await self.store.get(latest_uri), data)
            self.assertEqual(uri.version, "1")

        asyncio.run(async_test())


class TestPathToolMemoryStore(BaseTestPathToolAssetStore):
    """Test suite for MemoryStore."""

    def setUp(self):
        super().setUp()
        self.store = MemoryStore("memory_test")


if __name__ == '__main__':
    unittest.main()