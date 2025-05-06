import unittest
from typing import Any, Dict, List, Type
from Path.Tool.assets.adapter import AssetAdapter
from Path.Tool.assets.uri import Uri


class DummyAdapter(AssetAdapter):
    @property
    def asset_name(self) -> str:
        return "dummy"

    @property
    def asset_class(self) -> Type[Any]:
        return object

    def serialize(self, obj: Any) -> bytes:
        return b"dummy_serialized_data"

    def dependencies(self, data: bytes) -> List[Uri]:
        return []

    def create(self, data: bytes, dependencies: Dict[Uri, Any]) -> Any:
        return "dummy_object"

    def id_of(self, obj: Any) -> str:
        # Dummy implementation for testing purposes
        return "dummy_id"


class TestPathToolAssetAdapter(unittest.TestCase):
    def test_asset_adapter_cannot_be_instantiated(self):
        with self.assertRaises(TypeError):
            AssetAdapter()

    def test_dummy_adapter_can_be_instantiated_and_has_members(self):
        adapter = DummyAdapter()
        self.assertIsInstance(adapter, AssetAdapter)
        self.assertEqual(adapter.asset_name, "dummy")
        self.assertEqual(adapter.asset_class, object)
        self.assertEqual(adapter.serialize("some_obj"), b"dummy_serialized_data")
        self.assertEqual(adapter.dependencies(b"some_data"), [])
        self.assertEqual(adapter.create(b"some_data", {}), "dummy_object")

if __name__ == '__main__':
    unittest.main()