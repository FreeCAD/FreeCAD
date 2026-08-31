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
import re
import xml.etree.ElementTree as ET
from typing import Mapping, Optional
from functools import cached_property, lru_cache
from ...assets import Asset, AssetUri, AssetSerializer, DummyAssetSerializer
import Path.Tool.shape.util as util
from Path.Base.Gui.Theme import is_dark_theme
from PySide import QtCore, QtGui, QtSvg

_svg_ns = {"s": "http://www.w3.org/2000/svg"}

# Dimension lines, arrows and labels in the shape SVGs are drawn in sentinel
# colors that never appear in the tool artwork itself, and are recolored to suit
# the active theme before the icon is rendered. Two sentinels tell apart the
# dimensions drawn over the empty background from those drawn on top of the tool
# body; both currently render in the same color, but the distinction is kept in
# the artwork so the two can be tuned apart again without re-editing the SVGs.
DIMENSION_SENTINEL_BACKGROUND = b"#ff0000"  # dimension over the empty background
DIMENSION_SENTINEL_OVER_BODY = b"#00ff00"  # dimension over the tool body
DIMENSION_SENTINELS = (DIMENSION_SENTINEL_BACKGROUND, DIMENSION_SENTINEL_OVER_BODY)
DIMENSION_COLOR_DARK = b"#23818d"  # amber: the artwork is grayscale, so a hue reads clearly
DIMENSION_COLOR_LIGHT = b"#111111"

# The tool artwork is drawn as light gray steel for a white page. On a dark theme
# it is scaled toward black so that it does not glare, and so that the dimensions
# crossing it stay legible in the same color as the ones beside it.
ARTWORK_DARK_FACTOR = 0.85

# Color of the one dimension the user is pointing at, in either theme.
DIMENSION_COLOR_HIGHLIGHT = b"#ff8c00"

_tag_re = re.compile(rb'<(/?)([A-Za-z_][\w.:-]*)((?:[^<>"]|"[^"]*")*?)(/?)>', re.S)


def _element_span(data: bytes, element_id: str):
    """Byte range of the element carrying `element_id`, its subtree included."""
    needle = b'id="%s"' % element_id.encode()
    for match in _tag_re.finditer(data):
        closing, tag, attrs, selfclose = match.groups()
        if closing or needle not in attrs:
            continue
        if selfclose:
            return match.start(), match.end()
        depth = 0
        for tag_match in _tag_re.finditer(data, match.start()):
            if tag_match.group(2) != tag:
                continue
            depth += -1 if tag_match.group(1) else (0 if tag_match.group(4) else 1)
            if depth == 0:
                return match.start(), tag_match.end()
        return match.start(), match.end()
    return None


def highlight_elements(data: bytes, element_ids, color: bytes = DIMENSION_COLOR_HIGHLIGHT):
    """Repaint some elements of an already themed SVG, leaving the rest alone."""
    spans = []
    for element_id in element_ids:
        span = _element_span(data, element_id)
        if span is not None:
            spans.append(span)
    for start, end in sorted(set(spans), reverse=True):
        chunk = data[start:end]
        for themed in (DIMENSION_COLOR_DARK, DIMENSION_COLOR_LIGHT):
            chunk = chunk.replace(themed, color)
        data = data[:start] + chunk + data[end:]
    return data


_marker_re = re.compile(rb"marker-(?:start|mid|end)\s*:\s*url\(#([^)]+)\)")


def _iter_dimension_roots(elem):
    """
    Yield the outermost elements that draw nothing but dimensioning.

    Descends through containers that hold a mix of artwork and dimensioning,
    such as the drawing's layer group.
    """
    for child in elem:
        if not isinstance(child.tag, str):
            continue
        tag = child.tag.split("}")[-1]
        if tag in ("defs", "metadata", "namedview"):
            continue
        if _is_dimension_subtree(child):
            yield child
        elif tag in ("g", "svg"):
            yield from _iter_dimension_roots(child)


def _is_dimension_subtree(elem) -> bool:
    """True when everything painted below `elem` is dimensioning, nothing else."""
    blob = ET.tostring(elem, encoding="unicode")
    sentinels = [s.decode() for s in DIMENSION_SENTINELS]
    if not any(s in blob for s in sentinels):
        return False
    for sentinel in sentinels:
        blob = blob.replace(sentinel, "")
    return not ("fill:#" in blob or "stroke:#" in blob or "url(#linearGradient" in blob)


_hex_color_re = re.compile(rb"#[0-9a-fA-F]{6}")


def _darken_artwork(data: bytes, factor: float) -> bytes:
    """Scale every color of the tool artwork toward black, leaving dimensions alone."""

    def scale(match):
        color = match.group(0)
        if color.lower() in DIMENSION_SENTINELS:
            return color
        channels = (int(color[i : i + 2], 16) for i in (1, 3, 5))
        return b"#%02x%02x%02x" % tuple(int(c * factor) for c in channels)

    return _hex_color_re.sub(scale, data)


