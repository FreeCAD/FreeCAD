# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import pathlib
from typing import Optional, Tuple, Type, Iterable
from PySide.QtWidgets import QFileDialog, QMessageBox
from ...camassets import cam_assets
from ..manager import AssetManager
from ..serializer import AssetSerializer, Asset
from .util import (
    make_import_filters,
    make_export_filters,
    get_serializer_from_extension,
)
import Path
import Path.Preferences as Preferences


class AssetOpenDialog(QFileDialog):
    def __init__(
        self,
        asset_manager: AssetManager,
        asset_class: Type[Asset],
        serializers: Iterable[Type[AssetSerializer]],
        parent=None,
    ):
        super().__init__(parent)

        # Set default directory based on asset type
        default_dir = self._get_default_directory(asset_class)
        self.setDirectory(default_dir.as_posix())

        self.asset_class = asset_class
        self.asset_manager = asset_manager
        self.serializers = list(serializers)
        self.setFileMode(QFileDialog.ExistingFile)
        filters = make_import_filters(self.serializers)
        self.setNameFilters(filters)
        if filters:
            self.selectNameFilter(filters[0])  # Default to "All supported files"

    def deserialize_file(self, file_path: pathlib.Path, quiet=False) -> Optional[Asset]:
        """Deserialize the selected file using the appropriate serializer."""
        # Find the correct serializer for the file.
        file_extension = file_path.suffix.lower()
        serializer_class = get_serializer_from_extension(
            self.serializers, file_extension, for_import=True
        )
        if not serializer_class:
            message = f"No supported serializer found for file extension '{file_extension}'"
            if quiet:
                Path.Log.error(message)
            else:
                QMessageBox.critical(
                    self,
                    "Error",
                    message,
                )
            return None

        # Check whether all dependencies for importing the file exist.
        try:
            raw_data = file_path.read_bytes()
            dependencies = serializer_class.extract_dependencies(raw_data)
            external_toolbits = []  # Track toolbits found externally

            for dependency_uri in dependencies:
                # First check if dependency exists in asset manager stores
                if self.asset_manager.exists(dependency_uri, store=["local", "builtin"]):
                    continue

                # If not in stores, check if it exists relative to the library file
                dependency_found = False
                if dependency_uri.asset_type == "toolbit":
                    # Look for toolbit files in parallel Bit directory
                    # Library is in Library/, toolbits are in parallel Bit/
                    library_dir = file_path.parent  # e.g., /path/to/Library/
                    tools_dir = library_dir.parent  # e.g., /path/to/
                    bit_dir = tools_dir / "Bit"  # e.g., /path/to/Bit/

                    if bit_dir.exists():
                        possible_extensions = [".fctb", ".json", ".yaml", ".yml"]
                        for ext in possible_extensions:
                            toolbit_file = bit_dir / f"{dependency_uri.asset_id}{ext}"
                            if toolbit_file.exists():
                                dependency_found = True
                                external_toolbits.append((dependency_uri, toolbit_file))
                                break

                if not dependency_found:
                    message = f"Failed to import {file_path}: required dependency {dependency_uri} not found in stores or in parallel Bit directory"
                    if quiet:
                        Path.Log.error(message)
                    else:
                        QMessageBox.critical(
                            self,
                            "Error",
                            message,
                        )
                    return None

            # If we found external toolbits, ask user if they want to import them
            if external_toolbits:
                toolbit_names = [uri.asset_id for uri, _ in external_toolbits]
                if quiet:
                    Path.Log.info("Importing tool bits for the library")
                    reply = QMessageBox.Yes
                else:
                    reply = QMessageBox.question(
                        self,
                        "Import External Toolbits",
                        f"This library references {len(external_toolbits)} toolbit(s) that are not in your local store:\n\n"
                        + "\n".join(f"• {name}" for name in toolbit_names)
                        + f"\n\nWould you like to import these toolbits into your local store?\n"
                        + "This will make them permanently available for use in other libraries.",
                        QMessageBox.Yes | QMessageBox.No,
                        QMessageBox.Yes,
                    )

                if reply == QMessageBox.Yes:
                    # Import the external toolbits into local store
                    self._import_external_toolbits(external_toolbits, quiet=quiet)
                    # After importing, use regular deserialization since toolbits are now in local store
                else:
                    # User declined import, use context-aware deserialization for external loading
                    pass
            else:
                # No external toolbits found, use regular deserialization
                pass
        except Exception as e:
            QMessageBox.critical(self, "Error", f"{file_path}: Failed to check dependencies: {e}")
            return None

        # Load and return the asset.
        try:
            # Always use context-aware deserialization for libraries to get meaningful names
            if hasattr(serializer_class, "deep_deserialize_with_context"):
                # Pass file path context for meaningful library names and external dependency resolution
                asset = serializer_class.deep_deserialize_with_context(raw_data, file_path)
            else:
                # Fallback to regular deserialization
                asset = serializer_class.deep_deserialize(raw_data)

            if not isinstance(asset, self.asset_class):
                raise TypeError(f"Deserialized asset is not of type {self.asset_class.__name__}")
            return asset
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to import asset: {e}")
            return None

    def exec_(self) -> Optional[Tuple[pathlib.Path, Asset]]:
        if super().exec_():
            filenames = self.selectedFiles()
            if filenames:
                file_path = pathlib.Path(filenames[0])
                asset = self.deserialize_file(file_path)
                if asset:
                    return file_path, asset
        return None

    def _get_default_directory(self, asset_class: Type[Asset]) -> pathlib.Path:
        """Get the appropriate default directory based on asset type."""
        try:
            asset_path = Preferences.getAssetPath()

            # Check asset type to determine subdirectory
            asset_type = getattr(asset_class, "asset_type", None)
            if asset_type == "toolbit":
                return asset_path / "Tool" / "Bit"
            elif asset_type == "library" or asset_type == "toolbitlibrary":
                return asset_path / "Tool" / "Library"
            else:
                # Default to asset path root for unknown types
                return asset_path
        except Exception:
            # Fallback to home directory if anything goes wrong
            return pathlib.Path.home()

    def _import_external_toolbits(self, external_toolbits, quiet=False):
        """Import external toolbits into the local asset store."""
        from ...toolbit.serializers import all_serializers as toolbit_serializers
        from .util import get_serializer_from_extension

        imported_count = 0
        failed_imports = []

        for dependency_uri, toolbit_file in external_toolbits:
            try:
                # Find appropriate serializer for the file
                file_extension = toolbit_file.suffix.lower()
                serializer_class = get_serializer_from_extension(
                    toolbit_serializers, file_extension, for_import=True
                )

                if not serializer_class:
                    failed_imports.append(
                        f"{dependency_uri.asset_id}: No serializer for {file_extension}"
                    )
                    continue

                # Load and deserialize the toolbit
                raw_toolbit_data = toolbit_file.read_bytes()
                toolbit = serializer_class.deep_deserialize(raw_toolbit_data)

                # Ensure the toolbit ID matches what the library expects
                if toolbit.id != dependency_uri.asset_id:
                    toolbit.id = dependency_uri.asset_id

                # Import the toolbit into local store
                cam_assets.add(toolbit)
                imported_count += 1

            except Exception as e:
                failed_imports.append(f"{dependency_uri.asset_id}: {str(e)}")

        # Show results to user
        if imported_count > 0:
            message = f"Successfully imported {imported_count} toolbit(s) into your local store."
            if failed_imports:
                message += f"\n\nFailed to import {len(failed_imports)} toolbit(s):\n" + "\n".join(
                    failed_imports
                )
            if quiet:
                Path.Log.info(message)
            else:
                QMessageBox.information(self, "Import Results", message)
        elif failed_imports:
            message = f"Failed to import all {len(failed_imports)} toolbit(s):\n" + "\n".join(
                failed_imports
            )
            if quiet:
                Path.Log.error(message)
            else:
                QMessageBox.warning(self, "Import Failed", message)


