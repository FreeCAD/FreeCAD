# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._TaskDialog`` PyCXX type."""

from __future__ import annotations

class _TaskDialog:
    """Base wrapper for task-panel dialogs."""

    def getDialogContent(self) -> list[object]:
        """Return the content widgets hosted by the dialog."""
        ...

    def getStandardButtons(self) -> int:
        """Return the standard button mask for the dialog."""
        ...

    def setEscapeButtonEnabled(self, enabled: bool, /) -> None:
        """Enable or disable the escape button behavior."""
        ...

    def isEscapeButtonEnabled(self) -> bool:
        """Return whether the escape button is enabled."""
        ...

    def setAutoCloseOnTransactionChange(self, enabled: bool, /) -> None:
        """Set whether transaction changes auto-close the dialog."""
        ...

    def isAutoCloseOnTransactionChange(self) -> bool:
        """Return whether transaction changes auto-close the dialog."""
        ...

    def setAutoCloseOnDeletedDocument(self, enabled: bool, /) -> None:
        """Set whether document deletion auto-closes the dialog."""
        ...

    def isAutoCloseOnDeletedDocument(self) -> bool:
        """Return whether document deletion auto-closes the dialog."""
        ...

    def getDocumentName(self) -> str:
        """Return the document name associated with the dialog."""
        ...

    def setDocumentName(self, name: str, /) -> None:
        """Associate the dialog with one document name."""
        ...

    def isAllowedAlterDocument(self) -> bool:
        """Return whether the dialog may alter the document."""
        ...

    def isAllowedAlterView(self) -> bool:
        """Return whether the dialog may alter the view."""
        ...

    def isAllowedAlterSelection(self) -> bool:
        """Return whether the dialog may alter the selection."""
        ...

    def needsFullSpace(self) -> bool:
        """Return whether the dialog prefers the full task-panel space."""
        ...

    def accept(self) -> None:
        """Accept the dialog."""
        ...

    def reject(self) -> None:
        """Reject the dialog."""
        ...