@lru_cache(maxsize=32)
def _themed_svg(data: bytes, dark: bool, dimensions: bool) -> bytes:
    if dark:
        data = _darken_artwork(data, ARTWORK_DARK_FACTOR)
    # "none" is a valid paint for both fill and stroke, so substituting it for the
    # sentinels hides the dimensioning - arrowhead markers included - and leaves
    # the tool artwork untouched.
    color = (DIMENSION_COLOR_DARK if dark else DIMENSION_COLOR_LIGHT) if dimensions else b"none"
    for sentinel in DIMENSION_SENTINELS:
        data = data.replace(sentinel, color).replace(sentinel.upper(), color)
    return data


def theme_svg(data: bytes, dimensions: bool = True) -> bytes:
    """Recolor a shape SVG - dimensions and artwork - to suit the active theme.

    Args:
        data: the raw SVG bytes.
        dimensions: when False the dimension lines, arrows and labels are dropped,
            leaving just the tool artwork. Used for the small icons, where the
            dimensioning is illegible anyway.
    """
    return _themed_svg(data, is_dark_theme(), dimensions)


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
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: AssetSerializer,
    ) -> "ToolBitShapeIcon":
        """
        Create a ToolBitShapeIcon instance from raw bytes.

        Args:
            data (bytes): The raw bytes of the icon file.
            id (str): The ID of the asset, including extension.
            dependencies (Optional[Mapping[AssetUri, Asset]]): A mapping of resolved dependencies (not used for icons).

        Returns:
            ToolBitShapeIcon: An instance of ToolBitShapeIcon.
        """
        assert serializer == DummyAssetSerializer, "ToolBitShapeIcon supports only native import"
        return cls(id=id, data=data)

    def to_bytes(self, serializer: AssetSerializer) -> bytes:
        """
        Serializes a ToolBitShapeIcon object to bytes.
        """
        assert serializer == DummyAssetSerializer, "ToolBitShapeIcon supports only native export"
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
        if filepath.suffix.lower() == ".png":
            return ToolBitShapePngIcon(id, data)
        elif filepath.suffix.lower() == ".svg":
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

    def get_png(self, icon_size: Optional[QtCore.QSize] = None, dimensions: bool = True) -> bytes:
        """
        Returns the icon data as PNG bytes.
        """
        raise NotImplementedError

    def get_qpixmap(
        self, icon_size: Optional[QtCore.QSize] = None, dimensions: bool = True
    ) -> QtGui.QPixmap:
        """
        Returns the icon data as a QPixmap.
        """
        raise NotImplementedError


