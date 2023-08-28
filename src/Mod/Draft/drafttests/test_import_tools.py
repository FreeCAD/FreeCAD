# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Unit tests for the Draft Workbench, tools import tests."""
## @package test_import_tools
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, tools import tests.

## \addtogroup drafttests
# @{
import unittest

import drafttests.auxiliary as aux


class DraftImportTools(unittest.TestCase):
    """Test for each individual module that defines a tool."""

    def setUp(self):
        """Draw the header.

        This is executed before every test.
        No document is needed to test the import of modules so no document
        is created, and `tearDown` isn't defined.
        """
        aux.draw_header()

    def test_import_gui_draftedit(self):
        """Import Draft Edit."""
        module = "draftguitools.gui_edit"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftplane(self):
        """Import Draft SelectPlane."""
        module = "draftguitools.gui_selectplane"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_workingplane(self):
        """Import Draft WorkingPlane."""
        module = "WorkingPlane"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

## @}
