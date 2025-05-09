# -*- coding: utf-8 -*-
import pathlib
import xml.etree.ElementTree as ET
from typing import Mapping, Optional, Type, List
from functools import cached_property
from ...assets import Asset, AssetUri, asset_manager
import Path.Tool.shape.util as util
from PySide import QtCore, QtGui, QtSvg

_svg_ns = {"s": "http://www.w3.org/2000/svg"}


class ToolBitShapeIcon(Asset):
    """Abstract base class for tool bit shape icons."""

    def __init__(self, id: str, data: bytes):
        """
        Initialize the icon.

        Args:
            id (str): The unique identifier for the icon, including extension.
            data (bytes): The raw icon data (e.g., SVG or PNG bytes).
        """
        self.id: str = id
        self.data: bytes = data

    def get_id(self) -> str:
        """
        Get the ID of the icon.

        Returns:
            str: The ID of the icon.
        """
        return self.id

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        """
        Extracts URIs of dependencies from serialized data.
        ToolBitShapeIcon files do not have external dependencies.
        """
        return []

    @classmethod
    def from_bytes(
        cls, data: bytes, id: str, dependencies: Mapping[AssetUri, Type]
    ) -> "ToolBitShapeIcon":
        """
        Create a ToolBitShapeIcon instance from raw bytes.

        Args:
            data (bytes): The raw bytes of the icon file.
            id (str): The ID of the asset, including extension.
            dependencies (Mapping[AssetUri, Type]): A mapping of resolved dependencies (not used).

        Returns:
            ToolBitShapeIcon: An instance of ToolBitShapeIcon.
        """
        return cls(id=id, data=data)

    def to_bytes(self) -> bytes:
        """
        Serializes a ToolBitShapeIcon object to bytes.
        """
        return self.data

    @classmethod
    def from_file(cls, filepath: pathlib.Path, id: str) -> "ToolBitShapeIcon":
        """
        Create a ToolBitShapeIcon instance from a file.

        Args:
            filepath (pathlib.Path): Path to the icon file (.svg or .png).
            shape_id_base (str): The base ID of the associated shape.

        Returns:
            ToolBitShapeIcon: An instance of ToolBitShapeIcon.

        Raises:
            FileNotFoundError: If the file does not exist.
        """
        if not filepath.exists():
            raise FileNotFoundError(f"Icon file not found: {filepath}")

        data = filepath.read_bytes()
        if filepath.suffix.lower() == '.png':
            return ToolBitShapePngIcon(id, data)
        elif filepath.suffix.lower() == '.svg':
            return ToolBitShapeSvgIcon(id, data)
        else:
            raise NotImplementedError(f"unsupported icon file: {filepath}")

    @classmethod
    def from_shape_data(cls, shape_data: bytes, id: str) -> Optional["ToolBitShapeIcon"]:
        """
        Create a thumbnail icon from shape data bytes.

        Args:
            shape_data (bytes): The raw bytes of the shape file (.FCStd).
            shape_id_base (str): The base ID of the associated shape.

        Returns:
            Optional[ToolBitShapeIcon]: An instance of ToolBitShapeIcon (PNG), or None.
        """
        image_bytes = util.create_thumbnail_from_data(shape_data)
        if not image_bytes:
            return None

        # Assuming create_thumbnail_from_data returns PNG data
        return ToolBitShapePngIcon(id=id, data=image_bytes)

    def get_size_in_bytes(self) -> int:
        """
        Get the size of the icon data in bytes.
        """
        return len(self.data)

    @cached_property
    def abbreviations(self) -> Mapping[str, str]:
        """
        Returns a cached mapping of parameter abbreviations from the icon data.
        """
        return {}

    def get_abbr(self, param_name: str) -> Optional[str]:
        """
        Retrieves the abbreviation for a given parameter name.

        Args:
            param_name: The name of the parameter.

        Returns:
            The abbreviation string, or None if not found.
        """
        normalized_param_name = param_name.lower().replace(" ", "_")
        return self.abbreviations.get(normalized_param_name)

    def get_png(self, icon_size: Optional[QtCore.QSize] = None) -> bytes:
        """
        Returns the icon data as PNG bytes.
        """
        raise NotImplementedError

    def get_qpixmap(self, icon_size: Optional[QtCore.QSize] = None) -> QtGui.QPixmap:
        """
        Returns the icon data as a QPixmap.
        """
        raise NotImplementedError


