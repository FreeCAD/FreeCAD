# SPDX-License-Identifier: LGPL-2.1-or-later

"""unittest base class for FreeCAD GUI tests."""

from __future__ import annotations

import unittest

from .Harness import GuiHarness
from .Wait import gui_available


class FreeCADGuiTestCase(unittest.TestCase):
    """Base class that owns one :class:`GuiHarness` per test."""

    gui: GuiHarness

    def setUp(self) -> None:
        """Skip without a GUI and initialize the per-test harness."""
        super().setUp()
        if not gui_available():
            self.skipTest("GUI not available")
        self.gui = GuiHarness()
        self.gui.set_up()

    def tearDown(self) -> None:
        """Restore GUI state before allowing ``unittest`` teardown."""
        try:
            if hasattr(self, "gui"):
                self.gui.tear_down()
        finally:
            super().tearDown()


# Compatibility name for code that already uses the generic concept.
GuiTestCase = FreeCADGuiTestCase
