import unittest
from typing import Any, List, Mapping
from Path.Tool.assets import Asset, AssetUri


class DummyAsset(Asset):
    asset_type: str = "dummy"

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def from_bytes(cls, data: bytes, dependencies: Mapping[AssetUri, Any]) -> Any:
        return "dummy_object"

    def to_bytes(self) -> bytes:
        return b"dummy_serialized_data"

    def get_id(self) -> str:
        # Dummy implementation for testing purposes
        return "dummy_id"


class TestPathToolAsset(unittest.TestCase):
    def test_asset_cannot_be_instantiated(self):
        with self.assertRaises(TypeError):
            Asset()

    def test_dummy_asset_can_be_instantiated_and_has_members(self):
        asset = DummyAsset()
        self.assertIsInstance(asset, Asset)
        self.assertEqual(asset.asset_type, "dummy")
        self.assertEqual(asset.to_bytes(), b"dummy_serialized_data")
        self.assertEqual(DummyAsset.dependencies(b"some_data"), [])
        self.assertEqual(DummyAsset.from_bytes(b"some_data", {}), "dummy_object")
        self.assertEqual(asset.get_id(), "dummy_id")


if __name__ == '__main__':
    unittest.main()