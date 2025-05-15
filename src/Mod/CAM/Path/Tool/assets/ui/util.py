from typing import List, Optional
from ..serializer import AssetSerializer


def make_file_selector_filters(
    serializers: List[AssetSerializer], for_import: bool = True
) -> List[str]:
    """
    Generates file dialog filters for a QFileDialog from a list of serializers.

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


def get_serializer_from_filter_string(
    serializers: List[AssetSerializer], filter_string: str
) -> Optional[AssetSerializer]:
    """
    Finds a serializer class based on the selected filter string from a QFileDialog.

    Args:
        serializers: A list of AssetSerializer classes.
        filter_string: The selected filter string from QFileDialog.

    Returns:
        The matching AssetSerializer class, or None if not found.
    """
    for ser in serializers:
        label = ser.get_label()
        extensions = " ".join([f"*{ext}" for ext in ser.extensions])
        expected_filter_string = f"{label} ({extensions})"
        if expected_filter_string == filter_string:
            return ser
    return None


def get_serializer_from_extension(
    serializers: List[AssetSerializer], file_extension: str, for_import: bool | None = None
) -> Optional[AssetSerializer]:
    """
    Finds a serializer class based on the file extension and import capability.

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
