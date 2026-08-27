# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD as App
import Surface


class TestGeomFillSurface(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestGeomFillSurface")
        self.surface = self.doc.addObject(f"{Surface.__name__}::GeomFillSurface", "Surface")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def test_reversed_list_starts_empty(self):
        self.assertEqual(tuple(self.surface.ReversedList), ())

    def test_surplus_legacy_orientation_flag_is_removed(self):
        edge1 = self.doc.addObject("PartDesign::Feature", "Edge1")
        edge2 = self.doc.addObject("PartDesign::Feature", "Edge2")
        self.surface.BoundaryList = [(edge1, ["Edge1"]), (edge2, ["Edge1"])]

        self.surface.ReversedList = [True, False, True]

        self.assertEqual(tuple(self.surface.ReversedList), (True, False))
