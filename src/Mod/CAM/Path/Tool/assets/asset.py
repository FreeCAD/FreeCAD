import abc
from abc import ABC
from typing import Any, Mapping, List, Type
from .uri import AssetUri

class Asset(ABC):
    asset_type: str

    def __init__(self, *args, **kwargs):
        if not hasattr(self, 'asset_type'):
            raise ValueError("Asset subclasses must define 'asset_type'.")

    def dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        return []

    @abc.abstractclassmethod
    def from_bytes(cls, data: bytes, dependencies: Mapping[AssetUri, Type]) -> Any:
        """Creates an object from serialized data and resolved dependencies."""
        pass

    @abc.abstractmethod
    def to_bytes(self) -> bytes:
        """Serializes an object into bytes."""
        pass

    @abc.abstractmethod
    def get_id(self) -> str:
        """Returns the unique ID of an asset object."""
        pass
