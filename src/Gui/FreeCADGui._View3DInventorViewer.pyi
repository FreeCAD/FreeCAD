# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._View3DInventorViewer`` PyCXX type."""

from __future__ import annotations

from typing import Any, overload

from FreeCAD import Matrix, Vector

class _View3DInventorViewer:
    """Low-level Coin viewer wrapper behind the 3D view."""

    def getSoRenderManager(self) -> Any:
        """Return the underlying Coin render manager."""
        ...

    def getSoEventManager(self) -> Any:
        """Return the underlying Coin event manager."""
        ...

    def getSceneGraph(self) -> Any:
        """Return the current root scene graph."""
        ...

    def setSceneGraph(self, node: object, /) -> None:
        """Replace the root scene graph."""
        ...

    def seekToPoint(self, point: tuple[int, int] | tuple[float, float, float], /) -> None:
        """Center the viewer on one screen or world point."""
        ...

    def setFocalDistance(self, distance: float, /) -> None:
        """Set the camera focal distance."""
        ...

    def getFocalDistance(self) -> float:
        """Return the camera focal distance."""
        ...

    @overload
    def getPoint(self, x: int, y: int, /) -> Vector:
        """Project one screen-space point into world coordinates."""
        ...

    @overload
    def getPoint(self, point: tuple[int, int], /) -> Vector: ...
    @overload
    def getPointOnFocalPlane(self, x: int, y: int, /) -> Vector:
        """Project one screen-space point onto the focal plane."""
        ...

    @overload
    def getPointOnFocalPlane(self, point: tuple[int, int], /) -> Vector: ...
    def getPickRadius(self) -> float:
        """Return the current pick radius."""
        ...

    def setPickRadius(self, radius: float, /) -> None:
        """Set the pick radius used for hit-testing."""
        ...

    def setupEditingRoot(self, node: object | None = None, matrix: Matrix | None = None, /) -> None:
        """Install one temporary editing root node."""
        ...

    def resetEditingRoot(self, update_links: bool = True, /) -> None:
        """Remove the temporary editing root node."""
        ...

    def setBackgroundColor(self, red: float, green: float, blue: float, /) -> None:
        """Set a solid background color."""
        ...

    def setGradientBackground(self, background: str, /) -> None:
        """Set the predefined gradient background mode."""
        ...

    def setGradientBackgroundColor(
        self,
        from_color: tuple[float, float, float],
        to_color: tuple[float, float, float],
        mid_color: tuple[float, float, float] | None = None,
        /,
    ) -> None:
        """Set explicit gradient background colors."""
        ...

    def setRedirectToSceneGraph(self, redirect: bool, /) -> None:
        """Enable or disable redirection to the scene graph."""
        ...

    def isRedirectedToSceneGraph(self) -> bool:
        """Return whether rendering is redirected to the scene graph."""
        ...

    def grabFramebuffer(self) -> Any:
        """Capture the current framebuffer."""
        ...

    def setOverrideMode(self, mode: str, /) -> None:
        """Set the viewer override rendering mode."""
        ...

    def setEnabledNaviCube(self, enabled: bool, /) -> None:
        """Enable or disable the navigation cube."""
        ...

    def isEnabledNaviCube(self) -> bool:
        """Return whether the navigation cube is enabled."""
        ...

    def setNaviCubeCorner(self, corner: int, /) -> None:
        """Set the navigation-cube corner."""
        ...

    def getNavigationStyle(self) -> object | None:
        """Return the current navigation-style object, if any."""
        ...
