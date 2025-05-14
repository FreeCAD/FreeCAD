import abc
from abc import ABC
from typing import Mapping, List, Optional, Tuple, Type
from .uri import AssetUri
from .asset import Asset


class AssetSerializer(ABC):
    for_class: Type[Asset]
    extensions: Tuple[str] = tuple()
    mime_type: str

    @classmethod
    @abc.abstractmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        pass

    @classmethod
    @abc.abstractmethod
    def serialize(cls, asset: Asset) -> bytes:
        """Serializes an asset object into bytes."""
        pass

    @classmethod
    @abc.abstractmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> "Asset":
        """
        Creates an asset object from serialized data and resolved dependencies.
        If dependencies is None, it indicates a shallow load where dependencies
        were not resolved.
        """
        pass


class DummyAssetSerializer(AssetSerializer):
    """
    A serializer that does nothing. Can be used by simple assets that don't
    need a non-native serialization. These type of assets can implement
    extract_dependencies(), to_bytes() and from_bytes() methods that ignore
    the given serializer.
    """
    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: Asset) -> bytes:
        return b""

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> Asset:
        raise RuntimeError("DummySerializer.deserialize() was called")
