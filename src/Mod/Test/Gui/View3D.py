# SPDX-License-Identifier: LGPL-2.1-or-later

"""HiDPI-independent viewport coordinate conversion."""

from __future__ import annotations

from typing import Any, TypeAlias

import FreeCAD

from . import Wait


ViewportPoint: TypeAlias = tuple[float, float]
"""A point in FreeCAD's physical viewport coordinate system."""


class View3D:
    """Convert between FreeCAD viewport pixels and Qt logical pixels."""

    def __init__(self, view: Any) -> None:
        self.view: Any = view

    @property
    def viewport(self) -> Any:
        """Return the Qt viewport receiving mouse events."""
        return self.view.graphicsView().viewport()

    @staticmethod
    def device_pixel_ratio(widget: Any) -> float:
        """Return ``widget``'s device-pixel ratio for Qt 5 or Qt 6."""
        method = getattr(widget, "devicePixelRatioF", None)
        if method is not None:
            return method()
        return widget.devicePixelRatio()

    def world_to_screen(self, point: FreeCAD.Vector) -> Any:
        """Convert a FreeCAD world point to a logical Qt viewport point."""
        return self.viewport_to_screen(self.view.getPointOnScreen(point))

    def viewport_to_screen(
        self,
        point: ViewportPoint,
        viewport: Any | None = None,
    ) -> Any:
        """Convert a physical viewport point to a logical Qt point."""
        _, height = self.view.getSize()
        if viewport is None:
            viewport = self.viewport
        scale = self.device_pixel_ratio(viewport)
        x = int(round(point[0] / scale))
        y = int(round((height - point[1] - 1) / scale))
        return Wait.QtCore.QPoint(x, y)
