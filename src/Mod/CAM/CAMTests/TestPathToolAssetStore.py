import unittest
import pathlib
import asyncio
import tempfile
from uuid import uuid4
from Path.Tool.assets import (
    AssetUri,
    AssetStore,
    MemoryStore,
    FileStore,
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
                asset_type="non_existent", asset_id="missing", version="1"
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
                asset_type="non_existent", asset_id="missing", version="1"
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

    def test_count_assets(self):
        async def async_test():
            self.assertEqual(await self.store.count_assets(), 0)
            self.assertEqual(await self.store.count_assets("type1"), 0)

            uri1 = await self.store.create("type1", f"asset1_{uuid4()}", b"data1")
            self.assertEqual(await self.store.count_assets(), 1)
            self.assertEqual(await self.store.count_assets("type1"), 1)
            self.assertEqual(await self.store.count_assets("type2"), 0)

            uri2 = await self.store.create("type2", f"asset2_{uuid4()}", b"data2")
            uri3 = await self.store.create("type1", f"asset3_{uuid4()}", b"data3")
            self.assertEqual(await self.store.count_assets(), 3)
            self.assertEqual(await self.store.count_assets("type1"), 2)
            self.assertEqual(await self.store.count_assets("type2"), 1)

            await self.store.delete(uri1)
            self.assertEqual(await self.store.count_assets(), 2)
            self.assertEqual(await self.store.count_assets("type1"), 1)
            self.assertEqual(await self.store.count_assets("type2"), 1)

            await self.store.delete(uri2)
            await self.store.delete(uri3)
            self.assertEqual(await self.store.count_assets(), 0)
            self.assertEqual(await self.store.count_assets("type1"), 0)
            self.assertEqual(await self.store.count_assets("type2"), 0)

        asyncio.run(async_test())

    def test_list_assets(self):
        async def async_test():
            asset_typedata = [
                ("type1", f"asset1_{uuid4()}", b"data1"),
                ("type1", f"asset2_{uuid4()}", b"data2"),
                ("type2", f"asset3_{uuid4()}", b"data3"),
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
            self.assertEqual(uri1.version, "1")
            self.assertEqual(uri2.version, "2")
            self.assertNotEqual(uri1.version, uri2.version)

            self.assertEqual(await self.store.get(uri1), initial_data)
            self.assertEqual(await self.store.get(uri2), updated_data)

            with self.assertRaises(FileNotFoundError):
                non_existent_uri = AssetUri.build(
                    asset_type="non_existent", asset_id="missing", version="1"
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
                asset_type="non_existent", asset_id="missing", version="1"
            )
            self.assertEqual(await self.store.list_versions(non_existent_uri), [])

        asyncio.run(async_test())

    def test_create_with_empty_data(self):
        async def async_test():
            data = b""
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            uri = await self.store.create(asset_type, asset_id, data)

            self.assertIsInstance(uri, AssetUri)
            retrieved_data = await self.store.get(uri)
            self.assertEqual(retrieved_data, data)

        asyncio.run(async_test())

    def test_list_assets_non_existent_type(self):
        async def async_test():
            assets = await self.store.list_assets(asset_type=f"non_existent_type_{uuid4()}")
            self.assertEqual(len(assets), 0)

        asyncio.run(async_test())

    def test_list_assets_pagination_offset_too_high(self):
        async def async_test():
            await self.store.create("shape", f"asset1_{uuid4()}", b"data")
            assets = await self.store.list_assets(offset=100)  # Assuming less than 100 assets
            self.assertEqual(len(assets), 0)

        asyncio.run(async_test())

    def test_list_assets_pagination_limit_zero(self):
        async def async_test():
            await self.store.create("shape", f"asset1_{uuid4()}", b"data")
            assets = await self.store.list_assets(limit=0)
            self.assertEqual(len(assets), 0)

        asyncio.run(async_test())

    def test_create_delete_recreate(self):
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            data1 = b"first data"
            data2 = b"second data"

            uri1 = await self.store.create(asset_type, asset_id, data1)
            self.assertEqual(await self.store.get(uri1), data1)
            # For versioned stores, this would be version "1"
            # For stores that don't deeply track versions, it's important what happens on recreate

            await self.store.delete(uri1)
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri1)

            uri2 = await self.store.create(asset_type, asset_id, data2)
            self.assertEqual(await self.store.get(uri2), data2)

            # Behavior of uri1.version vs uri2.version depends on store implementation
            # For a fully versioned store that starts fresh:
            self.assertEqual(
                uri2.version, "1", "Recreating should yield version 1 for a fresh start"
            )

            # Ensure only the new asset exists if the store fully removes old versions
            versions = await self.store.list_versions(
                AssetUri.build(asset_type=asset_type, asset_id=asset_id)
            )
            self.assertEqual(len(versions), 1)
            self.assertEqual(versions[0].version, "1")

        asyncio.run(async_test())

    def test_get_non_existent_specific_version(self):
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"
            await self.store.create(asset_type, asset_id, b"data_v1")

            non_existent_version_uri = AssetUri.build(
                asset_type=asset_type,
                asset_id=asset_id,
                version="99",  # Assuming version 99 won't exist
            )
            with self.assertRaises(FileNotFoundError):
                await self.store.get(non_existent_version_uri)

        asyncio.run(async_test())

    def test_delete_last_version(self):
        async def async_test():
            asset_type = "shape"
            asset_id = f"asset_{uuid4()}"

            uri_v1 = await self.store.create(asset_type, asset_id, b"v1_data")
            uri_v2 = await self.store.update(uri_v1, b"v2_data")

            await self.store.delete(uri_v2)  # Delete latest version
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri_v2)

            # v1 should still exist
            self.assertEqual(await self.store.get(uri_v1), b"v1_data")
            versions_after_v2_delete = await self.store.list_versions(uri_v1)
            self.assertEqual(len(versions_after_v2_delete), 1)
            self.assertEqual(versions_after_v2_delete[0].version, "1")

            await self.store.delete(uri_v1)  # Delete the now last version (v1)
            with self.assertRaises(FileNotFoundError):
                await self.store.get(uri_v1)

            versions_after_all_delete = await self.store.list_versions(uri_v1)
            self.assertEqual(len(versions_after_all_delete), 0)

            # Asset should not appear in list_assets
            listed_assets = await self.store.list_assets(asset_type=asset_type)
            self.assertFalse(any(a.asset_id == asset_id for a in listed_assets))
            self.assertTrue(await self.store.is_empty(asset_type))

        asyncio.run(async_test())

    def test_get_latest_on_non_existent_asset(self):
        async def async_test():
            latest_uri = AssetUri.build(
                asset_type="shape",
                asset_id=f"non_existent_id_for_latest_{uuid4()}",
                version="latest",
            )
            with self.assertRaises(
                FileNotFoundError
            ):  # Or custom NoVersionsFoundError if that's how store behaves
                await self.store.get(latest_uri)

        asyncio.run(async_test())


class TestPathToolFileStore(BaseTestPathToolAssetStore):
    """Test suite for FileStore with full versioning support."""

    def setUp(self):
        super().setUp()
        asset_type_map = {
            "*": "{asset_type}/{asset_id}/{version}",
            "special1": "Especial/{asset_id}/{version}",
            "special2": "my/super/{asset_id}.spcl",
        }
        self.store = FileStore("versioned", self.tmp_path, asset_type_map)

    def test_get_latest_version(self):
        async def async_test():
            asset_type = f"latest_{uuid4()}"
            asset_id = f"asset_{uuid4()}"

            uri1 = await self.store.create(asset_type, asset_id, b"v1")
            await self.store.update(uri1, b"v2")

            latest_uri = AssetUri.build(asset_type=asset_type, asset_id=asset_id, version="latest")
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


class TestPathToolMemoryStore(BaseTestPathToolAssetStore):
    """Test suite for MemoryStore."""

    def setUp(self):
        super().setUp()
        self.store = MemoryStore("memory_test")


if __name__ == "__main__":
    unittest.main()