class AssetSaveDialog(QFileDialog):
    def __init__(
        self,
        asset_class: Type[Asset],
        serializers: Iterable[Type[AssetSerializer]],
        parent=None,
    ):
        super().__init__(parent)

        # Set default directory based on asset type
        default_dir = self._get_default_directory(asset_class)
        self.setDirectory(default_dir.as_posix())
        self.asset_class = asset_class
        self.serializers = list(serializers)
        self.setFileMode(QFileDialog.AnyFile)
        self.setAcceptMode(QFileDialog.AcceptSave)
        self.filters, self.serializer_map = make_export_filters(self.serializers)
        self.setNameFilters(self.filters)
        if self.filters:
            self.selectNameFilter(self.filters[0])  # Default to "Automatic"
        self.filterSelected.connect(self.update_default_suffix)

    def update_default_suffix(self, filter_str: str):
        """Update the default suffix based on the selected filter."""
        if filter_str == "Automatic (*)":
            self.setDefaultSuffix("")  # No default for Automatic
        else:
            serializer = self.serializer_map.get(filter_str)
            if serializer and serializer.extensions:
                self.setDefaultSuffix(serializer.extensions[0])

    def _serialize_selected_file(
        self,
        file_path: pathlib.Path,
        asset: Asset,
        serializer_class: Type[AssetSerializer],
    ) -> bool:
        """Serialize and save the asset."""
        try:
            raw_data = serializer_class.serialize(asset)
            file_path.write_bytes(raw_data)
            return True
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to export asset: {e}")
            return False

    def _get_default_directory(self, asset_class: Type[Asset]) -> pathlib.Path:
        """Get the appropriate default directory based on asset type."""
        # For exports, default to home directory instead of CAM assets path
        return pathlib.Path.home()

    def exec_(self, asset: Asset) -> Optional[Tuple[pathlib.Path, Type[AssetSerializer]]]:
        self.setWindowTitle(f"Save {asset.label or self.asset_class.asset_type}")
        if super().exec_():
            selected_filter = self.selectedNameFilter()
            file_path = pathlib.Path(self.selectedFiles()[0])
            if selected_filter == "Automatic (*)":
                if not file_path.suffix:
                    QMessageBox.critical(
                        self,
                        "Error",
                        "Please specify a file extension for automatic serializer selection.",
                    )
                    return None
                file_extension = file_path.suffix.lower()
                serializer_class = get_serializer_from_extension(
                    self.serializers, file_extension, for_import=False
                )
                if not serializer_class:
                    QMessageBox.critical(
                        self,
                        "Error",
                        f"No serializer found for extension '{file_extension}'",
                    )
                    return None
            else:
                serializer_class = self.serializer_map.get(selected_filter)
                if not serializer_class:
                    raise ValueError(f"No serializer found for filter '{selected_filter}'")
            if self._serialize_selected_file(file_path, asset, serializer_class):
                return file_path, serializer_class
        return None
