"""Unit test for the Draft module, tools import tests.
"""
# ***************************************************************************
# *   (c) 2013 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import unittest
import FreeCAD as App
from .auxiliary import _draw_header
from .auxiliary import _no_gui
from .auxiliary import _import_test


class DraftImportTools(unittest.TestCase):
    """Test for each individual module that defines a tool."""
    # No document is needed to test 'import' of other modules
    # thus 'setUp' just draws a line, and 'tearDown' isn't defined.
    def setUp(self):
        _draw_header()

    def test_import_gui_draftedit(self):
        """Import Draft Edit."""
        module = "DraftEdit"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftfillet(self):
        """Import Draft Fillet."""
        module = "DraftFillet"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftlayer(self):
        """Import Draft Layer."""
        module = "DraftLayer"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftplane(self):
        """Import Draft SelectPlane."""
        module = "DraftSelectPlane"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_workingplane(self):
        """Import Draft WorkingPlane."""
        module = "WorkingPlane"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))
