import pathlib
import FreeCAD
import FreeCADGui
from typing import Optional, Tuple, cast
from PySide.QtWidgets import QFileDialog
from ..serializers import all_serializers, get_serializer_from_extension
from .. import ToolBit


def get_supported_filters():
    all_extensions = []
    filters = []

    for serializer_class in all_serializers:
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

class ToolBitOpenDialog(QFileDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Open a Tool Bit")

    def _deserialize_selected_file(
        self, file_path: pathlib.Path
    ) -> Optional[ToolBit]:
        """Reads and deserializes the selected file."""
        try:
            raw_data = file_path.read_bytes()
            file_extension = file_path.suffix.lstrip('.').lower()
            serializer_class = get_serializer_from_extension(file_extension)

            if not serializer_class:
                raise ValueError(
                    f"BUG: No supported serializer found for file "
                    f"extension '{file_extension}'"
                )

            toolbit = serializer_class.deep_deserialize(raw_data)
            return cast(ToolBit, toolbit)

        except Exception as e:
            FreeCADGui.QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM_ToolBit", "Error Importing Toolbit"),
                str(e),
            )
            return None

    def exec(self) -> Optional[Tuple[pathlib.Path, ToolBit]]:
        self.setFileMode(QFileDialog.ExistingFile)

        filters = get_supported_filters()
        self.setNameFilters(filters)

        # Select the "All Supported Files" filter by default if it exists
        # Note: The filter string is dynamically generated now, so the exact
        # string "All Supported Files (*)" might not exist. We should select
        # the first filter, which is the combined one.
        if filters:
             self.selectNameFilter(filters[0])

        if super().exec_():
            filenames = self.selectedFiles()
            if filenames:
                file_path = pathlib.Path(filenames[0])
                toolbit = self._deserialize_selected_file(file_path)
                if toolbit:
                    return file_path, toolbit

        return None # Return None if dialog is canceled or no file selected