class ToolBitShapeSvgIcon(ToolBitShapeIcon):
    asset_type: str = "toolbitshapesvg"

    def get_png(self, icon_size: Optional[QtCore.QSize] = None, dimensions: bool = True) -> bytes:
        """
        Converts SVG icon data to PNG and returns it using QtSvg.
        """
        if icon_size is None:
            icon_size = QtCore.QSize(48, 48)
        image = QtGui.QImage(icon_size, QtGui.QImage.Format_ARGB32)
        image.fill(QtGui.Qt.transparent)
        painter = QtGui.QPainter(image)

        buffer = QtCore.QBuffer(QtCore.QByteArray(theme_svg(self.data, dimensions)))
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

    @cached_property
    def dimension_elements(self) -> Mapping[str, list]:
        """
        Cached map of property name to the ids of every element that draws its
        dimension: the label, the dimension line with its arrowheads, and the
        extension lines running out to it.

        The drawings only name their labels, so the rest has to be matched
        geometrically. A dimension line takes the label it lies closest to, and
        an extension line takes the dimension line it runs to rather than the
        nearest label, which would misfile the ones that run past a neighbour.
        Distances are edge to edge, so a long line is judged by its nearest end
        instead of its middle.
        """
        groups = {prop: [element_id] for prop, element_id in self.label_elements.items()}
        if not groups or not self.data:
            return groups

        try:
            tree = ET.fromstring(self.data)
        except ET.ParseError:
            return groups
        renderer = QtSvg.QSvgRenderer(QtCore.QByteArray(self.data))
        if not renderer.isValid():
            return groups

        def bounds(element_id):
            box = renderer.boundsOnElement(element_id)
            if box.isEmpty() or not hasattr(renderer, "transformForElement"):
                return box
            return renderer.transformForElement(element_id).mapRect(box)

        def gap(a, b):
            """Edge to edge distance between two rectangles, 0 if they overlap."""
            dx = max(a.left() - b.right(), b.left() - a.right(), 0)
            dy = max(a.top() - b.bottom(), b.top() - a.bottom(), 0)
            return dx * dx + dy * dy

        labels, lines, extensions = {}, [], []
        for elem in _iter_dimension_roots(tree):
            element_id = elem.get("id")
            if not element_id:
                continue
            box = bounds(element_id)
            if box.isEmpty():
                continue
            tag = elem.tag.split("}")[-1]
            if tag == "text":
                labels[element_id] = box
            elif tag == "g" or "marker-" in (elem.get("style") or ""):
                lines.append((element_id, box, elem))
            else:
                extensions.append((element_id, box))
        if not labels:
            return groups

        by_element = {element_id: prop for prop, element_id in self.label_elements.items()}
        owners = {}  # dimension line id -> (property name, its bounds)
        for element_id, box, elem in lines:
            nearest = min(labels, key=lambda lid: gap(box, labels[lid]))
            prop = by_element.get(nearest)
            if prop is None:
                continue
            groups[prop].append(element_id)
            # arrowheads drawn by a <marker> live in <defs>, follow them too
            for marker in _marker_re.findall(ET.tostring(elem)):
                groups[prop].append(marker.decode())
            owners[element_id] = (prop, box)

        for element_id, box in extensions:
            if not owners:
                break
            nearest = min(owners, key=lambda lid: gap(box, owners[lid][1]))
            groups[owners[nearest][0]].append(element_id)

        # An arrowhead <marker> is a shared definition: some drawings point two
        # dimensions at the same one. Recoloring it would light up the other
        # dimension too, so leave those out and highlight only the line itself.
        shared = {
            element_id
            for element_id in {i for ids in groups.values() for i in ids}
            if sum(element_id in ids for ids in groups.values()) > 1
        }
        if shared:
            groups = {prop: [i for i in ids if i not in shared] for prop, ids in groups.items()}
        return groups

    def get_themed_data(self, dimensions: bool = True, highlight: Optional[str] = None) -> bytes:
        """
        The SVG bytes as they will be rendered: recolored for the active theme,
        optionally with the dimensions dropped, and optionally with the dimension
        of one property picked out in the highlight color.

        Callers that need to hit-test the drawing should build their QSvgRenderer
        from this, so element bounds match what is on screen.
        """
        data = theme_svg(self.data, dimensions)
        if highlight and dimensions:
            data = highlight_elements(data, self.dimension_elements.get(highlight, []))
        return data

    def get_qpixmap(
        self,
        icon_size: Optional[QtCore.QSize] = None,
        dimensions: bool = True,
        highlight: Optional[str] = None,
    ) -> QtGui.QPixmap:
        """
        Returns the SVG icon data as a QPixmap using QtSvg.
        """
        if icon_size is None:
            icon_size = QtCore.QSize(48, 48)
        icon_ba = QtCore.QByteArray(self.get_themed_data(dimensions, highlight))
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
        return self.abbreviations.get(param_name)

    @cached_property
    def label_elements(self) -> Mapping[str, str]:
        """
        Cached map of property name to the id of the <text> element labelling it
        in the drawing, e.g. {"ShankDiameter": "shank_diameter"}.
        """
        if self.data:
            return self.get_label_elements_from_svg(self.data)
        return {}

    @staticmethod
    def get_label_elements_from_svg(svg: bytes) -> Mapping[str, str]:
        """
        Map each dimensioned property to the id of its <text> label, using the
        same name normalization as the abbreviations.
        """
        try:
            tree = ET.fromstring(svg)
        except ET.ParseError:
            return {}

        def _upper(match):
            return match.group(1).upper()

        result = {}
        for text_elem in tree.findall(".//s:text", _svg_ns):
            element_id = text_elem.attrib.get("id")
            if not isinstance(element_id, str):
                continue
            result[re.sub(r"_(\w)", _upper, element_id.capitalize())] = element_id
        return result

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

            # Backward compatibility: Normalize to match FreeCAD property
            # name structure:
            # Old: property_name New: PropertyName
            def _upper(match):
                return match.group(1).upper()

            id = re.sub(r"_(\w)", _upper, id.capitalize())

            abbr = text_elem.text
            if abbr is not None:
                result[id] = abbr

            span_elem = text_elem.find(".//s:tspan", _svg_ns)
            if span_elem is None:
                continue
            abbr = span_elem.text
            result[id] = abbr

        return result


class ToolBitShapePngIcon(ToolBitShapeIcon):
    asset_type: str = "toolbitshapepng"

    def get_png(self, icon_size: Optional[QtCore.QSize] = None, dimensions: bool = True) -> bytes:
        """
        Returns the PNG icon data. `dimensions` is accepted for signature parity
        with the SVG icon; a rasterized thumbnail has nothing to strip.
        """
        # For PNG, resizing might be needed if icon_size is different
        # from the original size. Simple return for now.
        return self.data

    def get_qpixmap(
        self, icon_size: Optional[QtCore.QSize] = None, dimensions: bool = True
    ) -> QtGui.QPixmap:
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
