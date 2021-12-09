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
"""Unit tests for the Draft Workbench, Coin (Pivy) tests."""
## @package test_pivy
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, Coin (Pivy) tests.

## \addtogroup drafttests
# @{
import unittest

import FreeCAD as App
import FreeCADGui as Gui
import drafttests.auxiliary as aux

from draftutils.messages import _msg


class DraftPivy(unittest.TestCase):
    """Test for the presence of Pivy and that it works with Coin3D."""

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

    def test_pivy_import(self):
        """Import Coin (Pivy)."""
        module = "pivy.coin"
        imported = aux.import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_pivy_draw(self):
        """Use Coin (pivy.coin) to draw a cube on the active view."""
        import pivy.coin as coin
        cube = coin.SoCube()
        _msg("  Draw cube")
        Gui.ActiveDocument.ActiveView.getSceneGraph().addChild(cube)
        _msg("  Adding cube to the active view scene")
        self.assertTrue(cube, "Pivy is not working properly.")

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)

## @}
