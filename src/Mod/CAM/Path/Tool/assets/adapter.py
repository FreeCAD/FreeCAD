import abc
from abc import ABC
from typing import Any, Dict, List, Type
from .uri import AssetUri

class AssetAdapter(ABC):
    asset_name: str  # name of the asset type handled by this adapter
    asset_class: Type[Any]  # class of the asset type handled by this adapter

    def __init__(self):
        if not hasattr(self, 'asset_name'):
            raise ValueError("AssetAdapter subclasses must define 'asset_name'.")
        if not hasattr(self, 'asset_class'):
            raise ValueError("AssetAdapter subclasses must define 'asset_class'.")

    @abc.abstractmethod
    def serialize(self, obj: Any) -> bytes:
        """Serializes an object into bytes."""
        pass

    @abc.abstractmethod
    def dependencies(self, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        pass

    @abc.abstractmethod
    def create(self, data: bytes, dependencies: Dict[AssetUri, Any]) -> Any:
        """Creates an object from serialized data and resolved dependencies."""
        pass

    @abc.abstractmethod
    def id_of(self, obj: Any) -> str:
        """Returns the unique ID of an asset object."""
        pass
