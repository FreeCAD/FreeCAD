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
"""Unit tests for the Draft Workbench, Airfoil DAT import and export tests."""
## @package test_airfoildat
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, Airfoil DAT tests.

## \addtogroup drafttests
# @{
import os
import unittest

import FreeCAD as App
import Draft
import drafttests.auxiliary as aux

from draftutils.messages import _msg


class DraftAirfoilDAT(unittest.TestCase):
    """Test reading and writing of AirfoilDAT with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        aux.draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_airfoildat(self):
        """Read an airfoil DAT file and import its elements as objects."""
        operation = "importAirfoilDAT.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires a DAT file with airfoil data to read.")

        file = 'Mod/Draft/drafttest/test.dat'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_airfoildat = aux.fake_function
        obj = Draft.import_airfoildat(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_airfoildat(self):
        """Create some figures and export them to an airfoil DAT file."""
        operation = "importAirfoilDAT.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.dat'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_airfoildat = aux.fake_function
        obj = Draft.export_airfoildat(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)

## @}
