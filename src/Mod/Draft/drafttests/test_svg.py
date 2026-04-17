# SPDX-License-Identifier: LGPL-2.1-or-later

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
"""Unit tests for the Draft Workbench, SVG import and export tests."""

## @package test_svg
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, SVG import and export tests.

## \addtogroup drafttests
# @{

import os
import tempfile
import unittest
from unittest import mock

import FreeCAD as App
import Draft
import Part
import draftfunctions.svgshapes as svgshapes
import importSVG
from drafttests import auxiliary as aux
from drafttests import test_base
from draftutils.messages import _msg

try:
    import Arch
except ModuleNotFoundError:
    have_arch = False
else:
    have_arch = True


class DraftSVG(test_base.DraftTestCaseDoc):
    """Test reading and writing of SVGs with Draft."""

    def test_read_svg(self):
        """Read an SVG file and import its elements as Draft objects."""
        operation = "importSVG.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires an SVG file to read.")

        file = "Mod/Draft/drafttest/test.svg"
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        obj = aux.fake_function(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_svg(self):
        """Create some figures and export them to an SVG file."""
        operation = "importSVG.export"
        _msg("  Test '{}'".format(operation))

        file = "Mod/Draft/drafttest/out_test.svg"
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        obj = aux.fake_function(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    @unittest.skipIf(not have_arch, "BIM module is not installed")
    def test_get_svg_from_arch_space_with_zero_vector(self):
        """Try to get a svg string from an Arch Space with a zero-vector as direction."""
        import Part
        import Draft

        sb = Part.makeBox(1, 1, 1)
        b = self.doc.addObject("Part::Feature", "Box")
        b.Shape = sb

        s = Arch.makeSpace(b)
        self.doc.recompute()

        try:
            Draft.get_svg(s, direction=App.Vector(0, 0, 0))
        except AttributeError as err:
            self.fail("Cryptic exception thrown: {}".format(err))
        except ValueError as err:
            App.Console.PrintLog("Exception thrown, OK: {}".format(err))
        else:
            self.fail("no exception thrown")


class DraftSVGExportRegression(test_base.DraftTestCaseDoc):
    """Regression tests for SVG export edge cases."""

    @staticmethod
    def _make_face_with_circular_island():
        outer = Part.makePolygon(
            [
                App.Vector(0, 0, 0),
                App.Vector(20, 0, 0),
                App.Vector(20, 20, 0),
                App.Vector(0, 20, 0),
                App.Vector(0, 0, 0),
            ]
        )
        inner = Part.Wire([Part.makeCircle(5, App.Vector(10, 10, 0), App.Vector(0, 0, 1))])
        return Part.Face([outer, inner])

    @staticmethod
    def _make_circular_face():
        wire = Part.Wire([Part.makeCircle(5, App.Vector(0, 0, 0), App.Vector(0, 0, 1))])
        return Part.Face([wire])

    @staticmethod
    def _make_face_with_rectangular_island_in_circle():
        outer = Part.Wire([Part.makeCircle(10, App.Vector(10, 10, 0), App.Vector(0, 0, 1))])
        inner = Part.makePolygon(
            [
                App.Vector(7, 8, 0),
                App.Vector(13, 8, 0),
                App.Vector(13, 12, 0),
                App.Vector(7, 12, 0),
                App.Vector(7, 8, 0),
            ]
        )
        return Part.Face([outer, inner])

    def _add_shape(self, name, shape):
        obj = self.doc.addObject("Part::Feature", name)
        obj.Shape = shape
        self.doc.recompute()
        return obj

    def _assert_island_svg(self, svg):
        self.assertIn("<path ", svg)
        self.assertIn("fill-rule: evenodd", svg)
        self.assertNotIn("<circle ", svg)
        self.assertEqual(svg.count("M "), 2)
        self.assertEqual(svg.count("Z "), 2)
        self.assertEqual(svg.count("A "), 2)
        self.assertEqual(svg.count("L "), 4)

    @staticmethod
    def _iter_island_cases():
        return (
            ("rect_outer_circle_hole", DraftSVGExportRegression._make_face_with_circular_island),
            (
                "circle_outer_rect_hole",
                DraftSVGExportRegression._make_face_with_rectangular_island_in_circle,
            ),
        )

    @staticmethod
    def _force_circle_fallback():
        return mock.patch.object(svgshapes.TechDraw, "projectToSVG", return_value="")

    def test_get_svg_from_face_with_circular_island(self):
        with self._force_circle_fallback():
            for name, shape_factory in self._iter_island_cases():
                with self.subTest(case=name):
                    obj = self._add_shape(f"IslandFace_{name}", shape_factory())
                    svg = Draft.get_svg(obj, direction=App.Vector(0, 0, 1))
                    self._assert_island_svg(svg)

    def test_get_svg_from_face_with_circular_island_for_techdraw(self):
        with self._force_circle_fallback():
            for name, shape_factory in self._iter_island_cases():
                with self.subTest(case=name):
                    obj = self._add_shape(f"TechDrawIslandFace_{name}", shape_factory())
                    svg = Draft.get_svg(obj, direction=App.Vector(0, 0, 1), techdraw=True)
                    self._assert_island_svg(svg)

    def test_export_svg_from_face_with_circular_island(self):
        with self._force_circle_fallback():
            for name, shape_factory in self._iter_island_cases():
                with self.subTest(case=name):
                    obj = self._add_shape(f"ExportIslandFace_{name}", shape_factory())
                    with tempfile.TemporaryDirectory() as temp_dir:
                        out_file = os.path.join(temp_dir, "island_face.svg")
                        importSVG.export([obj], out_file)
                        with open(out_file, "r", encoding="utf-8") as svg_file:
                            svg = svg_file.read()
                    self._assert_island_svg(svg)

    def test_get_svg_from_circular_face_stays_compact(self):
        obj = self._add_shape("DiskFace", self._make_circular_face())

        svg = Draft.get_svg(obj, direction=App.Vector(0, 0, 1))

        self.assertIn("<circle ", svg)
        self.assertNotIn("fill-rule: evenodd", svg)


## @}
