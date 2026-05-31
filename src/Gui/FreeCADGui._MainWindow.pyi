# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._MainWindow`` PyCXX type."""

from __future__ import annotations

from FreeCADGui import InputHint, _MDIView

class _MainWindow:
    """Wrapper around the FreeCAD main application window."""

    def getWindows(self) -> list[_MDIView]:
        """Return the currently open MDI views."""
        ...

    def getWindowsOfType(self, type_id: object, /) -> list[_MDIView]:
        """Return the open MDI views of one runtime type."""
        ...

    def setActiveWindow(self, view: _MDIView, /) -> None:
        """Make one MDI view the active window."""
        ...

    def getActiveWindow(self) -> _MDIView | None:
        """Return the active MDI view, if any."""
        ...

    def addWindow(self, window: object, /) -> _MDIView | None:
        """Add one window object to the MDI area."""
        ...

    def removeWindow(self, view: _MDIView, /) -> None:
        """Remove one MDI view from the main window."""
        ...

    def showHint(self, *hints: InputHint) -> None:
        """Show one or more input hints in the main window."""
        ...

    def hideHint(self) -> None:
        """Hide the currently visible input hints."""
        ...
