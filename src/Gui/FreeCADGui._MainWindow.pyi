# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._MainWindow`` PyCXX type."""

# source: `Gui/MainWindowPy.cpp`

from FreeCAD import Base
from FreeCADGui import InputHint, _MDIView
from PySide import QtWidgets
from Document import Document
from typing import Protocol

class _MDIConvertible(Protocol):
    def cast_to_base(self) -> _MDIView: ...

class _DocumentLike(Protocol):
    @property
    def document(self) -> Document: ...

class _MainWindow(QtWidgets.QMainWindow):
    """Wrapper around the FreeCAD main application window."""

    def getWindows(self) -> list[_MDIView]:
        """Return the currently open MDI views."""

    def getWindowsOfType(self, type_id: Base.TypeId, /) -> list[_MDIView]:
        """Return the open MDI views of one runtime type."""

    def setActiveWindow(self, view: _MDIConvertible, /) -> None:
        """Make one MDI view the active window."""

    def getActiveWindow(self) -> _MDIView | None:
        """Return the active MDI view, if any."""

    def addWindow(self, window: _DocumentLike, /) -> _MDIView | None:
        """Add one window object to the MDI area."""

    def removeWindow(self, view: _MDIConvertible, /) -> None:
        """Remove one MDI view from the main window."""

    def showHint(self, /, *hints: InputHint) -> None:
        """Show one or more input hints in the main window."""

    def hideHint(self) -> None:
        """Hide the currently visible input hints."""

    def addStatusBarItem(
        self,
        widget: QtWidgets.QWidget,
        id: str,
        /,
        title: str = "",
        slot: str = "Right",
        order: int = 0,
        persistentVisibility: bool = True,
        stretch: int = 0,
    ) -> None:
        """
        Registers a widget in the status bar. MainWindow owns its
        placement, ordering, visibility persistence and context-menu entry.
        """

    def removeStatusBarItem(self, id: str, /) -> None:
        """Remove status bar item with id."""
