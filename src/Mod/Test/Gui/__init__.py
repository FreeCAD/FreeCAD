# SPDX-License-Identifier: LGPL-2.1-or-later

"""Small public entry point for shared Python GUI test support."""

from .Harness import GuiHarness
from .TestCase import FreeCADGuiTestCase
from .Wait import gui_available

__all__ = [
    "FreeCADGuiTestCase",
    "GuiHarness",
    "gui_available",
]
