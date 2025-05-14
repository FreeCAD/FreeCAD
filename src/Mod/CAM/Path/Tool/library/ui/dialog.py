import pathlib
import FreeCAD
import FreeCADGui
from typing import Optional, Tuple, cast
from PySide.QtWidgets import QFileDialog
from ...assets.serializer import make_file_selector_filters, AssetSerializer
from ..serializers import all_serializers
from .. import Library


class LibraryOpenDialog(QFileDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Open a Tool Library")

    def _deserialize_selected_file(
        self, file_path: pathlib.Path
    ) -> Optional[Library]:
        """Reads and deserializes the selected file."""
        try:
            raw_data = file_path.read_bytes()
            file_extension = file_path.suffix.lstrip('.').lower()

            # Find the correct serializer based on extension and import capability
            serializer_class: Optional[AssetSerializer] = None
            for ser in all_serializers:
                if file_extension in ser.extensions and ser.can_import:
                    serializer_class = ser
                    break

            if not serializer_class:
                raise ValueError(
                    f"BUG: No supported serializer found for file "
                    f"extension '{file_extension}'"
                )

            library = serializer_class.deep_deserialize(raw_data)
            return cast(Library, library)

        except Exception as e:
            FreeCADGui.QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM_Library", "Error Importing Tool Library"),
                str(e),
            )
            return None

    def exec(self) -> Optional[Tuple[pathlib.Path, Library]]:
        self.setFileMode(QFileDialog.ExistingFile)

        # Use the generic helper function for filters
        filters = make_file_selector_filters(all_serializers, for_import=True)
        self.setNameFilters(filters)

        # Select the "All Supported Files" filter by default if it exists
        if filters:
             self.selectNameFilter(filters[0])

        if super().exec_():
            filenames = self.selectedFiles()
            if filenames:
                file_path = pathlib.Path(filenames[0])
                library = self._deserialize_selected_file(file_path)
                if library:
                    return file_path, library

        return None # Return None if dialog is canceled or no file selected
