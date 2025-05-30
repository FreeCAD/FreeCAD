# -*- coding: utf-8 -*-
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
import FreeCAD
import Path
from typing import Optional, Tuple, Type, Iterable
from PySide.QtWidgets import QFileDialog, QMessageBox
from ..serializer import AssetSerializer, Asset
from .util import (
    make_import_filters,
    make_export_filters,
    get_serializer_from_extension,
)


class AssetOpenDialog(QFileDialog):
    def __init__(
        self,
        asset_class: Type[Asset],
        serializers: Iterable[Type[AssetSerializer]],
        parent=None,
    ):
        super().__init__(parent)
        self.asset_class = asset_class
        self.serializers = list(serializers)
        self.setFileMode(QFileDialog.ExistingFile)
        filters = make_import_filters(self.serializers)
        self.setNameFilters(filters)
        if filters:
            self.selectNameFilter(filters[0])  # Default to "All supported files"

    def _deserialize_selected_file(self, file_path: pathlib.Path) -> Optional[Asset]:
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
                asset = self._deserialize_selected_file(file_path)
                if asset:
                    return file_path, asset
        return None


class AssetSaveDialog(QFileDialog):
    def __init__(
        self,
        asset_class: Type[Asset],
        serializers: Iterable[Type[AssetSerializer]],
        parent=None,
    ):
        super().__init__(parent)
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
