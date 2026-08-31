# SPDX-License-Identifier: LGPL-2.1-or-later

"""Compatibility imports for the former Sketcher-only GUI test helper.

Generic GUI tests should inherit from :class:`Gui.TestCase.FreeCADGuiTestCase`;
Sketcher tests should use :class:`SketcherTests.Support.SketcherGuiTestCase`.
This module remains so out-of-tree Sketcher tests can migrate independently.
The former implicit key-event settling delay is not retained; callers should
wait for the resulting GUI state explicitly.
"""

from Gui.TestCase import FreeCADGuiTestCase
from Gui.Wait import (
    GUI_MODULE_AVAILABLE,
    FreeCAD,
    FreeCADGui,
    QtCore,
    QtGui,
    gui_available,
)
from SketcherTests.Support import SketcherGuiTestCase

QT_MODULE_AVAILABLE = QtCore is not None
