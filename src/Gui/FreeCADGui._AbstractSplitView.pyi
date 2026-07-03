# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._AbstractSplitView`` PyCXX type."""

from __future__ import annotations

from FreeCADGui import _MDIView, _View3DInventorViewer

class _AbstractSplitView:
    """Split-view container that manages multiple 3D viewers."""

    def fitAll(self, factor: float = 1.0, /) -> None:
        """Fit all visible content into the active split view."""
        ...

    def viewBottom(self) -> None:
        """Orient the active split view to the bottom direction."""
        ...

    def viewFront(self) -> None:
        """Orient the active split view to the front direction."""
        ...

    def viewLeft(self) -> None:
        """Orient the active split view to the left direction."""
        ...

    def viewRear(self) -> None:
        """Orient the active split view to the rear direction."""
        ...

    def viewRight(self) -> None:
        """Orient the active split view to the right direction."""
        ...

    def viewTop(self) -> None:
        """Orient the active split view to the top direction."""
        ...

    def viewAxometric(self) -> None:
        """Orient the active split view axometrically."""
        ...

    def viewIsometric(self) -> None:
        """Orient the active split view isometrically."""
        ...

    def getViewer(self, index: int, /) -> _View3DInventorViewer:
        """Return one contained viewer by index."""
        ...

    def close(self) -> None:
        """Close the split-view container."""
        ...

    def cast_to_base(self) -> _MDIView:
        """Return the base MDI view wrapper for this split view."""
        ...
