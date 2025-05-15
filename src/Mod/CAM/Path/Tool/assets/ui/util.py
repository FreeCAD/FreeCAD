from typing import List, Dict, Optional, Iterable, Type
from ..serializer import AssetSerializer

def make_import_filters(
    serializers: Iterable[Type[AssetSerializer]]
) -> List[str]:
    """
    Generates file dialog filters for importing assets.

    Args:
        serializers: A list of AssetSerializer classes.

    Returns:
        A list of filter strings, starting with "All supported files".
    """
    all_extensions = []
    filters = []

    for serializer_class in serializers:
        if not serializer_class.can_import or not serializer_class.extensions:
            continue
        all_extensions.extend(serializer_class.extensions)
        label = serializer_class.get_label()
        extensions = " ".join([f"*{ext}" for ext in serializer_class.extensions])
        filters.append(f"{label} ({extensions})")

    # Add "All supported files" filter if there are any extensions
    if all_extensions:
        combined_extensions = " ".join([f"*{ext}" for ext in sorted(list(set(all_extensions)))])
        filters.insert(0, f"All supported files ({combined_extensions})")

    return filters

def make_export_filters(
    serializers: Iterable[Type[AssetSerializer]]
) -> tuple[List[str], Dict[str, Type[AssetSerializer]]]:
    """
    Generates file dialog filters for exporting assets and a serializer map.

    Args:
        serializers: A list of AssetSerializer classes.

    Returns:
        A tuple of (filters, serializer_map) where filters is a list of filter strings
        starting with "Automatic", and serializer_map maps filter strings to serializers.
    """
    filters = ["Automatic (*)"]
    serializer_map = {}

    for serializer_class in serializers:
        if not serializer_class.can_export or not serializer_class.extensions:
            continue
        label = serializer_class.get_label()
        extensions = " ".join([f"*{ext}" for ext in serializer_class.extensions])
        filter_str = f"{label} ({extensions})"
        filters.append(filter_str)
        serializer_map[filter_str] = serializer_class

    return filters, serializer_map

def get_serializer_from_extension(
    serializers: Iterable[Type[AssetSerializer]], file_extension: str, for_import: bool | None = None
) -> Optional[Type[AssetSerializer]]:
    """
    Finds a serializer class based on the file extension and import/export capability.

    Args:
        serializers: A list of AssetSerializer classes.
        file_extension: The file extension (without the leading dot).
        for_import: None = both, True = import, False = export

    Returns:
        The matching AssetSerializer class, or None if not found.
    """
    for_export = for_import is not True
    for_import = for_import is True

    for ser in serializers:
        if for_import and not ser.can_import:
            continue
        if for_export and not ser.can_export:
            continue
        if file_extension in ser.extensions:
            return ser
    return None
