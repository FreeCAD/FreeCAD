from typing import Type
from ...assets import AssetSerializer
from .camotics import CamoticsToolBitSerializer
from .fctb import FCTBSerializer


all_serializers = CamoticsToolBitSerializer, FCTBSerializer


def get_serializer_from_extension(extension: str) -> Type[AssetSerializer] | None:
    """
    Finds the appropriate serializer class based on the file extension.
    """
    normalized_extension = extension.lstrip('.').lower()
    for name in __all__:
        serializer_class = globals()[name]
        extensions = [ext.lstrip('.').lower() for ext in serializer_class.extensions]
        if normalized_extension in extensions:
            return serializer_class
    return None


__all__ = [
    "CamoticsToolBitSerializer",
    "FCTBSerializer",
    "all_serializers",
    "get_serializer_from_extension",
]
