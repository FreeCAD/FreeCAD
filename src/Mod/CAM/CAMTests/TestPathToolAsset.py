import unittest
from typing import Any, List, Mapping
from Path.Tool.assets import Asset, AssetUri


class TestAsset(Asset):
    asset_type: str = "test_asset"

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def from_bytes(cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]) -> Any:
        return "dummy_object"

    def to_bytes(self) -> bytes:
        return b"dummy_serialized_data"

    def get_id(self) -> str:
        # Dummy implementation for testing purposes
        return "dummy_id"


class TestPathToolAsset(unittest.TestCase):
    def test_asset_cannot_be_instantiated(self):
        with self.assertRaises(TypeError):
            Asset()  # type: ignore

    def test_asset_can_be_instantiated_and_has_members(self):
        asset = TestAsset()
        self.assertIsInstance(asset, Asset)
        self.assertEqual(asset.asset_type, "test_asset")
        self.assertEqual(asset.to_bytes(), b"dummy_serialized_data")
        self.assertEqual(TestAsset.dependencies(b"some_data"), [])
        self.assertEqual(TestAsset.from_bytes(b"some_data", "some_id", {}), "dummy_object")
        self.assertEqual(asset.get_id(), "dummy_id")


if __name__ == "__main__":
    unittest.main()
