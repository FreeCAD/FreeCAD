# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui.UiLoader`` PyCXX type."""

# source: `Gui/UiLoader.cpp`

from PySide import QtCore, QtWidgets
from os import PathLike

class UiLoader:
    """Qt Designer UI loader used by the FreeCAD GUI wrappers."""

    def load(
        self,
        source: str | PathLike[str] | QtCore.QIODevice,
        parent: QtWidgets.QWidget | None = None,
        /,
    ) -> QtWidgets.QWidget | None:
        """Load one `.ui` source and return the created widget tree."""

    def createWidget(
        self, class_name: str, parent: QtWidgets.QWidget | None = None, name: str = "", /,
    ) -> QtWidgets.QWidget | None:
        """Create one widget by Qt class name."""

    def availableWidgets(self) -> list[str]:
        """Return the widget classes the loader can create."""

    def clearPluginPaths(self) -> None:
        """Remove all custom plugin search paths."""

    def pluginPaths(self) -> list[str]:
        """Return the current custom plugin search paths."""

    def addPluginPath(self, path: str | PathLike[str], /) -> None:
        """Add one custom plugin search path."""

    def errorString(self) -> str:
        """Return the most recent loader error string."""

    def isLanguageChangeEnabled(self) -> bool:
        """Return whether live language-change handling is enabled."""

    def setLanguageChangeEnabled(self, enabled: bool, /) -> None:
        """Enable or disable live language-change handling."""

    def setWorkingDirectory(self, path: str | PathLike[str], /) -> None:
        """Set the working directory used for relative UI resources."""

    def workingDirectory(self) -> str:
        """Return the current working directory used for UI resources."""
