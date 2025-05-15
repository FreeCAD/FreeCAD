import pathlib
import FreeCAD
from typing import Optional, Tuple, List, Type
from PySide.QtWidgets import QFileDialog, QMessageBox
from ..serializer import AssetSerializer, Asset
from .util import (
    get_serializer_from_extension,
    make_file_selector_filters,
    get_serializer_from_filter_string,
)


class AssetOpenDialog(QFileDialog):
    def __init__(self, asset_type: Type[Asset], serializers: List[AssetSerializer], parent=None):
        super().__init__(parent)
        self.asset_type = asset_type
        self.serializers = serializers
        self.setWindowTitle(f"Open a {asset_type.__name__}")

    def _deserialize_selected_file(
        self, file_path: pathlib.Path
    ) -> Optional[Asset]:
        """Reads and deserializes the selected file."""
        try:
            raw_data = file_path.read_bytes()
            file_extension = file_path.suffix.lower()

            # Find the correct serializer based on extension and import capability
            serializer_class = get_serializer_from_extension(
                self.serializers, file_extension, for_import=True
            )

            if not serializer_class:
                raise ValueError(
                    f"BUG: No supported serializer found for file "
                    f"extension '{file_extension}'"
                )

            asset = serializer_class.deep_deserialize(raw_data)
            if not isinstance(asset, self.asset_type):
                 raise TypeError(
                    f"Deserialized asset is not of expected type "
                    f"{self.asset_type.__name__}"
                 )
            return asset

        except Exception as e:
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM_Asset", f"Error Importing {self.asset_type.__name__}"),
                str(e),
            )
            return None

    def exec(self) -> Optional[Tuple[pathlib.Path, Asset]]:
        self.setFileMode(QFileDialog.ExistingFile)

        # Use the generic helper function for filters
        filters = make_file_selector_filters(self.serializers, for_import=True)
        self.setNameFilters(filters)

        # Select the "All Supported Files" filter by default if it exists
        if filters:
             self.selectNameFilter(filters[0])

        if super().exec_():
            filenames = self.selectedFiles()
            if filenames:
                file_path = pathlib.Path(filenames[0])
                asset = self._deserialize_selected_file(file_path)
                if asset:
                    return file_path, asset

        return None # Return None if dialog is canceled or no file selected


class AssetSaveDialog(QFileDialog):
    def __init__(self, asset_type: Type[Asset], serializers: List[AssetSerializer], parent=None):
        super().__init__(parent)
        self.asset_type = asset_type
        self.serializers = serializers
        self.setWindowTitle(f"Save a {asset_type.__name__}")

    def _serialize_selected_file(
        self, file_path: pathlib.Path, asset: Asset, serializer_class: AssetSerializer
    ) -> bool:
        """Serializes and writes the selected file."""
        try:
            raw_data = serializer_class.serialize(asset)
            file_path.write_bytes(raw_data)
            return True

        except Exception as e:
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM_Asset", f"Error Exporting {self.asset_type.__name__}"),
                str(e),
            )
            return False

    def exec(self, asset: Asset) -> Optional[Tuple[pathlib.Path, AssetSerializer]]:
        self.setFileMode(QFileDialog.AnyFile)
        self.setAcceptMode(QFileDialog.AcceptSave)

        # Use the generic helper function for filters
        filters = make_file_selector_filters(self.serializers, for_import=False)
        self.setNameFilters(filters)

        # Select the "All Supported Files" filter by default if it exists
        if filters:
             self.selectNameFilter(filters[0])

        if not super().exec_():
            return None
        filenames = self.selectedFiles()
        if not filenames:
            return None

        file_path = pathlib.Path(filenames[0])
        selected_filter = self.selectedNameFilter()

        # Find the serializer based on the selected filter
        serializer_class = get_serializer_from_filter_string(
            self.serializers, selected_filter
        )

        if not serializer_class:
            # This should not happen if filters are generated correctly
            raise ValueError(
                f"BUG: No supported serializer found for filter "
                f"'{selected_filter}'"
            )

        # Ensure the file has the correct extension for the selected
        # serializer if it doesn't already
        if serializer_class.extensions and file_path.suffix.lower() not in serializer_class.extensions:
            file_path = file_path.with_suffix(f".{serializer_class.extensions[0]}")

        if self._serialize_selected_file(file_path, asset, serializer_class):
            return file_path, serializer_class

        return None # Return None if save failed
