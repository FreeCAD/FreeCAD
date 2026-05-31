# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._Control`` PyCXX type."""

from __future__ import annotations

from FreeCADGui import Document, _TaskDialog

class _Control:
    """Global task-panel control surface for the FreeCAD GUI."""

    def showDialog(self, dialog: object, document: Document | None = None, /) -> _TaskDialog:
        """Show one task dialog, optionally for a specific document."""
        ...

    def activeDialog(self, document: Document | None = None, /) -> bool:
        """Return whether a task dialog is active."""
        ...

    def activeTaskDialog(self, document: Document | None = None, /) -> _TaskDialog | None:
        """Return the active task dialog, if any."""
        ...

    def closeDialog(self, document: Document | None = None, /) -> None:
        """Close the active task dialog."""
        ...

    def addTaskWatcher(self, watchers: object, /) -> None:
        """Register one task watcher object."""
        ...

    def clearTaskWatcher(self) -> None:
        """Clear the registered task watchers."""
        ...

    def isAllowedAlterDocument(self, document: Document | None = None, /) -> bool:
        """Return whether the active task dialog may alter the document."""
        ...

    def isAllowedAlterView(self, document: Document | None = None, /) -> bool:
        """Return whether the active task dialog may alter the view."""
        ...

    def isAllowedAlterSelection(self, document: Document | None = None, /) -> bool:
        """Return whether the active task dialog may alter the selection."""
        ...

    def showTaskView(self) -> None:
        """Show the task view pane."""
        ...

    def showModelView(self) -> None:
        """Show the model view pane."""
        ...
