# ***************************************************************************
# *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
"""Unit tests for the Draft Workbench, array tests."""
## @package test_array
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, array tests.

## \addtogroup drafttests
# @{
import unittest
import math

import FreeCAD as App
import Draft

from FreeCAD import Vector
from draftutils.messages import _msg


class DraftArray(unittest.TestCase):
    """Test Draft array functions."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != doc_name:
                App.newDocument(doc_name)
        else:
            App.newDocument(doc_name)
        App.setActiveDocument(doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc.Name))

    def test_link_array(self):
        """Create a link array."""
        box = self.doc.addObject("Part::Box","Box")
        box.Label = "Box"
        self.doc.recompute()

        array = Draft.make_ortho_array(box, v_x=App.Vector(100.0, 0.0, 0.0),
                                            v_y=App.Vector(0.0, 100.0, 0.0),
                                            v_z=App.Vector(0.0, 0.0, 100.0),
                                            n_x=12, n_y=1, n_z=1, use_link=True)

        Draft.autogroup(array)
        array.ExpandArray = True
        array.Fuse = False
        self.doc.recompute(None,True,True)

        array.NumberX = 6
        self.doc.recompute(None,True,True)
        self.assertEqual(array.Count, array.NumberX)

        array.NumberX = 24
        self.doc.recompute(None,True,True)
        self.assertEqual(array.Count, array.NumberX)

        self.doc.recompute(None,True,True)
        self.assertEqual(array.Count, array.NumberX)

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc.Name)

## @}
