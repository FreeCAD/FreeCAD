import abc
from abc import ABC
from typing import Mapping, List, Optional, Tuple, Type
from .uri import AssetUri
from .asset import Asset


class AssetSerializer(ABC):
    for_class: Type[Asset]
    extensions: Tuple[str] = tuple()
    mime_type: str
    can_import: bool = True
    can_export: bool = True

    @classmethod
    @abc.abstractmethod
    def get_label(cls) -> str:
        pass

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

    @classmethod
    @abc.abstractmethod
    def deep_deserialize(cls, data: bytes) -> Asset:
        """
        Like deserialize(), but builds dependencies itself if they are
        sufficiently defined in the data.

        This method is used for export/import, where some dependencies
        may be embedded in the data, while others may not.
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


def make_file_selector_filters(
    serializers: List[AssetSerializer], for_import: bool = True
) -> List[str]:
    """
    Generates file dialog filters based on a list of serializers.

    Args:
        serializers: A list of AssetSerializer classes.
        for_import: If True, filters for importable assets; otherwise,
                    filters for exportable assets.

    Returns:
        A list of filter strings for use in QFileDialog.
    """
    all_extensions = []
    filters = []

    for serializer_class in serializers:
        if for_import and not serializer_class.can_import:
            continue
        if not for_import and not serializer_class.can_export:
            continue
        if not serializer_class.extensions:
            continue

        # Collect extensions for the combined filter
        all_extensions.extend(serializer_class.extensions)

        # Add individual serializer filter
        label = serializer_class.get_label()
        extensions = " ".join([f"*{ext}" for ext in serializer_class.extensions])
        filters.append(f"{label} ({extensions})")

    # Create the "All Supported Files" filter
    combined_extensions = " ".join([f"*{ext}" for ext in sorted(list(set(all_extensions)))])
    all_supported_filter = f"All Supported Files ({combined_extensions})"

    # Insert the combined filter at the beginning
    filters.insert(0, all_supported_filter)

    return filters