class ToolBitShapeSvgIcon(ToolBitShapeIcon):
    asset_type: str = "toolbitshapesvg"

    def get_png(self, icon_size: Optional[QtCore.QSize] = None) -> bytes:
        """
        Converts SVG icon data to PNG and returns it using QtSvg.
        """
        if icon_size is None:
            icon_size = QtCore.QSize(48, 48)
        image = QtGui.QImage(icon_size, QtGui.QImage.Format_ARGB32)
        image.fill(QtGui.Qt.transparent)
        painter = QtGui.QPainter(image)

        buffer = QtCore.QBuffer(QtCore.QByteArray(self.data))
        buffer.open(QtCore.QIODevice.ReadOnly)
        svg_renderer = QtSvg.QSvgRenderer(buffer)
        svg_renderer.setAspectRatioMode(QtCore.Qt.KeepAspectRatio)
        svg_renderer.render(painter)
        painter.end()

        byte_array = QtCore.QByteArray()
        buffer = QtCore.QBuffer(byte_array)
        buffer.open(QtCore.QIODevice.WriteOnly)
        image.save(buffer, "PNG")

        return bytes(byte_array)

    def get_qpixmap(self, icon_size: Optional[QtCore.QSize] = None) -> QtGui.QPixmap:
        """
        Returns the SVG icon data as a QPixmap using QtSvg.
        """
        if icon_size is None:
            icon_size = QtCore.QSize(48, 48)
        icon_ba = QtCore.QByteArray(self.data)
        image = QtGui.QImage(icon_size, QtGui.QImage.Format_ARGB32)
        image.fill(QtGui.Qt.transparent)
        painter = QtGui.QPainter(image)

        buffer = QtCore.QBuffer(icon_ba)  # PySide6
        buffer.open(QtCore.QIODevice.ReadOnly)
        data = QtCore.QXmlStreamReader(buffer)
        renderer = QtSvg.QSvgRenderer(data)
        renderer.setAspectRatioMode(QtCore.Qt.KeepAspectRatio)
        renderer.render(painter)
        painter.end()

        return QtGui.QPixmap.fromImage(image)

    @cached_property
    def abbreviations(self) -> Mapping[str, str]:
        """
        Returns a cached mapping of parameter abbreviations from the icon data.

        Only applicable for SVG icons.
        """
        if self.data:
            return self.get_abbreviations_from_svg(self.data)
        return {}

    def get_abbr(self, param_name: str) -> Optional[str]:
        """
        Retrieves the abbreviation for a given parameter name.

        Args:
            param_name: The name of the parameter.

        Returns:
            The abbreviation string, or None if not found.
        """
        normalized_param_name = param_name.lower().replace(" ", "_")
        return self.abbreviations.get(normalized_param_name)

    @staticmethod
    def get_abbreviations_from_svg(svg: bytes) -> Mapping[str, str]:
        """
        Extract abbreviations from SVG text elements.
        """
        try:
            tree = ET.fromstring(svg)
        except ET.ParseError:
            return {}

        result = {}
        for text_elem in tree.findall(".//s:text", _svg_ns):
            id = text_elem.attrib.get("id", _svg_ns)
            if id is None or not isinstance(id, str):
                continue

            abbr = text_elem.text
            if abbr is not None:
                result[id.lower()] = abbr

            span_elem = text_elem.find(".//s:tspan", _svg_ns)
            if span_elem is None:
                continue
            abbr = span_elem.text
            result[id.lower()] = abbr

        return result


class ToolBitShapePngIcon(ToolBitShapeIcon):
    asset_type: str = "toolbitshapepng"

    def get_png(self, icon_size: Optional[QtCore.QSize] = None) -> bytes:
        """
        Returns the PNG icon data.
        """
        # For PNG, resizing might be needed if icon_size is different
        # from the original size. Simple return for now.
        return self.data

    def get_qpixmap(self, icon_size: Optional[QtCore.QSize] = None) -> QtGui.QPixmap:
        """
        Returns the PNG icon data as a QPixmap.
        """
        if icon_size is None:
            icon_size = QtCore.QSize(48, 48)
        pixmap = QtGui.QPixmap()
        pixmap.loadFromData(self.data, "PNG")
        # Scale the pixmap if the requested size is different
        if pixmap.size() != icon_size:
            pixmap = pixmap.scaled(
                icon_size,
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation,
            )
        return pixmap


# Register the asset with the asset manager
asset_manager.register_asset(ToolBitShapeSvgIcon)
asset_manager.register_asset(ToolBitShapePngIcon)
