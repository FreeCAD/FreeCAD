import pathlib
from typing import Optional, List, Type, Iterable
from PySide.QtWidgets import QFileDialog, QMessageBox
from ...assets import AssetSerializer
from ...assets.ui.util import (
    make_import_filters,
    get_serializer_from_extension,
)
from ..models.base import ToolBit
from ..serializers import all_serializers


class ToolBitOpenDialog(QFileDialog):
    def __init__(
        self,
        serializers: Iterable[Type[AssetSerializer]] | None,
        parent=None,
    ):
        super().__init__(parent)
        self.serializers = list(serializers) if serializers else all_serializers
        self.setWindowTitle("Open ToolBit(s)")
        self.setFileMode(QFileDialog.ExistingFiles)  # Allow multiple files
        filters = make_import_filters(self.serializers)
        self.setNameFilters(filters)
        if filters:
            self.selectNameFilter(filters[0])

    def _deserialize_selected_file(self, file_path: pathlib.Path) -> Optional[ToolBit]:
        """Deserialize the selected file using the appropriate serializer."""
        file_extension = file_path.suffix.lower()
        serializer_class = get_serializer_from_extension(
            self.serializers, file_extension, for_import=True
        )
        if not serializer_class:
            QMessageBox.critical(
                self,
                "Error",
                f"No supported serializer found for file extension '{file_extension}'",
            )
            return None
        try:
            raw_data = file_path.read_bytes()
            toolbit = serializer_class.deep_deserialize(raw_data)
            if not isinstance(toolbit, ToolBit):
                raise TypeError("Deserialized asset is not of type ToolBit")
            return toolbit
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to import toolbit: {e}")
            return None

    def exec(self) -> List[ToolBit]:
        toolbits = []
        if super().exec_():
            filenames = self.selectedFiles()
            for filename in filenames:
                file_path = pathlib.Path(filename)
                toolbit = self._deserialize_selected_file(file_path)
                if toolbit:
                    toolbits.append(toolbit)
        return toolbits
