# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public signatures for the ``FreeCADGui.PySideUic`` helper module.

This source-adjacent stub file carries the PySide UI-loading convenience
helpers that FreeCAD exposes from the GUI layer.
"""

from __future__ import annotations

from os import PathLike
from typing import TypeAlias

_UiPath: TypeAlias = str | PathLike[str]

def loadUiType(ui_file: _UiPath, /) -> tuple[type[object], type[object]] | None:
    """Compile one Qt Designer `.ui` file in memory and return its form and base classes."""
    ...

def loadUi(ui_file: _UiPath, base: object | None = None, /) -> object | None:
    """Load one Qt Designer `.ui` file and return the created widget tree."""
    ...

def createCustomWidget(
    class_name: str,
    parent: object | None = None,
    name: str = "",
    /,
) -> object | None:
    """Create one FreeCAD custom widget by class name."""
    ...
