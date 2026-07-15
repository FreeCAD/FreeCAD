# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._MDIView`` PyCXX type."""

from __future__ import annotations

from typing import Literal, overload

from FreeCAD import DocumentObject

class _MDIView:
    """Base MDI view wrapper for FreeCAD GUI view types."""

    def printView(self) -> None:
        """Print the current view."""
        ...

    def printPdf(self) -> None:
        """Export the current view to PDF."""
        ...

    def printPreview(self) -> None:
        """Open a print preview for the current view."""
        ...

    def undoActions(self) -> list[str]:
        """Return the undo actions exposed by the view."""
        ...

    def redoActions(self) -> list[str]:
        """Return the redo actions exposed by the view."""
        ...

    def message(self, message: str, /) -> bool:
        """Handle one generic message in the view."""
        ...

    def sendMessage(self, message: str, /) -> bool:
        """Send one generic message to the view."""
        ...

    def supportMessage(self, message: str, /) -> bool:
        """Return whether the view supports one generic message."""
        ...

    def fitAll(self) -> None:
        """Fit the full visible content in the view."""
        ...

    def setActiveObject(
        self,
        name: str,
        document_object: DocumentObject | None = None,
        subname: str | None = None,
        /,
    ) -> None:
        """Set one active object slot for the view."""
        ...

    @overload
    def getActiveObject(self, name: str, resolve: Literal[True] = True, /) -> DocumentObject | None:
        """Return the resolved active object for one slot name."""
        ...

    @overload
    def getActiveObject(
        self, name: str, resolve: Literal[False], /
    ) -> tuple[DocumentObject | None, DocumentObject | None, str]: ...
    def cast_to_base(self) -> _MDIView:
        """Return this view as the base MDI view wrapper."""
        ...
