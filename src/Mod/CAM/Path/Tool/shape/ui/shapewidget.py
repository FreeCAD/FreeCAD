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
from typing import Optional
from PySide import QtGui, QtCore, QtSvg
from ..models.base import ToolBitShape
from ..models.icon import ToolBitShapeSvgIcon

# Dimension labels are small; give the mouse a little slack around them.
_HIT_MARGIN = 4


def _png2qpixmap(data, icon_size):
    pixmap = QtGui.QPixmap()
    pixmap.loadFromData(data, "PNG")
    # Scale the pixmap if the requested size is different
    if pixmap.size() != icon_size:
        pixmap = pixmap.scaled(
            icon_size,
            QtCore.Qt.KeepAspectRatio,
            QtCore.Qt.SmoothTransformation,
        )
    return pixmap


class ShapeWidget(QtGui.QWidget):
    """
    Displays the drawing of a tool bit shape.

    When `interactive` is set, the dimension labels become hover targets:
    pointing at one emits `dimensionHovered` with the property it dimensions and
    picks it out in the drawing, and clicking emits `dimensionClicked`. The
    reverse direction is driven by `set_highlight()`.
    """

    dimensionHovered = QtCore.Signal(str)  # property name, "" when none
    dimensionClicked = QtCore.Signal(str)

    def __init__(
        self,
        shape: ToolBitShape,
        icon_size: Optional[QtCore.QSize] = None,
        parent=None,
        dimensions: bool = True,
        interactive: bool = False,
    ):
        super(ShapeWidget, self).__init__(parent)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.setAlignment(QtCore.Qt.AlignHCenter)

        self.shape = shape
        self.icon_size = icon_size or QtCore.QSize(263, 372)  # A4 aspect ratio
        self.dimensions = dimensions
        self.icon_widget = QtGui.QLabel()
        self.icon_widget.setAlignment(QtCore.Qt.AlignCenter)
        self.layout.addWidget(self.icon_widget)

        self._interactive = interactive and dimensions
        self._hovered = ""  # dimension under the mouse
        self._pinned = ""  # dimension highlighted from the outside
        self._rects = None  # cached hit boxes, see _dimension_rects()
        self._rects_key = None
        if self._interactive:
            self.setMouseTracking(True)
            # The label covers this widget, so it - not us - is what the mouse
            # is over. Watch it directly instead of relying on propagation.
            self.icon_widget.setMouseTracking(True)
            self.icon_widget.installEventFilter(self)

        self._update_icon()

    # ------------------------------------------------------------------ paint

    def _svg_icon(self) -> Optional[ToolBitShapeSvgIcon]:
        """The shape's icon, when it is an SVG we can pick elements out of."""
        icon = self.shape.get_icon()
        return icon if isinstance(icon, ToolBitShapeSvgIcon) else None

    def _highlighted(self) -> str:
        """Hovering wins over an externally pinned highlight."""
        return self._hovered or self._pinned

    def set_highlight(self, prop_name: str):
        """Pick out the dimension of `prop_name`; pass "" to clear."""
        prop_name = prop_name or ""
        if prop_name == self._pinned:
            return
        was = self._highlighted()
        self._pinned = prop_name
        if self._highlighted() != was:
            self._update_icon()

    def _update_icon(self):
        ratio = self.devicePixelRatioF()
        size = self.icon_size * ratio
        icon = self.shape.get_icon()
        if icon:
            if isinstance(icon, ToolBitShapeSvgIcon):
                pixmap = icon.get_qpixmap(size, self.dimensions, self._highlighted() or None)
            else:
                pixmap = icon.get_qpixmap(size, self.dimensions)
            pixmap.setDevicePixelRatio(ratio)
            self.icon_widget.setPixmap(pixmap)
            return

        thumbnail = self.shape.get_thumbnail()
        if thumbnail:
            pixmap = _png2qpixmap(thumbnail, size)
            pixmap.setDevicePixelRatio(ratio)
            self.icon_widget.setPixmap(pixmap)
            return

        self.icon_widget.clear()  # Clear pixmap if no icon

    # ------------------------------------------------------------- hit testing

    def _content_rect(self, renderer) -> Optional[QtCore.QRectF]:
        """
        Where the drawing actually lands, in this widget's coordinates.

        The pixmap is centered in the label, and the SVG is rendered into it
        with KeepAspectRatio, so it may be letterboxed inside the pixmap when
        the requested size does not match the page aspect.
        """
        view_box = renderer.viewBoxF()
        if view_box.isEmpty():
            return None

        label = self.icon_widget.geometry()
        pixmap = QtCore.QRectF(0, 0, self.icon_size.width(), self.icon_size.height())
        pixmap.moveCenter(QtCore.QRectF(label).center())

        scale = min(pixmap.width() / view_box.width(), pixmap.height() / view_box.height())
        content = QtCore.QRectF(0, 0, view_box.width() * scale, view_box.height() * scale)
        content.moveCenter(pixmap.center())
        return content

    def _renderer(self) -> Optional[QtSvg.QSvgRenderer]:
        icon = self._svg_icon()
        if icon is None:
            return None
        data = QtCore.QByteArray(icon.get_themed_data(self.dimensions))
        renderer = QtSvg.QSvgRenderer(data)
        return renderer if renderer.isValid() else None

    def _dimension_rects(self):
        """
        Widget-space rectangle of every dimension label, by property name.

        Cached against the icon geometry, since this is consulted on every
        mouse move and parsing the drawing each time would be wasteful.
        """
        geometry = self.icon_widget.geometry()
        key = (geometry.x(), geometry.y(), geometry.width(), geometry.height(), id(self.shape))
        if self._rects_key == key and self._rects is not None:
            return self._rects

        self._rects_key = key
        self._rects = {}

        icon = self._svg_icon()
        renderer = self._renderer()
        if icon is None or renderer is None:
            return self._rects
        content = self._content_rect(renderer)
        if content is None:
            return self._rects

        view_box = renderer.viewBoxF()
        sx = content.width() / view_box.width()
        sy = content.height() / view_box.height()

        rects = {}
        for prop_name, element_ids in icon.dimension_elements.items():
            # One rect per element rather than a union: a union of a label and a
            # long dimension line spans a box that would swallow its neighbours.
            for element_id in element_ids:
                # An id the renderer does not know reports the bounds of the
                # whole drawing, which as a hit box would cover everything.
                if hasattr(renderer, "elementExists") and not renderer.elementExists(element_id):
                    continue
                bounds = renderer.boundsOnElement(element_id)
                if bounds.isEmpty():
                    continue
                # Element bounds are local; map them through the element's
                # transform so drawings whose layer carries a translate() line up.
                if hasattr(renderer, "transformForElement"):
                    bounds = renderer.transformForElement(element_id).mapRect(bounds)
                rects.setdefault(prop_name, []).append(
                    QtCore.QRectF(
                        content.x() + (bounds.x() - view_box.x()) * sx,
                        content.y() + (bounds.y() - view_box.y()) * sy,
                        bounds.width() * sx,
                        bounds.height() * sy,
                    ).adjusted(-_HIT_MARGIN, -_HIT_MARGIN, _HIT_MARGIN, _HIT_MARGIN)
                )
        self._rects = rects
        return rects

    def dimension_at(self, pos) -> str:
        """Property name of the dimension under `pos`, or "" for none."""
        point = QtCore.QPointF(pos)
        best, best_area = "", None
        for prop_name, rects in self._dimension_rects().items():
            for rect in rects:
                # Overlapping elements: the smallest wins, it is the tighter target.
                if rect.contains(point):
                    area = rect.width() * rect.height()
                    if best_area is None or area < best_area:
                        best, best_area = prop_name, area
        return best

    # ----------------------------------------------------------------- events

    def _set_hovered(self, prop_name: str):
        if prop_name == self._hovered:
            return
        was = self._highlighted()
        self._hovered = prop_name
        self.setCursor(QtCore.Qt.PointingHandCursor if prop_name else QtCore.Qt.ArrowCursor)
        if self._highlighted() != was:
            self._update_icon()
        self.dimensionHovered.emit(prop_name)

    def eventFilter(self, obj, event):
        """Handle the icon label's mouse events as if they were our own."""
        if self._interactive and obj is self.icon_widget:
            kind = event.type()
            if kind == QtCore.QEvent.MouseMove:
                self._set_hovered(self.dimension_at(self._to_widget(event)))
            elif kind == QtCore.QEvent.Leave:
                self._set_hovered("")
            elif kind in (QtCore.QEvent.MouseButtonPress, QtCore.QEvent.MouseButtonDblClick):
                prop_name = self.dimension_at(self._to_widget(event))
                if prop_name:
                    self.dimensionClicked.emit(prop_name)
                    # Keep the click here. Letting it travel on hands the focus
                    # we just gave the field to whichever ancestor takes clicks.
                    return True
        return super(ShapeWidget, self).eventFilter(obj, event)

    def _to_widget(self, event):
        """Map a mouse event on the icon label into this widget's coordinates."""
        pos = event.position() if hasattr(event, "position") else event.pos()
        return self.icon_widget.mapToParent(pos.toPoint() if hasattr(pos, "toPoint") else pos)

    def mouseMoveEvent(self, event):
        if self._interactive:
            self._set_hovered(self.dimension_at(event.pos()))
        super(ShapeWidget, self).mouseMoveEvent(event)

    def leaveEvent(self, event):
        if self._interactive:
            self._set_hovered("")
        super(ShapeWidget, self).leaveEvent(event)

    def resizeEvent(self, event):
        self._rects_key = None  # geometry moved, hit boxes are stale
        super(ShapeWidget, self).resizeEvent(event)

    def mousePressEvent(self, event):
        if self._interactive:
            prop_name = self.dimension_at(event.pos())
            if prop_name:
                self.dimensionClicked.emit(prop_name)
                event.accept()
                return
        super(ShapeWidget, self).mousePressEvent(event)
