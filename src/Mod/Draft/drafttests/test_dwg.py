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
"""Unit tests for the Draft Workbench, DWG import and export tests."""

## @package test_dwg
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, DWG import and export tests.

## \addtogroup drafttests
# @{

import os

import FreeCAD as App
import Draft
from drafttests import auxiliary as aux
from drafttests import test_base
from draftutils.messages import _msg


class DraftDWG(test_base.DraftTestCaseDoc):
    """Test reading and writing of DWG files with Draft."""

    def test_read_dwg(self):
        """Read a DWG file and import its elements as Draft objects."""
        operation = "importDWG.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires a DWG file to read.")

        file = "Mod/Draft/drafttest/test.dwg"
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        obj = aux.fake_function(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_dwg(self):
        """Create some figures and export them to a DWG file."""
        operation = "importDWG.export"
        _msg("  Test '{}'".format(operation))

        file = "Mod/Draft/drafttest/out_test.dwg"
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        obj = aux.fake_function(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

## @}
