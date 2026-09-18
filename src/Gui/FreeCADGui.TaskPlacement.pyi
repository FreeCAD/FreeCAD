# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui.TaskPlacement`` PyCXX type."""

from __future__ import annotations

from collections.abc import Sequence

from FreeCAD import DocumentObject, Placement

class TaskPlacement:
    """Task-panel dialog used to edit object placement interactively."""

    def setPropertyName(self, name: str, /) -> None:
        """Set the placement property name that the dialog edits."""
        ...

    def setPlacement(self, placement: Placement, /) -> None:
        """Set the placement currently shown by the dialog."""
        ...

    def setSelection(self, selection: Sequence[DocumentObject], /) -> None:
        """Set the document-object selection used by the dialog."""
        ...

    def bindObject(self) -> None:
        """Bind the current placement to the selected object."""
        ...

    def setPlacementAndBindObject(
        self, document_object: DocumentObject, property_name: str, /
    ) -> None:
        """Bind the dialog directly to one object and placement property."""
        ...

    def setIgnoreTransactions(self, ignore: bool, /) -> None:
        """Enable or disable transaction handling for placement edits."""
        ...

    def showDefaultButtons(self, show: bool, /) -> None:
        """Show or hide the default task-panel buttons."""
        ...

    def accept(self) -> bool:
        """Accept the current placement edits."""
        ...

    def reject(self) -> bool:
        """Reject the current placement edits."""
        ...

    def clicked(self, button: int, /) -> None:
        """Handle one clicked task-panel button."""
        ...

    def open(self) -> None:
        """Open or initialize the task dialog."""
        ...

    def isAllowedAlterDocument(self) -> bool:
        """Return whether the dialog may alter the active document."""
        ...

    def isAllowedAlterView(self) -> bool:
        """Return whether the dialog may alter the active view."""
        ...

    def isAllowedAlterSelection(self) -> bool:
        """Return whether the dialog may alter the active selection."""
        ...

    def getStandardButtons(self) -> int:
        """Return the standard button mask exposed by the dialog."""
        ...
