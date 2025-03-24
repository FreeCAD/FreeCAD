# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
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

"""Unit tests for the Draft Workbench, GUI import tests."""

## @package test_import_gui
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, GUI import tests.

## \addtogroup drafttests
# @{

from drafttests import auxiliary as aux
from drafttests import test_base


class DraftGuiImport(test_base.DraftTestCaseNoDoc):
    """Import the Draft graphical modules."""

    def test_import_gui_draftgui(self):
        """Import Draft TaskView GUI tools."""
        module = "DraftGui"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_snap(self):
        """Import Draft snapping."""
        module = "draftguitools.gui_snapper"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_tools(self):
        """Import Draft graphical commands."""
        module = "DraftTools"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_trackers(self):
        """Import Draft tracker utilities."""
        module = "draftguitools.gui_trackers"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

## @}
