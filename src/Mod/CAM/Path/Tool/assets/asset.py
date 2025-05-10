import abc
from abc import ABC
from typing import Any, Mapping, List
from .uri import AssetUri

class Asset(ABC):
    asset_type: str

    def __init__(self, *args, **kwargs):
        if not hasattr(self, 'asset_type'):
            raise ValueError("Asset subclasses must define 'asset_type'.")

    @abc.abstractmethod
    def get_id(self) -> str:
        """Returns the unique ID of an asset object."""
        pass

    def get_uri(self) -> AssetUri:
        return AssetUri.build(
            asset_type=self.asset_type,
            asset_id=self.get_id()
        )

    @classmethod
    def get_url_from_id(cls, asset_id):
        return AssetUri.build(cls.asset_type, asset_id=asset_id)

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        return []

    @classmethod
    @abc.abstractmethod
    def from_bytes(cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Any]) -> Any:
        """Creates an object from serialized data and resolved dependencies."""
        pass

    @abc.abstractmethod
    def to_bytes(self) -> bytes:
        """Serializes an object into bytes."""
        pass
